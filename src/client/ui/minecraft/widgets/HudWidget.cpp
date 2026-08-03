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

#include "HudWidget.hpp"
#include "client/renderer/map/MapRenderer.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/renderer/trident/gui/GuiSpriteAtlas.hpp"
#include "client/renderer/trident/item/ItemRenderer.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "client/world/ClientMapDataCache.hpp"
#include "common/core/Types.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/food/FoodStats.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/map/FilledMapItem.hpp"
#include "common/world/map/MapData.hpp"
#include <algorithm>
#include <string>

namespace mc::client::ui::minecraft::widgets {

// ============================================================================
// 常量
// ============================================================================

namespace {
// 快捷栏尺寸
constexpr f32 HOTBAR_WIDTH = 182.0f;
constexpr f32 HOTBAR_HEIGHT = 22.0f;
constexpr f32 SLOT_SIZE = 18.0f;
constexpr f32 SLOT_SPACING = 20.0f;
constexpr f32 HOTBAR_OFFSET_Y = 3.0f;

// 心形图标尺寸（图标 9x9px，间距 8px）
constexpr f32 HEART_SIZE = 9.0f;
constexpr f32 HEART_SPACING = 8.0f;
constexpr f32 HEALTH_OFFSET_Y = 12.0f;

// 饥饿图标尺寸（图标 9x9px，间距 8px）
constexpr f32 HUNGER_SIZE = 9.0f;
constexpr f32 HUNGER_SPACING = 8.0f;

// 盔甲图标尺寸（图标 9x9px，间距 8px）
constexpr f32 ARMOR_SIZE = 9.0f;
constexpr f32 ARMOR_SPACING = 8.0f;

// 经验条尺寸
constexpr f32 XP_BAR_WIDTH = 182.0f;
constexpr f32 XP_BAR_HEIGHT = 5.0f;
constexpr f32 XP_TEXT_OFFSET_Y = -7.0f;

// 物品提示
constexpr f32 TOOLTIP_PADDING = 4.0f;
constexpr f32 TOOLTIP_OFFSET_Y = 12.0f;

} // namespace

// ============================================================================
// 构造函数
// ============================================================================

HudWidget::HudWidget()
    : Widget("hud")
{
    setVisible(true);
    setActive(true);
}

// ============================================================================
// 渲染
// ============================================================================

void HudWidget::paint(kagero::widget::PaintContext& ctx)
{
    if (!isVisible() || m_player == nullptr) {
        return;
    }

    // 渲染经验条（在快捷栏下方）
    _renderExperience(ctx);

    // 渲染快捷栏（需要 GuiRenderer 用于物品渲染）
    if (m_gui != nullptr) {
        _renderHotbar(ctx, *m_gui);
    }

    // 渲染生命值和盔甲
    _renderHealth(ctx);

    // 渲染饥饿值
    _renderHunger(ctx);

    // 渲染手持地图内容（屏幕层，玩家手持已填充地图时）
    _renderHeldMap();
}

void HudWidget::_renderHeldMap()
{
    // 手持地图内容由 GuiRenderer 在屏幕层绘制（双手举起姿态由 FirstPersonRenderer 负责）。
    if (m_mapRenderer == nullptr || m_mapDataCache == nullptr || m_player == nullptr) {
        return;
    }

    // 优先主手，其次副手（Java 仅主手持握时双手举起，副手地图不展开）
    const ItemStack mainHand = m_player->getHeldItem(Hand::MainHand);
    if (!item::items::FilledMapItem::isFilledMap(mainHand)) {
        return;
    }

    const i32 mapId = item::items::FilledMapItem::getMapId(mainHand);
    if (mapId < 0) {
        return;
    }

    // 缓存中无该 mapId 的数据则跳过（服务端尚未下发或地图物品无数据）
    const mc::world::map::MapData* mapData = m_mapDataCache->getMapData(mapId);
    if (mapData == nullptr) {
        return;
    }

    // 地图居中偏上绘制，尺寸取屏幕短边的 45%（对齐 Java 手持地图屏幕占比）
    const f32 screenWidth = static_cast<f32>(width());
    const f32 screenHeight = static_cast<f32>(height());
    const f32 mapSize = std::min(screenWidth, screenHeight) * 0.45f;
    const f64 mapX = static_cast<f64>((screenWidth - mapSize) / 2.0f);
    const f64 mapY = static_cast<f64>((screenHeight - mapSize) / 2.0f - screenHeight * 0.08f);

    m_mapRenderer->renderMap(mapId, mapX, mapY, static_cast<f64>(mapSize), true, mapData);
}

void HudWidget::_renderHotbar(kagero::widget::PaintContext& ctx, renderer::trident::gui::GuiRenderer& gui)
{
    const f32 screenWidth = static_cast<f32>(width());
    const f32 screenHeight = static_cast<f32>(height());

    // 快捷栏位置（底部居中）
    f32 hotbarX = (screenWidth - HOTBAR_WIDTH) / 2.0f;
    f32 hotbarY = screenHeight - HOTBAR_HEIGHT - HOTBAR_OFFSET_Y;

    // 尝试使用纹理绘制快捷栏背景（使用 widgetsAtlas）
    if (m_widgetsAtlas != nullptr && m_widgetsAtlas->hasSprite("hotbar_bg")) {
        auto image = m_widgetsAtlas->createTextureImage("hotbar_bg");
        if (image.isValid()) {
            ctx.drawImage(image, static_cast<i32>(hotbarX), static_cast<i32>(hotbarY));
        }
    } else {
        // 纯色后备绘制（当纹理不可用时）
        ctx.drawFilledRect(kagero::Rect{static_cast<i32>(hotbarX),
                               static_cast<i32>(hotbarY),
                               static_cast<i32>(HOTBAR_WIDTH),
                               static_cast<i32>(HOTBAR_HEIGHT)},
            HudColors::HOTBAR_BACKGROUND);
        ctx.drawBorder(kagero::Rect{static_cast<i32>(hotbarX),
                           static_cast<i32>(hotbarY),
                           static_cast<i32>(HOTBAR_WIDTH),
                           static_cast<i32>(HOTBAR_HEIGHT)},
            1.0f,
            HudColors::HOTBAR_BORDER);
    }

    // 绘制槽位
    const auto& inventory = m_player->inventory();
    i32 selectedSlot = inventory.getSelectedSlot();

    for (i32 i = 0; i < 9; ++i) {
        f32 slotX = hotbarX + i * SLOT_SPACING + 1.0f;
        f32 slotY = hotbarY + 2.0f;

        // 选中槽位高亮 - 使用纹理或纯色（使用 widgetsAtlas）
        if (i == selectedSlot) {
            if (m_widgetsAtlas != nullptr && m_widgetsAtlas->hasSprite("hotbar_selection")) {
                auto image = m_widgetsAtlas->createTextureImage("hotbar_selection");
                if (image.isValid()) {
                    ctx.drawImage(image, static_cast<i32>(slotX - 1), static_cast<i32>(slotY - 1));
                }
            } else {
                ctx.drawFilledRect(kagero::Rect{static_cast<i32>(slotX - 1),
                                       static_cast<i32>(slotY - 1),
                                       static_cast<i32>(SLOT_SIZE + 2),
                                       static_cast<i32>(SLOT_SIZE + 2)},
                    HudColors::HOTBAR_SLOT_HIGHLIGHT);
            }
        }

        // 绘制物品（如果有）
        const ItemStack& stack = inventory.getItem(i);
        if (!stack.isEmpty()) {
            // 绘制物品图标
            if (m_itemRenderer != nullptr) {
                m_itemRenderer->renderItem(gui, stack, slotX + 1, slotY + 1, 16.0f);
            }

            // 绘制物品数量
            if (stack.getCount() > 1) {
                std::string countText = std::to_string(stack.getCount());
                f32 textWidth = ctx.getTextWidth(std::string(countText.begin(), countText.end()));
                ctx.drawText(std::string(countText.begin(), countText.end()),
                    static_cast<i32>(slotX + SLOT_SIZE - textWidth - 1.0f),
                    static_cast<i32>(slotY + SLOT_SIZE - 8.0f),
                    HudColors::TOOLTIP_TEXT);
            }
        }
    }
}

void HudWidget::_renderHealth(kagero::widget::PaintContext& ctx)
{
    const f32 screenWidth = static_cast<f32>(width());
    const f32 screenHeight = static_cast<f32>(height());

    // 生命值位置（快捷栏左上方）
    f32 healthX = (screenWidth - HOTBAR_WIDTH) / 2.0f;
    f32 healthY = screenHeight - HOTBAR_HEIGHT - HEALTH_OFFSET_Y - HEART_SIZE;

    // 获取生命值
    f32 health = m_player->health();
    i32 absorption = static_cast<i32>(m_player->absorptionAmount());
    i32 armor = m_player->armorValue();

    // 绘制盔甲值（在生命值上方）
    if (armor > 0) {
        f32 armorX = healthX;
        f32 armorY = healthY - ARMOR_SIZE - 2.0f;
        for (i32 i = 0; i < 10; ++i) {
            bool full = i < armor / 2;
            _drawArmor(ctx, armorX + i * ARMOR_SPACING, armorY, full);
        }
    }

    // 绘制心形（每行最多10颗）
    for (i32 i = 0; i < 10; ++i) {
        f32 heartX = healthX + i * HEART_SPACING;

        // 计算当前心的状态
        i32 heartPoints = static_cast<i32>(health) - i * 2;
        bool full = heartPoints >= 2;
        bool half = heartPoints == 1;

        // 吸收心显示为黄色
        if (absorption > 0 && i < absorption / 2) {
            _drawHeart(ctx, heartX, healthY, full, half, true);
        } else {
            _drawHeart(ctx, heartX, healthY, full, half, false);
        }
    }
}

void HudWidget::_renderHunger(kagero::widget::PaintContext& ctx)
{
    const f32 screenWidth = static_cast<f32>(width());
    const f32 screenHeight = static_cast<f32>(height());

    // 饥饿值位置（快捷栏右上方，从右到左绘制，间距 8px）
    f32 hungerX = (screenWidth + HOTBAR_WIDTH) / 2.0f - HUNGER_SPACING * 10;
    f32 hungerY = screenHeight - HOTBAR_HEIGHT - HEALTH_OFFSET_Y - HUNGER_SIZE;

    // 获取饥饿数据
    const auto& foodStats = m_player->foodStats();
    i32 food = foodStats.foodLevel();
    f32 saturation = foodStats.saturationLevel();
    bool hasHungerEffect = m_player->hasEffect(entity::effect::EffectType::Hunger);

    // 饥饿条抖动动画
    // 当饱和度 <= 0 时，饥饿图标会上下抖动
    // 抖动频率与饥饿值成反比：饥饿值越低抖动越快
    // 使用确定性随机种子确保帧内一致性
    const u32 tickCount = m_player->ticksExisted();
    m_random.setSeed(static_cast<u64>(tickCount) * 312871ULL);

    // 绘制饥饿图标（从右到左）
    for (i32 i = 0; i < 10; ++i) {
        f32 iconX = hungerX + (9 - i) * HUNGER_SPACING;
        f32 iconY = hungerY;

        // 抖动偏移：饱和度 <= 0 且满足 tick 条件时随机偏移 ±1px
        if (saturation <= 0.0f && food > 0 && tickCount % (static_cast<u32>(food) * 3 + 1) == 0) {
            iconY += static_cast<f32>(m_random.nextInt(3) - 1);
        }

        i32 foodPoints = food - i * 2;
        bool full = foodPoints >= 2;
        bool half = foodPoints == 1;
        _drawHunger(ctx, iconX, iconY, full, half, hasHungerEffect);
    }
}

void HudWidget::_renderExperience(kagero::widget::PaintContext& ctx)
{
    const f32 screenWidth = static_cast<f32>(width());
    const f32 screenHeight = static_cast<f32>(height());

    // 经验条位置（快捷栏下方）
    f32 xpX = (screenWidth - XP_BAR_WIDTH) / 2.0f;
    f32 xpY = screenHeight - HOTBAR_HEIGHT - XP_BAR_HEIGHT - 4.0f;

    // 获取经验值
    i32 level = m_player->experienceLevel();
    f32 progress = m_player->experienceProgress();

    // 绘制经验条（使用纹理或纯色）
    _drawExperienceBar(ctx, xpX, xpY, progress, XP_BAR_WIDTH, XP_BAR_HEIGHT);

    // 绘制等级文字（在经验条上方居中）
    if (level > 0) {
        std::string levelText = std::to_string(level);
        std::string levelStr(levelText.begin(), levelText.end());
        f32 textWidth = ctx.getTextWidth(levelStr);
        ctx.drawText(levelStr,
            static_cast<i32>((screenWidth - textWidth) / 2.0f),
            static_cast<i32>(xpY + XP_TEXT_OFFSET_Y),
            HudColors::XP_TEXT);
    }
}

// ============================================================================
// 私有方法
// ============================================================================

void HudWidget::_drawHeart(kagero::widget::PaintContext& ctx, f32 x, f32 y, bool full, bool half, bool absorbing)
{
    // 尝试使用纹理绘制（使用 iconsAtlas）
    if (m_iconsAtlas != nullptr) {
        std::string spriteId;
        if (absorbing) {
            // 吸收心（黄色）
            if (full) {
                spriteId = "heart_absorbing_full";
            } else if (half) {
                spriteId = "heart_absorbing_half";
            } else {
                spriteId = "heart_empty";
            }
        } else {
            // 正常心（红色）
            if (full) {
                spriteId = "heart_full";
            } else if (half) {
                spriteId = "heart_half";
            } else {
                spriteId = "heart_empty";
            }
        }

        if (m_iconsAtlas->hasSprite(spriteId)) {
            auto image = m_iconsAtlas->createTextureImage(spriteId);
            if (image.isValid()) {
                ctx.drawImage(image, static_cast<i32>(x), static_cast<i32>(y));
                return;
            }
        }
    }

    // 后备：纯色绘制
    u32 color = absorbing ? HudColors::HEALTH_YELLOW : HudColors::HEALTH_RED;
    if (full || half) {
        ctx.drawFilledRect(
            kagero::Rect{
                static_cast<i32>(x), static_cast<i32>(y), static_cast<i32>(HEART_SIZE), static_cast<i32>(HEART_SIZE)},
            color);
    } else {
        ctx.drawFilledRect(
            kagero::Rect{
                static_cast<i32>(x), static_cast<i32>(y), static_cast<i32>(HEART_SIZE), static_cast<i32>(HEART_SIZE)},
            HudColors::HEALTH_EMPTY);
    }
}

void HudWidget::_drawHunger(kagero::widget::PaintContext& ctx, f32 x, f32 y, bool full, bool half, bool hasHungerEffect)
{
    // 尝试使用纹理绘制（使用 iconsAtlas）
    if (m_iconsAtlas != nullptr) {
        std::string spriteId;
        if (hasHungerEffect) {
            // 饥饿效果（绿色变体图标）
            if (full) {
                spriteId = "hunger_saturated_full";
            } else if (half) {
                spriteId = "hunger_saturated_half";
            } else {
                spriteId = "hunger_saturated_empty";
            }
        } else {
            // 正常图标
            if (full) {
                spriteId = "hunger_full";
            } else if (half) {
                spriteId = "hunger_half";
            } else {
                spriteId = "hunger_empty";
            }
        }

        if (m_iconsAtlas->hasSprite(spriteId)) {
            auto image = m_iconsAtlas->createTextureImage(spriteId);
            if (image.isValid()) {
                ctx.drawImage(image, static_cast<i32>(x), static_cast<i32>(y));
                return;
            }
        }
    }

    // 后备：纯色绘制
    u32 color = (full || half) ? HudColors::HUNGER_FULL : HudColors::HUNGER_EMPTY;
    if (hasHungerEffect && (full || half)) {
        // 饥饿效果时使用绿色变体（对应 MC 原版的绿色调）
        color = HudColors::HUNGER_EFFECT;
    }
    ctx.drawFilledRect(
        kagero::Rect{
            static_cast<i32>(x), static_cast<i32>(y), static_cast<i32>(HUNGER_SIZE), static_cast<i32>(HUNGER_SIZE)},
        color);
}

void HudWidget::_drawArmor(kagero::widget::PaintContext& ctx, f32 x, f32 y, bool full)
{
    // 尝试使用纹理绘制（使用 iconsAtlas）
    if (m_iconsAtlas != nullptr) {
        std::string spriteId = full ? "armor_full" : "armor_empty";

        if (m_iconsAtlas->hasSprite(spriteId)) {
            auto image = m_iconsAtlas->createTextureImage(spriteId);
            if (image.isValid()) {
                ctx.drawImage(image, static_cast<i32>(x), static_cast<i32>(y));
                return;
            }
        }
    }

    // 后备：纯色绘制
    u32 color = full ? HudColors::HEALTH_RED : HudColors::HEALTH_EMPTY;
    ctx.drawFilledRect(
        kagero::Rect{
            static_cast<i32>(x), static_cast<i32>(y), static_cast<i32>(ARMOR_SIZE), static_cast<i32>(ARMOR_SIZE)},
        color);
}

void HudWidget::_drawExperienceBar(kagero::widget::PaintContext& ctx, f32 x, f32 y, f32 progress, f32 width, f32 height)
{
    // 尝试使用纹理绘制经验条背景（使用 iconsAtlas）
    if (m_iconsAtlas != nullptr && m_iconsAtlas->hasSprite("xp_bar_empty")) {
        auto emptyImage =
            m_iconsAtlas->createTextureImage("xp_bar_empty", static_cast<i32>(width), static_cast<i32>(height));
        if (emptyImage.isValid()) {
            ctx.drawImage(emptyImage, static_cast<i32>(x), static_cast<i32>(y));
        }

        // 绘制进度部分
        if (progress > 0.0f && m_iconsAtlas->hasSprite("xp_bar_full")) {
            auto fullImage = m_iconsAtlas->createTextureImage(
                "xp_bar_full", static_cast<i32>(width * progress), static_cast<i32>(height));
            if (fullImage.isValid()) {
                // 使用裁剪绘制进度条
                ctx.drawImage(fullImage, static_cast<i32>(x), static_cast<i32>(y));
            }
        }
        return;
    }

    // 后备：纯色绘制
    ctx.drawFilledRect(
        kagero::Rect{static_cast<i32>(x), static_cast<i32>(y), static_cast<i32>(width), static_cast<i32>(height)},
        HudColors::XP_BACKGROUND);

    if (progress > 0.0f) {
        ctx.drawFilledRect(
            kagero::Rect{
                static_cast<i32>(x), static_cast<i32>(y), static_cast<i32>(width * progress), static_cast<i32>(height)},
            HudColors::XP_FOREGROUND);
    }
}

} // namespace mc::client::ui::minecraft::widgets
