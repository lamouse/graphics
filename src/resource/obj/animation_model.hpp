#pragma once
#include <string>
#include <map>
#include "resource/obj/bone_info.hpp"
namespace graphics::animation {
class Model {
    public:
        std::string directory_;
        bool gamma_correction_;

        Model(std::string_view path, bool gamma = false) : gamma_correction_(gamma) {
            loadModel(path);
        }

    private:
        void loadModel(std::string_view path);
        int bone_counter_;
        std::map<std::string, BoneInfo> bone_info_map_;
};

}  // namespace graphics::animation