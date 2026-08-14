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

#include "AbstractGroupFishEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <memory>
#include <optional>

namespace mc {

namespace resource {
class ResourceLocation;
}

/**
 * @brief 热带鱼实体
 *
 * 支持变种编码和群游行为，包含预定义花纹、颜色等属性。
 */
class TropicalFishEntity : public AbstractGroupFishEntity {
public:
    /**
     * @brief 热带鱼形状
     */
    enum class FishShape : u8 {
        Kob = 0,
        SunStreak = 1,
        Snooper = 2,
        Dasher = 3,
        Brinely = 4,
        Spotty = 5,
        Flopper = 6,
        Stripey = 7,
        Glitter = 8,
        Blockfish = 9,
        Betty = 10,
        Clayfish = 11
    };

    /**
     * @brief 构造热带鱼实体
     * @param id 实体 ID
     * @param registry 实体注册表（ECS）
     */
    TropicalFishEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~TropicalFishEntity() override = default;

    TropicalFishEntity(const TropicalFishEntity&) = delete;
    TropicalFishEntity& operator=(const TropicalFishEntity&) = delete;
    TropicalFishEntity(TropicalFishEntity&&) noexcept = delete;
    TropicalFishEntity& operator=(TropicalFishEntity&&) noexcept = delete;

    /// 本类继承链标识（parent = AbstractGroupFishEntity::classInfo()）。见 Entity::classInfo()。
    // vanilla 1.21.11 TropicalFish 自带 DATA_VARIANT(Int,id17，编码 shape/baseColor/patternColor)，
    // 见 registerData。setVariant/randomizeVariant 同步写入 DataParameter。
    static const entity::EntityClassInfo& classInfo();

    /**
     * @brief 创建热带鱼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /**
     * @brief 获取变种 ID
     */
    [[nodiscard]] i32 getVariant() const { return m_variant; }

    /**
     * @brief 设置变种 ID
     *
     * 同步写入 DATA_VARIANT 元数据（id17），客户端据此渲染热带鱼花纹/颜色。
     */
    void setVariant(i32 variant)
    {
        m_variant = variant;
        m_dataManager.set(DATA_VARIANT_PARAM, variant);
    }

    /**
     * @brief 获取鱼体形状
     */
    [[nodiscard]] FishShape getShape() const;

    /**
     * @brief 获取主色
     */
    [[nodiscard]] u8 getBaseColor() const;

    /**
     * @brief 获取花纹色
     */
    [[nodiscard]] u8 getPatternColor() const;

    /**
     * @brief 随机生成一个变种
     */
    void randomizeVariant();

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.1f; }

    /**
     * @brief 获取扑腾声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getFlopSound() const override;

    /**
     * @brief 获取环境声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取受伤声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

protected:
    void registerAttributes() override;

    // ========== 同步数据注册 ==========
    // 派生类构造函数须显式调用 registerData()（C++ 基类构造期虚函数不派发，参考 AbstractFishEntity）。
    void registerData() override;

private:
    i32 m_variant = 0;

    // ========== 同步数据参数（vanilla 1.21.11 TropicalFish.DATA_VARIANT，见 registerData） ==========
    // id17 DATA_VARIANT（packed: shape | baseColor<<8 | patternColor<<16）。由 setVariant 同步。
    static entity::DataParameter<i32> DATA_VARIANT_PARAM;

    static constexpr i32 SHAPE_MASK = 0xFF;
    static constexpr i32 BASE_COLOR_MASK = 0xFF00;
    static constexpr i32 PATTERN_COLOR_MASK = 0xFF0000;
};

} // namespace mc
