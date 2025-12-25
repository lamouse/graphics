// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

import render.vulkan.common;
namespace render::vulkan {

namespace scheduler {
class Scheduler;

}

class AntiAliasPass {
    public:
        virtual ~AntiAliasPass() = default;
        virtual void Draw(scheduler::Scheduler& scheduler, size_t image_index,
                          vk::Image* inout_image, vk::ImageView* inout_image_view) = 0;
};

class NoAA final : public AntiAliasPass {
    public:
        void Draw(scheduler::Scheduler& scheduler, size_t image_index, vk::Image* inout_image,
                  vk::ImageView* inout_image_view) override {}
};

}  // namespace render::vulkan
