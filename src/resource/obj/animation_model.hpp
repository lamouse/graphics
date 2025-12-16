#pragma once
#include <string>
#include <map>
#include <assimp/scene.h>
#include "resource/obj/bone_info.hpp"
#include "resource/obj/animal_mesh.hpp"
namespace graphics::animation {
class Model {
    public:
        std::string directory_;
        bool gamma_correction_;

        Model(std::string_view path, bool gamma = false) : gamma_correction_(gamma) {
            loadModel(path);
        }

    private:
        void processNode(aiNode* root_node, const aiScene* scene);
        auto processMesh(aiMesh* mesh, const aiScene* scene) -> Mesh;
        void loadModel(std::string_view path);
        std::vector<Mesh> meshes_;
        int bone_counter_{};
        std::map<std::string, BoneInfo> bone_info_map_;
};

}  // namespace graphics::animation