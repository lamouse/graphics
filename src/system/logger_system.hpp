#pragma once
#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/async_logger.h>
#include <ranges>
#include "common/settings.hpp"
#include <mutex>
#include <deque>
#include <imgui.h>

#include <absl/strings/match.h>

namespace sys {
template <typename Mutex>
class ImGuiLogSink : public spdlog::sinks::base_sink<Mutex> {
    public:
        void draw(const char* title, bool& popen) {
            if (!popen) {
                return;
            }
            ImGui::Begin(title, &popen);

            std::lock_guard lock(mutex_);
            ImGui::BeginTable("Options", 2);
            ImGui::TableNextColumn();
            ImGui::Checkbox("Auto-scroll", &auto_scroll_);

            auto canon =
                settings::enums::EnumMetadata<settings::enums::LogLevel>::canonicalizations();
            std::vector<const char*> names;
            for (auto& key : canon | std::views::keys) {
                names.push_back(key.c_str());
            }

            static int item_current = static_cast<int>(settings::values.log_level.GetValue());
            ImGui::TableNextColumn();
            ImGui::Combo("log level", &item_current, names.data(), static_cast<int>(names.size()));
            ImGui::EndTable();
            const auto level =
                settings::enums::ToEnum<settings::enums::LogLevel>(names[item_current]);
            settings::values.log_level.SetValue(level);
            ImGui::Separator();

            ImGui::BeginChild("LogRegion", ImVec2(0, 0), false,
                              ImGuiWindowFlags_HorizontalScrollbar);
            for (const auto& line : lines_) {
                ImVec4 color = ImVec4(1, 1, 1, 1);  // 默认白色

                if (absl::StrContains(line, "[error]") || absl::StrContains(line, "ERROR")) {
                    color = ImVec4(1.0F, 0.4F, 0.4F, 1.0F);  // 红色
                } else if (absl::StrContains(line, "[warn]") ||
                           absl::StrContains(line, "WARNING") ||
                           absl::StrContains(line, "warning")) {
                    if (item_current > static_cast<int>(settings::enums::LogLevel::warn)) {
                        continue;  // 过滤掉非当前级别的日志
                    }
                    color = ImVec4(1.0F, 1.0F, 0.4F, 1.0F);  // 黄色

                } else if (absl::StrContains(line, "[info]") || absl::StrContains(line, "INFO")) {
                    if (item_current > static_cast<int>(settings::enums::LogLevel::info)) {
                        continue;  // 过滤掉非当前级别的日志
                    }
                    color = ImVec4(0.6F, 1.0F, 0.6F, 1.0F);  // 绿色

                } else if (absl::StrContains(line, "[debug]") || absl::StrContains(line, "DEBUG")) {
                    color = ImVec4(0.6F, 0.8F, 1.0F, 1.0F);  // 蓝色
                    if (item_current > static_cast<int>(settings::enums::LogLevel::debug)) {
                        continue;  // 过滤掉非当前级别的日志
                    }
                }

                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextUnformatted(line.c_str());
                ImGui::PopStyleColor();
            }
            if (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                ImGui::SetScrollHereY(1.0F);
            }
            ImGui::EndChild();
            ImGui::End();
        }

    protected:
        void sink_it_(const spdlog::details::log_msg& msg) override {
            std::lock_guard lock(mutex_);
            spdlog::memory_buf_t formatted;
            this->formatter_->format(msg, formatted);
            lines_.emplace_back(fmt::to_string(formatted));
            if (lines_.size() > 1000) {
                lines_.pop_front();  // 限制最大行数
            }
        }
        void flush_() override {}

    private:
        std::mutex mutex_;
        std::deque<std::string> lines_;
        bool auto_scroll_ = true;
};
using ImGuiLogSink_mt = ImGuiLogSink<std::mutex>;
class LoggerSystem {
    public:
        explicit LoggerSystem();
        LoggerSystem(const LoggerSystem&) = default;
        LoggerSystem(LoggerSystem&&) noexcept = default;
        auto operator=(const LoggerSystem&) -> LoggerSystem& = default;
        auto operator=(LoggerSystem&&) noexcept -> LoggerSystem& = default;
        ~LoggerSystem();
        void drawUi(bool& show);

    private:
        spdlog::level::level_enum log_level_ = spdlog::level::info;
        std::shared_ptr<ImGuiLogSink_mt> imgui_sink;
        std::shared_ptr<spdlog::async_logger> logger_;
};
}  // namespace sys