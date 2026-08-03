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

#include "GuiSpriteRegistry.hpp"
#include "GuiSpriteAtlas.hpp"
#include "GuiSpriteManager.hpp"
#include "GuiTextureAtlas.hpp"
#include "client/renderer/trident/gui/GuiSprite.hpp"
#include "common/core/Types.hpp"
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::trident::gui {

// ============================================================================
// GuiSpriteManager 重载（不依赖Vulkan）
// ============================================================================

void GuiSpriteRegistry::registerWidgetsSprites(GuiSpriteManager& manager, i32 atlasWidth, i32 atlasHeight)
{
    // 按钮精灵（宽度200，高度20）
    // Y坐标：禁用46，正常66，悬停86
    manager.registerSprite("button_disabled", 0, 46, 200, 20, atlasWidth, atlasHeight);
    manager.registerSprite("button_normal", 0, 66, 200, 20, atlasWidth, atlasHeight);
    manager.registerSprite("button_hover", 0, 86, 200, 20, atlasWidth, atlasHeight);

    // 按钮九宫格（边距各4像素）
    if (auto* sprite = manager.getSprite("button_disabled")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(4, 4, 196, 16);
    }
    if (auto* sprite = manager.getSprite("button_normal")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(4, 4, 196, 16);
    }
    if (auto* sprite = manager.getSprite("button_hover")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(4, 4, 196, 16);
    }

    // 快捷栏背景 (0, 0) 182x22
    manager.registerSprite("hotbar_bg", 0, 0, 182, 22, atlasWidth, atlasHeight);

    // 快捷栏选中高亮 (0, 22) 24x22
    manager.registerSprite("hotbar_selection", 0, 22, 24, 22, atlasWidth, atlasHeight);

    // 副手槽位 (24, 22) 29x24 和 (53, 22) 29x24
    manager.registerSprite("hotbar_offhand_left", 24, 22, 29, 24, atlasWidth, atlasHeight);
    manager.registerSprite("hotbar_offhand_right", 53, 22, 29, 24, atlasWidth, atlasHeight);

    // 滑动条精灵（MC 1.21+ 独立精灵，旧版 widgets.png 中复用按钮纹理区域）
    // slider/slider_highlighted: 200x20 轨道背景，九宫格边距1像素
    // slider_handle/slider_handle_highlighted: 8x20 手柄，九宫格边距(2,2,2,3)
    // 旧版回退坐标：slider 复用 button_normal/button_hover 区域，handle 取按钮最左侧8像素
    // TODO(consumer): 滑动条精灵目前尚无消费者。Cubium 现有的 kagero::SliderWidget
    // （src/client/ui/kagero/widget/SliderWidget.hpp）使用纯色 drawFilledRect 绘制，
    // 未使用本精灵。MC 原版中由 AbstractSliderButton（net/minecraft/client/gui/components/
    // AbstractSliderButton.java）通过 blitSprite 消费这4个精灵。未来实现 MC 风格的
    // AbstractSliderButton 组件（用于选项屏幕如视频设置、音量滑块等）时，应在此接入：
    // 1. 创建 AbstractSliderButton 组件（参考 MC 源码），renderWidget 中调用
    //    GuiSpriteAtlas::createTextureImage("slider"/"slider_highlighted") 绘制轨道背景，
    //    createTextureImage("slider_handle"/"slider_handle_highlighted") 绘制手柄；
    // 2. 根据 isHovered/isFocused/isActive 选择 normal/highlighted 变体（参考 MC getSprite/
    //    getHandleSprite）；3. 手柄 X = getX() + (int)(value * (width - 8))。在消费者实现
    //    之前，这些精灵注册仅保证图集布局就绪，不构成功能闭环。
    manager.registerSprite("slider", 0, 66, 200, 20, atlasWidth, atlasHeight);
    manager.registerSprite("slider_highlighted", 0, 86, 200, 20, atlasWidth, atlasHeight);
    manager.registerSprite("slider_handle", 0, 66, 8, 20, atlasWidth, atlasHeight);
    manager.registerSprite("slider_handle_highlighted", 0, 86, 8, 20, atlasWidth, atlasHeight);

    // 滑动条九宫格
    if (auto* sprite = manager.getSprite("slider")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(1, 1, 199, 19);
    }
    if (auto* sprite = manager.getSprite("slider_highlighted")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(1, 1, 199, 19);
    }
    if (auto* sprite = manager.getSprite("slider_handle")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(2, 2, 6, 17);
    }
    if (auto* sprite = manager.getSprite("slider_handle_highlighted")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(2, 2, 6, 17);
    }
}

void GuiSpriteRegistry::registerIconsSprites(GuiSpriteManager& manager, i32 atlasWidth, i32 atlasHeight)
{
    // ========== 心形图标 (9x9) ==========
    // 基础X坐标：满心52，半心61，空心16
    // Y坐标：正常0， Hardcore（困难模式）+45

    // 正常心形
    manager.registerSprite("heart_full", 52, 0, 9, 9, atlasWidth, atlasHeight);
    manager.registerSprite("heart_half", 61, 0, 9, 9, atlasWidth, atlasHeight);
    manager.registerSprite("heart_empty", 16, 0, 9, 9, atlasWidth, atlasHeight);

    // Hardcore心形（Y+45）
    manager.registerSprite("heart_full_hardcore", 52, 45, 9, 9, atlasWidth, atlasHeight);
    manager.registerSprite("heart_half_hardcore", 61, 45, 9, 9, atlasWidth, atlasHeight);
    manager.registerSprite("heart_empty_hardcore", 16, 45, 9, 9, atlasWidth, atlasHeight);

    // 吸收心（黄色）- X+36
    manager.registerSprite("heart_absorbing_full", 88, 0, 9, 9, atlasWidth, atlasHeight);
    manager.registerSprite("heart_absorbing_half", 97, 0, 9, 9, atlasWidth, atlasHeight);

    // 中毒心（绿色）- X+36 基础偏移
    manager.registerSprite("heart_poisoned_full", 88, 0, 9, 9, atlasWidth, atlasHeight);
    manager.registerSprite("heart_poisoned_half", 97, 0, 9, 9, atlasWidth, atlasHeight);
    manager.registerSprite("heart_poisoned_empty", 16, 0, 9, 9, atlasWidth, atlasHeight);

    // 凋零心（黑色）- X+72 基础偏移
    manager.registerSprite("heart_wither_full", 124, 0, 9, 9, atlasWidth, atlasHeight);
    manager.registerSprite("heart_wither_half", 133, 0, 9, 9, atlasWidth, atlasHeight);
    manager.registerSprite("heart_wither_empty", 16, 0, 9, 9, atlasWidth, atlasHeight);

    // ========== 盔甲图标 (9x9) ==========
    // X坐标：空16，半25，满34
    // Y坐标：9
    manager.registerSprite("armor_empty", 16, 9, 9, 9, atlasWidth, atlasHeight);
    manager.registerSprite("armor_half", 25, 9, 9, 9, atlasWidth, atlasHeight);
    manager.registerSprite("armor_full", 34, 9, 9, 9, atlasWidth, atlasHeight);

    // ========== 饥饿图标 (9x9) ==========
    // X坐标：空16，半25，满34
    // Y坐标：27
    // 饱和效果：Y+3 -> 30

    manager.registerSprite("hunger_empty", 16, 27, 9, 9, atlasWidth, atlasHeight);
    manager.registerSprite("hunger_half", 25, 27, 9, 9, atlasWidth, atlasHeight);
    manager.registerSprite("hunger_full", 34, 27, 9, 9, atlasWidth, atlasHeight);

    // 饱和效果饥饿图标
    manager.registerSprite("hunger_saturated_empty", 16, 30, 9, 9, atlasWidth, atlasHeight);
    manager.registerSprite("hunger_saturated_half", 25, 30, 9, 9, atlasWidth, atlasHeight);
    manager.registerSprite("hunger_saturated_full", 34, 30, 9, 9, atlasWidth, atlasHeight);

    // ========== 经验条 (182x5) ==========
    // 空条：Y=64，满条：Y=69
    manager.registerSprite("xp_bar_empty", 0, 64, 182, 5, atlasWidth, atlasHeight);
    manager.registerSprite("xp_bar_full", 0, 69, 182, 5, atlasWidth, atlasHeight);

    // ========== 跳跃条 (182x5) ==========
    // 用于马匹跳跃
    manager.registerSprite("jump_bar_background", 0, 84, 182, 5, atlasWidth, atlasHeight);
    manager.registerSprite("jump_bar_foreground", 0, 89, 182, 5, atlasWidth, atlasHeight);

    // ========== 气泡 (9x9) ==========
    // X坐标：空16，满25
    // Y坐标：18
    manager.registerSprite("bubble_empty", 16, 18, 9, 9, atlasWidth, atlasHeight);
    manager.registerSprite("bubble_full", 25, 18, 9, 9, atlasWidth, atlasHeight);

    // ========== 准星 (15x15) ==========
    // 位于(0, 0)
    manager.registerSprite("crosshair", 0, 0, 15, 15, atlasWidth, atlasHeight);

    // ========== 攻击指示器 ==========
    // 旧版 icons.png 中位于 Y=94 行
    // 准星模式：full(16x16) 在(68,94)，background(16x4) 在(36,94)，progress(16x4) 在(52,94)
    // 快捷栏模式：background(18x18) 在(0,94)，progress(18x18) 在(18,94)
    // TODO(consumer): 攻击指示器精灵目前尚无消费者。MC 原版中由 Gui.renderCrosshair()
    // （准星模式，net/minecraft/client/gui/Gui.java:432）和 Gui.renderHotbar()（快捷栏
    // 模式，同文件:594）根据 options.attackIndicator() 的值（CROSSHAIR/HOTBAR）消费：
    // - 准星模式：f = player.getAttackStrengthScale(0.0F)；f>=1.0F 且当前目标是 LivingEntity
    //   且 currentItemAttackStrengthDelay>5 时绘制 full（暴击指示）；f<1.0F 时绘制 background
    //   + 裁剪宽度为 (int)(f*17) 的 progress。
    // - 快捷栏模式：f<1.0F 时在副手槽对面绘制 background + 裁剪高度为 (int)(f*19) 的 progress
    //   （从底部向上填充）。
    // Cubium 现状：CrosshairWidget（src/client/ui/minecraft/widgets/CrosshairWidget.cpp）使用
    //   纯色十字线绘制准星，未渲染攻击指示器；HudWidget（同目录）未渲染攻击指示器；
    //   且尚无 AttackIndicatorStatus 选项枚举。Player::getCooledAttackStrength() 已实现
    //   （src/common/entity/entities/player/Player.hpp:1690），可作为冷却进度数据源。
    // 未来接入步骤：1. 在客户端选项中新增 AttackIndicatorStatus 枚举（NONE/CROSSHAIR/HOTBAR）；
    // 2. CrosshairWidget 增加攻击指示器渲染分支（准星下方16px处，参考 MC Gui.java:442-450）；
    // 3. HudWidget._renderHotbar() 增加攻击指示器渲染分支（副手槽对面，参考 MC Gui.java:594-606）；
    // 4. 需访问 crosshairPickEntity 判断 full 暴击指示条件。在消费者实现之前，这些精灵注册
    // 仅保证图集布局就绪，不构成功能闭环。
    manager.registerSprite("crosshair_attack_indicator_full", 68, 94, 16, 16, atlasWidth, atlasHeight);
    manager.registerSprite("crosshair_attack_indicator_background", 36, 94, 16, 4, atlasWidth, atlasHeight);
    manager.registerSprite("crosshair_attack_indicator_progress", 52, 94, 16, 4, atlasWidth, atlasHeight);
    manager.registerSprite("hotbar_attack_indicator_background", 0, 94, 18, 18, atlasWidth, atlasHeight);
    manager.registerSprite("hotbar_attack_indicator_progress", 18, 94, 18, 18, atlasWidth, atlasHeight);
}

void GuiSpriteRegistry::registerContainerSprites(GuiSpriteManager& manager, i32 atlasWidth, i32 atlasHeight)
{
    // 槽位背景 (18x18)
    // 这是程序生成的默认槽位，实际纹理来自各容器纹理
    manager.registerSprite("slot_background", 0, 0, 18, 18, atlasWidth, atlasHeight);

    // 容器背景 (176x166) - 默认尺寸
    // 实际纹理来自 container/*.png
    manager.registerSprite("container_background", 0, 0, 176, 166, atlasWidth, atlasHeight);

    // 物品栏背景（玩家背包）
    manager.registerSprite("inventory_background", 0, 0, 176, 166, atlasWidth, atlasHeight);
}

void GuiSpriteRegistry::registerAllDefaults(GuiSpriteManager& manager, i32 atlasWidth, i32 atlasHeight)
{
    registerWidgetsSprites(manager, atlasWidth, atlasHeight);
    registerIconsSprites(manager, atlasWidth, atlasHeight);
    registerContainerSprites(manager, atlasWidth, atlasHeight);
}

// ============================================================================
// GuiTextureAtlas 重载（依赖Vulkan）
// ============================================================================

void GuiSpriteRegistry::registerWidgetsSprites(GuiTextureAtlas& atlas, i32 atlasWidth, i32 atlasHeight)
{
    // 按钮精灵（宽度200，高度20）
    // Y坐标：禁用46，正常66，悬停86
    atlas.registerSprite("button_disabled", 0, 46, 200, 20, atlasWidth, atlasHeight);
    atlas.registerSprite("button_normal", 0, 66, 200, 20, atlasWidth, atlasHeight);
    atlas.registerSprite("button_hover", 0, 86, 200, 20, atlasWidth, atlasHeight);

    // 按钮九宫格（边距各4像素）
    if (auto* sprite = atlas.getSprite("button_disabled")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(4, 4, 196, 16);
    }
    if (auto* sprite = atlas.getSprite("button_normal")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(4, 4, 196, 16);
    }
    if (auto* sprite = atlas.getSprite("button_hover")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(4, 4, 196, 16);
    }

    // 快捷栏背景 (0, 0) 182x22
    atlas.registerSprite("hotbar_bg", 0, 0, 182, 22, atlasWidth, atlasHeight);

    // 快捷栏选中高亮 (0, 22) 24x22
    atlas.registerSprite("hotbar_selection", 0, 22, 24, 22, atlasWidth, atlasHeight);

    // 副手槽位 (24, 22) 29x24 和 (53, 22) 29x24
    atlas.registerSprite("hotbar_offhand_left", 24, 22, 29, 24, atlasWidth, atlasHeight);
    atlas.registerSprite("hotbar_offhand_right", 53, 22, 29, 24, atlasWidth, atlasHeight);

    // 滑动条精灵（MC 1.21+ 独立精灵，旧版 widgets.png 中复用按钮纹理区域）
    // slider/slider_highlighted: 200x20 轨道背景，九宫格边距1像素
    // slider_handle/slider_handle_highlighted: 8x20 手柄，九宫格边距(2,2,2,3)
    // 旧版回退坐标：slider 复用 button_normal/button_hover 区域，handle 取按钮最左侧8像素
    // TODO(consumer): 滑动条精灵尚无消费者，详见 GuiSpriteManager 重载中的 TODO(consumer) 说明。
    atlas.registerSprite("slider", 0, 66, 200, 20, atlasWidth, atlasHeight);
    atlas.registerSprite("slider_highlighted", 0, 86, 200, 20, atlasWidth, atlasHeight);
    atlas.registerSprite("slider_handle", 0, 66, 8, 20, atlasWidth, atlasHeight);
    atlas.registerSprite("slider_handle_highlighted", 0, 86, 8, 20, atlasWidth, atlasHeight);

    // 滑动条九宫格
    if (auto* sprite = atlas.getSprite("slider")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(1, 1, 199, 19);
    }
    if (auto* sprite = atlas.getSprite("slider_highlighted")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(1, 1, 199, 19);
    }
    if (auto* sprite = atlas.getSprite("slider_handle")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(2, 2, 6, 17);
    }
    if (auto* sprite = atlas.getSprite("slider_handle_highlighted")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(2, 2, 6, 17);
    }
}

void GuiSpriteRegistry::registerIconsSprites(GuiTextureAtlas& atlas, i32 atlasWidth, i32 atlasHeight)
{
    // ========== 心形图标 (9x9) ==========
    // 基础X坐标：满心52，半心61，空心16
    // Y坐标：正常0， Hardcore（困难模式）+45

    // 正常心形
    atlas.registerSprite("heart_full", 52, 0, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_half", 61, 0, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_empty", 16, 0, 9, 9, atlasWidth, atlasHeight);

    // Hardcore心形（Y+45）
    atlas.registerSprite("heart_full_hardcore", 52, 45, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_half_hardcore", 61, 45, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_empty_hardcore", 16, 45, 9, 9, atlasWidth, atlasHeight);

    // 吸收心（黄色）- X+36
    atlas.registerSprite("heart_absorbing_full", 88, 0, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_absorbing_half", 97, 0, 9, 9, atlasWidth, atlasHeight);

    // 中毒心（绿色）- X+36 基础偏移
    atlas.registerSprite("heart_poisoned_full", 88, 0, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_poisoned_half", 97, 0, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_poisoned_empty", 16, 0, 9, 9, atlasWidth, atlasHeight);

    // 凋零心（黑色）- X+72 基础偏移
    atlas.registerSprite("heart_wither_full", 124, 0, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_wither_half", 133, 0, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_wither_empty", 16, 0, 9, 9, atlasWidth, atlasHeight);

    // ========== 盔甲图标 (9x9) ==========
    // X坐标：空16，半25，满34
    // Y坐标：9
    atlas.registerSprite("armor_empty", 16, 9, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("armor_half", 25, 9, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("armor_full", 34, 9, 9, 9, atlasWidth, atlasHeight);

    // ========== 饥饿图标 (9x9) ==========
    // X坐标：空16，半25，满34
    // Y坐标：27
    // 饱和效果：Y+3 -> 30

    atlas.registerSprite("hunger_empty", 16, 27, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("hunger_half", 25, 27, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("hunger_full", 34, 27, 9, 9, atlasWidth, atlasHeight);

    // 饱和效果饥饿图标
    atlas.registerSprite("hunger_saturated_empty", 16, 30, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("hunger_saturated_half", 25, 30, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("hunger_saturated_full", 34, 30, 9, 9, atlasWidth, atlasHeight);

    // ========== 经验条 (182x5) ==========
    // 空条：Y=64，满条：Y=69
    atlas.registerSprite("xp_bar_empty", 0, 64, 182, 5, atlasWidth, atlasHeight);
    atlas.registerSprite("xp_bar_full", 0, 69, 182, 5, atlasWidth, atlasHeight);

    // ========== 跳跃条 (182x5) ==========
    // 用于马匹跳跃
    atlas.registerSprite("jump_bar_background", 0, 84, 182, 5, atlasWidth, atlasHeight);
    atlas.registerSprite("jump_bar_foreground", 0, 89, 182, 5, atlasWidth, atlasHeight);

    // ========== 气泡 (9x9) ==========
    // X坐标：空16，满25
    // Y坐标：18
    atlas.registerSprite("bubble_empty", 16, 18, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("bubble_full", 25, 18, 9, 9, atlasWidth, atlasHeight);

    // ========== 准星 (15x15) ==========
    // 位于(0, 0)
    atlas.registerSprite("crosshair", 0, 0, 15, 15, atlasWidth, atlasHeight);

    // ========== 攻击指示器 ==========
    // 旧版 icons.png 中位于 Y=94 行
    // 准星模式：full(16x16) 在(68,94)，background(16x4) 在(36,94)，progress(16x4) 在(52,94)
    // 快捷栏模式：background(18x18) 在(0,94)，progress(18x18) 在(18,94)
    // TODO(consumer): 攻击指示器精灵尚无消费者，详见 GuiSpriteManager 重载中的 TODO(consumer) 说明。
    atlas.registerSprite("crosshair_attack_indicator_full", 68, 94, 16, 16, atlasWidth, atlasHeight);
    atlas.registerSprite("crosshair_attack_indicator_background", 36, 94, 16, 4, atlasWidth, atlasHeight);
    atlas.registerSprite("crosshair_attack_indicator_progress", 52, 94, 16, 4, atlasWidth, atlasHeight);
    atlas.registerSprite("hotbar_attack_indicator_background", 0, 94, 18, 18, atlasWidth, atlasHeight);
    atlas.registerSprite("hotbar_attack_indicator_progress", 18, 94, 18, 18, atlasWidth, atlasHeight);

    // TODO: 经验等级数字精灵位于 icons.png 中，但目前使用字体渲染替代，
    // 因为经验等级需要根据等级值动态着色，字体渲染更灵活。
}

void GuiSpriteRegistry::registerContainerSprites(GuiTextureAtlas& atlas, i32 atlasWidth, i32 atlasHeight)
{
    // 槽位背景 (18x18)
    // 这是程序生成的默认槽位，实际纹理来自各容器纹理
    atlas.registerSprite("slot_background", 0, 0, 18, 18, atlasWidth, atlasHeight);

    // 容器背景 (176x166) - 默认尺寸
    // 实际纹理来自 container/*.png
    atlas.registerSprite("container_background", 0, 0, 176, 166, atlasWidth, atlasHeight);

    // 物品栏背景（玩家背包）
    atlas.registerSprite("inventory_background", 0, 0, 176, 166, atlasWidth, atlasHeight);
}

void GuiSpriteRegistry::registerAllDefaults(GuiTextureAtlas& atlas, i32 atlasWidth, i32 atlasHeight)
{
    registerWidgetsSprites(atlas, atlasWidth, atlasHeight);
    registerIconsSprites(atlas, atlasWidth, atlasHeight);
    registerContainerSprites(atlas, atlasWidth, atlasHeight);
}

// ============================================================================
// GuiSpriteAtlas 重载（整合类）
// ============================================================================

void GuiSpriteRegistry::registerWidgetsSprites(GuiSpriteAtlas& atlas, i32 atlasWidth, i32 atlasHeight)
{
    // 如果未指定尺寸，使用图集的实际尺寸
    if (atlasWidth <= 0 || atlasHeight <= 0) {
        atlasWidth = atlas.atlasWidth();
        atlasHeight = atlas.atlasHeight();
    }

    spdlog::info("[GuiSpriteRegistry] registerWidgetsSprites: atlasSize={}x{}, hasTexture={}",
        atlasWidth,
        atlasHeight,
        atlas.hasTexture());

    // 按钮精灵（宽度200，高度20）
    // Y坐标：禁用46，正常66，悬停86
    atlas.registerSprite("button_disabled", 0, 46, 200, 20, atlasWidth, atlasHeight);
    atlas.registerSprite("button_normal", 0, 66, 200, 20, atlasWidth, atlasHeight);
    atlas.registerSprite("button_hover", 0, 86, 200, 20, atlasWidth, atlasHeight);

    // 按钮九宫格（边距各4像素）
    if (auto* sprite = atlas.getSprite("button_disabled")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(4, 4, 196, 16);
    }
    if (auto* sprite = atlas.getSprite("button_normal")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(4, 4, 196, 16);
    }
    if (auto* sprite = atlas.getSprite("button_hover")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(4, 4, 196, 16);
    }

    // 快捷栏背景 (0, 0) 182x22
    atlas.registerSprite("hotbar_bg", 0, 0, 182, 22, atlasWidth, atlasHeight);

    // 快捷栏选中高亮 (0, 22) 24x22
    atlas.registerSprite("hotbar_selection", 0, 22, 24, 22, atlasWidth, atlasHeight);

    // 副手槽位 (24, 22) 29x24 和 (53, 22) 29x24
    atlas.registerSprite("hotbar_offhand_left", 24, 22, 29, 24, atlasWidth, atlasHeight);
    atlas.registerSprite("hotbar_offhand_right", 53, 22, 29, 24, atlasWidth, atlasHeight);

    // 滑动条精灵（MC 1.21+ 独立精灵，旧版 widgets.png 中复用按钮纹理区域）
    // slider/slider_highlighted: 200x20 轨道背景，九宫格边距1像素
    // slider_handle/slider_handle_highlighted: 8x20 手柄，九宫格边距(2,2,2,3)
    // 旧版回退坐标：slider 复用 button_normal/button_hover 区域，handle 取按钮最左侧8像素
    // TODO(consumer): 滑动条精灵尚无消费者，详见 GuiSpriteManager 重载中的 TODO(consumer) 说明。
    atlas.registerSprite("slider", 0, 66, 200, 20, atlasWidth, atlasHeight);
    atlas.registerSprite("slider_highlighted", 0, 86, 200, 20, atlasWidth, atlasHeight);
    atlas.registerSprite("slider_handle", 0, 66, 8, 20, atlasWidth, atlasHeight);
    atlas.registerSprite("slider_handle_highlighted", 0, 86, 8, 20, atlasWidth, atlasHeight);

    // 滑动条九宫格
    if (auto* sprite = atlas.getSprite("slider")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(1, 1, 199, 19);
    }
    if (auto* sprite = atlas.getSprite("slider_highlighted")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(1, 1, 199, 19);
    }
    if (auto* sprite = atlas.getSprite("slider_handle")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(2, 2, 6, 17);
    }
    if (auto* sprite = atlas.getSprite("slider_handle_highlighted")) {
        const_cast<GuiSprite*>(sprite)->setNinePatch(2, 2, 6, 17);
    }
}

void GuiSpriteRegistry::registerIconsSprites(GuiSpriteAtlas& atlas, i32 atlasWidth, i32 atlasHeight)
{
    // 如果未指定尺寸，使用图集的实际尺寸
    if (atlasWidth <= 0 || atlasHeight <= 0) {
        atlasWidth = atlas.atlasWidth();
        atlasHeight = atlas.atlasHeight();
    }

    spdlog::info("[GuiSpriteRegistry] registerIconsSprites: atlasSize={}x{}, hasTexture={}",
        atlasWidth,
        atlasHeight,
        atlas.hasTexture());

    // ========== 心形图标 (9x9) ==========
    // 基础X坐标：满心52，半心61，空心16
    // Y坐标：正常0， Hardcore（困难模式）+45

    // 正常心形
    atlas.registerSprite("heart_full", 52, 0, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_half", 61, 0, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_empty", 16, 0, 9, 9, atlasWidth, atlasHeight);

    // Hardcore心形（Y+45）
    atlas.registerSprite("heart_full_hardcore", 52, 45, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_half_hardcore", 61, 45, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_empty_hardcore", 16, 45, 9, 9, atlasWidth, atlasHeight);

    // 吸收心（黄色）- X+36
    atlas.registerSprite("heart_absorbing_full", 88, 0, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_absorbing_half", 97, 0, 9, 9, atlasWidth, atlasHeight);

    // 中毒心（绿色）- X+36 基础偏移
    atlas.registerSprite("heart_poisoned_full", 88, 0, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_poisoned_half", 97, 0, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_poisoned_empty", 16, 0, 9, 9, atlasWidth, atlasHeight);

    // 凋零心（黑色）- X+72 基础偏移
    atlas.registerSprite("heart_wither_full", 124, 0, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_wither_half", 133, 0, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("heart_wither_empty", 16, 0, 9, 9, atlasWidth, atlasHeight);

    // ========== 盔甲图标 (9x9) ==========
    // X坐标：空16，半25，满34
    // Y坐标：9
    atlas.registerSprite("armor_empty", 16, 9, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("armor_half", 25, 9, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("armor_full", 34, 9, 9, 9, atlasWidth, atlasHeight);

    // ========== 饥饿图标 (9x9) ==========
    // X坐标：空16，半25，满34
    // Y坐标：27
    // 饱和效果：Y+3 -> 30

    atlas.registerSprite("hunger_empty", 16, 27, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("hunger_half", 25, 27, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("hunger_full", 34, 27, 9, 9, atlasWidth, atlasHeight);

    // 饱和效果饥饿图标
    atlas.registerSprite("hunger_saturated_empty", 16, 30, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("hunger_saturated_half", 25, 30, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("hunger_saturated_full", 34, 30, 9, 9, atlasWidth, atlasHeight);

    // ========== 经验条 (182x5) ==========
    // 空条：Y=64，满条：Y=69
    atlas.registerSprite("xp_bar_empty", 0, 64, 182, 5, atlasWidth, atlasHeight);
    atlas.registerSprite("xp_bar_full", 0, 69, 182, 5, atlasWidth, atlasHeight);

    // ========== 跳跃条 (182x5) ==========
    // 用于马匹跳跃
    atlas.registerSprite("jump_bar_background", 0, 84, 182, 5, atlasWidth, atlasHeight);
    atlas.registerSprite("jump_bar_foreground", 0, 89, 182, 5, atlasWidth, atlasHeight);

    // ========== 气泡 (9x9) ==========
    // X坐标：空16，满25
    // Y坐标：18
    atlas.registerSprite("bubble_empty", 16, 18, 9, 9, atlasWidth, atlasHeight);
    atlas.registerSprite("bubble_full", 25, 18, 9, 9, atlasWidth, atlasHeight);

    // ========== 准星 (15x15) ==========
    // 位于(0, 0)
    atlas.registerSprite("crosshair", 0, 0, 15, 15, atlasWidth, atlasHeight);

    // ========== 攻击指示器 ==========
    // 旧版 icons.png 中位于 Y=94 行
    // 准星模式：full(16x16) 在(68,94)，background(16x4) 在(36,94)，progress(16x4) 在(52,94)
    // 快捷栏模式：background(18x18) 在(0,94)，progress(18x18) 在(18,94)
    // TODO(consumer): 攻击指示器精灵尚无消费者，详见 GuiSpriteManager 重载中的 TODO(consumer) 说明。
    atlas.registerSprite("crosshair_attack_indicator_full", 68, 94, 16, 16, atlasWidth, atlasHeight);
    atlas.registerSprite("crosshair_attack_indicator_background", 36, 94, 16, 4, atlasWidth, atlasHeight);
    atlas.registerSprite("crosshair_attack_indicator_progress", 52, 94, 16, 4, atlasWidth, atlasHeight);
    atlas.registerSprite("hotbar_attack_indicator_background", 0, 94, 18, 18, atlasWidth, atlasHeight);
    atlas.registerSprite("hotbar_attack_indicator_progress", 18, 94, 18, 18, atlasWidth, atlasHeight);

    // TODO: 经验等级数字精灵位于 icons.png 中，但目前使用字体渲染替代，
    // 因为经验等级需要根据等级值动态着色，字体渲染更灵活。
}

void GuiSpriteRegistry::registerContainerSprites(GuiSpriteAtlas& atlas, i32 atlasWidth, i32 atlasHeight)
{
    // 如果未指定尺寸，使用图集的实际尺寸
    if (atlasWidth <= 0 || atlasHeight <= 0) {
        atlasWidth = atlas.atlasWidth();
        atlasHeight = atlas.atlasHeight();
    }

    // 槽位背景 (18x18)
    // 这是程序生成的默认槽位，实际纹理来自各容器纹理
    atlas.registerSprite("slot_background", 0, 0, 18, 18, atlasWidth, atlasHeight);

    // 容器背景 (176x166) - 默认尺寸
    // 实际纹理来自 container/*.png
    atlas.registerSprite("container_background", 0, 0, 176, 166, atlasWidth, atlasHeight);

    // 物品栏背景（玩家背包）
    atlas.registerSprite("inventory_background", 0, 0, 176, 166, atlasWidth, atlasHeight);
}

void GuiSpriteRegistry::registerAllDefaults(GuiSpriteAtlas& atlas, i32 atlasWidth, i32 atlasHeight)
{
    registerWidgetsSprites(atlas, atlasWidth, atlasHeight);
    registerIconsSprites(atlas, atlasWidth, atlasHeight);
    registerContainerSprites(atlas, atlasWidth, atlasHeight);
}

// ============================================================================
// 获取精灵列表（用于调试）
// ============================================================================

std::vector<GuiSprite> GuiSpriteRegistry::getWidgetsSpriteList(i32 atlasWidth, i32 atlasHeight)
{
    std::vector<GuiSprite> sprites;
    sprites.emplace_back("button_disabled", 0, 46, 200, 20, atlasWidth, atlasHeight);
    sprites.emplace_back("button_normal", 0, 66, 200, 20, atlasWidth, atlasHeight);
    sprites.emplace_back("button_hover", 0, 86, 200, 20, atlasWidth, atlasHeight);
    sprites.emplace_back("hotbar_bg", 0, 0, 182, 22, atlasWidth, atlasHeight);
    sprites.emplace_back("hotbar_selection", 0, 22, 24, 22, atlasWidth, atlasHeight);
    sprites.emplace_back("hotbar_offhand_left", 24, 22, 29, 24, atlasWidth, atlasHeight);
    sprites.emplace_back("hotbar_offhand_right", 53, 22, 29, 24, atlasWidth, atlasHeight);
    sprites.emplace_back("slider", 0, 66, 200, 20, atlasWidth, atlasHeight);
    sprites.emplace_back("slider_highlighted", 0, 86, 200, 20, atlasWidth, atlasHeight);
    sprites.emplace_back("slider_handle", 0, 66, 8, 20, atlasWidth, atlasHeight);
    sprites.emplace_back("slider_handle_highlighted", 0, 86, 8, 20, atlasWidth, atlasHeight);
    return sprites;
}

std::vector<GuiSprite> GuiSpriteRegistry::getIconsSpriteList(i32 atlasWidth, i32 atlasHeight)
{
    std::vector<GuiSprite> sprites;
    sprites.emplace_back("heart_full", 52, 0, 9, 9, atlasWidth, atlasHeight);
    sprites.emplace_back("heart_half", 61, 0, 9, 9, atlasWidth, atlasHeight);
    sprites.emplace_back("heart_empty", 16, 0, 9, 9, atlasWidth, atlasHeight);
    sprites.emplace_back("armor_full", 34, 9, 9, 9, atlasWidth, atlasHeight);
    sprites.emplace_back("armor_half", 25, 9, 9, 9, atlasWidth, atlasHeight);
    sprites.emplace_back("armor_empty", 16, 9, 9, 9, atlasWidth, atlasHeight);
    sprites.emplace_back("hunger_full", 34, 27, 9, 9, atlasWidth, atlasHeight);
    sprites.emplace_back("hunger_half", 25, 27, 9, 9, atlasWidth, atlasHeight);
    sprites.emplace_back("hunger_empty", 16, 27, 9, 9, atlasWidth, atlasHeight);
    sprites.emplace_back("xp_bar_empty", 0, 64, 182, 5, atlasWidth, atlasHeight);
    sprites.emplace_back("xp_bar_full", 0, 69, 182, 5, atlasWidth, atlasHeight);
    sprites.emplace_back("crosshair", 0, 0, 15, 15, atlasWidth, atlasHeight);
    sprites.emplace_back("crosshair_attack_indicator_full", 68, 94, 16, 16, atlasWidth, atlasHeight);
    sprites.emplace_back("crosshair_attack_indicator_background", 36, 94, 16, 4, atlasWidth, atlasHeight);
    sprites.emplace_back("crosshair_attack_indicator_progress", 52, 94, 16, 4, atlasWidth, atlasHeight);
    sprites.emplace_back("hotbar_attack_indicator_background", 0, 94, 18, 18, atlasWidth, atlasHeight);
    sprites.emplace_back("hotbar_attack_indicator_progress", 18, 94, 18, 18, atlasWidth, atlasHeight);
    return sprites;
}

} // namespace mc::client::renderer::trident::gui
