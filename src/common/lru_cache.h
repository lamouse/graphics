// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <deque>
#include <memory>
#include <type_traits>

#include "common/common_types.hpp"

namespace common {

template <class Traits>
class LeastRecentlyUsedCache {
        // 定义对象类型和计时器类型
        using ObjectType = typename Traits::ObjectType;
        using TickType = typename Traits::TickType;

        struct Item {
                ObjectType obj;  // 缓存的对象
                TickType tick;   // 对象的时间戳
                Item* next{};    // 指向下一个缓存项的指针
                Item* prev{};    // 指向上一个缓存项的指针
        };

    public:
        LeastRecentlyUsedCache() = default;
        ~LeastRecentlyUsedCache() = default;
        // 插入一个新对象到缓存中，并返回其ID
        auto Insert(ObjectType obj, TickType tick) -> size_t {
            const auto new_id = Build();     // 获取一个新的缓存项ID
            auto& item = item_pool[new_id];  // 获取对应的缓存项
            item.obj = obj;                  // 设置缓存项的对象
            item.tick = tick;                // 设置缓存项的时间戳
            Attach(item);                    // 将缓存项添加到链表中
            return new_id;                   // 返回缓存项的ID
        }
        // 更新缓存项的时间戳，并将其移动到链表末尾（表示最近使用）
        void Touch(size_t id, TickType tick) {
            auto& item = item_pool[id];  // 获取对应的缓存项
            if (item.tick >= tick) {     // 如果新时间戳不大于旧时间戳，直接返回
                return;
            }
            item.tick = tick;          // 更新时间戳
            if (&item == last_item) {  // 如果已经是链表末尾，直接返回
                return;
            }
            Detach(item);  // 从链表中移除
            Attach(item);  // 重新添加到链表末尾
        }
        // 释放一个缓存项，将其从链表中移除并放入空闲列表
        void Free(size_t id) {
            auto& item = item_pool[id];  // 获取对应的缓存项
            Detach(item);                // 从链表中移除
            item.prev = nullptr;         // 重置前后指针
            item.next = nullptr;
            free_items.push_back(id);  // 将ID放入空闲列表
        }
        // 遍历所有时间戳小于给定值的缓存项，并对每个项执行给定的函数
        template <typename Func>
        void ForEachItemBelow(TickType tick, Func&& func) {
            // 判断函数返回值是否为bool类型
            static constexpr bool RETURNS_BOOL =
                std::is_same_v<std::invoke_result<Func, ObjectType>, bool>;
            Item* iterator = first_item;  // 从链表头部开始遍历
            while (iterator) {
                // 如果当前项的时间戳不小于给定值，停止遍历
                if (static_cast<s64>(tick) - static_cast<s64>(iterator->tick) < 0) {
                    return;
                }
                Item* next = iterator->next;  // 保存下一个项的指针
                if constexpr (RETURNS_BOOL) {
                    // 如果函数返回bool类型，且返回true，停止遍历
                    if (func(iterator->obj)) {
                        return;
                    }
                } else {
                    // 否则，直接执行函数
                    func(iterator->obj);
                }
                iterator = next;  // 移动到下一个项
            }
        }

    private:
        // 构建一个新的缓存项，返回其ID
        size_t Build() {
            if (free_items.empty()) {                     // 如果空闲列表为空
                const size_t item_id = item_pool.size();  // 获取新的ID
                auto& item = item_pool.emplace_back();    // 在池中添加一个新的缓存项
                item.next = nullptr;                      // 初始化前后指针
                item.prev = nullptr;
                return item_id;  // 返回新缓存项的ID
            }
            const size_t item_id = free_items.front();  // 从空闲列表中获取一个ID
            free_items.pop_front();                     // 从空闲列表中移除该ID
            auto& item = item_pool[item_id];            // 获取对应的缓存项
            item.next = nullptr;                        // 初始化前后指针
            item.prev = nullptr;
            return item_id;  // 返回缓存项的ID
        }
        // 将缓存项添加到链表末尾
        void Attach(Item& item) {
            if (!first_item) {  // 如果链表为空，设置为首项
                first_item = &item;
            }
            if (!last_item) {  // 如果链表为空，设置为尾项
                last_item = &item;
            } else {
                item.prev = last_item;    // 将当前项的前指针指向原尾项
                last_item->next = &item;  // 将原尾项的后指针指向当前项
                item.next = nullptr;      // 当前项的后指针置空
                last_item = &item;        // 更新尾项为当前项
            }
        }

        void Detach(Item& item) {
            if (item.prev) {  // 如果存在前项，更新前项的后指针
                item.prev->next = item.next;
            }
            if (item.next) {  // 如果存在后项，更新后项的前指针
                item.next->prev = item.prev;
            }
            if (&item == first_item) {  // 如果当前项是首项，更新首项
                first_item = item.next;
                if (first_item) {
                    first_item->prev = nullptr;
                }
            }
            if (&item == last_item) {  // 如果当前项是尾项，更新尾项
                last_item = item.prev;
                if (last_item) {
                    last_item->next = nullptr;
                }
            }
        }

        std::deque<Item> item_pool;     // 缓存项池
        std::deque<size_t> free_items;  // 空闲缓存项ID列表
        Item* first_item{};             // 链表首项指针
        Item* last_item{};              // 链表尾项指针
};

}  // namespace common
