#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <VkBootstrap.h>

#include "engine_logger.h"

namespace srtv_engine::utils {

#define CHECK_VK_ERROR(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            ENGINE_LOG_ERROR("Detected Vulkan error: " + string_VkResult(err));; \
        }                                                               \
    } while (0)

#define CHECK_VK_FATAL_ERROR(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            ENGINE_LOG_FATAL("Detected Vulkan fatal error: " + string_VkResult(err));; \
            abort();                                                    \
        }                                                               \
    } while (0)

#define CHECK_VKB_ERROR(x)                                                     \
    do {                                                                \
        auto err = x;                                               \
        if (!err) {                                                      \
            ENGINE_LOG_ERROR("Detected Vulkan Bootstrap error: " + err.error().message()); \
        }                                                               \
    } while (0)

#define CHECK_VKB_FATAL_ERROR(x)                                                     \
    do {                                                                \
        auto err = x;                                               \
        if (!err) {                                                      \
            if (err.error()  == vkb::PhysicalDeviceError::no_suitable_device) {     \
                auto& detailed_reasons = err.detailed_failure_reasons();      \
                if (!detailed_reasons.empty()) {                                                               \
                    ENGINE_LOG_ERROR("GPU Selection failure reasons: ");                                 \
                    for (const std::string& reason : detailed_reasons) {                                        \
                        ENGINE_LOG_ERROR(reason);                                                         \
                    }                                                                                      \
                }                                                                                           \
            }                                                                                                \
            ENGINE_LOG_FATAL("Detected Vulkan Bootstrap fatal error: " + err.error().message()); \
        }                                                               \
    } while (0)

} // namespace srtv_engine::utils