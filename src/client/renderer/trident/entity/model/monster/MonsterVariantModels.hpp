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

#include "../base/BipedModel.hpp"
#include "SpiderModel.hpp"

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 僵尸村民模型
 *
 * 参考 MC 1.16.5 ZombieVillagerModel
 */
class ZombieVillagerModel : public BipedModel {
public:
    using BipedModel::setupParts;

    ZombieVillagerModel();
    explicit ZombieVillagerModel(f32 scale, bool slim);
    ~ZombieVillagerModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    void setHeadVisible(bool visible);

private:
    void setupParts(f32 scale, bool slim);

    std::shared_ptr<ModelRenderer> m_villagerNose;
};

/**
 * @brief 溺尸模型
 *
 * 参考 MC 1.16.5 DrownedModel (extends ZombieModel)
 */
class DrownedModel : public BipedModel {
public:
    using BipedModel::setupParts;

    DrownedModel();
    explicit DrownedModel(f32 scale, bool slim);
    ~DrownedModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void setupParts(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight);
};

/**
 * @brief 流浪者模型
 *
 * 参考 MC 1.16.5 SkeletonModel (same structure, different texture)
 */
class StrayModel : public BipedModel {
public:
    StrayModel();
    ~StrayModel() override = default;
};

/**
 * @brief 尸壳模型
 *
 * 参考 MC 1.16.5 ZombieModel (same structure, different texture)
 */
class HuskModel : public BipedModel {
public:
    HuskModel();
    ~HuskModel() override = default;
};

/**
 * @brief 洞穴蜘蛛模型
 *
 * 参考 MC 1.16.5 - CaveSpiderRenderer 直接使用 SpiderModel 并缩放 0.7 倍
 * Java 中没有独立的 CaveSpiderModel 类，而是复用 SpiderModel
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
 * 参考 MC 1.16.5 GiantModel (same as ZombieModel but larger)
 */
class GiantModel : public BipedModel {
public:
    GiantModel();
    ~GiantModel() override = default;
};

} // namespace mc::client::renderer::entity::model::monster
