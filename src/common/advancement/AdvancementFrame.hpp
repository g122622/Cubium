#pragma once

#include "common/core/Types.hpp"
#include <string>

namespace mc::advancement {

/**
 * @brief 成就框架类型
 *
 * 决定成就在UI中的显示样式。
 * 参考 MC 1.16.5: net.minecraft.advancements.FrameType
 */
enum class AdvancementFrame : u8 {
    Task,       ///< 任务（普通），绿色标题
    Challenge,  ///< 挑战，紫色标题，完成时有特殊音效
    Goal        ///< 目标，绿色标题，与Task相似但图标不同
};

/**
 * @brief 将框架类型转换为字符串
 * @param frame 框架类型
 * @return 字符串表示
 */
inline constexpr const char* toString(AdvancementFrame frame) noexcept {
    switch (frame) {
        case AdvancementFrame::Task: return "task";
        case AdvancementFrame::Challenge: return "challenge";
        case AdvancementFrame::Goal: return "goal";
    }
    return "task";
}

/**
 * @brief 从字符串解析框架类型
 * @param str 字符串
 * @return 框架类型，解析失败返回 Task
 */
inline AdvancementFrame parseFrame(const std::string& str) noexcept {
    if (str == "challenge") return AdvancementFrame::Challenge;
    if (str == "goal") return AdvancementFrame::Goal;
    return AdvancementFrame::Task;
}

} // namespace mc::advancement
