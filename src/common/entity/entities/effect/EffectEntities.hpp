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
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "../../../core/Types.hpp"
#include "../../../entity/effect/EffectInstance.hpp"
#include "../../../resource/ResourceLocation.hpp"
#include "../../../util/AxisAlignedBB.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../core/Entity.hpp"
#include <map>
#include <memory>
#include <vector>

namespace mc {

// Forward declarations
class Player;
class LivingEntity;

namespace entity {

/**
 * @brief 末影水晶实体
 *
 * 在末地生成，用于治愈末影龙。
 *
 * MC 1.16.5 对齐：
 * - innerRotation: 递增的旋转计数器，用于渲染动画
 * - showBottom: 是否显示基岩底座
 * - beamTarget: 光束指向的目标位置（末地传送门）
 *
 * 参考 MC 1.16.5 EnderCrystalEntity
 */
class EnderCrystalEntity : public Entity {
public:
    EnderCrystalEntity();
    ~EnderCrystalEntity() override = default;

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    void tick() override;

    [[nodiscard]] f32 width() const override;
    [[nodiscard]] f32 height() const override;
    [[nodiscard]] bool isPushable() const { return false; }
    [[nodiscard]] bool canBeCollidedWith() const override { return false; }

    /**
     * @brief 检查是否显示基岩
     */
    [[nodiscard]] bool shouldShowBottom() const { return m_showBottom; }
    void setShowBottom(bool show) { m_showBottom = show; }

    /**
     * @brief 获取内部旋转计数器（用于渲染动画）
     */
    [[nodiscard]] i32 innerRotation() const { return m_innerRotation; }

    /**
     * @brief 设置关联的末地传送门位置
     */
    void setBeamTarget(BlockPos pos);
    [[nodiscard]] const BlockPos& getBeamTarget() const { return m_beamTarget; }
    [[nodiscard]] bool hasBeamTarget() const;

    /**
     * @brief 治愈末影龙
     */
    void healDragon();

    /**
     * @brief 爆炸
     */
    void explode();

private:
    BlockPos m_beamTarget;
    bool m_showBottom = false;
    i32 m_innerRotation = 0; ///< 内部旋转计数器（用于渲染动画）
    i32 m_healCooldown = 0;
    static constexpr i32 HEAL_COOLDOWN = 10;
    static constexpr f32 EXPLOSION_RADIUS = 6.0f;
};

/**
 * @brief 闪电实体
 *
 * 雷暴天气时生成的闪电。
 *
 * MC 1.16.5 对齐字段和逻辑：
 * - lightningState: 初始值为 2，每 tick 递减
 * - boltVertex: 随机种子，用于渲染闪电形状
 * - boltLivingTime: 随机 1-3，控制闪电视觉效果的"复活"次数
 * - effectOnly: 是否只有效果（不造成伤害）
 * - caster: 触发者（用于引雷附魔等）
 *
 * 参考 MC 1.16.5 LightningBoltEntity
 */
class LightningBoltEntity : public Entity {
public:
    LightningBoltEntity();
    ~LightningBoltEntity() override = default;

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    void tick() override;

    [[nodiscard]] f32 width() const override;
    [[nodiscard]] f32 height() const override;
    [[nodiscard]] bool isPushable() const { return false; }
    [[nodiscard]] bool canBeCollidedWith() const override { return false; }

    /**
     * @brief 设置是否只有效果（不造成伤害、不点燃方块）
     */
    void setEffectOnly(bool effectOnly) { m_effectOnly = effectOnly; }
    [[nodiscard]] bool isEffectOnly() const { return m_effectOnly; }

    /**
     * @brief 设置触发者（用于引雷附魔等）
     * @param casterId 触发者的玩家ID
     */
    void setCaster(PlayerId casterId) { m_caster = casterId; }
    [[nodiscard]] PlayerId caster() const { return m_caster; }

    /**
     * @brief 获取闪电状态值
     * 用于渲染闪电的闪烁效果
     */
    [[nodiscard]] i32 lightningState() const { return m_lightningState; }

    /**
     * @brief 获取闪电视觉效果剩余次数
     */
    [[nodiscard]] i32 boltLivingTime() const { return m_boltLivingTime; }

    /**
     * @brief 获取闪电形状随机种子
     * 用于渲染器生成一致的闪电形状
     */
    [[nodiscard]] u64 boltVertex() const { return m_boltVertex; }

    /**
     * @brief 获取声音类别
     */
    [[nodiscard]] sound::SoundCategory getSoundCategory() const override { return sound::SoundCategory::Weather; }

private:
    /**
     * @brief 点燃周围方块
     * @param extraIgnitions 额外点燃数量（基于难度）
     *
     * MC 1.16.5 igniteBlocks():
     * - 检查游戏规则 doFireTick
     * - 在当前位置放置火焰
     * - 根据 difficulty 决定额外点燃数量
     */
    void igniteBlocks(i32 extraIgnitions);

    /**
     * @brief 伤害周围实体
     *
     * MC 1.16.5 伤害逻辑：
     * - 获取 3x6x3 范围内的实体
     * - 调用实体的 onStruckByLightning() 方法
     */
    void damageEntities();

    /**
     * @brief 初始化闪电状态
     */
    void initializeState();

    // MC 1.16.5 字段
    i32 m_lightningState = 2;   ///< 闪电状态，初始 2，递减控制音效和伤害
    u64 m_boltVertex = 0;       ///< 随机种子，用于渲染闪电形状
    i32 m_boltLivingTime = 1;   ///< 闪电视觉效果重复次数 (1-3)
    bool m_effectOnly = false;  ///< 是否只有效果（不造成伤害）
    PlayerId m_caster = 0;      ///< 触发者ID（用于引雷附魔）
    bool m_initialized = false; ///< 是否已初始化

    // 常量
    static constexpr f32 DAMAGE_RADIUS_XZ = 3.0f;       ///< 伤害范围 X/Z 轴
    static constexpr f32 DAMAGE_RADIUS_Y = 6.0f;        ///< 伤害范围 Y 轴（向上扩展）
    static constexpr f32 DAMAGE_RADIUS_Y_OFFSET = 3.0f; ///< 伤害范围 Y 轴偏移
};

/**
 * @brief 区域效果云实体
 *
 * 由滞留药水或苦力怕爆炸产生的效果云。
 *
 * MC 1.16.5 对齐：
 * - radius: 效果半径
 * - duration: 持续时间
 * - waitTime: 等待时间（应用效果前的延迟）
 * - reapplicationDelay: 效果重应用延迟
 * - effects: 效果列表
 * - color: 效果颜色（ARGB）
 * - owner: 拥有者（造成效果的实体）
 *
 * 参考 MC 1.16.5 AreaEffectCloudEntity
 */
class AreaEffectCloudEntity : public Entity {
public:
    AreaEffectCloudEntity();
    ~AreaEffectCloudEntity() override = default;

    void tick() override;

    [[nodiscard]] f32 width() const override;
    [[nodiscard]] f32 height() const override;
    [[nodiscard]] bool isPushable() const { return false; }
    [[nodiscard]] bool canBeCollidedWith() const override { return false; }

    // ========== 半径 ==========

    /**
     * @brief 设置效果半径
     * MC 1.16.5: setRadius()
     */
    void setRadius(f32 radius);

    /**
     * @brief 获取效果半径
     */
    [[nodiscard]] f32 getRadius() const { return m_radius; }

    /**
     * @brief 设置每次应用效果时半径变化
     * MC 1.16.5: setRadiusOnUse()
     */
    void setRadiusOnUse(f32 radiusOnUse) { m_radiusOnUse = radiusOnUse; }

    /**
     * @brief 设置每tick半径变化
     * MC 1.16.5: setRadiusPerTick()
     */
    void setRadiusPerTick(f32 radiusPerTick) { m_radiusPerTick = radiusPerTick; }

    // ========== 持续时间 ==========

    /**
     * @brief 设置持续时间
     * MC 1.16.5: setDuration()
     */
    void setDuration(i32 duration) { m_duration = duration; }
    [[nodiscard]] i32 getDuration() const { return m_duration; }

    /**
     * @brief 设置每次应用效果时持续时间变化
     * MC 1.16.5: setDurationOnUse()
     */
    void setDurationOnUse(i32 durationOnUse) { m_durationOnUse = durationOnUse; }

    // ========== 等待时间 ==========

    /**
     * @brief 设置等待时间（应用效果前的延迟）
     * MC 1.16.5: setWaitTime()
     */
    void setWaitTime(i32 waitTime) { m_waitTime = waitTime; }
    [[nodiscard]] i32 getWaitTime() const { return m_waitTime; }

    // ========== 重应用延迟 ==========

    /**
     * @brief 设置效果重应用延迟
     * MC 1.16.5: setReapplicationDelay()
     */
    void setReapplicationDelay(i32 delay) { m_reapplicationDelay = delay; }
    [[nodiscard]] i32 getReapplicationDelay() const { return m_reapplicationDelay; }

    // ========== 颜色 ==========

    /**
     * @brief 设置效果颜色（ARGB）
     */
    void setColor(u32 color)
    {
        m_color = color;
        m_colorSet = true;
    }
    [[nodiscard]] u32 getColor() const { return m_color; }

    /**
     * @brief 设置是否固定颜色
     * MC 1.16.5: setColorFixed()
     */
    void setColorFixed(bool fixed) { m_colorSet = fixed; }

    // ========== 效果管理 ==========

    /**
     * @brief 添加效果实例
     * MC 1.16.5: addEffect()
     */
    void addEffect(const effect::EffectInstance& effect);

    /**
     * @brief 获取所有效果
     */
    [[nodiscard]] const std::vector<effect::EffectInstance>& getEffects() const { return m_effects; }

    /**
     * @brief 清空所有效果
     */
    void clearEffects() { m_effects.clear(); }

    // ========== 拥有者 ==========

    /**
     * @brief 设置拥有者（造成效果的实体）
     * MC 1.16.5: setOwner()
     */
    void setOwner(LivingEntity* owner);
    [[nodiscard]] LivingEntity* getOwner() const { return m_owner; }

    // ========== 粒子 ==========

    /**
     * @brief 设置粒子类型
     * MC 1.16.5: setParticle()
     * 注：当前项目粒子系统简化，此方法预留接口
     */
    void setParticleType(u32 particleType) { m_particleType = particleType; }
    [[nodiscard]] u32 getParticleType() const { return m_particleType; }

    // ========== 创建工厂 ==========

    /**
     * @brief 创建实体工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

private:
    /**
     * @brief 应用效果到范围内的实体
     * MC 1.16.5: affectEntities()
     */
    void applyEffects();

    /**
     * @brief 更新半径
     */
    void updateRadius();

    /**
     * @brief 更新颜色（根据效果列表自动计算）
     */
    void updateColor();

    /**
     * @brief 计算效果颜色
     * 参考 MC 1.16.5 PotionUtils.getPotionColorFromEffectList()
     */
    [[nodiscard]] static u32 calculateEffectsColor(const std::vector<effect::EffectInstance>& effects);

    // MC 1.16.5 字段
    std::vector<effect::EffectInstance> m_effects; ///< 效果列表
    std::map<EntityId, i32> m_reapplicationMap;    ///< 重应用延迟映射（实体ID -> 下次可应用时间）

    f32 m_radius = 3.0f;        ///< 当前半径
    f32 m_initialRadius = 3.0f; ///< 初始半径
    f32 m_radiusOnUse = 0.0f;   ///< 每次使用时半径变化
    f32 m_radiusPerTick = 0.0f; ///< 每tick半径变化

    i32 m_duration = 600;          ///< 持续时间 (ticks)，默认 30 秒
    i32 m_waitTime = 20;           ///< 等待时间 (ticks)
    i32 m_reapplicationDelay = 20; ///< 效果重应用延迟 (ticks)
    i32 m_durationOnUse = 0;       ///< 每次使用时持续时间变化

    i32 m_ticksLived = 0;    ///< 已存活时间 (ticks)
    u32 m_color = 0;         ///< 效果颜色 (ARGB)
    bool m_colorSet = false; ///< 颜色是否已设置

    LivingEntity* m_owner = nullptr; ///< 拥有者（苦力怕等）
    u32 m_particleType = 0;          ///< 粒子类型

    // MC 1.16.5 默认值常量
    static constexpr f32 DEFAULT_RADIUS = 3.0f;
    static constexpr i32 DEFAULT_DURATION = 600;
    static constexpr i32 DEFAULT_WAIT_TIME = 20;
    static constexpr i32 DEFAULT_REAPPLICATION_DELAY = 20;
};

// 注意: ExperienceOrbEntity 已移动到独立的 orb/ 目录
// 文件位置: src/common/entity/entities/orb/ExperienceOrbEntity.hpp

/**
 * @brief 盔甲架实体
 *
 * 可以展示和穿戴盔甲的实体。
 *
 * 参考 MC 1.16.5 ArmorStandEntity
 */
class ArmorStandEntity : public Entity {
public:
    ArmorStandEntity();
    ~ArmorStandEntity() override = default;

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    void tick() override;

    [[nodiscard]] f32 width() const override;
    [[nodiscard]] f32 height() const override;
    [[nodiscard]] bool isPushable() const { return !m_marker; }
    [[nodiscard]] bool canBeCollidedWith() const override { return !m_marker; }

    /**
     * @brief 检查是否有重力
     */
    [[nodiscard]] bool hasGravity() const { return m_hasGravity; }
    void setGravity(bool gravity) { m_hasGravity = gravity; }

    /**
     * @brief 检查是否可见（非标记模式）
     */
    [[nodiscard]] bool isVisible() const { return !m_invisible && !m_marker; }

    /**
     * @brief 检查是否为标记模式
     */
    [[nodiscard]] bool isMarker() const { return m_marker; }
    void setMarker(bool marker) { m_marker = marker; }

    /**
     * @brief 检查是否有底座
     */
    [[nodiscard]] bool hasBasePlate() const { return m_basePlate; }
    void setBasePlate(bool basePlate) { m_basePlate = basePlate; }

    /**
     * @brief 检查是否显示手臂
     */
    [[nodiscard]] bool hasArms() const { return m_arms; }
    void setArms(bool arms) { m_arms = arms; }

    /**
     * @brief 设置头部旋转
     */
    void setHeadRotation(f32 x, f32 y, f32 z);
    void setBodyRotation(f32 x, f32 y, f32 z);
    void setLeftArmRotation(f32 x, f32 y, f32 z);
    void setRightArmRotation(f32 x, f32 y, f32 z);
    void setLeftLegRotation(f32 x, f32 y, f32 z);
    void setRightLegRotation(f32 x, f32 y, f32 z);

    /**
     * @brief 检查是否是小型
     */
    [[nodiscard]] bool isSmall() const { return m_small; }
    void setSmall(bool small) { m_small = small; }

private:
    bool m_hasGravity = true;
    bool m_invisible = false;
    bool m_marker = false;
    bool m_basePlate = true;
    bool m_arms = false;
    bool m_small = false;

    // 身体部位旋转（欧拉角）
    struct EulerAngles {
        f32 x = 0.0f, y = 0.0f, z = 0.0f;
    } m_head, m_body, m_leftArm, m_rightArm, m_leftLeg, m_rightLeg;
};

} // namespace entity
} // namespace mc
