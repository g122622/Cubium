#pragma once

#include "common/entity/inventory/CreativeInventory.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/screen/IScreen.hpp"
#include "core/Types.hpp"
#include <functional>
#include <vector>

namespace mc::client::renderer::trident::gui {
class GuiRenderer;
class GuiTextureManager;
}

namespace mc::client::renderer::trident::item {
class ItemRenderer;
}

namespace mc::client {

/**
 * @brief 创造模式物品库屏幕
 *
 * 该屏幕使用本地模型维护物品条目、搜索框、滚动窗口和玩家背包编辑。
 * 点击物品条目会将物品装入本地“持有物品”，再通过槽位点击完成背包写回。
 */
class CreativeScreen : public IScreen {
public:
    using CreativeActionSender = std::function<void(i32, const ItemStack&)>;

    /**
     * @brief 构造创造模式屏幕
     * @param inventory 玩家背包引用
     * @param actionSender 槽位变更发送器
     */
    explicit CreativeScreen(PlayerInventory& inventory, CreativeActionSender actionSender);

    /**
     * @brief 设置渲染器
     * @param gui GUI 渲染器
     * @param textureManager GUI 纹理管理器
     * @param itemRenderer 物品渲染器
     */
    void setRenderers(renderer::trident::gui::GuiRenderer* gui,
                      renderer::trident::gui::GuiTextureManager* textureManager,
                      renderer::trident::item::ItemRenderer* itemRenderer);

    /**
     * @brief 设置屏幕尺寸
     * @param width 屏幕宽度
     * @param height 屏幕高度
     */
    void setScreenSize(i32 width, i32 height);

    /**
     * @brief 初始化屏幕
     */
    void init() override;

    /**
     * @brief 渲染屏幕
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param partialTick 部分tick时间
     */
    void render(i32 mouseX, i32 mouseY, f32 partialTick) override;

    /**
     * @brief 处理鼠标点击
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param button 鼠标按钮
     * @return 如果事件被处理返回true
     */
    bool onClick(i32 mouseX, i32 mouseY, i32 button) override;

    /**
     * @brief 处理键盘按键
     * @param key 键码
     * @param scanCode 扫描码
     * @param action 动作
     * @param mods 修饰键
     * @return 如果事件被处理返回true
     */
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

    /**
     * @brief 处理字符输入
     * @param codePoint Unicode 码点
     * @return 如果事件被处理返回true
     */
    bool onChar(u32 codePoint) override;

    /**
     * @brief 处理鼠标滚轮
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param delta 滚轮增量
     * @return 如果事件被处理返回true
     */
    bool onScroll(i32 mouseX, i32 mouseY, f64 delta) override;

    /**
     * @brief 屏幕关闭
     */
    void onClose() override;

    /**
     * @brief 创造屏幕不暂停游戏
     */
    [[nodiscard]] bool isPauseScreen() const override;

    /**
     * @brief 获取屏幕标题
     */
    [[nodiscard]] String getTitle() const override;

    /**
     * @brief 窗口尺寸变化时调用
     * @param width 新宽度
     * @param height 新高度
     */
    void onResize(i32 width, i32 height) override;

private:
    static constexpr i32 SLOT_SIZE = 16;
    static constexpr i32 SLOT_SPACING = 18;
    static constexpr i32 PALETTE_COLUMNS = 9;
    static constexpr i32 PALETTE_VISIBLE_ROWS = 5;
    static constexpr i32 GUI_WIDTH = 360;
    static constexpr i32 GUI_HEIGHT = 226;
    static constexpr i32 PALETTE_X = 8;
    static constexpr i32 PALETTE_Y = 26;
    static constexpr i32 SEARCH_X = 8;
    static constexpr i32 SEARCH_Y = 6;
    static constexpr i32 SEARCH_WIDTH = 160;
    static constexpr i32 SEARCH_HEIGHT = 16;
    static constexpr i32 TRASH_X = 170;
    static constexpr i32 TRASH_Y = 6;
    static constexpr i32 INVENTORY_X = 180;
    static constexpr i32 INVENTORY_Y = 6;
    static constexpr i32 ARMOR_X = 8;
    static constexpr i32 ARMOR_Y_HEAD = 8;
    static constexpr i32 ARMOR_Y_CHEST = 26;
    static constexpr i32 ARMOR_Y_LEGS = 44;
    static constexpr i32 ARMOR_Y_FEET = 62;
    static constexpr i32 OFFHAND_X = 77;
    static constexpr i32 OFFHAND_Y = 62;
    static constexpr i32 PLAYER_INV_X = 8;
    static constexpr i32 PLAYER_INV_Y = 84;
    static constexpr i32 HOTBAR_X = 8;
    static constexpr i32 HOTBAR_Y = 142;
    static constexpr i32 TITLE_X = 8;
    static constexpr i32 TITLE_Y = 6;

    void updateLayout();
    void rebuildVisibleEntries();
    void renderBackground();
    void renderPanelBackground();
    void renderSearchBox();
    void renderPaletteGrid(i32 mouseX, i32 mouseY);
    void renderPlayerInventory(i32 mouseX, i32 mouseY);
    void renderItemIcon(const ItemStack& stack, i32 screenX, i32 screenY);
    void renderItemCount(i32 count, i32 screenX, i32 screenY);
    void renderSlotFrame(i32 screenX, i32 screenY, u32 borderColor, u32 fillColor);
    void renderItemTooltip(const ItemStack& stack, i32 mouseX, i32 mouseY);
    void renderCarriedItem(i32 mouseX, i32 mouseY);
    void handlePaletteClick(i32 paletteIndex, i32 button);
    void handleInventoryClick(i32 slotIndex, i32 button);
    void sendInventorySlotUpdate(i32 slotIndex);
    [[nodiscard]] i32 getPaletteIndexAt(i32 mouseX, i32 mouseY) const;
    [[nodiscard]] i32 getInventorySlotAt(i32 mouseX, i32 mouseY) const;
    [[nodiscard]] i32 getPaletteCellX(i32 column) const;
    [[nodiscard]] i32 getPaletteCellY(i32 row) const;
    [[nodiscard]] i32 getMaxScrollRows() const;
    [[nodiscard]] bool isMouseOver(i32 mouseX, i32 mouseY, i32 x, i32 y, i32 width, i32 height) const;
    [[nodiscard]] String normalizeSearchText(StringView text) const;
    [[nodiscard]] bool matchesSearch(const CreativeInventoryEntry& entry) const;

    PlayerInventory* m_inventory;
    CreativeActionSender m_actionSender;
    std::vector<CreativeInventoryEntry> m_paletteEntries;
    std::vector<i32> m_visibleEntries;
    ItemStack m_carriedItem;
    String m_searchText;
    i32 m_scrollRows = 0;
    i32 m_leftPos = 0;
    i32 m_topPos = 0;
    i32 m_screenWidth = 0;
    i32 m_screenHeight = 0;
    bool m_initialized = false;

    renderer::trident::gui::GuiRenderer* m_gui = nullptr;
    renderer::trident::gui::GuiTextureManager* m_textureManager = nullptr;
    renderer::trident::item::ItemRenderer* m_itemRenderer = nullptr;
};

} // namespace mc::client