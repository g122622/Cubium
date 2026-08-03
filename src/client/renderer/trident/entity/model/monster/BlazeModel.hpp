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

#include <array>
#include <memory>

#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 烈焰人模型
 *
 * 烈焰人由漂浮的头部和环绕的烟雾棒组成。
 */
class BlazeModel : public model::EntityModel {
public:
    BlazeModel();
    ~BlazeModel() override = default;

    void render(f64 scale) override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置烟雾棒数量（12根）
     */
    static constexpr i32 SMOKE_ROD_COUNT = 12;

private:
    void _setupParts();

    // 头部
    std::shared_ptr<model::ModelRenderer> m_head;

    // 烟雾棒（12根）
    std::array<std::shared_ptr<model::ModelRenderer>, SMOKE_ROD_COUNT> m_smokeRods;

    // 动画参数
    f64 m_ageInTicks = 0.0;
};

} // namespace mc::client::renderer::entity::model::monster
