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

#include "SquidEntity.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <memory>
#include <optional>

namespace mc {

// 前向声明
class DamageSource;

/**
 * @brief 发光鱿鱼实体
 *
 * 生活在黑暗地下水域中的发光鱿鱼，是鱿鱼的发光变种。
 *
 * 特性：
 * - 发光：身体持续发出 GLOW 粒子
 * - 暗化：受到伤害后会暗化 100 tick（纹理变暗）
 * - 喷墨：受击时喷出荧光墨汁（GlowSquidInk 粒子）
 * - 游泳：继承鱿鱼的游泳行为
 * - 生成：仅在繁茂洞穴等黑暗地下水域生成（Y <= 海平面 - 33 且亮度为 0 的水方块）
 *
 * 同步数据：
 * - DarkTicksRemaining（i32）：剩余暗化 tick 数，通过 EntityDataManager 同步到客户端
 *
 * 音效：
 * - ENTITY_GLOW_SQUID_AMBIENT: 环境音
 * - ENTITY_GLOW_SQUID_HURT: 受伤音效
 * - ENTITY_GLOW_SQUID_DEATH: 死亡音效
 * - ENTITY_GLOW_SQUID_SQUIRT: 喷墨音效
 */
class GlowSquidEntity : public SquidEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    GlowSquidEntity(EntityInstanceId id);
    ~GlowSquidEntity() override = default;

    // 禁止拷贝和移动
    GlowSquidEntity(const GlowSquidEntity&) = delete;
    GlowSquidEntity& operator=(const GlowSquidEntity&) = delete;
    GlowSquidEntity(GlowSquidEntity&&) = delete;
    GlowSquidEntity& operator=(GlowSquidEntity&&) = delete;

    /**
     * @brief 创建发光鱿鱼实体
     * @param world 世界实例（未使用，实体 ID 由 spawnEntity 重新分配）
     * @return 新的发光鱿鱼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 暗化状态 ==========

    /**
     * @brief 获取剩余暗化 tick 数
     *
     * 优先从 DataParameter 读取以获取同步值。
     */
    [[nodiscard]] i32 getDarkTicksRemaining() const;

    /**
     * @brief 设置暗化 tick 数
     *
     * 通过 DataParameter 同步到客户端，驱动纹理暗化效果。
     *
     * @param ticks 剩余暗化 tick 数
     */
    void setDarkTicks(i32 ticks);

    /**
     * @brief 获取暗化状态 DataParameter ID（客户端同步用）
     */
    [[nodiscard]] static u16 getDarkTicksRemainingParamId() { return DATA_DARK_TICKS_REMAINING_PARAM.id(); }

    // ========== 声音重写 ==========

    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取喷墨粒子类型
     *
     * 重写 SquidEntity 的喷墨粒子钩子（MC Java 中 Squid.getInkParticle()）。
     * 鱿鱼返回 SquidInk，发光鱿鱼返回 GlowSquidInk。
     */
    [[nodiscard]] particle::ParticleTypeId getInkParticle() const override
    {
        return particle::ParticleTypeId::GlowSquidInk;
    }

    /**
     * @brief 获取喷墨音效
     *
     * 重写 SquidEntity 的喷墨音效钩子（MC Java 中 Squid.getSquirtSound()）。
     * SquidEntity 默认返回空，发光鱿鱼返回 GLOW_SQUID_SQUIRT。
     */
    [[nodiscard]] std::optional<ResourceLocation> getSquirtSound() const override
    {
        return SoundEvents::ENTITY_GLOW_SQUID_SQUIRT;
    }

    // ========== 受伤重写 ==========

    bool hurt(DamageSource& source, f32 amount) override;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== 同步数据注册 ==========
    void registerData() override;

    // ========== 存档 ==========
    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

private:
    // 剩余暗化 tick 数（镜像值，权威值在 DataParameter 中）
    i32 m_darkTicksRemaining = 0;

    // 同步数据参数：剩余暗化 tick 数
    static entity::DataParameter<i32> DATA_DARK_TICKS_REMAINING_PARAM;

protected:
    /// 本类继承链标识（parent = SquidEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

private:
    // 受击后暗化的持续时间（tick）
    static constexpr i32 DARK_TICKS_ON_HURT = 100;
};

} // namespace mc
