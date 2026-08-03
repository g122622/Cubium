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

#include "client/renderer/trident/entity/model/animal/VillagerModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 女巫模型
 *
 * 继承自 VillagerModel，在村民模型基础上替换头部为女巫特有的大帽子，
 * 并添加鼻子上的痣（mole）。
 *
 * 女巫模型的独特特征：
 * - 分层式尖顶帽（hat → hat2 → hat3 → hat4），带有微妙的倾斜
 * - 鼻子上的痣（喝药水时鼻子上扬）
 * - 鼻子摆动动画（轻微的摇头效果）
 * - 持有物品时鼻子的特殊姿态
 *
 * 纹理尺寸：64x128（比村民的 64x64 更高，因为帽子占据额外空间）
 *
 * 参考: net.minecraft.client.model.monster.witch.WitchModel (MC 1.21.11)
 */
class WitchModel : public animal::VillagerModel {
public:
    WitchModel();
    ~WitchModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置是否持有物品
     *
     * 当女巫持有药水时，鼻子会上扬并向前突出。
     */
    void setHoldingItem(bool holding) { m_holdingItem = holding; }

    /**
     * @brief 获取是否持有物品
     */
    [[nodiscard]] bool isHoldingItem() const { return m_holdingItem; }

    /**
     * @brief 设置实体ID
     *
     * 实体ID用于计算每个女巫独特的鼻子摆动频率，
     * 频率公式为 0.01 * (entityId % 10)，与MC原版一致。
     */
    void setEntityId(i32 entityId) { m_entityId = entityId; }

    /**
     * @brief 获取实体ID
     */
    [[nodiscard]] i32 entityId() const { return m_entityId; }

    /**
     * @brief 获取鼻子模型部件
     */
    [[nodiscard]] std::shared_ptr<ModelRenderer> getNose() const { return m_nose; }

private:
    // 女巫帽子部件（分层结构：hat → hat2 → hat3 → hat4）
    std::shared_ptr<ModelRenderer> m_witchHat; // 帽檐
    std::shared_ptr<ModelRenderer> m_hat2;     // 帽身下层
    std::shared_ptr<ModelRenderer> m_hat3;     // 帽身中层
    std::shared_ptr<ModelRenderer> m_hat4;     // 帽尖

    // 鼻子上的痣
    std::shared_ptr<ModelRenderer> m_mole;

    // 状态
    bool m_holdingItem = false;
    i32 m_entityId = 0;
};

} // namespace mc::client::renderer::entity::model::monster
