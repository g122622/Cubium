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
#include <algorithm>
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

bool KageroEngine::handleClick(i32 x, i32 y, i32 button)
{
    // 从顶层开始处理（Z索引高的先处理）
    for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
        if (!it->visible || !it->widget->isActive()) {
            continue;
        }

        if (it->widget->onClick(x, y, button)) {
            m_draggingWidget = it->widget.get();
            m_dragButton = button;
            return true;
        }

        // modal层阻止事件向下传播
        if (it->modal) {
            return false;
        }
    }

    return false;
}

bool KageroEngine::handleRelease(i32 x, i32 y, i32 button)
{
    // 如果有正在拖动的Widget，发送释放事件
    if (m_draggingWidget != nullptr) {
        widget::Widget* w = m_draggingWidget;
        m_draggingWidget = nullptr;
        m_dragButton = 0;
        return w->onRelease(x, y, button);
    }

    // 从顶层开始处理
    for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
        if (!it->visible || !it->widget->isActive()) {
            continue;
        }

        if (it->widget->onRelease(x, y, button)) {
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
        return m_draggingWidget->onDrag(x, y, deltaX, deltaY);
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
