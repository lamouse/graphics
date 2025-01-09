#pragma once
#include <vulkan/vulkan.hpp>
#include "render_core/vulkan_common/vulkan_common.hpp"
#include "render_core/render_vulkan/master_semaphore.hpp"
#include "common/alignment.hpp"
#include <array>
#include <memory>
#include <thread>
#include <mutex>
#include <vector>
#include <gsl/gsl>
namespace render::vulkan {
class TextureFramebuffer;
class GraphicsPipeline;
class Device;
namespace resource {
class CommandPool;
}
}  // namespace render::vulkan

namespace render::vulkan::scheduler {

class Scheduler {
    public:
        explicit Scheduler(const Device& device);
        ~Scheduler();
        std::mutex submit_mutex_;
        /// Returns the master timeline semaphore.
        [[nodiscard]] auto getMasterSemaphore() const noexcept -> semaphore::MasterSemaphore& {
            return *master_semaphore_;
        }
        auto flush(vk::Semaphore signal_semaphore = nullptr, vk::Semaphore wait_semaphore = nullptr)
            -> u64;

        /// Sends the current execution context to the GPU and waits for it to complete.
        void finish(vk::Semaphore signal_semaphore = nullptr, VkSemaphore wait_semaphore = nullptr);
        void waitWorker();
        void dispatchWork();
        void invalidateState();
        /// Returns true when a tick has been triggered by the GPU.
        [[nodiscard]] auto isFree(u64 tick) const noexcept -> bool {
            return master_semaphore_->isFree(tick);
        }

        /// Waits for the given tick to trigger on the GPU.
        void wait(u64 tick) {
            if (tick >= master_semaphore_->currentTick()) {
                // Make sure we are not waiting for the current tick without signalling
                flush();
            }
            master_semaphore_->wait(tick);
        }
        /// Returns the current command buffer tick.
        [[nodiscard]] auto currentTick() const noexcept -> u64 {
            return master_semaphore_->currentTick();
        }
        /// Send work to a separate thread.
        template <typename T>
            requires std::is_invocable_v<T, vk::CommandBuffer, vk::CommandBuffer>
        void recordWithUploadBuffer(T&& command) {
            if (chunk_->record(command)) {
                return;
            }
            dispatchWork();
            (void)chunk_->record(command);
        }
        template <typename T>
            requires std::is_invocable_v<T, vk::CommandBuffer>
        void record(T&& c) {
            this->recordWithUploadBuffer(
                [command = std::forward<T>(c)](vk::CommandBuffer cmdbuf, vk::CommandBuffer) {
                    command(cmdbuf);
                });
        }

        void requestOutsideRenderPassOperationContext() { endRenderPass(); }

        void requestRenderpass(const TextureFramebuffer* framebuffer);

        // Registers a callback to perform on queue submission.
        void registerOnSubmit(std::function<void()>&& func) { on_submit_ = std::move(func); }

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
        State state_;
        void workerThread(std::stop_token stop_token);
        void allocateWorkerCommandBuffer();
        void acquireNewChunk();
        void endPendingOperations();
        auto submitExecution(vk::Semaphore signal_semaphore, vk::Semaphore wait_semaphore) -> u64;
        void endRenderPass();
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