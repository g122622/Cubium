/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/resource/ResourceLocation.hpp"
#include <string>
#include <utility>
#include <vector>

namespace mc::client::renderer::trident::gui {

/**
 * @brief MC 1.21+ 独立精灵文件路径映射
 *
 * MC 1.21+ 资源包将 icons.png / widgets.png 拆分为独立精灵 PNG 文件，
 * 存放在 textures/gui/sprites/ 目录下。
 * 此映射表将旧版精灵ID（如 "heart_full"）映射到新格式的资源路径。
 *
 * 映射格式：{ 旧精灵ID, ResourceLocation(命名空间, 精灵路径) }
 * 精灵路径不含 "textures/" 前缀和 ".png" 后缀，与 TextureAtlasBuilder 的约定一致。
 */

/**
 * @brief HUD精灵映射（对应旧版 icons.png）
 *
 * 包括心形、盔甲、饥饿、经验条、气泡、准星、快捷栏等精灵。
 * MC 1.21+ 中这些精灵位于 textures/gui/sprites/hud/ 目录下。
 */
const std::vector<std::pair<std::string, ResourceLocation>> HUD_SPRITE_MAPPINGS = {
    // 心形 - 普通状态
    {"heart_full", ResourceLocation("minecraft", "textures/gui/sprites/hud/heart/full")},
    {"heart_half", ResourceLocation("minecraft", "textures/gui/sprites/hud/heart/half")},
    {"heart_empty", ResourceLocation("minecraft", "textures/gui/sprites/hud/heart/container")},

    // 心形 - 极限模式
    {"heart_full_hardcore", ResourceLocation("minecraft", "textures/gui/sprites/hud/heart/hardcore_full")},
    {"heart_half_hardcore", ResourceLocation("minecraft", "textures/gui/sprites/hud/heart/hardcore_half")},
    {"heart_empty_hardcore", ResourceLocation("minecraft", "textures/gui/sprites/hud/heart/container_hardcore")},

    // 心形 - 吸收效果
    {"heart_absorbing_full", ResourceLocation("minecraft", "textures/gui/sprites/hud/heart/absorbing_full")},
    {"heart_absorbing_half", ResourceLocation("minecraft", "textures/gui/sprites/hud/heart/absorbing_half")},

    // 心形 - 中毒效果
    {"heart_poisoned_full", ResourceLocation("minecraft", "textures/gui/sprites/hud/heart/poisoned_full")},
    {"heart_poisoned_half", ResourceLocation("minecraft", "textures/gui/sprites/hud/heart/poisoned_half")},
    // MC 1.21+ 无 poisoned_empty，用 container 近似
    {"heart_poisoned_empty", ResourceLocation("minecraft", "textures/gui/sprites/hud/heart/poisoned_full")},

    // 心形 - 凋零效果
    {"heart_wither_full", ResourceLocation("minecraft", "textures/gui/sprites/hud/heart/withered_full")},
    {"heart_wither_half", ResourceLocation("minecraft", "textures/gui/sprites/hud/heart/withered_half")},
    // MC 1.21+ 无 withered_empty，用 withered_full 近似
    {"heart_wither_empty", ResourceLocation("minecraft", "textures/gui/sprites/hud/heart/withered_full")},

    // 盔甲
    {"armor_empty", ResourceLocation("minecraft", "textures/gui/sprites/hud/armor_empty")},
    {"armor_half", ResourceLocation("minecraft", "textures/gui/sprites/hud/armor_half")},
    {"armor_full", ResourceLocation("minecraft", "textures/gui/sprites/hud/armor_full")},

    // 饥饿 - 普通状态
    {"hunger_empty", ResourceLocation("minecraft", "textures/gui/sprites/hud/food_empty")},
    {"hunger_half", ResourceLocation("minecraft", "textures/gui/sprites/hud/food_half")},
    {"hunger_full", ResourceLocation("minecraft", "textures/gui/sprites/hud/food_full")},

    // 饥饿 - 饱和效果（hunger 变体）
    {"hunger_saturated_empty", ResourceLocation("minecraft", "textures/gui/sprites/hud/food_empty_hunger")},
    {"hunger_saturated_half", ResourceLocation("minecraft", "textures/gui/sprites/hud/food_half_hunger")},
    {"hunger_saturated_full", ResourceLocation("minecraft", "textures/gui/sprites/hud/food_full_hunger")},

    // 经验条
    {"xp_bar_empty", ResourceLocation("minecraft", "textures/gui/sprites/hud/experience_bar_background")},
    {"xp_bar_full", ResourceLocation("minecraft", "textures/gui/sprites/hud/experience_bar_progress")},

    // 跳跃条（马匹）
    {"jump_bar_background", ResourceLocation("minecraft", "textures/gui/sprites/hud/jump_bar_background")},
    {"jump_bar_foreground", ResourceLocation("minecraft", "textures/gui/sprites/hud/jump_bar_progress")},

    // 气泡（水下呼吸）
    {"bubble_empty", ResourceLocation("minecraft", "textures/gui/sprites/hud/air_empty")},
    {"bubble_full", ResourceLocation("minecraft", "textures/gui/sprites/hud/air")},

    // 准星
    {"crosshair", ResourceLocation("minecraft", "textures/gui/sprites/hud/crosshair")},

    // 攻击指示器 - 准星模式（AttackIndicatorStatus.CROSSHAIR）
    // 位于准星下方，显示武器冷却进度
    // full: 武器完全充能且可暴击时显示（16x16）
    // background: 冷却中背景条（16x4）
    // progress: 冷却中进度条（16x4，从左向右填充）
    {"crosshair_attack_indicator_full",
        ResourceLocation("minecraft", "textures/gui/sprites/hud/crosshair_attack_indicator_full")},
    {"crosshair_attack_indicator_background",
        ResourceLocation("minecraft", "textures/gui/sprites/hud/crosshair_attack_indicator_background")},
    {"crosshair_attack_indicator_progress",
        ResourceLocation("minecraft", "textures/gui/sprites/hud/crosshair_attack_indicator_progress")},

    // 攻击指示器 - 快捷栏模式（AttackIndicatorStatus.HOTBAR）
    // 位于快捷栏副手槽对面，显示武器冷却进度
    // background: 冷却中背景（18x18）
    // progress: 冷却中进度（18x18，从底部向上填充）
    {"hotbar_attack_indicator_background",
        ResourceLocation("minecraft", "textures/gui/sprites/hud/hotbar_attack_indicator_background")},
    {"hotbar_attack_indicator_progress",
        ResourceLocation("minecraft", "textures/gui/sprites/hud/hotbar_attack_indicator_progress")},

    // 快捷栏（MC 1.21+ 中 hotbar 位于 hud/ 目录下）
    {"hotbar_bg", ResourceLocation("minecraft", "textures/gui/sprites/hud/hotbar")},
    {"hotbar_selection", ResourceLocation("minecraft", "textures/gui/sprites/hud/hotbar_selection")},
    {"hotbar_offhand_left", ResourceLocation("minecraft", "textures/gui/sprites/hud/hotbar_offhand_left")},
    {"hotbar_offhand_right", ResourceLocation("minecraft", "textures/gui/sprites/hud/hotbar_offhand_right")},
};

/**
 * @brief Widget精灵映射（对应旧版 widgets.png）
 *
 * MC 1.21+ 中这些精灵位于 textures/gui/sprites/widget/ 目录下。
 */
const std::vector<std::pair<std::string, ResourceLocation>> WIDGET_SPRITE_MAPPINGS = {
    // 按钮
    {"button_disabled", ResourceLocation("minecraft", "textures/gui/sprites/widget/button_disabled")},
    {"button_normal", ResourceLocation("minecraft", "textures/gui/sprites/widget/button")},
    {"button_hover", ResourceLocation("minecraft", "textures/gui/sprites/widget/button_highlighted")},

    // 滑动条（AbstractSliderButton）
    // slider/slider_highlighted: 200x20 轨道背景，nine_slice border=1
    // slider_handle/slider_handle_highlighted: 8x20 手柄，nine_slice border(2,2,2,3)
    {"slider", ResourceLocation("minecraft", "textures/gui/sprites/widget/slider")},
    {"slider_highlighted", ResourceLocation("minecraft", "textures/gui/sprites/widget/slider_highlighted")},
    {"slider_handle", ResourceLocation("minecraft", "textures/gui/sprites/widget/slider_handle")},
    {"slider_handle_highlighted",
        ResourceLocation("minecraft", "textures/gui/sprites/widget/slider_handle_highlighted")},
};

} // namespace mc::client::renderer::trident::gui
