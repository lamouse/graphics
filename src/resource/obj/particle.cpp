#include "particle.hpp"
#include <numbers>
#include <random>
namespace graphics {

Particle::Particle() {
    // Initialize particles
    std::default_random_engine randEngine((unsigned)time(nullptr));
    std::uniform_real_distribution<float> rndDist(0.0F, 1.0F);

    particles.resize(PARTICLE_COUNT);
    const float aspect = 1920.0F / 1080.0F; // 修正为浮点除法
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

[[nodiscard]] auto Particle::getMesh() const -> std::span<const float> {
        return std::span<const float>(reinterpret_cast<const float*>(particles.data()),
                                  particles.size() * sizeof(Vertex) / sizeof(float));
}
}  // namespace graphics