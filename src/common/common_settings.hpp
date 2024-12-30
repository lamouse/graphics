#pragma once
namespace common::settings {

template <typename Setting>
auto get() -> decltype(auto) {
    return Setting::get();
}

template <typename T>
class BaseSetting {
    public:
        auto get(this auto&& self) -> T { return static_cast<T*>(self)->getImpl(); }
        static auto get() {}
};

}  // namespace common::settings