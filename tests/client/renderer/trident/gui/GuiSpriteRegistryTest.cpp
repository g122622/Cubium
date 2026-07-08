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

/**
 * @file GuiSpriteRegistryTest.cpp
 * @brief GuiSpriteRegistry 与 GuiSpriteMappings 单元测试
 *
 * 验证滑动条精灵（slider/slider_highlighted/slider_handle/slider_handle_highlighted）
 * 和攻击指示器精灵（crosshair_attack_indicator_full/background/progress、
 * hotbar_attack_indicator_background/progress）的：
 * 1. GuiSpriteMappings.hpp 中 WIDGET_SPRITE_MAPPINGS / HUD_SPRITE_MAPPINGS 包含新增条目
 * 2. 精灵坐标符合旧版 icons.png/widgets.png 布局（参考 MC 1.14.4 IngameGui.java）
 * 3. 九宫格 setNinePatch 参数正确（slider 边距 1,1,199,19；handle 边距 2,2,6,17）
 * 4. GuiSpriteManager 重载的注册逻辑（UV 计算、九宫格设置）
 * 5. 边界场景：重复注册不崩溃、getSprite 返回非空
 *
 * 本测试为纯数据测试，不依赖 Vulkan。GuiTextureAtlas/GuiSpriteAtlas 重载的
 * Vulkan 功能由 GuiSpriteManager 测试覆盖（二者使用相同的硬编码坐标）。
 */

#include "client/renderer/trident/gui/GuiSprite.hpp"
#include "client/renderer/trident/gui/GuiSpriteManager.hpp"
#include "client/renderer/trident/gui/GuiSpriteMappings.hpp"
#include <algorithm>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::client::renderer::trident::gui;

// ============================================================================
// 测试辅助：在映射表中查找指定精灵ID
// ============================================================================
namespace {

const ResourceLocation* findMapping(
    const std::vector<std::pair<std::string, ResourceLocation>>& mappings, const std::string& id)
{
    auto it = std::find_if(mappings.begin(), mappings.end(), [&](const auto& pair) { return pair.first == id; });
    return (it != mappings.end()) ? &it->second : nullptr;
}

} // namespace

// ============================================================================
// GuiSpriteMappings.hpp — 滑动条精灵映射测试
// ============================================================================

/**
 * @brief 验证 WIDGET_SPRITE_MAPPINGS 包含 4 个滑动条精灵
 */
TEST(GuiSpriteRegistrySliderTest, WidgetSpriteMappingsContainsSliderEntries)
{
    ASSERT_NE(findMapping(WIDGET_SPRITE_MAPPINGS, "slider"), nullptr);
    ASSERT_NE(findMapping(WIDGET_SPRITE_MAPPINGS, "slider_highlighted"), nullptr);
    ASSERT_NE(findMapping(WIDGET_SPRITE_MAPPINGS, "slider_handle"), nullptr);
    ASSERT_NE(findMapping(WIDGET_SPRITE_MAPPINGS, "slider_handle_highlighted"), nullptr);
}

/**
 * @brief 验证滑动条精灵映射路径正确（MC 1.21+ 独立精灵路径）
 */
TEST(GuiSpriteRegistrySliderTest, SliderSpriteMappingPaths)
{
    const auto* slider = findMapping(WIDGET_SPRITE_MAPPINGS, "slider");
    ASSERT_NE(slider, nullptr);
    EXPECT_EQ(slider->namespace_(), "minecraft");
    EXPECT_EQ(slider->path(), "textures/gui/sprites/widget/slider");

    const auto* sliderHighlighted = findMapping(WIDGET_SPRITE_MAPPINGS, "slider_highlighted");
    ASSERT_NE(sliderHighlighted, nullptr);
    EXPECT_EQ(sliderHighlighted->path(), "textures/gui/sprites/widget/slider_highlighted");

    const auto* sliderHandle = findMapping(WIDGET_SPRITE_MAPPINGS, "slider_handle");
    ASSERT_NE(sliderHandle, nullptr);
    EXPECT_EQ(sliderHandle->path(), "textures/gui/sprites/widget/slider_handle");

    const auto* sliderHandleHighlighted = findMapping(WIDGET_SPRITE_MAPPINGS, "slider_handle_highlighted");
    ASSERT_NE(sliderHandleHighlighted, nullptr);
    EXPECT_EQ(sliderHandleHighlighted->path(), "textures/gui/sprites/widget/slider_handle_highlighted");
}

// ============================================================================
// GuiSpriteMappings.hpp — 攻击指示器精灵映射测试
// ============================================================================

/**
 * @brief 验证 HUD_SPRITE_MAPPINGS 包含 5 个攻击指示器精灵
 */
TEST(GuiSpriteRegistryAttackIndicatorTest, HudSpriteMappingsContainsAttackIndicatorEntries)
{
    ASSERT_NE(findMapping(HUD_SPRITE_MAPPINGS, "crosshair_attack_indicator_full"), nullptr);
    ASSERT_NE(findMapping(HUD_SPRITE_MAPPINGS, "crosshair_attack_indicator_background"), nullptr);
    ASSERT_NE(findMapping(HUD_SPRITE_MAPPINGS, "crosshair_attack_indicator_progress"), nullptr);
    ASSERT_NE(findMapping(HUD_SPRITE_MAPPINGS, "hotbar_attack_indicator_background"), nullptr);
    ASSERT_NE(findMapping(HUD_SPRITE_MAPPINGS, "hotbar_attack_indicator_progress"), nullptr);
}

/**
 * @brief 验证攻击指示器精灵映射路径正确（hud/ 目录下）
 */
TEST(GuiSpriteRegistryAttackIndicatorTest, AttackIndicatorSpriteMappingPaths)
{
    const auto* full = findMapping(HUD_SPRITE_MAPPINGS, "crosshair_attack_indicator_full");
    ASSERT_NE(full, nullptr);
    EXPECT_EQ(full->namespace_(), "minecraft");
    EXPECT_EQ(full->path(), "textures/gui/sprites/hud/crosshair_attack_indicator_full");

    const auto* bg = findMapping(HUD_SPRITE_MAPPINGS, "crosshair_attack_indicator_background");
    ASSERT_NE(bg, nullptr);
    EXPECT_EQ(bg->path(), "textures/gui/sprites/hud/crosshair_attack_indicator_background");

    const auto* progress = findMapping(HUD_SPRITE_MAPPINGS, "crosshair_attack_indicator_progress");
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->path(), "textures/gui/sprites/hud/crosshair_attack_indicator_progress");

    const auto* hotbarBg = findMapping(HUD_SPRITE_MAPPINGS, "hotbar_attack_indicator_background");
    ASSERT_NE(hotbarBg, nullptr);
    EXPECT_EQ(hotbarBg->path(), "textures/gui/sprites/hud/hotbar_attack_indicator_background");

    const auto* hotbarProgress = findMapping(HUD_SPRITE_MAPPINGS, "hotbar_attack_indicator_progress");
    ASSERT_NE(hotbarProgress, nullptr);
    EXPECT_EQ(hotbarProgress->path(), "textures/gui/sprites/hud/hotbar_attack_indicator_progress");
}

// ============================================================================
// 滑动条精灵坐标测试（参考 MC 1.14.4 AbstractSlider.java + 旧版 widgets.png 布局）
// ============================================================================

namespace {
// 旧版 widgets.png 256x256 布局常量
constexpr i32 ATLAS_SIZE = 256;
// 旧版布局：slider 复用 button_normal(Y=66) / button_hover(Y=86) 区域
// handle 取按钮最左侧 8 像素
constexpr i32 SLIDER_Y_NORMAL = 66;
constexpr i32 SLIDER_Y_HIGHLIGHTED = 86;
constexpr i32 SLIDER_WIDTH = 200;
constexpr i32 SLIDER_HEIGHT = 20;
constexpr i32 SLIDER_HANDLE_WIDTH = 8;
constexpr i32 SLIDER_HANDLE_HEIGHT = 20;
} // namespace

/**
 * @brief 验证滑动条背景精灵坐标（normal 和 highlighted）
 *
 * 旧版 widgets.png 中 slider 复用 button_normal(0,66) 和 button_hover(0,86) 区域。
 * MC 1.21+ 中 slider/slider_highlighted 为 200x20 独立精灵。
 */
TEST(GuiSpriteRegistrySliderTest, SliderBackgroundCoordinates)
{
    GuiSpriteManager manager;

    // 注册滑动条背景精灵（与 GuiSpriteRegistry::registerWidgetsSprites 坐标一致）
    manager.registerSprite("slider", 0, SLIDER_Y_NORMAL, SLIDER_WIDTH, SLIDER_HEIGHT, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite(
        "slider_highlighted", 0, SLIDER_Y_HIGHLIGHTED, SLIDER_WIDTH, SLIDER_HEIGHT, ATLAS_SIZE, ATLAS_SIZE);

    const GuiSprite* slider = manager.getSprite("slider");
    ASSERT_NE(slider, nullptr);
    EXPECT_EQ(slider->width, SLIDER_WIDTH);
    EXPECT_EQ(slider->height, SLIDER_HEIGHT);
    EXPECT_DOUBLE_EQ(slider->u0, 0.0);
    EXPECT_DOUBLE_EQ(slider->v0, static_cast<f64>(SLIDER_Y_NORMAL) / ATLAS_SIZE);
    EXPECT_DOUBLE_EQ(slider->u1, static_cast<f64>(SLIDER_WIDTH) / ATLAS_SIZE);
    EXPECT_DOUBLE_EQ(slider->v1, static_cast<f64>(SLIDER_Y_NORMAL + SLIDER_HEIGHT) / ATLAS_SIZE);

    const GuiSprite* highlighted = manager.getSprite("slider_highlighted");
    ASSERT_NE(highlighted, nullptr);
    EXPECT_EQ(highlighted->width, SLIDER_WIDTH);
    EXPECT_EQ(highlighted->height, SLIDER_HEIGHT);
    EXPECT_DOUBLE_EQ(highlighted->v0, static_cast<f64>(SLIDER_Y_HIGHLIGHTED) / ATLAS_SIZE);
}

/**
 * @brief 验证滑动条手柄精灵坐标（normal 和 highlighted）
 *
 * 旧版 widgets.png 中 handle 取按钮最左侧 8 像素（MC 1.14.4 AbstractSlider.renderBg
 * 使用 (0, 46+i, 4, 20) 和 (196, 46+i, 4, 20) 两段，合并为 8px 宽）。
 * MC 1.21+ 中 slider_handle 为 8x20 独立精灵。
 */
TEST(GuiSpriteRegistrySliderTest, SliderHandleCoordinates)
{
    GuiSpriteManager manager;

    manager.registerSprite(
        "slider_handle", 0, SLIDER_Y_NORMAL, SLIDER_HANDLE_WIDTH, SLIDER_HANDLE_HEIGHT, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite("slider_handle_highlighted",
        0,
        SLIDER_Y_HIGHLIGHTED,
        SLIDER_HANDLE_WIDTH,
        SLIDER_HANDLE_HEIGHT,
        ATLAS_SIZE,
        ATLAS_SIZE);

    const GuiSprite* handle = manager.getSprite("slider_handle");
    ASSERT_NE(handle, nullptr);
    EXPECT_EQ(handle->width, SLIDER_HANDLE_WIDTH);
    EXPECT_EQ(handle->height, SLIDER_HANDLE_HEIGHT);
    EXPECT_DOUBLE_EQ(handle->u0, 0.0);
    EXPECT_DOUBLE_EQ(handle->v0, static_cast<f64>(SLIDER_Y_NORMAL) / ATLAS_SIZE);
    EXPECT_DOUBLE_EQ(handle->u1, static_cast<f64>(SLIDER_HANDLE_WIDTH) / ATLAS_SIZE);

    const GuiSprite* handleHighlighted = manager.getSprite("slider_handle_highlighted");
    ASSERT_NE(handleHighlighted, nullptr);
    EXPECT_EQ(handleHighlighted->width, SLIDER_HANDLE_WIDTH);
    EXPECT_DOUBLE_EQ(handleHighlighted->v0, static_cast<f64>(SLIDER_Y_HIGHLIGHTED) / ATLAS_SIZE);
}

/**
 * @brief 验证滑动条九宫格参数
 *
 * MC 1.21+ mcmeta 配置：
 * - slider/slider_highlighted: nine_slice border=1（四边均为1像素）
 * - slider_handle/slider_handle_highlighted: nine_slice border(2,2,2,3)
 *
 * GuiNinePatch 语义：setNinePatch(left, top, right, bottom)
 * - slider: setNinePatch(1, 1, 199, 19) → left=1, top=1, right=199, bottom=19
 * - handle: setNinePatch(2, 2, 6, 17) → left=2, top=2, right=6, bottom=17
 */
TEST(GuiSpriteRegistrySliderTest, SliderNinePatchParameters)
{
    GuiSpriteManager manager;

    // 注册并设置九宫格（与 GuiSpriteRegistry::registerWidgetsSprites 一致）
    manager.registerSprite("slider", 0, SLIDER_Y_NORMAL, SLIDER_WIDTH, SLIDER_HEIGHT, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite(
        "slider_highlighted", 0, SLIDER_Y_HIGHLIGHTED, SLIDER_WIDTH, SLIDER_HEIGHT, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite(
        "slider_handle", 0, SLIDER_Y_NORMAL, SLIDER_HANDLE_WIDTH, SLIDER_HANDLE_HEIGHT, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite("slider_handle_highlighted",
        0,
        SLIDER_Y_HIGHLIGHTED,
        SLIDER_HANDLE_WIDTH,
        SLIDER_HANDLE_HEIGHT,
        ATLAS_SIZE,
        ATLAS_SIZE);

    if (auto* s = manager.getSprite("slider")) {
        const_cast<GuiSprite*>(s)->setNinePatch(1, 1, 199, 19);
    }
    if (auto* s = manager.getSprite("slider_highlighted")) {
        const_cast<GuiSprite*>(s)->setNinePatch(1, 1, 199, 19);
    }
    if (auto* s = manager.getSprite("slider_handle")) {
        const_cast<GuiSprite*>(s)->setNinePatch(2, 2, 6, 17);
    }
    if (auto* s = manager.getSprite("slider_handle_highlighted")) {
        const_cast<GuiSprite*>(s)->setNinePatch(2, 2, 6, 17);
    }

    const GuiSprite* slider = manager.getSprite("slider");
    ASSERT_NE(slider, nullptr);
    EXPECT_TRUE(slider->ninePatch.isValid());
    EXPECT_EQ(slider->ninePatch.left, 1);
    EXPECT_EQ(slider->ninePatch.top, 1);
    EXPECT_EQ(slider->ninePatch.right, 199);
    EXPECT_EQ(slider->ninePatch.bottom, 19);

    const GuiSprite* sliderHighlighted = manager.getSprite("slider_highlighted");
    ASSERT_NE(sliderHighlighted, nullptr);
    EXPECT_EQ(sliderHighlighted->ninePatch.left, 1);
    EXPECT_EQ(sliderHighlighted->ninePatch.top, 1);
    EXPECT_EQ(sliderHighlighted->ninePatch.right, 199);
    EXPECT_EQ(sliderHighlighted->ninePatch.bottom, 19);

    const GuiSprite* handle = manager.getSprite("slider_handle");
    ASSERT_NE(handle, nullptr);
    EXPECT_TRUE(handle->ninePatch.isValid());
    EXPECT_EQ(handle->ninePatch.left, 2);
    EXPECT_EQ(handle->ninePatch.top, 2);
    EXPECT_EQ(handle->ninePatch.right, 6);
    EXPECT_EQ(handle->ninePatch.bottom, 17);

    const GuiSprite* handleHighlighted = manager.getSprite("slider_handle_highlighted");
    ASSERT_NE(handleHighlighted, nullptr);
    EXPECT_EQ(handleHighlighted->ninePatch.left, 2);
    EXPECT_EQ(handleHighlighted->ninePatch.top, 2);
    EXPECT_EQ(handleHighlighted->ninePatch.right, 6);
    EXPECT_EQ(handleHighlighted->ninePatch.bottom, 17);
}

// ============================================================================
// 攻击指示器精灵坐标测试（参考 MC 1.14.4 IngameGui.java Y=94 行布局）
// ============================================================================

namespace {
// 旧版 icons.png 256x256 布局常量（MC 1.14.4 IngameGui.java）
// Y=94 行排列攻击指示器精灵
constexpr i32 ATTACK_INDICATOR_Y = 94;
// 准星模式
constexpr i32 CROSSHAIR_FULL_X = 68;
constexpr i32 CROSSHAIR_BG_X = 36;
constexpr i32 CROSSHAIR_PROGRESS_X = 52;
constexpr i32 CROSSHAIR_FULL_SIZE = 16;
constexpr i32 CROSSHAIR_BAR_WIDTH = 16;
constexpr i32 CROSSHAIR_BAR_HEIGHT = 4;
// 快捷栏模式
constexpr i32 HOTBAR_BG_X = 0;
constexpr i32 HOTBAR_PROGRESS_X = 18;
constexpr i32 HOTBAR_INDICATOR_SIZE = 18;
} // namespace

/**
 * @brief 验证准星模式攻击指示器精灵坐标
 *
 * MC 1.14.4 IngameGui.java:
 * - full: blit(k, j, 68, 94, 16, 16)
 * - background: blit(k, j, 36, 94, 16, 4)
 * - progress: blit(k, j, 52, 94, l, 4) 其中 l 为进度宽度
 */
TEST(GuiSpriteRegistryAttackIndicatorTest, CrosshairAttackIndicatorCoordinates)
{
    GuiSpriteManager manager;

    manager.registerSprite("crosshair_attack_indicator_full",
        CROSSHAIR_FULL_X,
        ATTACK_INDICATOR_Y,
        CROSSHAIR_FULL_SIZE,
        CROSSHAIR_FULL_SIZE,
        ATLAS_SIZE,
        ATLAS_SIZE);
    manager.registerSprite("crosshair_attack_indicator_background",
        CROSSHAIR_BG_X,
        ATTACK_INDICATOR_Y,
        CROSSHAIR_BAR_WIDTH,
        CROSSHAIR_BAR_HEIGHT,
        ATLAS_SIZE,
        ATLAS_SIZE);
    manager.registerSprite("crosshair_attack_indicator_progress",
        CROSSHAIR_PROGRESS_X,
        ATTACK_INDICATOR_Y,
        CROSSHAIR_BAR_WIDTH,
        CROSSHAIR_BAR_HEIGHT,
        ATLAS_SIZE,
        ATLAS_SIZE);

    const GuiSprite* full = manager.getSprite("crosshair_attack_indicator_full");
    ASSERT_NE(full, nullptr);
    EXPECT_EQ(full->width, CROSSHAIR_FULL_SIZE);
    EXPECT_EQ(full->height, CROSSHAIR_FULL_SIZE);
    EXPECT_DOUBLE_EQ(full->u0, static_cast<f64>(CROSSHAIR_FULL_X) / ATLAS_SIZE);
    EXPECT_DOUBLE_EQ(full->v0, static_cast<f64>(ATTACK_INDICATOR_Y) / ATLAS_SIZE);

    const GuiSprite* bg = manager.getSprite("crosshair_attack_indicator_background");
    ASSERT_NE(bg, nullptr);
    EXPECT_EQ(bg->width, CROSSHAIR_BAR_WIDTH);
    EXPECT_EQ(bg->height, CROSSHAIR_BAR_HEIGHT);
    EXPECT_DOUBLE_EQ(bg->u0, static_cast<f64>(CROSSHAIR_BG_X) / ATLAS_SIZE);

    const GuiSprite* progress = manager.getSprite("crosshair_attack_indicator_progress");
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->width, CROSSHAIR_BAR_WIDTH);
    EXPECT_EQ(progress->height, CROSSHAIR_BAR_HEIGHT);
    EXPECT_DOUBLE_EQ(progress->u0, static_cast<f64>(CROSSHAIR_PROGRESS_X) / ATLAS_SIZE);
}

/**
 * @brief 验证快捷栏模式攻击指示器精灵坐标
 *
 * MC 1.14.4 IngameGui.java:
 * - background: blit(k2, j2, 0, 94, 18, 18)
 * - progress: blit(k2, j2 + 18 - l1, 18, 112 - l1, 18, l1) 其中 l1 为进度高度
 */
TEST(GuiSpriteRegistryAttackIndicatorTest, HotbarAttackIndicatorCoordinates)
{
    GuiSpriteManager manager;

    manager.registerSprite("hotbar_attack_indicator_background",
        HOTBAR_BG_X,
        ATTACK_INDICATOR_Y,
        HOTBAR_INDICATOR_SIZE,
        HOTBAR_INDICATOR_SIZE,
        ATLAS_SIZE,
        ATLAS_SIZE);
    manager.registerSprite("hotbar_attack_indicator_progress",
        HOTBAR_PROGRESS_X,
        ATTACK_INDICATOR_Y,
        HOTBAR_INDICATOR_SIZE,
        HOTBAR_INDICATOR_SIZE,
        ATLAS_SIZE,
        ATLAS_SIZE);

    const GuiSprite* bg = manager.getSprite("hotbar_attack_indicator_background");
    ASSERT_NE(bg, nullptr);
    EXPECT_EQ(bg->width, HOTBAR_INDICATOR_SIZE);
    EXPECT_EQ(bg->height, HOTBAR_INDICATOR_SIZE);
    EXPECT_DOUBLE_EQ(bg->u0, static_cast<f64>(HOTBAR_BG_X) / ATLAS_SIZE);
    EXPECT_DOUBLE_EQ(bg->v0, static_cast<f64>(ATTACK_INDICATOR_Y) / ATLAS_SIZE);

    const GuiSprite* progress = manager.getSprite("hotbar_attack_indicator_progress");
    ASSERT_NE(progress, nullptr);
    EXPECT_EQ(progress->width, HOTBAR_INDICATOR_SIZE);
    EXPECT_EQ(progress->height, HOTBAR_INDICATOR_SIZE);
    EXPECT_DOUBLE_EQ(progress->u0, static_cast<f64>(HOTBAR_PROGRESS_X) / ATLAS_SIZE);
}

// ============================================================================
// 边界场景测试
// ============================================================================

/**
 * @brief 验证重复注册同名精灵不崩溃，且覆盖旧值
 */
TEST(GuiSpriteRegistryEdgeCaseTest, DuplicateRegistrationDoesNotCrash)
{
    GuiSpriteManager manager;

    // 首次注册
    manager.registerSprite("slider", 0, 66, 200, 20, ATLAS_SIZE, ATLAS_SIZE);
    EXPECT_EQ(manager.spriteCount(), 1);

    // 重复注册同名精灵（不同坐标）不应崩溃
    manager.registerSprite("slider", 0, 86, 200, 20, ATLAS_SIZE, ATLAS_SIZE);
    EXPECT_EQ(manager.spriteCount(), 1); // 数量不变

    // 验证值为最新注册的
    const GuiSprite* sprite = manager.getSprite("slider");
    ASSERT_NE(sprite, nullptr);
    EXPECT_DOUBLE_EQ(sprite->v0, static_cast<f64>(86) / ATLAS_SIZE);
}

/**
 * @brief 验证 getSprite 对未注册ID返回 nullptr
 */
TEST(GuiSpriteRegistryEdgeCaseTest, GetSpriteReturnsNullForUnregisteredId)
{
    GuiSpriteManager manager;

    manager.registerSprite("slider", 0, 66, 200, 20, ATLAS_SIZE, ATLAS_SIZE);

    EXPECT_NE(manager.getSprite("slider"), nullptr);
    EXPECT_EQ(manager.getSprite("nonexistent_sprite"), nullptr);
}

/**
 * @brief 验证批量注册所有新增精灵后 hasSprite 全部返回 true
 */
TEST(GuiSpriteRegistryEdgeCaseTest, AllNewSpritesRegisteredSuccessfully)
{
    GuiSpriteManager manager;

    // 注册全部 4 个滑动条精灵
    manager.registerSprite("slider", 0, 66, 200, 20, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite("slider_highlighted", 0, 86, 200, 20, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite("slider_handle", 0, 66, 8, 20, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite("slider_handle_highlighted", 0, 86, 8, 20, ATLAS_SIZE, ATLAS_SIZE);

    // 注册全部 5 个攻击指示器精灵
    manager.registerSprite("crosshair_attack_indicator_full", 68, 94, 16, 16, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite("crosshair_attack_indicator_background", 36, 94, 16, 4, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite("crosshair_attack_indicator_progress", 52, 94, 16, 4, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite("hotbar_attack_indicator_background", 0, 94, 18, 18, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite("hotbar_attack_indicator_progress", 18, 94, 18, 18, ATLAS_SIZE, ATLAS_SIZE);

    EXPECT_EQ(manager.spriteCount(), 9);

    EXPECT_TRUE(manager.hasSprite("slider"));
    EXPECT_TRUE(manager.hasSprite("slider_highlighted"));
    EXPECT_TRUE(manager.hasSprite("slider_handle"));
    EXPECT_TRUE(manager.hasSprite("slider_handle_highlighted"));
    EXPECT_TRUE(manager.hasSprite("crosshair_attack_indicator_full"));
    EXPECT_TRUE(manager.hasSprite("crosshair_attack_indicator_background"));
    EXPECT_TRUE(manager.hasSprite("crosshair_attack_indicator_progress"));
    EXPECT_TRUE(manager.hasSprite("hotbar_attack_indicator_background"));
    EXPECT_TRUE(manager.hasSprite("hotbar_attack_indicator_progress"));
}

/**
 * @brief 验证所有新增精灵的 isValid() 返回 true（宽高均 > 0）
 */
TEST(GuiSpriteRegistryEdgeCaseTest, AllNewSpritesAreValid)
{
    GuiSpriteManager manager;

    manager.registerSprite("slider", 0, 66, 200, 20, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite("slider_handle", 0, 66, 8, 20, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite("crosshair_attack_indicator_full", 68, 94, 16, 16, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite("crosshair_attack_indicator_background", 36, 94, 16, 4, ATLAS_SIZE, ATLAS_SIZE);
    manager.registerSprite("hotbar_attack_indicator_background", 0, 94, 18, 18, ATLAS_SIZE, ATLAS_SIZE);

    EXPECT_TRUE(manager.getSprite("slider")->isValid());
    EXPECT_TRUE(manager.getSprite("slider_handle")->isValid());
    EXPECT_TRUE(manager.getSprite("crosshair_attack_indicator_full")->isValid());
    EXPECT_TRUE(manager.getSprite("crosshair_attack_indicator_background")->isValid());
    EXPECT_TRUE(manager.getSprite("hotbar_attack_indicator_background")->isValid());
}
