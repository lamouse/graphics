#pragma once
namespace common::settings {

template <typename Setting>
auto get() -> decltype(auto) {
    return Setting::get();
}

template <typename T>
class BaseSetting {
    public:
        /**
         * @brief
         * @hiderefby
         * @param self
         * @return T
         */
        /// @cond
        // TODO doxygen seems to have problems with this等待支持此6语法
        auto get(this auto&& self) -> T { return static_cast<T*>(self)->getImpl(); }
        /// @endcond
};

}  // namespace common::settings