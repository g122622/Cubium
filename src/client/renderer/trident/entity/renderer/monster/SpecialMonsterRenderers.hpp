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

#include "client/renderer/trident/entity/core/LivingRenderer.hpp"
#include "client/renderer/trident/entity/model/animal/PolarBearModel.hpp"
#include "client/renderer/trident/entity/model/monster/MoreMonsterModels.hpp"
#include "client/renderer/trident/entity/model/monster/SkeletonModel.hpp"
#include "client/renderer/trident/entity/model/monster/SpecialMonsterModels.hpp"
#include "client/renderer/trident/entity/model/monster/WitchModel.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/monster/illager/WitchEntity.hpp"
#include "common/entity/entities/passive/golem/CopperGolemEntity.hpp"
#include "common/entity/entities/passive/golem/CopperGolemTypes.hpp"
#include "common/resource/ResourceLocation.hpp"

namespace mc::client::renderer::entity::renderer::monster {

/**
 * @brief 凋灵渲染器
 *
 * MC Java 中 WitherBossRenderer.getBlockLightLevel() 返回 15，
 * 凋灵在黑暗中也会发光，使用全亮光照。
 */
class WitherRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::WitherModel> {
public:
    WitherRenderer() { m_shadowSize = 0.5f; }
    ~WitherRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/wither/wither.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/wither/wither.png");
    }

    [[nodiscard]] bool isFullbright() const override { return true; }
};

/**
 * @brief 凋灵（护甲）渲染器
 *
 * 凋灵护甲同样使用全亮光照。
 */
class WitherArmorRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::WitherModel> {
public:
    WitherArmorRenderer() { m_shadowSize = 0.5f; }
    ~WitherArmorRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/wither/wither_armor.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/wither/wither_armor.png");
    }

    [[nodiscard]] bool isFullbright() const override { return true; }
};

/**
 * @brief 史莱姆渲染器
 */
class SlimeRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::SlimeModel> {
public:
    SlimeRenderer() { m_shadowSize = 0.25f; }
    ~SlimeRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/slime/slime.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/slime/slime.png");
    }
};

/**
 * @brief 守卫者渲染器
 */
class GuardianRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::GuardianModel> {
public:
    GuardianRenderer() { m_shadowSize = 0.5f; }
    ~GuardianRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/guardian.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/guardian.png");
    }
};

/**
 * @brief 远古守卫者渲染器
 */
class ElderGuardianRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::ElderGuardianModel> {
public:
    ElderGuardianRenderer() { m_shadowSize = 0.75f; }
    ~ElderGuardianRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/guardian_elder.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/guardian_elder.png");
    }
};

/**
 * @brief 潜影贝渲染器
 */
class ShulkerRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::ShulkerModel> {
public:
    ShulkerRenderer() { m_shadowSize = 0.0f; }
    ~ShulkerRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/shulker/shulker.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/shulker/shulker.png");
    }
};

/**
 * @brief 蠹虫渲染器
 */
class SilverfishRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::SilverfishModel> {
public:
    SilverfishRenderer() { m_shadowSize = 0.0f; }
    ~SilverfishRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/silverfish.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/silverfish.png");
    }
};

/**
 * @brief 末影螨渲染器
 */
class EndermiteRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::EndermiteModel> {
public:
    EndermiteRenderer() { m_shadowSize = 0.0f; }
    ~EndermiteRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/endermite.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/endermite.png");
    }
};

/**
 * @brief 灾厄村民渲染器
 */
class IllagerRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::IllagerModel> {
public:
    IllagerRenderer() { m_shadowSize = 0.5f; }
    ~IllagerRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/illager/pillager.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/illager/pillager.png");
    }
};

/**
 * @brief 恼鬼渲染器
 *
 * MC Java 中 VexRenderer.getBlockLightLevel() 返回 15，
 * 恼鬼在黑暗中也会发光，使用全亮光照。
 */
class VexRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::VexModel> {
public:
    VexRenderer() { m_shadowSize = 0.0f; }
    ~VexRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/illager/vex.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/illager/vex.png");
    }

    [[nodiscard]] bool isFullbright() const override { return true; }
};

/**
 * @brief 卫道士渲染器
 */
class VindicatorRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::IllagerModel> {
public:
    VindicatorRenderer() { m_shadowSize = 0.5f; }
    ~VindicatorRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/illager/vindicator.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/illager/vindicator.png");
    }
};

/**
 * @brief 唤魔者渲染器
 */
class EvokerRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::IllagerModel> {
public:
    EvokerRenderer() { m_shadowSize = 0.5f; }
    ~EvokerRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/illager/evoker.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/illager/evoker.png");
    }
};

/**
 * @brief 掠夺者渲染器
 */
class PillagerRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::IllagerModel> {
public:
    PillagerRenderer() { m_shadowSize = 0.5f; }
    ~PillagerRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/illager/pillager.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/illager/pillager.png");
    }
};

/**
 * @brief 劫掠兽渲染器
 */
class RavagerRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::RavagerModel> {
public:
    RavagerRenderer() { m_shadowSize = 0.8f; }
    ~RavagerRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/illager/ravager.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/illager/ravager.png");
    }
};

/**
 * @brief 女巫渲染器
 *
 * 使用 WitchModel 渲染女巫，包含独特的尖帽子和鼻子动画。
 * 当女巫正在喝药水时，鼻子会上扬。
 */
class WitchRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::WitchModel> {
public:
    WitchRenderer() { m_shadowSize = 0.5f; }
    ~WitchRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override
    {
        auto& witch = static_cast<::mc::WitchEntity&>(entity);
        m_model.setHoldingItem(witch.isDrinking());
        m_model.setEntityId(static_cast<i32>(witch.id()));
        LivingRenderer::render(entity, partialTicks);
    }

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/witch.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/witch.png");
    }
};

/**
 * @brief 铁傀儡渲染器
 */
class IronGolemRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::IronGolemModel> {
public:
    IronGolemRenderer() { m_shadowSize = 0.7f; }
    ~IronGolemRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/iron_golem/iron_golem.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/iron_golem/iron_golem.png");
    }
};

/**
 * @brief 雪傀儡渲染器
 */
class SnowGolemRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::SnowGolemModel> {
public:
    SnowGolemRenderer() { m_shadowSize = 0.0f; }
    ~SnowGolemRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/snow_golem.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/snow_golem.png");
    }
};

/**
 * @brief 铜傀儡渲染器
 *
 * 对应 MC 1.21.11: net.minecraft.client.renderer.entity.CopperGolemRenderer
 * 阴影大小 0.5F。纹理根据氧化等级选择：
 *   Unaffected → copper_golem.png
 *   Exposed    → exposed_copper_golem.png
 *   Weathered  → weathered_copper_golem.png
 *   Oxidized   → oxidized_copper_golem.png
 *
 * 注意：MC 原版还添加了发光眼睛层、手持物品层、天线方块装饰层、自定义头层，
 * 本项目当前未实现这些 Layer 子系统，仅渲染主体纹理。
 * TODO: 实现 LivingEntityEmissiveLayer / BlockDecorationLayer 后补充对应层。
 */
class CopperGolemRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::CopperGolemModel> {
public:
    CopperGolemRenderer() { m_shadowSize = 0.5f; }
    ~CopperGolemRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        return getCopperGolemTexture(entity);
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        return getCopperGolemTexture(entity);
    }

private:
    [[nodiscard]] static ResourceLocation getCopperGolemTexture(const ::mc::LivingEntity& entity)
    {
        // 通过 dynamic_cast 获取 CopperGolemEntity 的氧化等级
        const auto* golem = dynamic_cast<const ::mc::CopperGolemEntity*>(&entity);
        if (golem != nullptr) {
            switch (golem->getWeatherState()) {
                case ::mc::entity::CopperGolemWeatherState::Unaffected:
                    return ResourceLocation("minecraft", "textures/entity/copper_golem/copper_golem.png");
                case ::mc::entity::CopperGolemWeatherState::Exposed:
                    return ResourceLocation("minecraft", "textures/entity/copper_golem/exposed_copper_golem.png");
                case ::mc::entity::CopperGolemWeatherState::Weathered:
                    return ResourceLocation("minecraft", "textures/entity/copper_golem/weathered_copper_golem.png");
                case ::mc::entity::CopperGolemWeatherState::Oxidized:
                    return ResourceLocation("minecraft", "textures/entity/copper_golem/oxidized_copper_golem.png");
            }
        }
        // 兜底：未识别实体类型时使用 Unaffected 纹理
        return ResourceLocation("minecraft", "textures/entity/copper_golem/copper_golem.png");
    }
};

/**
 * @brief 蜜蜂渲染器
 */
class BeeRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::BeeModel> {
public:
    BeeRenderer() { m_shadowSize = 0.0f; }
    ~BeeRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/bee/bee.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/bee/bee.png");
    }
};

/**
 * @brief 蜜蜂（愤怒）渲染器
 */
class BeeAngryRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::BeeModel> {
public:
    BeeAngryRenderer() { m_shadowSize = 0.0f; }
    ~BeeAngryRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/bee/bee_angry.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/bee/bee_angry.png");
    }
};

/**
 * @brief 狐狸渲染器
 */
class FoxRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::FoxModel> {
public:
    FoxRenderer() { m_shadowSize = 0.4f; }
    ~FoxRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/fox/fox.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/fox/fox.png");
    }
};

/**
 * @brief 狐狸（白色）渲染器
 */
class FoxSnowRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::FoxModel> {
public:
    FoxSnowRenderer() { m_shadowSize = 0.4f; }
    ~FoxSnowRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/fox/snow_fox.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/fox/snow_fox.png");
    }
};

/**
 * @brief 熊猫渲染器
 */
class PandaRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::PandaModel> {
public:
    PandaRenderer() { m_shadowSize = 0.5f; }
    ~PandaRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/panda/panda.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/panda/panda.png");
    }
};

/**
 * @brief 鹦鹉渲染器
 */
class ParrotRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::ParrotModel> {
public:
    ParrotRenderer() { m_shadowSize = 0.0f; }
    ~ParrotRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/parrot/parrot_red_blue.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/parrot/parrot_red_blue.png");
    }
};

/**
 * @brief 幻翼渲染器
 */
class PhantomRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::PhantomModel> {
public:
    PhantomRenderer() { m_shadowSize = 0.0f; }
    ~PhantomRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/phantom.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/phantom.png");
    }
};

/**
 * @brief 北极熊渲染器
 *
 * 使用 PolarBearModel 渲染北极熊实体。
 * 支持:
 * - 成年/幼体渲染
 * - 站立动画（后腿站立）
 * - 四足行走动画
 */
class PolarBearRenderer : public core::LivingRenderer<::mc::LivingEntity, model::animal::PolarBearModel> {
public:
    PolarBearRenderer() { m_shadowSize = 0.7f; }
    ~PolarBearRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/bear/polarbear.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/bear/polarbear.png");
    }
};

/**
 * @brief 凋灵骷髅渲染器
 *
 * 复用 SkeletonModel，使用独立纹理和更大的阴影。
 * 参考 MC 1.16.5 WitherSkeletonRenderer
 */
class WitherSkeletonRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::SkeletonModel> {
public:
    WitherSkeletonRenderer() { m_shadowSize = 0.4f; }
    ~WitherSkeletonRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/skeleton/wither_skeleton.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/skeleton/wither_skeleton.png");
    }
};

/**
 * @brief 幻术师渲染器
 *
 * 使用 IllagerModel，与卫道士/唤魔者相同的骨架但不同纹理。
 * 参考 MC 1.16.5 IllusionerRenderer
 */
class IllusionerRenderer : public core::LivingRenderer<::mc::LivingEntity, model::monster::IllagerModel> {
public:
    IllusionerRenderer() { m_shadowSize = 0.5f; }
    ~IllusionerRenderer() override = default;

    [[nodiscard]] ResourceLocation getEntityTexture(::mc::LivingEntity& entity) override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/illager/illusioner.png");
    }
    [[nodiscard]] ResourceLocation getEntityTexture(const ::mc::LivingEntity& entity) const override
    {
        (void)entity;
        return ResourceLocation("minecraft", "textures/entity/illager/illusioner.png");
    }
};

} // namespace mc::client::renderer::entity::renderer::monster
