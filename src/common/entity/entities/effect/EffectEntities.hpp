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
#include "../../../util/nbt/Nbt.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../core/Entity.hpp"
#include "../../serialization/NbtHelper.hpp"
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
 * 具有内部旋转动画、光束指向、爆炸等特性。
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

    // 无战利品表，覆写基类方法返回空字符串
    [[nodiscard]] std::string getLootTableId() const override { return {}; }

    /**
     * @brief 受伤入口
     *
     * 末影水晶对大多数伤害免疫（仅可被玩家/实体破坏）。受伤时移除自身，
     * 若伤害来源非爆炸则触发一次破坏性爆炸（半径 6.0），随后通知
     * 末影龙战斗系统（EndDragonFight::onCrystalDestroyed）。
     *
     * 对应 MC 1.21.11 EndCrystal.hurtServer()。
     *
     * @param source 伤害来源
     * @param amount 伤害量（末影水晶不按血量结算，仅用于触发死亡）
     * @return true 表示伤害被接受（已触发破坏流程）
     */
    bool hurt(DamageSource& source, f32 amount) override;

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
 * 具有闪烁效果、点燃方块、伤害实体等特性。
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

    // 无战利品表，覆写基类方法返回空字符串
    [[nodiscard]] std::string getLootTableId() const override { return {}; }

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
     */
    void _igniteBlocks(i32 extraIgnitions);

    /**
     * @brief 伤害周围实体
     */
    void _damageEntities();

    /**
     * @brief 初始化闪电状态
     */
    void _initializeState();

    // 闪电状态
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
 * 支持半径变化、持续时间、效果应用等特性。
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

    // 无战利品表，覆写基类方法返回空字符串
    [[nodiscard]] std::string getLootTableId() const override { return {}; }

    // ========== 半径 ==========

    /**
     * @brief 设置效果半径
     */
    void setRadius(f32 radius);

    /**
     * @brief 获取效果半径
     */
    [[nodiscard]] f32 getRadius() const { return m_radius; }

    /**
     * @brief 设置每次应用效果时半径变化
     */
    void setRadiusOnUse(f32 radiusOnUse) { m_radiusOnUse = radiusOnUse; }

    /**
     * @brief 设置每tick半径变化
     */
    void setRadiusPerTick(f32 radiusPerTick) { m_radiusPerTick = radiusPerTick; }

    // ========== 持续时间 ==========

    /**
     * @brief 设置持续时间
     */
    void setDuration(i32 duration) { m_duration = duration; }
    [[nodiscard]] i32 getDuration() const { return m_duration; }

    /**
     * @brief 设置每次应用效果时持续时间变化
     */
    void setDurationOnUse(i32 durationOnUse) { m_durationOnUse = durationOnUse; }

    // ========== 等待时间 ==========

    /**
     * @brief 设置等待时间（应用效果前的延迟）
     */
    void setWaitTime(i32 waitTime) { m_waitTime = waitTime; }
    [[nodiscard]] i32 getWaitTime() const { return m_waitTime; }

    // ========== 重应用延迟 ==========

    /**
     * @brief 设置效果重应用延迟
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
     */
    void setColorFixed(bool fixed) { m_colorSet = fixed; }

    // ========== 效果管理 ==========

    /**
     * @brief 添加效果实例
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
     *
     * 同时记录拥有者的 UUID，以便在拥有者实体失效后通过 UUID 重新查找。
     * 采用双重追踪模式：缓存指针 + UUID 字符串。
     *
     * @param owner 拥有者实体（可以为 nullptr）
     */
    void setOwner(LivingEntity* owner);

    /**
     * @brief 获取拥有者实体
     *
     * 优先返回缓存的拥有者指针；如果缓存失效（实体已被移除），
     * 则尝试通过 UUID 在世界中重新查找拥有者。
     *
     * @return 拥有者实体指针，可能为 nullptr
     */
    [[nodiscard]] LivingEntity* getOwner();

    /**
     * @brief 获取拥有者实体（const 版本）
     *
     * 注意：const 版本不会尝试通过 UUID 重新查找拥有者。
     *
     * @return 拥有者实体指针，可能为 nullptr
     */
    [[nodiscard]] LivingEntity* getOwner() const { return m_owner; }

    /**
     * @brief 获取拥有者的 UUID
     * @return 拥有者 UUID 字符串（32 字符十六进制），如果无拥有者则为空字符串
     */
    [[nodiscard]] const std::string& ownerUuid() const { return m_ownerUuid; }

    /**
     * @brief 仅通过 UUID 设置拥有者（用于 NBT 反序列化）
     * @param uuid 拥有者 UUID 字符串
     */
    void setOwnerUuid(const std::string& uuid);

    // ========== NBT 序列化 ==========

    /**
     * @brief 序列化区域效果云特有数据到 NBT
     *
     * 保存字段：Age, Duration, WaitTime, ReapplicationDelay, DurationOnUse,
     * RadiusOnUse, RadiusPerTick, Radius, Owner (UUID), ParticleType, Effects
     */
    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;

    /**
     * @brief 从 NBT 反序列化区域效果云特有数据
     */
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

    // ========== 粒子 ==========

    /**
     * @brief 设置粒子类型
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
     */
    void _applyEffects();

    /**
     * @brief 更新半径
     */
    void _updateRadius();

    /**
     * @brief 更新颜色（根据效果列表自动计算）
     */
    void _updateColor();

    /**
     * @brief 计算效果颜色
     */
    [[nodiscard]] static u32 _calculateEffectsColor(const std::vector<effect::EffectInstance>& effects);

    // 效果配置
    std::vector<effect::EffectInstance> m_effects;      ///< 效果列表
    std::map<EntityInstanceId, i32> m_reapplicationMap; ///< 重应用延迟映射（实体ID -> 下次可应用时间）

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

    LivingEntity* m_owner = nullptr; ///< 拥有者缓存指针（苦力怕、玩家等）
    std::string m_ownerUuid;         ///< 拥有者 UUID（持久化，用于跨 tick 重新查找）
    u32 m_particleType = 0;          ///< 粒子类型

    // 默认值常量
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
 * 支持重力、标记模式、身体部位旋转等特性。
 *
 * TODO(实体同步对齐, 见 entity-sync-alignment-decisions-2026-07): vanilla 1.21.11
 * 中 ArmorStand 继承自 LivingEntity（非 Entity），同步字段集为 CLIENT_FLAGS(Byte id15)
 * + HEAD/BODY/LEFT_ARM/RIGHT_ARM/LEFT_LEG/RIGHT_LEG 六个 Rotations(id16-21)。
 * 本项目当前直接继承 Entity 且无 registerData/classInfo，故仅下发 Entity id0-7 字段。
 * 待补：改继承 LivingEntity + classInfo(parent=LivingEntity)+ClassRegisterGuard+7 字段
 * （Rotations 复用 Vector3f variant，serializerId=9）。需评估 LivingEntity 构造/tick
 * 语义（health/属性/装备 tick）对盔甲架的影响后再迁移，故本次分批推迟。
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
     * @brief 检查实体是否不触发压力板/绊线
     * @return 标记模式返回 true，否则返回 false
     *
     * 对应 MC Java 的 ArmorStand.isIgnoringBlockTriggers()
     * 标记模式的盔甲架不触发压力板和绊线。
     */
    [[nodiscard]] bool doesEntityNotTriggerPressurePlate() const override { return m_marker; }

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
