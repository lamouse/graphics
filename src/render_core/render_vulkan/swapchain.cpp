#include <algorithm>

#include "swapchain.hpp"
#include "vulkan_common/device.hpp"
#include "common/settings.hpp"
#include <spdlog/spdlog.h>
#include <vulkan/vk_enum_string_helper.h>
#include "scheduler.hpp"
#include "vulkan_common/vulkan_wrapper.hpp"

namespace render::vulkan {
namespace {
auto chooseAlphaFlags(const vk::SurfaceCapabilitiesKHR& capabilities)
    -> vk::CompositeAlphaFlagBitsKHR {
    if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque) {
        return vk::CompositeAlphaFlagBitsKHR::eOpaque;
    }
    if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) {
        return vk::CompositeAlphaFlagBitsKHR::eInherit;
    }
    SPDLOG_ERROR("Unknown composite alpha flags value");
    return vk::CompositeAlphaFlagBitsKHR::eOpaque;
}
auto chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats)
    -> vk::SurfaceFormatKHR {
    if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined) {
        vk::SurfaceFormatKHR format;
        format.format = Swapchain::DEFAULT_COLOR_FORMAT;
        format.colorSpace = Swapchain::DEFAULT_COLOR_SPACE;
        return format;
    }
    const auto& found = std::ranges::find_if(formats, [](const auto& format) {
        return format.format == Swapchain::DEFAULT_COLOR_FORMAT &&
               format.colorSpace == Swapchain::DEFAULT_COLOR_SPACE;
    });
    return found != formats.end() ? *found : formats[0];
}
auto chooseSwapPresentMode(bool has_imm, bool has_mailbox, const bool has_fifo_relaxed)
    -> vk::PresentModeKHR {
    // Mailbox doesn't lock the application like FIFO (vsync)
    // FIFO present mode locks the framerate to the monitor's refresh rate
    settings::enums::VSyncMode setting = [has_imm, has_mailbox]() -> settings::enums::VSyncMode {
        // Choose Mailbox or Immediate if unlocked and those modes are supported

        switch (const auto mode = settings::values.vsync_mode.GetValue()) {
            case settings::enums::VSyncMode::Fifo:
            case settings::enums::VSyncMode::FifoRelaxed:
                if (has_mailbox) {
                    return settings::enums::VSyncMode::Mailbox;
                } else if (has_imm) {
                    return settings::enums::VSyncMode::Immediate;
                }
                [[fallthrough]];
            default:
                return mode;
        }
    }();
    if ((setting == settings::enums::VSyncMode::Mailbox && !has_mailbox) ||
        (setting == settings::enums::VSyncMode::Immediate && !has_imm) ||
        (setting == settings::enums::VSyncMode::FifoRelaxed && !has_fifo_relaxed)) {
        setting = settings::enums::VSyncMode::Fifo;
    }

    switch (setting) {
        case settings::enums::VSyncMode::Immediate:
            return ::vk::PresentModeKHR::eImmediate;
        case settings::enums::VSyncMode::Mailbox:
            return ::vk::PresentModeKHR::eMailbox;
        case settings::enums::VSyncMode::Fifo:
            return ::vk::PresentModeKHR::eFifo;
        case settings::enums::VSyncMode::FifoRelaxed:
            return ::vk::PresentModeKHR::eFifoRelaxed;
        default:
            return ::vk::PresentModeKHR::eFifo;
    }
}

auto chooseSwapExtent(const ::vk::SurfaceCapabilitiesKHR& capabilities, uint32_t width,
                      uint32_t height) -> ::vk::Extent2D {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    ::vk::Extent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                    capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                     capabilities.maxImageExtent.height);

    return actualExtent;
}

}  // namespace

Swapchain::Swapchain(vk::SurfaceKHR surface, const Device& device, scheduler::Scheduler& scheduler,
                     uint32_t width, uint32_t height)
    : surface_(surface), device_(device), scheduler_(scheduler) {
    create(surface, width, height);
}

/// Creates (or recreates) the swapchain with a given size.
void Swapchain::create(vk::SurfaceKHR surface, uint32_t width, uint32_t height) {
    is_outdated_ = false;
    is_suboptimal_ = false;
    width_ = width;
    height_ = height;
    surface_ = surface;

    const auto physical_device = device_.getPhysical();
    const auto capabilities{physical_device.getSurfaceCapabilitiesKHR(surface)};
    if (capabilities.maxImageExtent.width == 0 || capabilities.maxImageExtent.height == 0) {
        return;
    }
    destroy();
    createSwapchain(capabilities);
    createSemaphores();
    resource_ticks_.clear();
    resource_ticks_.resize(image_count_);
    spdlog::info("创建 swapchain w:{}, h:{}, present mode {}", width, height,
                 vk::to_string(present_mode_));
}

auto Swapchain::acquireNextImage() -> bool {
    const auto result = device_.getLogical().acquireNextImageKHR(
        *swapchain_, std::numeric_limits<uint64_t>::max(), *present_semaphores_[frame_index_],
        nullptr, &image_index_);
    switch (result) {
        case vk::Result::eSuccess:
            break;
        case vk::Result::eSuboptimalKHR:
            is_suboptimal_ = true;
            break;
        case vk::Result::eErrorOutOfDateKHR:
            is_outdated_ = true;
            break;
        case vk::Result::eErrorSurfaceLostKHR:
            utils::check(result);
            break;
        default:
            SPDLOG_ERROR("vkAcquireNextImageKHR returned {}",
                         string_VkResult(static_cast<VkResult>(result)));
            break;
    }
    return is_suboptimal_ || is_outdated_;
}

void Swapchain::createSwapchain(const vk::SurfaceCapabilitiesKHR& capabilities) {
    const auto physical_device{device_.getPhysical()};
    const auto formats{physical_device.getSurfaceFormatsKHR(surface_)};

    const vk::CompositeAlphaFlagBitsKHR alpha_flags{chooseAlphaFlags(capabilities)};
    surface_format_ = chooseSwapSurfaceFormat(formats);
    init_sync_mode();
    present_mode_ = chooseSwapPresentMode(has_imm_, has_mailbox_, has_fifo_relaxed_);

    uint32_t requested_image_count{capabilities.minImageCount + 1};
    // Ensure Triple buffering if possible.
    if (capabilities.maxImageCount > 0) {
        if (requested_image_count > capabilities.maxImageCount) {
            requested_image_count = capabilities.maxImageCount;
        } else {
            requested_image_count =
                std::max(requested_image_count, std::min(3U, capabilities.maxImageCount));
        }
    } else {
        requested_image_count = std::max(requested_image_count, 3U);
    }

    vk::SwapchainCreateInfoKHR createInfo =
        vk::SwapchainCreateInfoKHR()
            .setSurface(surface_)
            .setMinImageCount(requested_image_count)
            .setImageFormat(surface_format_.format)
            .setImageColorSpace(surface_format_.colorSpace)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment |
                           vk::ImageUsageFlagBits::eTransferDst)
            .setImageSharingMode(vk::SharingMode::eExclusive)
            .setPreTransform(capabilities.currentTransform)
            .setCompositeAlpha(alpha_flags)
            .setPresentMode(present_mode_)
            .setClipped(VK_FALSE);

    const uint32_t graphics_family{device_.getGraphicsFamily()};
    const uint32_t present_family{device_.getPresentFamily()};
    const std::array<uint32_t, 2> queue_indices{graphics_family, present_family};
    if (graphics_family != present_family) {
        createInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
            .setQueueFamilyIndices(queue_indices);
    }
    static constexpr std::array view_formats{::vk::Format::eB8G8R8A8Unorm,
                                             ::vk::Format::eB8G8R8A8Srgb};
    vk::ImageFormatListCreateInfo format_list{view_formats};
    if (device_.isKhrSwapchainMutableFormatEnabled()) {
        format_list.pNext = std::exchange(createInfo.pNext, &format_list);
        createInfo.flags |= vk::SwapchainCreateFlagBitsKHR::eMutableFormat;
    }
    // Request the size again to reduce the possibility of a TOCTOU race condition.
    const auto updated_capabilities = physical_device.getSurfaceCapabilitiesKHR(surface_);
    createInfo.imageExtent = chooseSwapExtent(updated_capabilities, width_, height_);
    // Don't add code within this and the swapchain creation.
    swapchain_ = device_.logical().createSwapchainKHR(createInfo);

    extent_ = createInfo.imageExtent;

    images_ = device_.getLogical().getSwapchainImagesKHR(*swapchain_);
    image_count_ = static_cast<uint32_t>(images_.size());
    image_view_format_ = DEFAULT_COLOR_FORMAT;
}

void Swapchain::init_sync_mode() {
    const auto physical_device{device_.getPhysical()};
    const auto present_modes = physical_device.getSurfacePresentModesKHR(surface_);
    has_mailbox_ =
        std::ranges::find(present_modes, ::vk::PresentModeKHR::eMailbox) != present_modes.end();
    has_imm_ =
        std::ranges::find(present_modes, ::vk::PresentModeKHR::eImmediate) != present_modes.end();
    has_fifo_relaxed_ =
        std::ranges::find(present_modes, ::vk::PresentModeKHR::eFifo) != present_modes.end();
}

void Swapchain::destroy() {
    device_.getLogical().waitIdle();
    frame_index_ = 0;
    present_semaphores_.clear();
    swapchain_.reset();
}
void Swapchain::createSemaphores() {
    present_semaphores_.resize(image_count_);
    std::ranges::generate(present_semaphores_,
                          [this] { return device_.logical().createSemaphore(); });
    render_semaphores_.resize(image_count_);
    std::ranges::generate(render_semaphores_,
                          [this] { return device_.logical().createSemaphore(); });
}

void Swapchain::present(vk::Semaphore render_semaphore) {
    const auto present_queue = device_.getPresentQueue();
    vk::PresentInfoKHR present_info{};
    present_info.setWaitSemaphores(render_semaphore)
        .setSwapchainCount(1)
        .setPSwapchains(swapchain_.address())
        .setImageIndices(image_index_);

    try {
        switch (const vk::Result result = present_queue.presentKHR(&present_info)) {
            case vk::Result::eSuccess:
                break;
            case vk::Result::eSuboptimalKHR:
                SPDLOG_DEBUG("Suboptimal swapchain");
                break;
            case vk::Result::eErrorOutOfDateKHR:
                is_outdated_ = true;
                break;
            case vk::Result::eErrorSurfaceLostKHR:
                throw utils::VulkanException(result);
                break;
            default:
                SPDLOG_CRITICAL("Failed to present with error {}",
                                string_VkResult(static_cast<VkResult>(result)));
                break;
        }
        ++frame_index_;
        frame_index_ %= static_cast<uint32_t>(image_count_);
    } catch (const ::vk::OutOfDateKHRError&) {
        is_outdated_ = true;
    }
}

auto Swapchain::needsPresentModeUpdate() const -> bool {
    const auto requested_mode = chooseSwapPresentMode(has_imm_, has_mailbox_, has_fifo_relaxed_);
    return present_mode_ != requested_mode;
}
}  // namespace render::vulkan