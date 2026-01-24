#pragma once
#include "render_core/render_command.hpp"
#include <memory>
#include <boost/lockfree/spsc_queue.hpp>

namespace render {


class RenderThread {
    public:
        RenderThread();
        void submit(CommandList&& commandList);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
};
}  // namespace render