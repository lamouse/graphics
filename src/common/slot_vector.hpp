#pragma once

#include <bit>
#include <numeric>
#include <type_traits>
#include <utility>
#include <vector>
#include <cassert>

#include "common_types.hpp"

namespace common {
struct SlotId {
        static constexpr u32 INVALID_INDEX = std::numeric_limits<u32>::max();

        constexpr auto operator<=>(const SlotId&) const noexcept = default;

        constexpr explicit operator bool() const noexcept { return index != INVALID_INDEX; }

        u32 index = INVALID_INDEX;
};

template <class T>
    requires std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>
class SlotVector {
    public:
        SlotVector(const SlotVector&) = default;
        auto operator=(const SlotVector&) ->SlotVector& = default;
        SlotVector(SlotVector&&) noexcept = default;
        auto operator=(SlotVector&&) noexcept -> SlotVector& = default;
        SlotVector() = default;
        class Iterator {
                friend class SlotVector<T>;

            public:
                constexpr Iterator() = default;

                auto operator++() noexcept -> Iterator& {
                    const u64* const bitset = slot_vector->stored_bitset.data();
                    const u32 size = static_cast<u32>(slot_vector->stored_bitset.size()) * 64;
                    if (id.index < size) {
                        ++id.index;
                        while (id.index < size && !IsValid(bitset)) {
                            ++id.index;
                        }
                        if (id.index == size) {
                            id.index = SlotId::INVALID_INDEX;
                        }
                    }
                    return *this;
                }

                auto operator++(int) noexcept -> Iterator {
                    const Iterator copy{*this};
                    ++*this;
                    return copy;
                }

                auto operator==(const Iterator& other) const noexcept -> bool {
                    return id.index == other.id.index;
                }

                auto operator!=(const Iterator& other) const noexcept -> bool {
                    return id.index != other.id.index;
                }

                auto operator*() const noexcept -> std::pair<SlotId, T*> {
                    return {id, std::addressof((*slot_vector)[id])};
                }

                auto operator->() const noexcept -> T* {
                    return std::addressof((*slot_vector)[id]);
                }

            private:
                Iterator(SlotVector<T>* slot_vector_, SlotId id_) noexcept
                    : slot_vector{slot_vector_}, id{id_} {}

                auto IsValid(const u64* bitset) const noexcept -> bool {
                    return ((bitset[id.index / 64] >> (id.index % 64)) & 1U) != 0;
                }

                SlotVector<T>* slot_vector;
                SlotId id;
        };

        ~SlotVector() noexcept {
            size_t index = 0;
            for (u64 bits : stored_bitset) {
                for (size_t bit = 0; bits; ++bit, bits >>= 1U) {
                    if ((bits & 1U) != 0) {
                        values[index + bit].object.~T();
                    }
                }
                index += 64;
            }
            delete[] values;
        }

        [[nodiscard]] auto operator[](SlotId id) noexcept -> T& {
            ValidateIndex(id);
            return values[id.index].object;
        }

        [[nodiscard]] auto operator[](SlotId id) const noexcept -> const T& {
            ValidateIndex(id);
            return values[id.index].object;
        }

        template <typename... Args>
        [[nodiscard]] auto insert(Args&&... args) noexcept -> SlotId {
            const u32 index = FreeValueIndex();
            new (&values[index].object) T(std::forward<Args>(args)...);
            SetStorageBit(index);

            return SlotId{index};
        }

        void erase(SlotId id) noexcept {
            values[id.index].object.~T();
            free_list.push_back(id.index);
            ResetStorageBit(id.index);
        }

        [[nodiscard]] auto begin() noexcept -> Iterator {
            const auto it =
                std::ranges::find_if(stored_bitset, [](u64 value) { return value != 0; });
            if (it == stored_bitset.end()) {
                return end();
            }
            const u32 word_index = static_cast<u32>(std::distance(it, stored_bitset.begin()));
            const SlotId first_id{word_index * 64 + static_cast<u32>(std::countr_zero(*it))};
            return Iterator(this, first_id);
        }

        [[nodiscard]] auto end() noexcept -> Iterator {
            return Iterator(this, SlotId{SlotId::INVALID_INDEX});
        }

        [[nodiscard]] auto size() const noexcept -> size_t {
            return values_capacity - free_list.size();
        }

    private:
        struct NonTrivialDummy {
                NonTrivialDummy() noexcept = default;
        };

        union Entry {
                Entry() noexcept : dummy{} {}
                Entry(const Entry&) = default;
                Entry(Entry&&) noexcept = default;
                auto operator=(const Entry&) -> Entry& = default;
                auto operator=(Entry&&) noexcept -> Entry& = default;

                ~Entry() noexcept {}

                NonTrivialDummy dummy;
                T object;
        };

        void SetStorageBit(u32 index) noexcept {
            stored_bitset[index / 64] |= u64(1) << (index % 64);
        }

        void ResetStorageBit(u32 index) noexcept {
            stored_bitset[index / 64] &= ~(u64(1) << (index % 64));
        }

        auto ReadStorageBit(u32 index) noexcept -> bool {
            return ((stored_bitset[index / 64] >> (index % 64)) & 1U) != 0;
        }

        void ValidateIndex(SlotId id) const noexcept {
            assert(id && "id");
            assert(id.index / 64 < stored_bitset.size() && "id.index / 64 < stored_bitset.size()");
            assert(((stored_bitset[id.index / 64] >> (id.index % 64)) & 1U) != 0 &&
                   "((stored_bitset[id.index / 64] >> (id.index % 64)) & 1) != 0");
        }

        [[nodiscard]] auto FreeValueIndex() noexcept -> u32 {
            if (free_list.empty()) {
                Reserve(values_capacity ? (values_capacity << 1U) : 1);
            }
            const u32 free_index = free_list.back();
            free_list.pop_back();
            return free_index;
        }

        void Reserve(size_t new_capacity) noexcept {
            auto* const new_values =
                new Entry[new_capacity];
            size_t index = 0;
            for (u64 bits : stored_bitset) {
                u64 temp = bits;
                while (temp) {
                    const size_t bit = std::countr_zero(temp);
                    const std::size_t i = index + bit;
                    T& old_value = values[i].object;
                    new (&new_values[i].object) T(std::move(old_value));
                    old_value.~T();
                    temp &= temp - 1;
                }
                index += 64;
            }

            stored_bitset.resize((new_capacity + 63) / 64);

            const size_t old_free_size = free_list.size();
            free_list.resize(old_free_size + (new_capacity - values_capacity));
            std::iota(free_list.begin() + old_free_size, free_list.end(),
                      static_cast<u32>(values_capacity));

            delete[] values;
            values = new_values;
            values_capacity = new_capacity;
        }

        Entry* values = nullptr;
        size_t values_capacity = 0;

        std::vector<u64> stored_bitset;
        std::vector<u32> free_list;
};

}  // namespace common

template <>
struct std::hash<common::SlotId> {
        auto operator()(const common::SlotId& id) const noexcept -> size_t {
            return std::hash<u32>{}(id.index);
        }
};