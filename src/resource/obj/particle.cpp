#include "particle.hpp"
#include <numbers>
#include <random>
namespace graphics {

ParticleModel::ParticleModel(std::uint64_t count, float aspect) {
    // Initialize particles
    std::random_device rd;
    std::default_random_engine randEngine(rd());
    std::uniform_real_distribution<float> rndDist(0.0F, 1.0F);

    particles.resize(count);
    for (auto& particle : particles) {
        float r = 0.25F * std::sqrt(rndDist(randEngine));
        float theta = rndDist(randEngine) * 2.0F * std::numbers::pi_v<float>;
        float x = r * std::cos(theta) * aspect;
        float y = r * std::sin(theta);
        particle.position = glm::vec2(x, y);
        particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00025F;
        particle.color =
            glm::vec4(rndDist(randEngine), rndDist(randEngine), rndDist(randEngine), 1.0F);
    }
}

[[nodiscard]] auto ParticleModel::getMesh() const -> std::span<const float> {
    return std::span<const float>(reinterpret_cast<const float*>(particles.data()),
                                  particles.size() * sizeof(Vertex) / sizeof(float));
}
}  // namespace graphics