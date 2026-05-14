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

#include "client/ui/screen/AbstractContainerScreen.hpp"
#include "core/Types.hpp"
#include "server/menu/CraftingMenu.hpp"

namespace mc::client::renderer::trident::gui {
class GuiRenderer;
class GuiTextureManager;
} // namespace mc::client::renderer::trident::gui

namespace mc::client {

/**
 * @brief 工作台屏幕 (3x3合成)
 *
 * 显示工作台的合成网格、结果槽位和玩家背包。
 *
 * 槽位布局：
 * - 槽位 0-8: 合成网格 (3x3)
 * - 槽位 9: 结果槽位
 * - 槽位 10-36: 玩家主背包 (3x9)
 * - 槽位 37-45: 玩家快捷栏 (1x9)
 *
 * GUI尺寸：
 * - 宽度: 176 像素
 * - 高度: 166 像素
 */
class CraftingScreen : public AbstractContainerScreen<mc::CraftingMenu> {
public:
    /**
     * @brief 构造函数
     * @param menu 工作台菜单
     */
    explicit CraftingScreen(std::unique_ptr<mc::CraftingMenu> menu,
        ContainerClickSender clickSender = {},
        ContainerCloseSender closeSender = {});

    /**
     * @brief 获取屏幕标题
     */
    [[nodiscard]] std::string getTitle() const override { return "Crafting"; }

protected:
    /**
     * @brief 子类初始化
     */
    void onInit() override;

    /**
     * @brief 渲染容器背景
     */
    void renderContainerBackground() override;

    /**
     * @brief 渲染容器前景
     */
    void renderContainerForeground(i32 mouseX, i32 mouseY) override;

    /**
     * @brief 槽位点击处理
     */
    bool onSlotClick(mc::Slot& slot, i32 slotIndex, i32 button) override;

    /**
     * @brief 渲染物品图标（使用ItemRenderer）
     */
    void renderItemIcon(const mc::ItemStack& stack, i32 screenX, i32 screenY) override;

    /**
     * @brief 渲染悬停提示
     */
    void renderTooltip(i32 mouseX, i32 mouseY) override;

private:
    /**
     * @brief 渲染合成网格
     */
    void renderCraftingGrid();

    /**
     * @brief 渲染结果槽位
     */
    void renderResultSlot();

    /**
     * @brief 渲染玩家背包区域
     */
    void renderPlayerInventory();

    /**
     * @brief 检查是否是结果槽位
     */
    [[nodiscard]] bool isResultSlot(i32 slotIndex) const { return slotIndex == mc::CraftingMenu::RESULT_SLOT; }

    /**
     * @brief 检查是否是合成网格槽位
     */
    [[nodiscard]] bool isGridSlot(i32 slotIndex) const
    {
        return slotIndex >= mc::CraftingMenu::GRID_SLOT_START &&
            slotIndex < mc::CraftingMenu::GRID_SLOT_START + mc::CraftingMenu::GRID_SLOT_COUNT;
    }

    // GUI尺寸常量
    static constexpr i32 GUI_WIDTH = 176;
    static constexpr i32 GUI_HEIGHT = 166;

    // 合成网格位置（相对于GUI左上角）
    static constexpr i32 GRID_X = 30;
    static constexpr i32 GRID_Y = 17;

    // 结果槽位位置
    static constexpr i32 RESULT_X = 123;
    static constexpr i32 RESULT_Y = 35;

    // 玩家背包位置
    static constexpr i32 PLAYER_INV_X = 8;
    static constexpr i32 PLAYER_INV_Y = 84;

    // 标题位置
    static constexpr i32 TITLE_X = 28;
    static constexpr i32 TITLE_Y = 6;
};

/**
 * @brief 玩家背包合成屏幕 (2x2合成)
 *
 * 显示玩家背包内的2x2合成网格、护甲槽、副手槽和玩家背包。
 *
 * 槽位布局（参考 MC 1.16.5 PlayerContainer）：
 * - 槽位 0: 合成结果 (154, 28)
 * - 槽位 1-4: 合成网格 (2x2) (98, 18) 到 (116, 36)
 * - 槽位 5-8: 护甲 (8, 8), (8, 26), (8, 44), (8, 62)
 * - 槽位 9-35: 玩家主背包 (3x9) (8, 84)
 * - 槽位 36-44: 玩家快捷栏 (1x9) (8, 142)
 * - 槽位 45: 副手 (77, 62)
 *
 * GUI尺寸：176 x 166 像素
 * 纹理：minecraft:textures/gui/container/inventory.png
 */
class InventoryCraftingScreen : public AbstractContainerScreen<mc::InventoryCraftingMenu> {
public:
    /**
     * @brief 构造函数
     * @param menu 玩家背包合成菜单
     */
    explicit InventoryCraftingScreen(std::unique_ptr<mc::InventoryCraftingMenu> menu,
        ContainerClickSender clickSender = {},
        ContainerCloseSender closeSender = {});

    /**
     * @brief 获取屏幕标题
     */
    [[nodiscard]] std::string getTitle() const override { return "Inventory"; }

protected:
    /**
     * @brief 子类初始化
     */
    void onInit() override;

    /**
     * @brief 渲染容器背景
     */
    void renderContainerBackground() override;

    /**
     * @brief 渲染容器前景（标题等）
     */
    void renderContainerForeground(i32 mouseX, i32 mouseY) override;

    /**
     * @brief 槽位点击处理
     */
    bool onSlotClick(mc::Slot& slot, i32 slotIndex, i32 button) override;

    /**
     * @brief 渲染物品图标（使用ItemRenderer）
     */
    void renderItemIcon(const mc::ItemStack& stack, i32 screenX, i32 screenY) override;

    /**
     * @brief 渲染悬停提示
     */
    void renderTooltip(i32 mouseX, i32 mouseY) override;

private:
    /**
     * @brief 渲染合成网格
     */
    void renderCraftingGrid();

    /**
     * @brief 渲染结果槽位
     */
    void renderResultSlot();

    /**
     * @brief 渲染护甲槽位
     */
    void renderArmorSlots();

    /**
     * @brief 渲染副手槽位
     */
    void renderOffhandSlot();

    /**
     * @brief 渲染玩家背包区域
     */
    void renderPlayerInventory();

    /**
     * @brief 检查是否是结果槽位
     */
    [[nodiscard]] bool isResultSlot(i32 slotIndex) const { return slotIndex == mc::InventoryCraftingMenu::RESULT_SLOT; }

    /**
     * @brief 检查是否是合成网格槽位
     */
    [[nodiscard]] bool isGridSlot(i32 slotIndex) const
    {
        return slotIndex >= mc::InventoryCraftingMenu::GRID_SLOT_START &&
            slotIndex <= mc::InventoryCraftingMenu::GRID_SLOT_END;
    }

    /**
     * @brief 检查是否是护甲槽位
     */
    [[nodiscard]] bool isArmorSlot(i32 slotIndex) const
    {
        return slotIndex >= mc::InventoryCraftingMenu::ARMOR_SLOT_START &&
            slotIndex < mc::InventoryCraftingMenu::ARMOR_SLOT_START + mc::InventoryCraftingMenu::ARMOR_SLOT_COUNT;
    }

    /**
     * @brief 检查是否是副手槽位
     */
    [[nodiscard]] bool isOffhandSlot(i32 slotIndex) const
    {
        return slotIndex == mc::InventoryCraftingMenu::OFFHAND_SLOT;
    }

    // ========== GUI尺寸常量 ==========
    static constexpr i32 GUI_WIDTH = 176;
    static constexpr i32 GUI_HEIGHT = 166;

    // ========== 槽位位置常量（相对于GUI左上角）==========
    // 合成网格位置
    static constexpr i32 GRID_X = 98;
    static constexpr i32 GRID_Y = 18;

    // 结果槽位位置
    static constexpr i32 RESULT_X = 154;
    static constexpr i32 RESULT_Y = 28;

    // 护甲槽位置（从上到下：头盔、胸甲、护腿、靴子）
    static constexpr i32 ARMOR_X = 8;
    static constexpr i32 ARMOR_Y_HEAD = 8;
    static constexpr i32 ARMOR_Y_CHEST = 26;
    static constexpr i32 ARMOR_Y_LEGS = 44;
    static constexpr i32 ARMOR_Y_FEET = 62;

    // 副手槽位置
    static constexpr i32 OFFHAND_X = 77;
    static constexpr i32 OFFHAND_Y = 62;

    // 玩家背包位置
    static constexpr i32 PLAYER_INV_X = 8;
    static constexpr i32 PLAYER_INV_Y = 84;

    // 快捷栏位置
    static constexpr i32 HOTBAR_X = 8;
    static constexpr i32 HOTBAR_Y = 142;

    // 标题位置
    static constexpr i32 TITLE_X = 8;
    static constexpr i32 TITLE_Y = 6;
};

} // namespace mc::client
