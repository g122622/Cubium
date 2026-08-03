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

#include "Widget.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace mc {

// 前向声明
class Entity;
class Player;

} // namespace mc

namespace mc::client::ui::kagero::widget {

/**
 * @brief 3D视口组件
 *
 * 在UI中渲染3D实体的组件，用于：
 * - 玩家背包界面的玩家模型
 * - 物品展示
 * - 实体预览
 *
 * 使用示例：
 * @code
 * auto viewport = std::make_unique<Viewport3DWidget>("viewport", 50, 50, 80, 80);
 * viewport->setEntity(player);
 * viewport->setCameraDistance(30.0f);
 * viewport->setOnDrag([](Viewport3DWidget& v, i32 dx, i32 dy) {
 *     v.rotate(dx * 0.5f, dy * 0.5f);
 * });
 * @endcode
 */
class Viewport3DWidget : public Widget {
public:
    /**
     * @brief 渲染模式
     */
    enum class RenderMode : u8 {
        Entity, ///< 渲染实体
        Item,   ///< 渲染物品
        Block   ///< 渲染方块
    };

    /**
     * @brief 默认构造函数
     */
    Viewport3DWidget() = default;

    /**
     * @brief 构造函数
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     */
    Viewport3DWidget(std::string id, i32 x, i32 y, i32 width, i32 height)
        : Widget(std::move(id))
    {
        setBounds(Rect(x, y, width, height));
    }

    // ==================== 生命周期 ====================

    void init() override
    {
        // 初始化帧缓冲区等资源
        initFramebuffer();
    }

    void paint(PaintContext& ctx) override
    {
        if (!isVisible()) return;
        ctx.drawFilledRect(bounds(), Colors::fromARGB(255, 16, 16, 20));
        ctx.drawBorder(bounds(), 1.0f, Colors::fromARGB(255, 90, 90, 120));
    }

    // ==================== 事件处理 ====================

    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)mods;

        if (!isActive() || !isVisible()) return false;

        if (button == 0) { // 左键拖动旋转
            m_dragging = true;
            m_lastMouseX = mouseX;
            m_lastMouseY = mouseY;
            return true;
        }

        return false;
    }

    bool onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)mods;

        if (button == 0 && m_dragging) {
            m_dragging = false;
            return true;
        }

        return false;
    }

    bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)button;

        if (!m_dragging) return false;

        // 旋转视角
        m_yaw += static_cast<f32>(deltaX) * m_rotationSensitivity;
        m_pitch += static_cast<f32>(deltaY) * m_rotationSensitivity;

        // 限制俯仰角
        m_pitch = std::max(-90.0f, std::min(90.0f, m_pitch));

        return true;
    }

    bool onDragStart(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mods;
        // onClick 已经设置了 m_dragging 并记录起始坐标，onDragStart 仅确认拖拽按钮
        if (button != 0) return false;
        return m_dragging;
    }

    bool onDragEnd(i32 mouseX, i32 mouseY, i32 button, bool dropped) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)dropped;
        // onDragEnd 仅作为拖拽结束通知，不清理 m_dragging。
        // 状态清理由 onRelease 统一完成（与原设计一致，避免双重清理）。
        if (button != 0) return false;
        return m_dragging;
    }

    bool onScroll(i32 mouseX, i32 mouseY, f64 delta) override
    {
        (void)mouseX;
        (void)mouseY;

        if (!isActive() || !isVisible()) return false;

        // 缩放距离
        m_cameraDistance -= static_cast<f32>(delta) * m_zoomSensitivity;
        m_cameraDistance = std::max(m_minDistance, std::min(m_maxDistance, m_cameraDistance));

        return true;
    }

    // ==================== 实体操作 ====================

    /**
     * @brief 设置要渲染的实体
     */
    void setEntity(mc::Entity* entity)
    {
        m_entity = entity;
        m_renderMode = RenderMode::Entity;
    }

    /**
     * @brief 获取要渲染的实体
     */
    [[nodiscard]] mc::Entity* entity() const { return m_entity; }

    /**
     * @brief 设置要渲染的玩家
     */
    void setPlayer(mc::Player* player)
    {
        m_player = player;
        m_entity = nullptr; // Player继承自Entity
        m_renderMode = RenderMode::Entity;
    }

    /**
     * @brief 获取要渲染的玩家
     */
    [[nodiscard]] mc::Player* player() const { return m_player; }

    // ==================== 相机操作 ====================

    /**
     * @brief 设置相机距离
     */
    void setCameraDistance(f32 distance)
    {
        m_cameraDistance = std::max(m_minDistance, std::min(m_maxDistance, distance));
    }

    /**
     * @brief 获取相机距离
     */
    [[nodiscard]] f32 cameraDistance() const { return m_cameraDistance; }

    /**
     * @brief 设置相机旋转
     */
    void setRotation(f32 pitch, f32 yaw)
    {
        m_pitch = std::max(-90.0f, std::min(90.0f, pitch));
        m_yaw = yaw;
    }

    /**
     * @brief 旋转相机
     */
    void rotate(f32 deltaPitch, f32 deltaYaw)
    {
        m_pitch += deltaPitch;
        m_yaw += deltaYaw;
        m_pitch = std::max(-90.0f, std::min(90.0f, m_pitch));
    }

    /**
     * @brief 获取俯仰角
     */
    [[nodiscard]] f32 pitch() const { return m_pitch; }

    /**
     * @brief 获取偏航角
     */
    [[nodiscard]] f32 yaw() const { return m_yaw; }

    /**
     * @brief 重置相机角度
     */
    void resetRotation()
    {
        m_pitch = 0.0f;
        m_yaw = 180.0f; // 默认面向正面
    }

    // ==================== 显示属性 ====================

    /**
     * @brief 设置渲染模式
     */
    void setRenderMode(RenderMode mode) { m_renderMode = mode; }

    /**
     * @brief 获取渲染模式
     */
    [[nodiscard]] RenderMode renderMode() const { return m_renderMode; }

    /**
     * @brief 设置背景颜色
     */
    void setBackgroundColor(u32 color) { m_backgroundColor = color; }

    /**
     * @brief 获取背景颜色
     */
    [[nodiscard]] u32 backgroundColor() const { return m_backgroundColor; }

    /**
     * @brief 设置是否显示背景
     */
    void setShowBackground(bool show) { m_showBackground = show; }

    /**
     * @brief 是否显示背景
     */
    [[nodiscard]] bool showBackground() const { return m_showBackground; }

    /**
     * @brief 设置旋转灵敏度
     */
    void setRotationSensitivity(f32 sensitivity) { m_rotationSensitivity = sensitivity; }

    /**
     * @brief 设置缩放灵敏度
     */
    void setZoomSensitivity(f32 sensitivity) { m_zoomSensitivity = sensitivity; }

    /**
     * @brief 设置缩放范围
     */
    void setZoomRange(f32 minDistance, f32 maxDistance)
    {
        m_minDistance = minDistance;
        m_maxDistance = maxDistance;
        m_cameraDistance = std::max(minDistance, std::min(maxDistance, m_cameraDistance));
    }

    /**
     * @brief 设置是否自动旋转
     */
    void setAutoRotate(bool autoRotate) { m_autoRotate = autoRotate; }

    /**
     * @brief 是否自动旋转
     */
    [[nodiscard]] bool autoRotate() const { return m_autoRotate; }

    /**
     * @brief 设置自动旋转速度
     */
    void setAutoRotateSpeed(f32 speed) { m_autoRotateSpeed = speed; }

    /**
     * @brief 获取自动旋转速度
     */
    [[nodiscard]] f32 autoRotateSpeed() const { return m_autoRotateSpeed; }

protected:
    /**
     * @brief 初始化帧缓冲区
     */
    void initFramebuffer()
    {
        // TODO: 创建Vulkan帧缓冲区
        // m_framebuffer = ...
    }

    /**
     * @brief 渲染实体到帧缓冲区
     */
    void renderEntityToFBO()
    {
        // TODO: 实现实体渲染
    }

    /**
     * @brief 渲染物品到帧缓冲区
     */
    void renderItemToFBO()
    {
        // TODO: 实现物品渲染
    }

    /**
     * @brief 渲染方块到帧缓冲区
     */
    void renderBlockToFBO()
    {
        // TODO: 实现方块渲染
    }

    // 渲染目标
    mc::Entity* m_entity = nullptr;
    mc::Player* m_player = nullptr;
    RenderMode m_renderMode = RenderMode::Entity;

    // 相机参数
    f32 m_cameraDistance = 30.0f;
    f32 m_pitch = 0.0f;
    f32 m_yaw = 180.0f;
    f32 m_minDistance = 5.0f;
    f32 m_maxDistance = 100.0f;
    f32 m_rotationSensitivity = 2.0f;
    f32 m_zoomSensitivity = 2.0f;

    // 自动旋转
    bool m_autoRotate = false;
    f32 m_autoRotateSpeed = 1.0f;

    // 显示属性
    bool m_showBackground = false;
    u32 m_backgroundColor = 0x00000000; // 透明背景

    // 交互状态
    bool m_dragging = false;
    i32 m_lastMouseX = 0;
    i32 m_lastMouseY = 0;

    // Vulkan资源（后续实现）
    // VkFramebuffer m_framebuffer;
    // VkRenderPass m_renderPass;
    // VkImage m_colorImage;
    // VkImageView m_colorView;
    // VkDescriptorSet m_descriptorSet;
};

} // namespace mc::client::ui::kagero::widget
