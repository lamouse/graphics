module;
#include "common/thread_worker.hpp"
export module common.thread_worker;
namespace common {
export {
    using ThreadWorker = StatefulThreadWorker<>;
}
}  // namespace common
