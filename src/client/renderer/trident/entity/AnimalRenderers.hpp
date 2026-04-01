#pragma once

#include "LivingRenderer.hpp"
#include "model/AnimalModels.hpp"

namespace mc::client::renderer {

/**
 * @brief 猪渲染器
 *
 * 参考 MC 1.16.5 PigRenderer
 */
class PigRenderer : public LivingRenderer<PigModel> {
public:
    PigRenderer() {
        m_shadowSize = 0.5f;
    }
    ~PigRenderer() override = default;
};

/**
 * @brief 牛渲染器
 *
 * 参考 MC 1.16.5 CowRenderer
 */
class CowRenderer : public LivingRenderer<CowModel> {
public:
    CowRenderer() {
        m_shadowSize = 0.7f;
    }
    ~CowRenderer() override = default;
};

/**
 * @brief 羊渲染器
 *
 * 参考 MC 1.16.5 SheepRenderer
 */
class SheepRenderer : public LivingRenderer<SheepModel> {
public:
    SheepRenderer() {
        m_shadowSize = 0.7f;
    }
    ~SheepRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override {
        // TODO: 处理羊毛渲染
        // 羊有两个模型层：身体和羊毛
        LivingRenderer<SheepModel>::render(entity, partialTicks);
    }

private:
    // TODO: 羊毛模型
    // SheepWoolModel m_woolModel;
};

/**
 * @brief 鸡渲染器
 *
 * 参考 MC 1.16.5 ChickenRenderer
 */
class ChickenRenderer : public LivingRenderer<ChickenModel> {
public:
    ChickenRenderer() {
        m_shadowSize = 0.3f;
    }
    ~ChickenRenderer() override = default;
};

} // namespace mc::client::renderer
