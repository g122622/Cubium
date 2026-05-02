#pragma once

#include "common/core/Types.hpp"

namespace mc::client::ui::minecraft {

/**
 * @brief UI 常量
 *
 * 菜单屏幕共用的 UI 常量定义。
 */
namespace UiConstants {

/// 标准按钮宽度
static constexpr i32 BUTTON_WIDTH = 200;

/// 标准按钮高度
static constexpr i32 BUTTON_HEIGHT = 20;

/// 按钮间距
static constexpr i32 BUTTON_SPACING = 4;

/// 标题 Y 偏移
static constexpr i32 TITLE_Y_OFFSET = 60;

/// 按钮 Y 起始位置
static constexpr i32 BUTTON_Y_START = 120;

/// 小按钮宽度
static constexpr i32 SMALL_BUTTON_WIDTH = 100;

/// 中等按钮宽度
static constexpr i32 MEDIUM_BUTTON_WIDTH = 150;

/// 表单标签宽度
static constexpr i32 LABEL_WIDTH = 120;

/// 表单输入框宽度
static constexpr i32 FIELD_WIDTH = 200;

/// 表单行高
static constexpr i32 ROW_HEIGHT = 30;

} // namespace UiConstants

} // namespace mc::client::ui::minecraft
