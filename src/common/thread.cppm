module;
#include "common/thread.hpp"
export module common.thread;

namespace common::thread {
export {
    using ThreadPriority = common::thread::ThreadPriority;
    using common::thread::SetCurrentThreadName;
    using common::thread::SetCurrentThreadPriority;
}

}  // namespace common::thread