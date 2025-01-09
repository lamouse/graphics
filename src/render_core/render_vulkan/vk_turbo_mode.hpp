// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <mutex>

#include <condition_variable>

#include "render_core/vulkan_common/device.hpp"
#include "render_core/vulkan_common/memory_allocator.hpp"

namespace render::vulkan {

class TurboMode {
    public:
        explicit TurboMode(const Instance& instance);
        ~TurboMode();

        void QueueSubmitted();

    private:
        void Run(std::stop_token stop_token);

#ifndef ANDROID
        Device m_device;
        MemoryAllocator m_allocator;
#endif
        std::mutex m_submission_lock;
        std::condition_variable_any m_submission_cv;
        std::chrono::time_point<std::chrono::steady_clock> m_submission_time{};

        std::jthread m_thread;
};

}  // namespace render::vulkan
