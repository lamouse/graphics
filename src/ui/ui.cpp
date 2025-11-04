#include "ui.hpp"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <format>
#include <charconv>
#include <array>
namespace {
inline auto GetFloatFormatFromSpeed(float speed) -> std::string {
    std::string str;
    std::array<char, 32> buffer{};
    auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), speed);

    if (result.ec == std::errc()) {
        str = std::string(buffer.data(), result.ptr);
    } else {
        str = std::to_string(speed);
    }

    auto dotPos = str.find('.');
    auto precision = (dotPos != std::string::npos) ? str.size() - dotPos - 1 : 0;

    return std::format("%.{}f", precision);
}
}  // namespace

void DrawVec3ColorControl(const std::string& label, glm::vec3& value, const glm::vec3& resetValue,
                          float speed, float columnWidth) {
    ImGui::PushID(label.c_str());

    if (ImGui::BeginTable("Vec3ControlTable", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, columnWidth);
        ImGui::TableSetupColumn("Values", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();

        // Label column
        ImGui::TableNextColumn();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9F, 0.9F, 0.9F, 1.0F));
        ImGui::AlignTextToFramePadding();
        float cellWidth = ImGui::GetContentRegionAvail().x;
        float textWidth = ImGui::CalcTextSize(label.c_str()).x;
        float offsetX = (cellWidth - textWidth) * 0.5F;

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
        // NOLINTNEXTLINE(hicpp-vararg)
        ImGui::Text("%s", label.c_str());
        ImGui::PopStyleColor();

        // Value column
        ImGui::TableNextColumn();
        std::string precision = GetFloatFormatFromSpeed(speed);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{4, 1});

        float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
        ImVec2 dragSize = {ImGui::CalcItemWidth() / 3.0F - 8.0F, lineHeight};

        // X
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8F, 0.1F, 0.15F, 1.0F));
        ImGui::SetNextItemWidth(dragSize.x);
        ImGui::DragFloat("##X", &value.x, speed, 0.0F, 0.0F, precision.c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine();

        // Y
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2F, 0.7F, 0.2F, 1.0F));
        ImGui::SetNextItemWidth(dragSize.x);
        ImGui::DragFloat("##Y", &value.y, speed, 0.0F, 0.0F, precision.c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine();

        // Z
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1F, 0.25F, 0.8F, 1.0F));
        ImGui::SetNextItemWidth(dragSize.x);
        ImGui::DragFloat("##Z", &value.z, speed, 0.0F, 0.0F, precision.c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine();

        // Reset button
        if (ImGui::Button("\uead2", ImVec2{lineHeight, lineHeight})) {
            value = resetValue;
        }

        ImGui::PopStyleVar();
        ImGui::EndTable();
    }

    ImGui::PopID();
}

void DrawFloatControl(const std::string& label, float& value, float speed, float resetValue,
                      float minValue, float maxValue, float labelWidth) {
    ImGui::PushID(label.c_str());

    if (ImGui::BeginTable("FloatControlTable", 2, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();

        // Label column
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        float cellWidth = ImGui::GetContentRegionAvail().x;
        float textWidth = ImGui::CalcTextSize(label.c_str()).x;
        float offsetX = (cellWidth - textWidth) * 0.5F;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
        // NOLINTNEXTLINE(hicpp-vararg)
        ImGui::Text("%s", label.c_str());

        // Value column
        ImGui::TableNextColumn();
        std::string format = GetFloatFormatFromSpeed(speed);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{4, 1});

        float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
        ImVec2 dragSize = {ImGui::CalcItemWidth() - lineHeight - 4.0F, lineHeight};

        // DragFloat
        ImGui::SetNextItemWidth(dragSize.x);
        ImGui::DragFloat("##Value", &value, speed, minValue, maxValue, format.c_str());
        ImGui::SameLine();

        // Reset button
        if (ImGui::Button("\uead2", ImVec2{lineHeight, lineHeight})) {
            value = resetValue;
        }

        ImGui::PopStyleVar();
        ImGui::EndTable();
    }

    ImGui::PopID();
}

void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0F);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

namespace graphics {}  // namespace graphics