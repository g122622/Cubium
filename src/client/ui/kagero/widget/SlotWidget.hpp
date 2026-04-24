#pragma once

#include "Widget.hpp"
#include "../paint/PaintContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include <functional>
#include <string>

namespace mc::client::ui::kagero::widget {

// 前向声明
class ItemRenderer2D;

/**
 * @brief 物品槽组件
 *
 * 显示物品槽的组件，支持物品显示、交互和背景。
 *
 * 参考MC 1.16.5 Slot类实现
 *
 * 使用示例：
 * @code
 * auto slot = std::make_unique<SlotWidget>("slot_0", 10, 10);
 * slot->setItem(itemStack);
 * slot->setOnSlotClick([](i32 slotIndex, i32 button, bool shiftHeld) {
 *     // 处理点击
 * });
 * @endcode
 */
class SlotWidget : public Widget {
public:
    /**
     * @brief 槽位点击回调类型（与文档一致）
     *
     * 参数：slotIndex - 槽位索引
     *      button - 鼠标按钮
     *      shiftHeld - 是否按住Shift
     */
    using OnSlotClickCallback = std::function<void(i32, i32, bool)>;

    /**
     * @brief 槽位释放回调类型
     */
    using OnSlotReleaseCallback = std::function<void(SlotWidget&, i32)>;

    /**
     * @brief 默认构造函数
     */
    SlotWidget() = default;

    /**
     * @brief 构造函数
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     */
    SlotWidget(String id, i32 x, i32 y)
        : Widget(std::move(id)) {
        setBounds(Rect(x, y, 16, 16)); // 默认槽位大小16x16
    }

    /**
     * @brief 构造函数（带尺寸）
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param size 尺寸（宽高相等）
     */
    SlotWidget(String id, i32 x, i32 y, i32 size)
        : Widget(std::move(id)) {
        setBounds(Rect(x, y, size, size));
    }

    // ==================== 生命周期 ====================

    void paint(PaintContext& ctx) override {
        if (!isVisible()) return;
        if (m_showBackground) {
            ctx.drawFilledRect(bounds(), Colors::fromARGB(255, 40, 40, 40));
            ctx.drawBorder(bounds(), 1.0f, Colors::fromARGB(255, 100, 100, 100));
        }
        if (isHovered()) {
            ctx.drawBorder(bounds(), 1.0f, m_highlightColor);
        }
    }

    // ==================== 事件处理 ====================

    bool onClick(i32 mouseX, i32 mouseY, i32 button) override {
        (void)mouseX;
        (void)mouseY;

        if (!isActive() || !isVisible()) return false;

        if (m_onSlotClick) {
            m_onSlotClick(m_slotIndex, button, m_shiftHeld);
        }

        return true;
    }

    bool onRelease(i32 mouseX, i32 mouseY, i32 button) override {
        (void)mouseX;
        (void)mouseY;

        if (!isActive() || !isVisible()) return false;

        if (m_onRelease) {
            m_onRelease(*this, button);
        }

        return true;
    }

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override {
        (void)scanCode;

        if (action == 1 || action == 2) {
            // 检测Shift键状态
            m_shiftHeld = (mods & static_cast<i32>(KeyMods::Shift)) != 0;
        }
        return false;
    }

    // ==================== 物品操作 ====================

    /**
     * @brief 设置物品
     */
    void setItem(const mc::ItemStack& item) {
        m_item = item;
    }

    /**
     * @brief 获取物品
     */
    [[nodiscard]] const mc::ItemStack& item() const { return m_item; }

    /**
     * @brief 获取可变物品引用
     */
    [[nodiscard]] mc::ItemStack& item() { return m_item; }

    /**
     * @brief 检查槽位是否为空
     */
    [[nodiscard]] bool isEmpty() const { return m_item.isEmpty(); }

    /**
     * @brief 清空槽位
     */
    void clearItem() {
        m_item = mc::ItemStack();
    }

    // ==================== 属性设置 ====================

    /**
     * @brief 设置槽位索引
     */
    void setSlotIndex(i32 index) { m_slotIndex = index; }

    /**
     * @brief 获取槽位索引
     */
    [[nodiscard]] i32 slotIndex() const { return m_slotIndex; }

    /**
     * @brief 设置背景纹理路径
     */
    void setBackgroundTexture(const String& path) {
        m_backgroundTexture = path;
    }

    /**
     * @brief 获取背景纹理路径
     */
    [[nodiscard]] const String& backgroundTexture() const { return m_backgroundTexture; }

    /**
     * @brief 设置是否显示背景
     */
    void setShowBackground(bool show) { m_showBackground = show; }

    /**
     * @brief 是否显示背景
     */
    [[nodiscard]] bool showBackground() const { return m_showBackground; }

    /**
     * @brief 设置是否可交互
     */
    void setInteractive(bool interactive) { m_interactive = interactive; }

    /**
     * @brief 是否可交互
     */
    [[nodiscard]] bool isInteractive() const { return m_interactive; }

    /**
     * @brief 设置高亮颜色
     */
    void setHighlightColor(u32 color) { m_highlightColor = color; }

    /**
     * @brief 获取高亮颜色
     */
    [[nodiscard]] u32 highlightColor() const { return m_highlightColor; }

    /**
     * @brief 设置是否显示数量
     */
    void setShowCount(bool show) { m_showCount = show; }

    /**
     * @brief 是否显示数量
     */
    [[nodiscard]] bool showCount() const { return m_showCount; }

    // ==================== 回调设置 ====================

    /**
     * @brief 设置槽位点击回调（与文档一致）
     * @param callback 回调函数，参数为(槽位索引, 鼠标按钮, 是否按住Shift)
     */
    void setOnSlotClick(OnSlotClickCallback callback) {
        m_onSlotClick = std::move(callback);
    }

    /**
     * @brief 设置释放回调
     */
    void setOnRelease(OnSlotReleaseCallback callback) {
        m_onRelease = std::move(callback);
    }

protected:
    mc::ItemStack m_item;               ///< 槽位中的物品
    i32 m_slotIndex = -1;               ///< 槽位索引

    // 显示属性
    String m_backgroundTexture;         ///< 背景纹理路径
    bool m_showBackground = true;       ///< 是否显示背景
    bool m_interactive = true;          ///< 是否可交互
    bool m_showCount = true;            ///< 是否显示数量
    bool m_shiftHeld = false;           ///< Shift键是否按下
    u32 m_highlightColor = Colors::fromARGB(128, 255, 255, 255); ///< 高亮颜色

    // 回调
    OnSlotClickCallback m_onSlotClick;  ///< 槽位点击回调
    OnSlotReleaseCallback m_onRelease;  ///< 释放回调
};

} // namespace mc::client::ui::kagero::widget
