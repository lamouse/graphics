#include "render_core/render_thread.hpp"
#include "render_core/render_base.hpp"
#include <thread>
namespace render {

namespace {
struct CommandQueue {
        boost::lockfree::spsc_queue<CommandList, boost::lockfree::capacity<4096>> command_queue;
};

void run_thread(std::stop_token stop_token, Graphic* graphic, CommandQueue& queue) {
    while(!stop_token.stop_requested()){
        while (not queue.command_queue.empty()) {
            auto commandList = queue.command_queue.front();
            queue.command_queue.pop();
            for(auto& command : commandList.commands){
                //graphic->executeCommand(command);
            }
        }
    }
}


class ThreadManager {
    public:
        void start();
        void submitCommand(CommandList&& commandList){
            command_queue.command_queue.push(std::move(commandList));
        }

    private:
        CommandQueue command_queue;
};
}  // namespace

struct RenderThread::Impl {
        ThreadManager thread_manager;
        Graphic* graphic{};
};

RenderThread::RenderThread() : impl(std::make_unique<RenderThread::Impl>()) {}

void RenderThread::submit(CommandList&& commandList) {
    impl->thread_manager.submitCommand(std::move(commandList));
}

}  // namespace render