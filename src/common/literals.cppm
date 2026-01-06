module;

// 引入头文件
#include "common/literals.hpp"
export module common.literals;

// 导出整个命名空间里的符号
export namespace common::literals {
    using ::common::literals::operator""_KiB;
    using ::common::literals::operator""_MiB;
    using ::common::literals::operator""_GiB;
    using ::common::literals::operator""_TiB;
    using ::common::literals::operator""_PiB;
}
