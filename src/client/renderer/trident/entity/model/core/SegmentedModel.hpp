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

#include "EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <string>
#include <unordered_map>

namespace mc::client::renderer::entity::model {

/**
 * @brief 分段模型基类
 *
 * 用于复杂实体（如末影龙）的分段渲染。
 * 分段渲染允许模型的各个部分独立动画和渲染。
 */
class SegmentedModel : public EntityModel {
public:
    using SegmentFunc = std::function<void(ModelRenderer&, f64)>;

    SegmentedModel() = default;
    ~SegmentedModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    /**
     * @brief 设置分段渲染函数
     *
     * @param partName 部件名称
     * @param func 渲染函数
     */
    void setSegmentRenderer(const std::string& partName, SegmentFunc func);

    /**
     * @brief 渲染指定分段
     *
     * @param partName 部件名称
     * @param scale 缩放因子
     */
    void renderSegment(const std::string& partName, f64 scale);

    /**
     * @brief 检查分段是否存在
     */
    [[nodiscard]] bool hasSegment(const std::string& partName) const;

protected:
    std::unordered_map<std::string, SegmentFunc> m_segmentRenderers;
};

} // namespace mc::client::renderer::entity::model
