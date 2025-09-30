#pragma once
#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/base_sink.h>
#include <mutex>
#include <deque>
#include <imgui.h>
#include "absl/strings/match.h"
#include "resource/config.hpp"
#include "common/common_funcs.hpp"

namespace sys {
template <typename Mutex>
class ImGuiLogSink : public spdlog::sinks::base_sink<Mutex> {
    public:
        void draw(const char* title) {
            ImGui::Begin(title);

            std::lock_guard lock(mutex_);
            ImGui::Checkbox("Auto-scroll", &auto_scroll_);
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
                    color = ImVec4(1.0F, 1.0F, 0.4F, 1.0F);  // 黄色
                } else if (absl::StrContains(line, "[info]") || absl::StrContains(line, "INFO")) {
                    color = ImVec4(0.6F, 1.0F, 0.6F, 1.0F);  // 绿色
                } else if (absl::StrContains(line, "[debug]") || absl::StrContains(line, "DEBUG")) {
                    color = ImVec4(0.6F, 0.8F, 1.0F, 1.0F);  // 蓝色
                }

                ImGui::PushStyleColor(ImGuiCol_Text, color);

                // 保证每行至少显示 120 字符宽度（填充空格）
                std::string padded = line;
                if (padded.length() < 120) {
                    padded.append(120 - padded.length(), ' ');
                }

                ImGui::TextUnformatted(padded.c_str());

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
        explicit LoggerSystem(const g::Config& config);
        CLASS_NON_COPYABLE(LoggerSystem);
        CLASS_DEFAULT_MOVEABLE(LoggerSystem);
        ~LoggerSystem() = default;
        void drawUi(bool show);

    private:
        std::shared_ptr<ImGuiLogSink_mt> imgui_sink;
};
}  // namespace sys