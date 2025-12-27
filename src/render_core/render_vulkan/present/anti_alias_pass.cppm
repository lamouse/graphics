module;
#include<vulkan/vulkan.hpp>
export module render.vulkan.present.AntiAliasPass;
import render.vulkan.common;
import render.vulkan.scheduler;
export namespace render::vulkan {
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