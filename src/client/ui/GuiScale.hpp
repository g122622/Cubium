#pragma once

#include "common/core/Types.hpp"

namespace mc::client::ui {

/**
 * @brief GUI 缩放计算结果
 *
 * scaleFactor 表示最终用于渲染和输入换算的缩放倍率。
 * width 和 height 表示缩放后的逻辑 GUI 分辨率。
 */
struct GuiScaleState {
    i32 scaleFactor = 1;
    i32 width = 0;
    i32 height = 0;
};

/**
 * @brief 计算 GUI 缩放状态
 *
 * 规则接近 Minecraft 1.21：
 * - 0 表示自动缩放
 * - 1 到 4 表示手动指定缩放
 * - 实际缩放不会让逻辑分辨率低于 320x240
 * - 结果会被限制在 1 到 4 之间
 *
 * @param requestedScale 用户设置的 GUI 缩放值
 * @param windowWidth 窗口宽度
 * @param windowHeight 窗口高度
 * @return GUI 缩放状态
 */
[[nodiscard]] GuiScaleState calculateGuiScale(i32 requestedScale, i32 windowWidth, i32 windowHeight);

} // namespace mc::client::ui