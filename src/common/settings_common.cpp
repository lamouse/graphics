#include "common/settings_common.hpp"

namespace settings {
Linkage::Linkage(u32 initial_count) : count{initial_count} {}
Linkage::~Linkage() = default;

BasicSetting::BasicSetting(Linkage& linkage, const std::string& name, enum Category category_,
                           bool save_, bool runtime_modifiable_, u32 specialization_,
                           BasicSetting* other_setting_)
    : label{name},
      category{category_},
      id{linkage.count},
      save{save_},
      runtime_modifiable{runtime_modifiable_},
      specialization{specialization_},
      other_setting{other_setting_} {
    linkage.by_key.insert({name, this});
    linkage.by_category[category].push_back(this);
    linkage.count++;
}

auto BasicSetting::Save() const -> bool { return save; }

auto BasicSetting::RuntimeModifiable() const -> bool { return runtime_modifiable; }
auto BasicSetting::GetCategory() const -> Category { return category; }

auto BasicSetting::Specialization() const -> u32 { return specialization; }
auto BasicSetting::PairedSetting() const -> BasicSetting* { return other_setting; }
auto BasicSetting::GetLabel() const -> const std::string& { return label; }
BasicSetting::~BasicSetting() = default;
}  // namespace settings