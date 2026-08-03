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

#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/renderer/trident/entity/model/monster/SpiderModel.hpp"
#include "client/renderer/trident/entity/model/monster/ZombieModel.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 僵尸村民模型
 *
 * 继承自 ZombieModel（对应 MC 1.21.11 ZombieVillagerModel extends ZombieModel），
 * 从而获得僵尸的 animateZombieArms 手臂前伸/攻击动画与 setAggressive 状态。
 */
class ZombieVillagerModel : public ZombieModel {
public:
    ZombieVillagerModel();
    explicit ZombieVillagerModel(f32 scale, bool slim);
    ~ZombieVillagerModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    void setHeadVisible(bool visible);

private:
    void _setupParts(f32 scale, bool slim);

    std::shared_ptr<ModelRenderer> m_villagerNose;
};

/**
 * @brief 溺尸模型
 *
 * 继承自 ZombieModel（对应 MC 1.21.11 DrownedModel extends ZombieModel），
 * 从而获得僵尸的 animateZombieArms 手臂前伸/攻击动画与 setAggressive 状态。
 */
class DrownedModel : public ZombieModel {
public:
    DrownedModel();
    explicit DrownedModel(f32 scale, bool slim);
    ~DrownedModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight);
};

/**
 * @brief 流浪者模型
 *
 * 与骷髅结构相同，只是纹理不同
 */
class StrayModel : public BipedModel {
public:
    StrayModel();
    ~StrayModel() override = default;
};

/**
 * @brief 尸壳模型
 *
 * 继承自 ZombieModel（对应 MC 1.21.11 HuskModel extends ZombieModel），
 * 从而获得僵尸的 animateZombieArms 手臂前伸/攻击动画与 setAggressive 状态。
 * 与僵尸结构相同，只是纹理不同。
 */
class HuskModel : public ZombieModel {
public:
    HuskModel();
    ~HuskModel() override = default;
};

/**
 * @brief 洞穴蜘蛛模型
 *
 * 复用蜘蛛模型，渲染时缩放 0.7 倍
 */
class CaveSpiderModel : public SpiderModel {
public:
    CaveSpiderModel();
    ~CaveSpiderModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    /**
     * @brief 获取洞穴蜘蛛缩放因子
     * @return 0.7f - 洞穴蜘蛛比普通蜘蛛小 0.7 倍
     */
    static constexpr f32 getScaleFactor() { return 0.7f; }
};

/**
 * @brief 巨人模型
 *
 * 继承自 ZombieModel（对应 MC 1.21.11 GiantModel extends ZombieModel），
 * 从而获得僵尸的 animateZombieArms 手臂前伸/攻击动画与 setAggressive 状态。
 * 与僵尸模型相同，渲染时缩放更大。
 */
class GiantModel : public ZombieModel {
public:
    GiantModel();
    ~GiantModel() override = default;
};

} // namespace mc::client::renderer::entity::model::monster
