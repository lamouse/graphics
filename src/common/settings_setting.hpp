#pragma once

#ifndef GRAPHICS_SETTINGS_SETTING_HPP
#define GRAPHICS_SETTINGS_SETTING_HPP
#include "common/settings_common.hpp"
#include "common/settings_enums.hpp"
#include <algorithm>
#include <fmt/format.h>
namespace settings {
/** The Setting class is a simple resource manager. It defines a label and default value
 * alongside the actual value of the setting for simpler and less-error prone use with frontend
 * configurations. Specifying a default value and label is required. A minimum and maximum range
 * can be specified for sanitization.
 */
template <typename Type, bool ranged = false>
class Setting : public BasicSetting {
    protected:
        Setting() = default;

    public:
        /**
         * Sets a default value, label, and setting value.
         *
         * @param linkage Setting registry
         * @param default_val Initial value of the setting, and default value of the setting
         * @param name Label for the setting
         * @param category_ Category of the setting AKA INI group
         * @param specialization_ Suggestion for how frontend implementations represent this in a
         * config
         * @param save_ Suggests that this should or should not be saved to a frontend config file
         * @param runtime_modifiable_ Suggests whether this is modifiable while a guest is loaded
         * @param other_setting_ A second Setting to associate to this one in metadata
         */
        explicit Setting(Linkage& linkage, const Type& default_val, const std::string& name,
                         Category category_, u32 specialization_ = Specialization::Default,
                         bool save_ = true, bool runtime_modifiable_ = false,
                         BasicSetting* other_setting_ = nullptr)
            requires(!ranged)
            : BasicSetting(linkage, name, category_, save_, runtime_modifiable_, specialization_,
                           other_setting_),
              value{default_val},
              default_value{default_val} {}
        virtual ~Setting() = default;

        /**
         * Sets a default value, minimum value, maximum value, and label.
         *
         * @param linkage Setting registry
         * @param default_val Initial value of the setting, and default value of the setting
         * @param min_val Sets the minimum allowed value of the setting
         * @param max_val Sets the maximum allowed value of the setting
         * @param name Label for the setting
         * @param category_ Category of the setting AKA INI group
         * @param specialization_ Suggestion for how frontend implementations represent this in a
         * config
         * @param save_ Suggests that this should or should not be saved to a frontend config file
         * @param runtime_modifiable_ Suggests whether this is modifiable while a guest is loaded
         * @param other_setting_ A second Setting to associate to this one in metadata
         */
        explicit Setting(Linkage& linkage, const Type& default_val, const Type& min_val,
                         const Type& max_val, const std::string& name, Category category_,
                         u32 specialization_ = Specialization::Default, bool save_ = true,
                         bool runtime_modifiable_ = false, BasicSetting* other_setting_ = nullptr)
            requires(ranged)
            : BasicSetting(linkage, name, category_, save_, runtime_modifiable_, specialization_,
                           other_setting_),
              value{default_val},
              default_value{default_val},
              maximum{max_val},
              minimum{min_val} {}
        explicit Setting(Linkage& linkage, const Type& default_val, const std::string& name,
                         Category category_, u32 specialization_ = Specialization::Default,
                         bool save_ = true, bool runtime_modifiable_ = false,
                         BasicSetting* other_setting_ = nullptr)
            requires(ranged && std::is_enum_v<Type>)
            : BasicSetting(linkage, name, category_, save_, runtime_modifiable_, specialization_,
                           other_setting_),
              value{default_val},
              default_value{default_val},
              maximum{enums::EnumMetadata<Type>::GetLast()},
              minimum{enums::EnumMetadata<Type>::GetFirst()} {}
        /**
         *  Returns a reference to the setting's value.
         *
         * @returns A reference to the setting
         */
        [[nodiscard]] virtual auto GetValue() const -> const Type& { return value; }

        /**
         * Sets the setting to the given value.
         *
         * @param val The desired value
         */
        virtual void SetValue(const Type& val) {
            Type temp{ranged ? std::clamp(val, minimum, maximum) : val};
            std::swap(value, temp);
        }
        /**
         * Returns the value that this setting was created with.
         *
         * @returns A reference to the default value
         */
        [[nodiscard]] auto GetDefault() const -> const Type& { return default_value; }

        [[nodiscard]] constexpr auto IsEnum() const -> bool override {
            return std::is_enum_v<Type>;
        }

    protected:
        [[nodiscard]] auto ToString(const Type& value_) const -> std::string {
            if constexpr (std::is_same_v<Type, std::string>) {
                return value_;
            } else if constexpr (std::is_same_v<Type, std::optional<u32>>) {
                return value_.has_value() ? std::to_string(*value_) : "none";
            } else if constexpr (std::is_same_v<Type, bool>) {
                return value_ ? "true" : "false";
            } else if constexpr (std::is_floating_point_v<Type>) {
                return fmt::format("{:f}", value_);
            } else if constexpr (std::is_enum_v<Type>) {
                return std::to_string(static_cast<u32>(value_));
            } else {
                return std::to_string(value_);
            }
        }

    public:
        /**
         * Converts the value of the setting to a std::string. Respects the global state if the
         * setting has one.
         *
         * @returns The current setting as a std::string
         */
        [[nodiscard]] auto ToString() const -> std::string override {
            return ToString(this->GetValue());
        }

        /**
         * Returns the default value of the setting as a std::string.
         *
         * @returns The default value as a string.
         */
        [[nodiscard]] auto DefaultToString() const -> std::string override {
            return ToString(default_value);
        }

        /**
         * Assigns a value to the setting.
         *
         * @param val The desired setting value
         *
         * @returns A reference to the setting
         */
        virtual auto operator=(const Type& val) -> const Type& {
            Type temp{ranged ? std::clamp(val, minimum, maximum) : val};
            std::swap(value, temp);
            return value;
        }

        /**
         * Returns a reference to the setting.
         *
         * @returns A reference to the setting
         */
        explicit virtual operator const Type&() const { return value; }

        /**
         * Converts the given value to the Setting's type of value. Uses SetValue to enter the
         * setting, thus respecting its constraints.
         *
         * @param input The desired value
         */
        void LoadString(const std::string& input) override final {
            if (input.empty()) {
                this->SetValue(this->GetDefault());
                return;
            }
            try {
                if constexpr (std::is_same_v<Type, std::string>) {
                    this->SetValue(input);
                } else if constexpr (std::is_same_v<Type, std::optional<u32>>) {
                    this->SetValue(static_cast<u32>(std::stoul(input)));
                } else if constexpr (std::is_same_v<Type, bool>) {
                    this->SetValue(input == "true");
                } else if constexpr (std::is_same_v<Type, float>) {
                    this->SetValue(std::stof(input));
                } else {
                    this->SetValue(static_cast<Type>(std::stoll(input)));
                }
            } catch (std::invalid_argument&) {
                this->SetValue(this->GetDefault());
            } catch (std::out_of_range&) {
                this->SetValue(this->GetDefault());
            }
        }
        [[nodiscard]] auto Canonicalize() const -> std::string final {
            if constexpr (std::is_enum_v<Type>) {
                return std::string{enums::CanonicalizeEnum(this->GetValue())};
            } else {
                return ToString(this->GetValue());
            }
        }

        /**
         * Gives us another way to identify the setting without having to go through a string.
         *
         * @returns the type_index of the setting's type
         */
        [[nodiscard]] auto TypeId() const -> std::type_index final {
            return std::type_index(typeid(Type));
        }

        [[nodiscard]] constexpr auto EnumIndex() const -> u32 final {
            if constexpr (std::is_enum_v<Type>) {
                return enums::EnumMetadata<Type>::Index();
            } else {
                return (std::numeric_limits<u32>::max)();
            }
        }

        [[nodiscard]] constexpr auto IsFloatingPoint() const -> bool final {
            return std::is_floating_point_v<Type>;
        }

        [[nodiscard]] constexpr auto IsIntegral() const -> bool final {
            return std::is_integral_v<Type>;
        }

        [[nodiscard]] auto MinVal() const -> std::string final {
            if constexpr (std::is_arithmetic_v<Type> && !ranged) {
                return this->ToString((std::numeric_limits<Type>::min)());
            } else {
                return this->ToString(minimum);
            }
        }
        [[nodiscard]] auto MaxVal() const -> std::string final {
            if constexpr (std::is_arithmetic_v<Type> && !ranged) {
                return this->ToString((std::numeric_limits<Type>::max)());
            } else {
                return this->ToString(maximum);
            }
        }

        [[nodiscard]] constexpr auto Ranged() const -> bool override { return ranged; }

    protected:
        Type value{};                ///< The setting
        const Type default_value{};  ///< The default value
        const Type maximum{};        ///< Maximum allowed value of the setting
        const Type minimum{};        ///< Minimum allowed value of the setting
};

/**
 * The SwitchableSetting class is a slightly more complex version of the Setting class. This adds a
 * custom setting to switch to when a guest application specifically requires it. The effect is that
 * other components of the emulator can access the setting's intended value without any need for the
 * component to ask whether the custom or global setting is needed at the moment.
 *
 * By default, the global setting is used.
 */
template <typename Type, bool ranged = false>
class SwitchableSetting : virtual public Setting<Type, ranged> {
    public:
        /**
         * Sets a default value, label, and setting value.
         *
         * @param linkage Setting registry
         * @param default_val Initial value of the setting, and default value of the setting
         * @param name Label for the setting
         * @param category_ Category of the setting AKA INI group
         * @param specialization_ Suggestion for how frontend implementations represent this in a
         * config
         * @param save_ Suggests that this should or should not be saved to a frontend config file
         * @param runtime_modifiable_ Suggests whether this is modifiable while a guest is loaded
         * @param other_setting_ A second Setting to associate to this one in metadata
         */
        template <typename T = BasicSetting>
        explicit SwitchableSetting(Linkage& linkage, const Type& default_val,
                                   const std::string& name, Category category_,
                                   u32 specialization_ = Specialization::Default, bool save_ = true,
                                   bool runtime_modifiable_ = false, T* other_setting_ = nullptr)
            requires(!ranged)
            : Setting<Type, false>{
                  linkage, default_val,         name,          category_, specialization_,
                  save_,   runtime_modifiable_, other_setting_} {
            linkage.restore_functions.emplace_back([]() {});
        }
        virtual ~SwitchableSetting() = default;

        /// @brief Sets a default value, minimum value, maximum value, and label.
        /// @param linkage Setting registry
        /// @param default_val Initial value of the setting, and default value of the setting
        /// @param min_val Sets the minimum allowed value of the setting
        /// @param max_val Sets the maximum allowed value of the setting
        /// @param name Label for the setting
        /// @param category_ Category of the setting AKA INI group
        /// @param specialization_ Suggestion for how frontend implementations represent this in a
        /// config
        /// @param save_ Suggests that this should or should not be saved to a frontend config file
        /// @param runtime_modifiable_ Suggests whether this is modifiable while a guest is loaded
        /// @param other_setting_ A second Setting to associate to this one in metadata
        template <typename T = BasicSetting>
        explicit SwitchableSetting(Linkage& linkage, const Type& default_val, const Type& min_val,
                                   const Type& max_val, const std::string& name, Category category_,
                                   u32 specialization_ = Specialization::Default, bool save_ = true,
                                   bool runtime_modifiable_ = false, T* other_setting_ = nullptr)
            requires(ranged)
            : Setting<Type, true>{linkage,         default_val, min_val,
                                  max_val,         name,        category_,
                                  specialization_, save_,       runtime_modifiable_,
                                  other_setting_} {
            linkage.restore_functions.emplace_back([]() {});
        }

        template <typename T = BasicSetting>
        explicit SwitchableSetting(Linkage& linkage, const Type& default_val,
                                   const std::string& name, Category category_,
                                   u32 specialization_ = Specialization::Default, bool save_ = true,
                                   bool runtime_modifiable_ = false, T* other_setting_ = nullptr)
            requires(ranged)
            : Setting<Type, true>{linkage,
                                  default_val,
                                  enums::EnumMetadata<Type>::GetFirst(),
                                  enums::EnumMetadata<Type>::GetLast(),
                                  name,
                                  category_,
                                  specialization_,
                                  save_,
                                  runtime_modifiable_,
                                  other_setting_} {
            linkage.restore_functions.emplace_back([]() {});
        }

        /**
         * Returns either the global or custom setting depending on the values of this setting's
         * global state or if the global value was specifically requested.
         *
         * @param need_global Request global value regardless of setting's state; defaults to false
         *
         * @returns The required value of the setting
         */
        [[nodiscard]] auto GetValue() const -> const Type& final {
            if (use_global) {
                return this->value;
            }
            return custom;
        }

        /**
         * Sets the current setting value depending on the global state.
         *
         * @param val The new value
         */
        void SetValue(const Type& val) final {
            Type temp{ranged ? std::clamp(val, this->minimum, this->maximum) : val};
            if (use_global) {
                std::swap(this->value, temp);
            } else {
                std::swap(custom, temp);
            }
        }

        [[nodiscard]] constexpr auto Switchable() const -> bool final { return true; }

        /**
         * Assigns the current setting value depending on the global state.
         *
         * @param val The new value
         *
         * @returns A reference to the current setting value
         */
        auto operator=(const Type& val) -> const Type& final {
            Type temp{ranged ? std::clamp(val, this->minimum, this->maximum) : val};
            if (use_global) {
                std::swap(this->value, temp);
                return this->value;
            }
            std::swap(custom, temp);
            return custom;
        }

        /**
         * Returns the current setting value depending on the global state.
         *
         * @returns A reference to the current setting value
         */
        explicit operator const Type&() const final {
            if (use_global) {
                return this->value;
            }
            return custom;
        }

    protected:
        bool use_global{true};  ///< The setting's global state
        Type custom{};          ///< The custom value of the setting
};

}  // namespace settings

#endif  // GRAPHICS_SETTINGS_SETTING_HPP