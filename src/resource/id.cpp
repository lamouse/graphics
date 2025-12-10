#include "id.hpp"
namespace graphics {
auto getCurrentId() -> id_t {
    static id_t id = 0;
    return id++;
}
}  // namespace graphics