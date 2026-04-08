#pragma once

#include "common/screen/IScreen.hpp"
#include "entity/inventory/AbstractContainerMenu.hpp"
#include "entity/inventory/ContainerTypes.hpp"
#include "entity/inventory/Slot.hpp"
#include "network/packet/InventoryPackets.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "core/Types.hpp"
#include <memory>
#include <functional>

namespace mc::client::renderer::trident::gui {
class GuiRenderer;
class GuiTextureManager;
}

namespace mc::client::renderer::trident::item {
class ItemRenderer;
}

namespace mc::client {

/**
 * @brief 容器屏幕基类
 *
 * 管理容器菜单的客户端屏幕，处理槽位渲染和交互。
 * 与服务端的AbstractContainerMenu配对使用。
 *
 * 槽位渲染：
 * - 槽位背景
 * - 槽位物品
 * - 槽位高亮
 * - 鼠标悬停提示
 *
 * 使用示例：
 * @code
 * class CraftingScreen : public AbstractContainerScreen<CraftingMenu> {
 * public:
 *     void render(i32 mouseX, i32 mouseY, f32 partialTick) override;
 *     bool onClick(i32 mouseX, i32 mouseY, i32 button) override;
 * };
 * @endcode
 *
 * @tparam Menu 菜单类型
 */
template<typename Menu>
class AbstractContainerScreen : public IScreen {
public:
    using ContainerClickSender = std::function<void(ContainerId, i32, i32, ClickAction, const mc::ItemStack&)>;
    using ContainerCloseSender = std::function<void(ContainerId)>;

    /**
     * @brief 构造函数
     * @param menu 菜单实例
     */
    explicit AbstractContainerScreen(std::unique_ptr<Menu> menu,
                                     ContainerClickSender clickSender = {},
                                     ContainerCloseSender closeSender = {})
        : m_menu(std::move(menu))
        , m_clickSender(std::move(clickSender))
        , m_closeSender(std::move(closeSender))
        , m_leftPos(0)
        , m_topPos(0)
        , m_imageWidth(176)
        , m_imageHeight(166)
        , m_screenWidth(0)
        , m_screenHeight(0)
        , m_initialized(false) {
    }

    /**
     * @brief 析构函数
     */
    ~AbstractContainerScreen() override = default;

    // 禁止拷贝
    AbstractContainerScreen(const AbstractContainerScreen&) = delete;
    AbstractContainerScreen& operator=(const AbstractContainerScreen&) = delete;

    // 允许移动
    AbstractContainerScreen(AbstractContainerScreen&&) noexcept = default;
    AbstractContainerScreen& operator=(AbstractContainerScreen&&) noexcept = default;

    /**
     * @brief 设置渲染器
     * @param gui GUI渲染器
     * @param textureManager GUI纹理管理器（可选）
     * @param itemRenderer 物品渲染器（可选）
     */
    void setRenderers(renderer::trident::gui::GuiRenderer* gui,
                      renderer::trident::gui::GuiTextureManager* textureManager = nullptr,
                      renderer::trident::item::ItemRenderer* itemRenderer = nullptr) {
        m_gui = gui;
        m_textureManager = textureManager;
        m_itemRenderer = itemRenderer;
    }

    /**
     * @brief 设置屏幕尺寸
     * @param width 屏幕宽度
     * @param height 屏幕高度
     */
    void setScreenSize(i32 width, i32 height) {
        m_screenWidth = width;
        m_screenHeight = height;
        updatePosition();
    }

    /**
     * @brief 初始化屏幕
     */
    void init() override {
        if (!m_initialized) {
            m_initialized = true;
            onInit();
        }
    }

    /**
     * @brief 渲染屏幕
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param partialTick 部分tick时间
     */
    void render(i32 mouseX, i32 mouseY, f32 partialTick) override {
        (void)partialTick;

        if (m_gui == nullptr) {
            return;
        }

        // 开始GUI帧
        m_gui->beginFrame(static_cast<f32>(m_screenWidth), static_cast<f32>(m_screenHeight));

        // 渲染背景
        if (shouldRenderBackground()) {
            renderBackground();
        }

        // 渲染容器GUI
        renderContainerBackground();
        renderSlots();
        renderContainerForeground(mouseX, mouseY);

        // 渲染鼠标持有的物品
        renderCarriedItem(mouseX, mouseY);

        // 渲染悬停提示
        renderTooltip(mouseX, mouseY);
    }

    /**
     * @brief 处理鼠标点击
     */
    bool onClick(i32 mouseX, i32 mouseY, i32 button) override {
        if (m_menu == nullptr) {
            return false;
        }

        // 查找点击的槽位
        mc::Slot* slot = getSlotAt(mouseX, mouseY);
        if (slot != nullptr) {
            return onSlotClick(*slot, slot->getIndex(), button);
        }

        // 点击空白区域
        return onClickOutside(mouseX, mouseY, button);
    }

    /**
     * @brief 处理键盘按键
     */
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override {
        // ESC关闭屏幕
        if (key == 256 && action == 1) { // GLFW_KEY_ESCAPE
            onClose();
            return true;
        }

        // E键关闭容器屏幕
        if (key == 69 && action == 1) { // GLFW_KEY_E
            onClose();
            return true;
        }

        return false;
    }

    /**
     * @brief 屏幕关闭
     */
    void onClose() override {
        if (m_closeSender && m_menu != nullptr && m_menu->getId() != mc::inventory::PLAYER_CONTAINER_ID) {
            m_closeSender(m_menu->getId());
        }
    }

    /**
     * @brief 容器屏幕不暂停游戏
     */
    [[nodiscard]] bool isPauseScreen() const override {
        return false;
    }

    /**
     * @brief 获取菜单
     */
    [[nodiscard]] Menu* getMenu() { return m_menu.get(); }
    [[nodiscard]] const Menu* getMenu() const { return m_menu.get(); }

    /**
     * @brief 获取鼠标持有的物品
     */
    [[nodiscard]] mc::ItemStack& getCarriedItem() {
        return m_menu ? m_menu->getCarriedItem() : m_emptyStack;
    }
    [[nodiscard]] const mc::ItemStack& getCarriedItem() const {
        return m_menu ? m_menu->getCarriedItem() : m_emptyStack;
    }

    /**
     * @brief 获取GUI左边界
     */
    [[nodiscard]] i32 getLeftPos() const { return m_leftPos; }

    /**
     * @brief 获取GUI上边界
     */
    [[nodiscard]] i32 getTopPos() const { return m_topPos; }

    /**
     * @brief 获取GUI宽度
     */
    [[nodiscard]] i32 getImageWidth() const { return m_imageWidth; }

    /**
     * @brief 获取GUI高度
     */
    [[nodiscard]] i32 getImageHeight() const { return m_imageHeight; }

    /**
     * @brief 获取GUI渲染器
     */
    [[nodiscard]] renderer::trident::gui::GuiRenderer* getGuiRenderer() { return m_gui; }
    [[nodiscard]] const renderer::trident::gui::GuiRenderer* getGuiRenderer() const { return m_gui; }

protected:
    // 槽位尺寸常量
    static constexpr i32 SLOT_SIZE = 16;
    static constexpr i32 SLOT_SPACING = 18;

    /**
     * @brief 子类初始化回调
     */
    virtual void onInit() {}

    /**
     * @brief 设置GUI尺寸
     * @param width 宽度
     * @param height 高度
     */
    void setImageSize(i32 width, i32 height) {
        m_imageWidth = width;
        m_imageHeight = height;
        updatePosition();
    }

    /**
     * @brief 更新GUI位置（居中）
     */
    void updatePosition() {
        // 居中计算
        if (m_screenWidth > 0 && m_screenHeight > 0) {
            m_leftPos = (m_screenWidth - m_imageWidth) / 2;
            m_topPos = (m_screenHeight - m_imageHeight) / 2;
        } else {
            m_leftPos = 0;
            m_topPos = 0;
        }
    }

    /**
     * @brief 渲染背景暗化
     */
    virtual void renderBackground() {
        // 半透明黑色背景 (ARGB)
        if (m_gui != nullptr) {
            m_gui->fillRect(0.0f, 0.0f,
                           static_cast<f32>(m_screenWidth),
                           static_cast<f32>(m_screenHeight),
                           0x80000000);
        }
    }

    /**
     * @brief 渲染容器背景
     */
    virtual void renderContainerBackground() {
        // 子类实现具体的背景渲染
    }

    /**
     * @brief 渲染所有槽位
     */
    virtual void renderSlots() {
        if (m_menu == nullptr || m_gui == nullptr) {
            return;
        }

        for (i32 i = 0; i < m_menu->getSlotCount(); ++i) {
            const mc::Slot* slot = m_menu->getSlot(i);
            if (slot != nullptr) {
                renderSlot(*slot, m_leftPos + slot->getX(), m_topPos + slot->getY());
            }
        }
    }

    /**
     * @brief 渲染单个槽位
     * @param slot 槽位
     * @param screenX 屏幕X坐标
     * @param screenY 屏幕Y坐标
     */
    virtual void renderSlot(const mc::Slot& slot, i32 screenX, i32 screenY) {
        if (m_gui == nullptr) {
            return;
        }

        // 渲染槽位背景（可选，纹理中已包含槽位背景）
        // 如果需要单独渲染槽位高亮，可以在这里添加

        // 渲染物品
        const mc::ItemStack& stack = slot.getItem();
        if (!stack.isEmpty()) {
            renderItemIcon(stack, screenX, screenY);

            // 渲染物品数量
            if (stack.getCount() > 1) {
                renderItemCount(stack.getCount(), screenX + SLOT_SIZE - 2, screenY + SLOT_SIZE - 8);
            }
        }
    }

    /**
     * @brief 渲染物品图标（子类可重写以使用ItemRenderer）
     * @param stack 物品堆
     * @param screenX 屏幕X坐标
     * @param screenY 屏幕Y坐标
     */
    virtual void renderItemIcon(const mc::ItemStack& stack, i32 screenX, i32 screenY) {
        // 默认实现：绘制占位符矩形
        // 子类可以重写此方法，使用 ItemRenderer 渲染实际的物品图标
        if (m_gui == nullptr) {
            return;
        }

        (void)stack;
        // 使用半透明颜色表示物品存在
        m_gui->fillRect(static_cast<f32>(screenX),
                        static_cast<f32>(screenY),
                        static_cast<f32>(SLOT_SIZE),
                        static_cast<f32>(SLOT_SIZE),
                        0x80FFFFFF);
    }

    /**
     * @brief 渲染物品数量
     * @param count 数量
     * @param screenX 屏幕X坐标
     * @param screenY 屏幕Y坐标
     */
    void renderItemCount(i32 count, i32 screenX, i32 screenY) {
        if (m_gui == nullptr || m_gui->font() == nullptr || count <= 1) {
            return;
        }

        String countText = std::to_string(count);
        m_gui->drawText(countText,
                        static_cast<f32>(screenX),
                        static_cast<f32>(screenY),
                        0xFFFFFFFF, true);
    }

    /**
     * @brief 渲染槽位高亮
     * @param screenX 屏幕X坐标
     * @param screenY 屏幕Y坐标
     */
    void renderSlotHighlight(i32 screenX, i32 screenY) {
        if (m_gui == nullptr) {
            return;
        }

        // 槽位高亮颜色 (ARGB，半透明白色)
        m_gui->fillRect(static_cast<f32>(screenX),
                        static_cast<f32>(screenY),
                        static_cast<f32>(SLOT_SIZE),
                        static_cast<f32>(SLOT_SIZE),
                        0x40FFFFFF);
    }

    /**
     * @brief 渲染容器前景（标题等）
     */
    virtual void renderContainerForeground(i32 mouseX, i32 mouseY) {
        (void)mouseX;
        (void)mouseY;
        // 子类可重写以渲染标题
    }

    /**
     * @brief 渲染鼠标持有的物品
     */
    virtual void renderCarriedItem(i32 mouseX, i32 mouseY) {
        const auto& carried = getCarriedItem();
        if (carried.isEmpty() || m_gui == nullptr) {
            return;
        }

        // 渲染跟随鼠标的物品
        renderItemIcon(carried, mouseX - SLOT_SIZE / 2, mouseY - SLOT_SIZE / 2);

        // 渲染物品数量
        if (carried.getCount() > 1) {
            renderItemCount(carried.getCount(), mouseX + SLOT_SIZE / 2 - 2, mouseY + SLOT_SIZE / 2 - 8);
        }
    }

    /**
     * @brief 渲染悬停提示
     */
    virtual void renderTooltip(i32 mouseX, i32 mouseY) {
        mc::Slot* slot = getSlotAt(mouseX, mouseY);
        if (slot != nullptr && !slot->getItem().isEmpty()) {
            // TODO: 渲染物品提示
            (void)mouseX;
            (void)mouseY;
        }
    }

    /**
     * @brief 获取指定位置的槽位
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @return 槽位指针，如果无效返回nullptr
     */
    [[nodiscard]] mc::Slot* getSlotAt(i32 mouseX, i32 mouseY) {
        if (m_menu == nullptr) {
            return nullptr;
        }

        // 遍历所有槽位查找
        for (i32 i = 0; i < m_menu->getSlotCount(); ++i) {
            mc::Slot* slot = m_menu->getSlot(i);
            if (slot != nullptr && isMouseOverSlot(*slot, mouseX, mouseY)) {
                return slot;
            }
        }
        return nullptr;
    }

    /**
     * @brief 检查鼠标是否在槽位上
     */
    [[nodiscard]] virtual bool isMouseOverSlot(const mc::Slot& slot, i32 mouseX, i32 mouseY) const {
        i32 slotX = m_leftPos + slot.getX();
        i32 slotY = m_topPos + slot.getY();
        return mouseX >= slotX && mouseX < slotX + SLOT_SIZE &&
               mouseY >= slotY && mouseY < slotY + SLOT_SIZE;
    }

    /**
     * @brief 槽位点击处理
     */
    virtual bool onSlotClick(mc::Slot& slot, i32 slotIndex, i32 button) {
        if (m_menu == nullptr) {
            return false;
        }

        if (m_clickSender && m_menu->getId() != mc::inventory::PLAYER_CONTAINER_ID) {
            const ClickAction action = (button == 0) ? ClickAction::Pick : ClickAction::Pickup;
            m_clickSender(m_menu->getId(), slotIndex, button, action, m_menu->getCarriedItem());
        }

        (void)slot;
        return true;
    }

    /**
     * @brief 点击空白区域处理
     */
    virtual bool onClickOutside(i32 mouseX, i32 mouseY, i32 button) {
        (void)mouseX;
        (void)mouseY;
        (void)button;
        return false;
    }

    // 成员变量
    std::unique_ptr<Menu> m_menu;
    ContainerClickSender m_clickSender;
    ContainerCloseSender m_closeSender;

    // 渲染器
    renderer::trident::gui::GuiRenderer* m_gui = nullptr;
    renderer::trident::gui::GuiTextureManager* m_textureManager = nullptr;
    renderer::trident::item::ItemRenderer* m_itemRenderer = nullptr;

    i32 m_leftPos;          ///< GUI左边界（居中后的位置）
    i32 m_topPos;           ///< GUI上边界
    i32 m_imageWidth;       ///< GUI纹理宽度
    i32 m_imageHeight;      ///< GUI纹理高度
    i32 m_screenWidth;      ///< 屏幕宽度
    i32 m_screenHeight;     ///< 屏幕高度
    bool m_initialized;     ///< 是否已初始化

    // 空物品堆（用于空菜单时返回）
    static mc::ItemStack m_emptyStack;
};

// 静态成员定义
template<typename Menu>
mc::ItemStack AbstractContainerScreen<Menu>::m_emptyStack;

} // namespace mc::client
