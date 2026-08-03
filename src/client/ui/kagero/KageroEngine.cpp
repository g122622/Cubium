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

#include "KageroEngine.hpp"
#include "Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/paint/contracts/ICanvas.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::client::ui::kagero {

KageroEngine::KageroEngine() = default;

KageroEngine::~KageroEngine() = default;

Result<void> KageroEngine::initialize(paint::ICanvas& canvas, const KageroConfig& config)
{
    m_canvas = &canvas;
    m_screenWidth = config.screenWidth;
    m_screenHeight = config.screenHeight;

    // 创建绘图上下文
    m_context = std::make_unique<widget::PaintContext>(canvas);

    spdlog::info("KageroEngine initialized: {}x{}", m_screenWidth, m_screenHeight);
    return Result<void>::ok();
}

void KageroEngine::render()
{
    if (m_layers.empty()) {
        return;
    }

    // 按Z索引从低到高渲染
    for (const auto& layer : m_layers) {
        if (layer.visible) {
            layer.widget->paint(*m_context);
        }
    }
}

void KageroEngine::update(f32 dt)
{
    if (m_layers.empty()) {
        return;
    }

    // 更新所有可见且激活的层
    for (const auto& layer : m_layers) {
        if (layer.visible && layer.widget->isActive()) {
            layer.widget->tick(dt);
        }
    }
}

void KageroEngine::resize(i32 width, i32 height)
{
    m_screenWidth = width;
    m_screenHeight = height;

    // 通知所有层尺寸变化
    for (const auto& layer : m_layers) {
        layer.widget->onResize(width, height);
    }
}

size_t KageroEngine::addLayer(std::unique_ptr<widget::Widget> widget, i32 zIndex)
{
    if (widget == nullptr) {
        spdlog::warn("KageroEngine::addLayer: widget is null");
        return 0;
    }

    LayerInfo layer;
    layer.widget = std::move(widget);
    layer.zIndex = zIndex;
    layer.id = m_nextLayerId++;
    layer.visible = true;
    layer.modal = false;

    // 设置层的尺寸为屏幕尺寸
    layer.widget->setBounds(Rect(0, 0, m_screenWidth, m_screenHeight));

    // 初始化Widget生命周期
    layer.widget->init();

    m_layers.push_back(std::move(layer));

    // 保持排序
    _sortLayers();

    return m_layers.back().id;
}

bool KageroEngine::removeLayer(size_t layerId)
{
    size_t index = _findLayerIndex(layerId);
    if (index == SIZE_MAX) {
        return false;
    }

    m_layers.erase(m_layers.begin() + static_cast<ptrdiff_t>(index));
    return true;
}

void KageroEngine::setLayerVisible(size_t layerId, bool visible)
{
    size_t index = _findLayerIndex(layerId);
    if (index != SIZE_MAX) {
        m_layers[index].visible = visible;
        m_layers[index].widget->setVisible(visible);
    }
}

widget::Widget* KageroEngine::getLayer(size_t layerId)
{
    size_t index = _findLayerIndex(layerId);
    if (index != SIZE_MAX) {
        return m_layers[index].widget.get();
    }
    return nullptr;
}

const widget::Widget* KageroEngine::getLayer(size_t layerId) const
{
    size_t index = _findLayerIndex(layerId);
    if (index != SIZE_MAX) {
        return m_layers[index].widget.get();
    }
    return nullptr;
}

bool KageroEngine::handleClick(i32 x, i32 y, i32 button, i32 mods)
{
    // 从顶层开始处理（Z索引高的先处理）
    for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
        if (!it->visible || !it->widget->isActive()) {
            continue;
        }

        if (it->widget->onClick(x, y, button, mods)) {
            // 拖动跟踪
            m_draggingWidget = it->widget.get();
            m_dragButton = button;
            m_dragMods = mods;

            // 触发拖拽开始事件（与 MC Java版 ContainerEventHandler 一致：
            // 点击命中后立即触发 onDragStart，无阈值，由 Widget 自行管理拖拽状态）
            m_draggingWidget->onDragStart(x, y, button, mods);

            // 双击检测（与Java版MouseHandler一致：同一Widget、同一按钮、250ms内）
            auto now = std::chrono::steady_clock::now();
            auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            bool isDoubleClick = (m_lastClickWidget == it->widget.get() && m_lastClickButton == button &&
                (nowMs - m_lastClickTimeMs) < DOUBLE_CLICK_THRESHOLD_MS);

            if (isDoubleClick) {
                // 触发双击事件
                it->widget->onDoubleClick(x, y, button, mods);
                // 重置双击状态，防止三击被误判为双击
                m_lastClickWidget = nullptr;
                m_lastClickButton = -1;
                m_lastClickTimeMs = 0;
            } else {
                // 记录本次点击信息
                m_lastClickWidget = it->widget.get();
                m_lastClickX = x;
                m_lastClickY = y;
                m_lastClickButton = button;
                m_lastClickTimeMs = nowMs;
            }

            // 右键点击分发
            // 注意：右键点击时 onClick 和 onRightClick 都会触发，这是有意为之的设计。
            // 组件应在 onClick 中检查 button 参数来区分左右键，或仅处理左键点击。
            if (button == 1) {
                it->widget->onRightClick(x, y, mods);
            }

            // modal层阻止事件向下传播
            return true;
        }

        // modal层阻止事件向下传播
        if (it->modal) {
            return false;
        }
    }

    return false;
}

bool KageroEngine::handleRelease(i32 x, i32 y, i32 button, i32 mods)
{
    (void)mods;
    // 如果有正在拖动的Widget，先触发 onDragEnd，再发送释放事件
    if (m_draggingWidget != nullptr) {
        widget::Widget* w = m_draggingWidget;
        // 拖拽正常结束（鼠标释放），dropped = false
        //
        // TODO: 当前 dropped 标志始终为 false，未实现「拖拽被外部取消」的场景，例如：
        //   - 焦点丢失（窗口失焦、模态对话框弹出）
        //   - 拖拽过程中 Widget 被移除或销毁
        //   - 外部取消（Esc 键、强制清除 m_draggingWidget）
        // 未来应在 KageroEngine 增加拖拽取消路径（如 onCancelDrag / clearDragState），
        // 并在这些路径中调用 onDragEnd(..., dropped=true) 以让 Widget 区分「正常释放」
        // 与「异常取消」，从而决定是否提交/回滚拖拽结果。
        w->onDragEnd(x, y, m_dragButton, /*dropped=*/false);
        m_draggingWidget = nullptr;
        m_dragButton = 0;
        m_dragMods = 0;
        return w->onRelease(x, y, button, mods);
    }

    // 从顶层开始处理
    for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
        if (!it->visible || !it->widget->isActive()) {
            continue;
        }

        if (it->widget->onRelease(x, y, button, mods)) {
            return true;
        }

        // modal层阻止事件向下传播
        if (it->modal) {
            return false;
        }
    }

    return false;
}

bool KageroEngine::handleMouseMove(i32 x, i32 y)
{
    // 计算鼠标增量
    i32 deltaX = 0;
    i32 deltaY = 0;
    if (m_hasLastMousePos) {
        deltaX = x - m_lastMouseX;
        deltaY = y - m_lastMouseY;
    }
    m_lastMouseX = x;
    m_lastMouseY = y;
    m_hasLastMousePos = true;

    // 更新悬停状态
    for (auto& layer : m_layers) {
        if (layer.visible) {
            layer.widget->updateHover(x, y);
        }
    }

    // 如果有正在拖动的Widget，发送拖动事件
    if (m_draggingWidget != nullptr) {
        return m_draggingWidget->onDrag(x, y, deltaX, deltaY, m_dragButton);
    }

    return false;
}

bool KageroEngine::handleScroll(i32 x, i32 y, f64 delta)
{
    // 从顶层开始处理
    for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
        if (!it->visible || !it->widget->isActive()) {
            continue;
        }

        if (it->widget->onScroll(x, y, delta)) {
            return true;
        }

        // modal层阻止事件向下传播
        if (it->modal) {
            return false;
        }
    }

    return false;
}

bool KageroEngine::handleKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    // 从顶层开始处理
    for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
        if (!it->visible || !it->widget->isActive()) {
            continue;
        }

        if (it->widget->onKey(key, scanCode, action, mods)) {
            return true;
        }

        // modal层阻止事件向下传播
        if (it->modal) {
            return false;
        }
    }

    return false;
}

bool KageroEngine::handleChar(u32 codePoint)
{
    // 从顶层开始处理
    for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
        if (!it->visible || !it->widget->isActive()) {
            continue;
        }

        if (it->widget->onChar(codePoint)) {
            return true;
        }

        // modal层阻止事件向下传播
        if (it->modal) {
            return false;
        }
    }

    return false;
}

void KageroEngine::_sortLayers()
{
    std::stable_sort(
        m_layers.begin(), m_layers.end(), [](const LayerInfo& a, const LayerInfo& b) { return a.zIndex < b.zIndex; });
}

size_t KageroEngine::_findLayerIndex(size_t layerId) const
{
    for (size_t i = 0; i < m_layers.size(); ++i) {
        if (m_layers[i].id == layerId) {
            return i;
        }
    }
    return SIZE_MAX;
}

} // namespace mc::client::ui::kagero
