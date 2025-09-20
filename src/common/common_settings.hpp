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
        auto get(this auto&& self) -> T { return static_cast<T*>(self)->getImpl(); }
};

}  // namespace common::settings