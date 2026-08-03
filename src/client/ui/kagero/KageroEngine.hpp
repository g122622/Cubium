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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "paint/PaintContext.hpp"
#include "paint/contracts/ICanvas.hpp"
#include "widget/ContainerWidget.hpp"
#include "widget/Widget.hpp"
#include <chrono>
#include <cstddef>
#include <memory>
#include <vector>

namespace mc::client::ui::kagero {

/**
 * @brief UI引擎配置
 */
struct KageroConfig {
    i32 screenWidth = 0;
    i32 screenHeight = 0;
};

/**
 * @brief 层级信息
 *
 * 每个层代表UI系统中的一个独立渲染单元，
 * 层之间通过Z索引控制渲染顺序。
 */
struct LayerInfo {
    std::unique_ptr<widget::Widget> widget; ///< 层Widget
    bool visible = true;                    ///< 是否可见
    i32 zIndex = 0;                         ///< Z索引（越大越靠上）
    bool modal = false;                     ///< 是否阻止下层事件
    size_t id = 0;                          ///< 层ID
};

/**
 * @brief Kagero UI引擎
 *
 * 统一管理所有UI组件的渲染、更新和事件分发。
 * 采用分层架构，每层是一个Widget。
 *
 * 使用示例：
 * @code
 * auto engine = std::make_unique<KageroEngine>();
 * engine->initialize(*canvas, {1920, 1080});
 * engine->addLayer(std::make_unique<CrosshairWidget>(), 0);
 * engine->addLayer(std::make_unique<HudWidget>(), 10);
 *
 * // 主循环
 * engine->render();
 * engine->update(deltaTime);
 * @endcode
 */
class KageroEngine {
public:
    KageroEngine();
    ~KageroEngine();

    // 禁止拷贝
    KageroEngine(const KageroEngine&) = delete;
    KageroEngine& operator=(const KageroEngine&) = delete;

    // ==================== 生命周期 ====================

    /**
     * @brief 初始化引擎
     * @param canvas 画布接口
     * @param config 配置
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(paint::ICanvas& canvas, const KageroConfig& config);

    /**
     * @brief 渲染所有可见层
     *
     * 遍历所有注册的层，按Z索引从低到高渲染可见层
     */
    void render();

    /**
     * @brief 更新所有层
     * @param dt 增量时间（秒）
     */
    void update(f32 dt);

    /**
     * @brief 调整画布尺寸
     * @param width 新宽度
     * @param height 新高度
     */
    void resize(i32 width, i32 height);

    // ==================== 层管理 ====================

    /**
     * @brief 添加层
     * @param widget 层Widget
     * @param zIndex Z索引（越大越靠上）
     * @return 层ID（用于后续管理）
     */
    size_t addLayer(std::unique_ptr<widget::Widget> widget, i32 zIndex);

    /**
     * @brief 移除层
     * @param layerId 层ID
     * @return 是否成功移除
     */
    bool removeLayer(size_t layerId);

    /**
     * @brief 设置层可见性
     * @param layerId 层ID
     * @param visible 是否可见
     */
    void setLayerVisible(size_t layerId, bool visible);

    /**
     * @brief 获取层Widget
     * @param layerId 层ID
     * @return Widget指针，如果不存在返回nullptr
     */
    [[nodiscard]] widget::Widget* getLayer(size_t layerId);
    [[nodiscard]] const widget::Widget* getLayer(size_t layerId) const;

    /**
     * @brief 获取层数量
     */
    [[nodiscard]] size_t layerCount() const { return m_layers.size(); }

    // ==================== 事件处理 ====================

    /**
     * @brief 处理鼠标点击
     * @param x 鼠标X坐标
     * @param y 鼠标Y坐标
     * @param button 鼠标按钮
     * @param mods 修饰键状态（GLFW_MOD_SHIFT等）
     * @return 如果事件被处理返回true
     */
    bool handleClick(i32 x, i32 y, i32 button, i32 mods);

    /**
     * @brief 处理鼠标释放
     * @param x 鼠标X坐标
     * @param y 鼠标Y坐标
     * @param button 鼠标按钮
     * @param mods 修饰键状态（GLFW_MOD_SHIFT等）
     * @return 如果事件被处理返回true
     */
    bool handleRelease(i32 x, i32 y, i32 button, i32 mods);

    /**
     * @brief 处理鼠标移动
     * @param x 鼠标X坐标
     * @param y 鼠标Y坐标
     * @return 如果事件被处理返回true
     */
    bool handleMouseMove(i32 x, i32 y);

    /**
     * @brief 处理鼠标滚轮
     * @param x 鼠标X坐标
     * @param y 鼠标Y坐标
     * @param delta 滚轮增量
     * @return 如果事件被处理返回true
     */
    bool handleScroll(i32 x, i32 y, f64 delta);

    /**
     * @brief 处理键盘按键
     * @param key 键码
     * @param scanCode 扫描码
     * @param action 动作
     * @param mods 修饰键
     * @return 如果事件被处理返回true
     */
    bool handleKey(i32 key, i32 scanCode, i32 action, i32 mods);

    /**
     * @brief 处理字符输入
     * @param codePoint Unicode码点
     * @return 如果事件被处理返回true
     */
    bool handleChar(u32 codePoint);

    // ==================== 访问器 ====================

    /**
     * @brief 获取屏幕宽度
     */
    [[nodiscard]] i32 screenWidth() const { return m_screenWidth; }

    /**
     * @brief 获取屏幕高度
     */
    [[nodiscard]] i32 screenHeight() const { return m_screenHeight; }

    /**
     * @brief 获取绘图上下文
     */
    [[nodiscard]] widget::PaintContext& context() { return *m_context; }
    [[nodiscard]] const widget::PaintContext& context() const { return *m_context; }

    /**
     * @brief 获取画布接口
     */
    [[nodiscard]] paint::ICanvas& canvas() { return *m_canvas; }
    [[nodiscard]] const paint::ICanvas& canvas() const { return *m_canvas; }

private:
    /**
     * @brief 对层按Z索引排序
     */
    void _sortLayers();

    /**
     * @brief 查找层索引
     * @param layerId 层ID
     * @return 层索引，如果未找到返回 SIZE_MAX
     */
    [[nodiscard]] size_t _findLayerIndex(size_t layerId) const;

    paint::ICanvas* m_canvas = nullptr;
    std::unique_ptr<widget::PaintContext> m_context;

    std::vector<LayerInfo> m_layers;
    i32 m_screenWidth = 0;
    i32 m_screenHeight = 0;
    size_t m_nextLayerId = 1;

    // 上次鼠标位置（用于计算拖动增量）
    i32 m_lastMouseX = 0;
    i32 m_lastMouseY = 0;
    bool m_hasLastMousePos = false;

    // 当前拖动的Widget
    widget::Widget* m_draggingWidget = nullptr;
    // 当前拖动按钮（与 onDrag/onDragEnd 一起分发，用于区分多按钮场景）
    i32 m_dragButton = 0;
    i32 m_dragMods = 0; ///< 拖拽开始时的修饰键状态

    // 双击检测状态（与Java版MouseHandler的双击检测机制一致）
    static constexpr i64 DOUBLE_CLICK_THRESHOLD_MS = 250; ///< 双击时间阈值（毫秒）
    widget::Widget* m_lastClickWidget = nullptr;          ///< 上次点击的Widget
    i32 m_lastClickX = 0;                                 ///< 上次点击X坐标
    i32 m_lastClickY = 0;                                 ///< 上次点击Y坐标
    i32 m_lastClickButton = -1;                           ///< 上次点击的鼠标按钮（-1表示无）
    i64 m_lastClickTimeMs = 0;                            ///< 上次点击时间戳（毫秒）
};

} // namespace mc::client::ui::kagero
