#pragma once
#include <bitset>
#include <cstddef>
#include "fronted/ir/attribute.hpp"

namespace shader {
struct VaryingState {
        std::bitset<512> mask;

        void Set(IR::Attribute attribute, bool state = true) {
            mask[static_cast<size_t>(attribute)] = state;
        }

        [[nodiscard]] auto operator[](IR::Attribute attribute) const noexcept -> bool {
            return mask[static_cast<size_t>(attribute)];
        }

        [[nodiscard]] auto anyComponent(IR::Attribute base) const noexcept -> bool {
            return mask[static_cast<size_t>(base) + 0] || mask[static_cast<size_t>(base) + 1] ||
                   mask[static_cast<size_t>(base) + 2] || mask[static_cast<size_t>(base) + 3];
        }

        [[nodiscard]] auto allComponents(IR::Attribute base) const noexcept -> bool {
            return mask[static_cast<size_t>(base) + 0] && mask[static_cast<size_t>(base) + 1] &&
                   mask[static_cast<size_t>(base) + 2] && mask[static_cast<size_t>(base) + 3];
        }

        [[nodiscard]] auto isUniform(IR::Attribute base) const noexcept -> bool {
            return anyComponent(base) == allComponents(base);
        }

        [[nodiscard]] auto generic(size_t index, size_t component) const noexcept -> bool {
            return mask[static_cast<size_t>(IR::Attribute::Generic0X) + index * 4 + component];
        }

        [[nodiscard]] auto generic(size_t index) const noexcept -> bool {
            return generic(index, 0) || generic(index, 1) || generic(index, 2) || generic(index, 3);
        }

        [[nodiscard]] auto clipDistances() const noexcept -> bool {
            return anyComponent(IR::Attribute::ClipDistance0) ||
                   anyComponent(IR::Attribute::ClipDistance4);
        }

        [[nodiscard]] auto legacy() const noexcept -> bool {
            return anyComponent(IR::Attribute::ColorFrontDiffuseR) ||
                   anyComponent(IR::Attribute::ColorFrontSpecularR) ||
                   anyComponent(IR::Attribute::ColorBackDiffuseR) ||
                   anyComponent(IR::Attribute::ColorBackSpecularR) || fixedFunctionTexture() ||
                   mask[static_cast<size_t>(IR::Attribute::FogCoordinate)];
        }

        [[nodiscard]] auto fixedFunctionTexture() const noexcept -> bool {
            for (size_t index = 0; index < 10; ++index) {
                if (anyComponent(IR::Attribute::FixedFncTexture0S + index * 4)) {
                    return true;
                }
            }
            return false;
        }
};
}  // namespace shader