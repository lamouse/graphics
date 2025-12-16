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

        auto getBoneInfoMap() -> auto& { return bone_info_map_; }
        auto getBoneCount() -> int& { return bone_counter_; }

    private:
        void processNode(aiNode* root_node, const aiScene* scene);
        void processMesh(aiMesh* mesh, const aiScene* scene);
        void loadModel(std::string_view path);
        void extract_bone_weight(std::vector<graphics::animation::Vertex>& vertices, aiMesh* mesh);
        std::vector<Mesh> meshes_;
        int bone_counter_{};
        std::map<std::string, BoneInfo> bone_info_map_;
};

}  // namespace graphics::animation