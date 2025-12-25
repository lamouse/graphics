module;
#include<vulkan/vulkan.hpp>
#include "render_core/render_vulkan/scheduler.hpp"
export module render.vulkan.present.AntiAliasPass;
import render.vulkan.common;
export namespace render::vulkan{
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
}