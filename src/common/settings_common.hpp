#pragma once

#ifndef GRAPHICS_SETTING_COMMON_HPP
#define GRAPHICS_SETTING_COMMON_HPP
#include <string>
#include <typeindex>
#include <functional>
#include <unordered_map>
#include <limits>
#include "common/common_types.hpp"
#include "common/common_funcs.hpp"
namespace settings {
enum class Category : u16 {
    core = 0,
    render = 1,
    log = 2,
    system = 3,

    MAX_EUM = std::numeric_limits<u16>::max()
};

constexpr u8 SpecializationTypeMask = 0xf;
constexpr u8 SpecializationAttributeMask = 0xf0;
constexpr u8 SpecializationAttributeOffset = 4;

// Scalar and countable could have better names
enum Specialization : u8 {
    Default = 0,
    Time = 1,         // Duration or specific moment in time
    Hex = 2,          // Hexadecimal number
    List = 3,         // Setting has specific members
    RuntimeList = 4,  // Members of the list are determined during runtime
    Scalar = 5,       // Values are continuous
    Countable = 6,    // Can be stepped through
    Paired = 7,       // Another setting is associated with this setting
    Radio = 8,        // Setting should be presented in a radio group
    Checkbox = 9,     //check box

    Percentage = (1 << SpecializationAttributeOffset),  // Should be represented as a percentage
};

class BasicSetting;

class Linkage {
    public:
        explicit Linkage(u32 initial_count = 0);
        CLASS_DEFAULT_COPYABLE(Linkage);
        CLASS_DEFAULT_MOVEABLE(Linkage);
        ~Linkage();
        std::unordered_map<Category, std::vector<BasicSetting*>> by_category;
        std::unordered_map<std::string, settings::BasicSetting*> by_key;
        std::vector<std::function<void()>> restore_functions;
        u32 count;
};

/**
 * BasicSetting is an abstract class that only keeps track of metadata. The string methods are
 * available to get data values out.
 */
class BasicSetting {
    protected:
        explicit BasicSetting(Linkage& linkage, const std::string& name, Category category_,
                              bool save_, bool runtime_modifiable_, u32 specialization,
                              BasicSetting* other_setting);

    public:
        virtual ~BasicSetting();
        /*
         * Data retrieval
         */

        /**
         * Returns a string representation of the internal data. If the Setting is Switchable, it
         * respects the internal global state: it is based on GetValue().
         *
         * @returns A string representation of the internal data.
         */
        [[nodiscard]] virtual auto ToString() const -> std::string = 0;

        /**
         * @returns A string representation of the Setting's default value.
         */
        [[nodiscard]] virtual auto DefaultToString() const -> std::string = 0;

        /**
         * Returns a string representation of the minimum value of the setting. If the Setting is
         * not ranged, the string represents the default initialization of the data type.
         *
         * @returns A string representation of the minimum value of the setting.
         */
        [[nodiscard]] virtual auto MinVal() const -> std::string = 0;

        /**
         * Returns a string representation of the maximum value of the setting. If the Setting is
         * not ranged, the string represents the default initialization of the data type.
         *
         * @returns A string representation of the maximum value of the setting.
         */
        [[nodiscard]] virtual auto MaxVal() const -> std::string = 0;

        /**
         * Takes a string input, converts it to the internal data type if necessary, and then runs
         * SetValue with it.
         *
         * @param load String of the input data.
         */
        virtual void LoadString(const std::string& load) = 0;

        /**
         * Returns a string representation of the data. If the data is an enum, it returns a string
         * of the enum value. If the internal data type is not an enum, this is equivalent to
         * ToString.
         *
         * e.g. renderer_backend.Canonicalize() == "OpenGL"
         *
         * @returns Canonicalized string representation of the internal data
         */
        [[nodiscard]] virtual auto Canonicalize() const -> std::string = 0;

        /**
         * @returns A unique identifier for the Setting's internal data type.
         */
        [[nodiscard]] virtual auto TypeId() const -> std::type_index = 0;

        /**
         * Returns true if the Setting's internal data type is an enum.
         *
         * @returns True if the Setting's internal data type is an enum
         */
        [[nodiscard]] virtual constexpr auto IsEnum() const -> bool = 0;

        /**
         * Returns true if the current setting is Switchable.
         *
         * @returns If the setting is a SwitchableSetting
         */
        [[nodiscard]] virtual constexpr auto Switchable() const -> bool { return false; }

        /**
         * Returns true to suggest that a frontend can read or write the setting to a configuration
         * file.
         *
         * @returns The save preference
         */
        [[nodiscard]] bool Save() const;

        /**
         * @returns true if the current setting can be changed while the guest is running.
         */
        [[nodiscard]] auto RuntimeModifiable() const -> bool;

        /**
         * @returns A unique number corresponding to the setting.
         */
        [[nodiscard]] constexpr auto Id() const -> u32 { return id; }

        /**
         * Returns the setting's category AKA INI group.
         *
         * @returns The setting's category
         */
        [[nodiscard]] auto GetCategory() const -> Category;

        /**
         * @returns Extra metadata for data representation in frontend implementations.
         */
        [[nodiscard]] auto Specialization() const -> u32;

        /**
         * @returns Another BasicSetting if one is paired, or nullptr otherwise.
         */
        [[nodiscard]] auto PairedSetting() const -> BasicSetting*;

        /**
         * Returns the label this setting was created with.
         *
         * @returns A reference to the label
         */
        [[nodiscard]] auto GetLabel() const -> const std::string&;

        /**
         * @returns If the Setting checks input values for valid ranges.
         */
        [[nodiscard]] virtual constexpr auto Ranged() const -> bool = 0;

        /**
         * @returns The index of the enum if the underlying setting type is an enum, else max of
         * u32.
         */
        [[nodiscard]] virtual constexpr auto EnumIndex() const -> u32 = 0;

        /**
         * @returns True if the underlying type is a floating point storage
         */
        [[nodiscard]] virtual constexpr auto IsFloatingPoint() const -> bool = 0;

        /**
         * @returns True if the underlying type is an integer storage
         */
        [[nodiscard]] virtual constexpr auto IsIntegral() const -> bool = 0;

    private:
        const std::string label;  ///< The setting's label
        const Category category;  ///< The setting's category
        const u32 id;             ///< Unique integer for the setting
        const bool save;  ///< Suggests if the setting should be saved and read to a frontend config
        const bool runtime_modifiable;  ///< Suggests if the setting can be modified while a guest
                                        ///< is running
        const u32 specialization;       ///< Extra data to identify representation of a setting
        BasicSetting* const other_setting;  ///< A paired setting
};

}  // namespace settings

#endif  // GRAPHICS_SETTING_COMMON_HPP
