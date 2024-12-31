#pragma once
#include <vulkan/vulkan.hpp>
#include "vulkan_common/vulkan_common.hpp"
#include "master_semaphore.hpp"
#include "common/alignment.hpp"
#include <array>
#include <memory>
#include <thread>
#include <mutex>
#include <vector>
#include <gsl/gsl>
namespace render::vulkan {
class GraphicsPipeline;
class Device;
class StateTracker;
namespace resource {
class CommandPool;
}
}  // namespace render::vulkan

namespace render::vulkan::scheduler {

class Scheduler {
    public:
        explicit Scheduler(const Device& device, StateTracker& state_tracker);
        ~Scheduler();
        std::mutex submit_mutex_;
        /// Returns the master timeline semaphore.
        [[nodiscard]] auto getMasterSemaphore() const noexcept -> semaphore::MasterSemaphore& {
            return *master_semaphore_;
        }

    private:
        class Command {
            public:
                virtual ~Command() = default;

                virtual void execute(vk::CommandBuffer cmdbuf,
                                     vk::CommandBuffer upload_cmdbuf) const = 0;

                [[nodiscard]] auto getNext() const -> Command* { return next_; }

                void setNext(Command* next) { next_ = next; }

            private:
                gsl::owner<Command*> next_ = nullptr;
        };

        template <typename T>
        class TypedCommand final : public Command {
            public:
                explicit TypedCommand(T&& command_) : command{std::move(command_)} {}
                ~TypedCommand() override = default;

                TypedCommand(TypedCommand&&) = delete;
                auto operator=(TypedCommand&&) -> TypedCommand& = delete;

                void execute(vk::CommandBuffer cmdBuf,
                             vk::CommandBuffer uploadCmdBuf) const override {
                    command(cmdBuf, uploadCmdBuf);
                }

            private:
                T command;
        };

        class CommandChunk final {
            public:
                void executeAll(vk::CommandBuffer cmdbuf, vk::CommandBuffer upload_cmdbuf);

                template <typename T>
                auto record(T& command) -> bool {
                    using FuncType = TypedCommand<T>;
                    static_assert(sizeof(FuncType) < sizeof(data), "Lambda is too large");

                    command_offset = common::alignUp(command_offset, alignof(FuncType));
                    if (command_offset > sizeof(data) - sizeof(FuncType)) {
                        return false;
                    }
                    Command* const current_last = last;
                    last = new (data.data() + command_offset) FuncType(std::move(command));

                    if (current_last) {
                        current_last->setNext(last);
                    } else {
                        first = last;
                    }
                    command_offset += sizeof(FuncType);
                    return true;
                }

                void markSubmit() { submit = true; }

                [[nodiscard]] auto empty() const -> bool { return command_offset == 0; }

                [[nodiscard]] auto hasSubmit() const -> bool { return submit; }

            private:
                gsl::owner<Command*> first = nullptr;
                gsl::owner<Command*> last = nullptr;

                size_t command_offset = 0;
                bool submit = false;
                /**
                 * @brief An array of bytes aligned to the maximum alignment boundary of the target
                 * platform.
                 *
                 * This array is designed to be aligned to the largest alignment boundary required
                 * by the platform, ensuring that it can be used efficiently for operations
                 * requiring specific alignment.
                 */
                alignas(std::max_align_t) std::array<std::uint8_t, 0x8000> data{};
        };

        struct State {
                vk::RenderPass render_pass_ = nullptr;
                vk::Framebuffer framebuffer_ = nullptr;
                vk::Extent2D render_area_ = {0, 0};
                GraphicsPipeline* graphics_pipeline_ = nullptr;
                bool is_rescaling_ = false;
                bool rescaling_defined_ = false;
        };

        const Device& device_;
        StateTracker& state_tracker_;
        State state_;
        void workerThread(std::stop_token stop_token);
        void allocateWorkerCommandBuffer();
        void acquireNewChunk();

        std::unique_ptr<semaphore::MasterSemaphore> master_semaphore_;
        std::unique_ptr<resource::CommandPool> command_pool_;

        vk::CommandBuffer current_cmd_buf_;
        vk::CommandBuffer current_upload_cmd_buf_;
        std::unique_ptr<CommandChunk> chunk_;
        std::function<void()> on_submit_;

        uint32_t num_render_pass_images_ = 0;
        std::array<vk::Image, 9> render_pass_images_{};
        std::array<vk::ImageSubresourceRange, 9> render_pass_image_ranges_{};

        std::queue<std::unique_ptr<CommandChunk>> work_queue_;
        std::vector<std::unique_ptr<CommandChunk>> chunk_reserve_;
        std::mutex execution_mutex_;
        std::mutex reserve_mutex_;
        std::mutex queue_mutex_;
        std::condition_variable_any event_cv_;
        std::jthread worker_thread_;
};
}  // namespace render::vulkan::scheduler