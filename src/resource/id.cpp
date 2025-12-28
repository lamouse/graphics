#include "id.hpp"
namespace graphics {
auto getCurrentId() -> id_t {
    static id_t id = 1;
    return id++;
}
}  // namespace graphics