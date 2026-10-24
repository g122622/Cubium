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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/ecs/components/ArmorStandStateComponent.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mc {

// Forward declarations
class Player;
class LivingEntity;

namespace world::explosion {
struct ExplosionImmunityContext;
} // namespace world::explosion

namespace entity {

/**
 * @brief 应用瞬间效果到目标实体
 *
 * 对应 vanilla MobEffect#applyInstantenousEffect：瞬间治疗/伤害按施放者归属伤害源
 * （存在施放者时为 indirectMagic，否则为 magic），并按 multiplier 缩放后四舍五入。
 * 用于喷溅药水、药水云等场景。
 *
 * @param type 效果类型（必须是瞬间效果，否则无操作）
 * @param source 效果来源实体（投掷物或药水云自身）
 * @param caster 效果施放者（拥有者，如投掷药水的玩家），可为空
 * @param target 目标生物
 * @param amplifier 效果等级（0 = I, 1 = II）
 * @param multiplier 效果乘数（通常为距离因子 0..1）
 */
void applyInstantEffect(
    effect::EffectType type, Entity& source, LivingEntity* caster, LivingEntity& target, i32 amplifier, f32 multiplier);

/**
 * @brief 末影水晶实体
 *
 * 在末地生成，用于治愈末影龙。
 * 具有内部旋转动画、光束指向、爆炸等特性。
 */
class EnderCrystalEntity : public Entity {
public:
    EnderCrystalEntity(ecs::EntityRegistry& registry);
    ~EnderCrystalEntity() override = default;

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @param registry ECS 实体注册表，透传给构造函数
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    void tick() override;

    [[nodiscard]] f32 width() const override;
    [[nodiscard]] f32 height() const override;

    [[nodiscard]] bool isPushable() const override
    {
        return false;
    }

    [[nodiscard]] bool canBeCollidedWith() const override
    {
        return false;
    }

    // 无战利品表，覆写基类方法返回空字符串
    [[nodiscard]] std::string getLootTableId() const override
    {
        return {};
    }

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
    [[nodiscard]] bool shouldShowBottom() const
    {
        return m_showBottom;
    }

    void setShowBottom(bool show)
    {
        m_showBottom = show;
    }

    /**
     * @brief 获取内部旋转计数器（用于渲染动画）
     */
    [[nodiscard]] i32 innerRotation() const
    {
        return m_innerRotation;
    }

    /**
     * @brief 设置关联的末地传送门位置
     */
    void setBeamTarget(BlockPos pos);

    [[nodiscard]] const BlockPos& getBeamTarget() const
    {
        return m_beamTarget;
    }

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
    LightningBoltEntity(ecs::EntityRegistry& registry);
    ~LightningBoltEntity() override = default;

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @param registry ECS 实体注册表，透传给构造函数
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    void tick() override;

    [[nodiscard]] f32 width() const override;
    [[nodiscard]] f32 height() const override;

    [[nodiscard]] bool isPushable() const override
    {
        return false;
    }

    [[nodiscard]] bool canBeCollidedWith() const override
    {
        return false;
    }

    // 无战利品表，覆写基类方法返回空字符串
    [[nodiscard]] std::string getLootTableId() const override
    {
        return {};
    }

    /**
     * @brief 设置是否只有效果（不造成伤害、不点燃方块）
     */
    void setEffectOnly(bool effectOnly)
    {
        m_effectOnly = effectOnly;
    }

    [[nodiscard]] bool isEffectOnly() const
    {
        return m_effectOnly;
    }

    /// 视觉专用闪电（对齐 vanilla LightningBolt.getEffectOnly()）
    [[nodiscard]] bool getEffectOnly() const
    {
        return m_effectOnly;
    }

    /**
     * @brief 设置触发者（用于引雷附魔等）
     * @param casterId 触发者的玩家ID
     */
    void setCaster(PlayerId casterId)
    {
        m_caster = casterId;
    }

    [[nodiscard]] PlayerId caster() const
    {
        return m_caster;
    }

    /**
     * @brief 获取闪电状态值
     * 用于渲染闪电的闪烁效果
     */
    [[nodiscard]] i32 lightningState() const
    {
        return m_lightningState;
    }

    /**
     * @brief 获取闪电视觉效果剩余次数
     */
    [[nodiscard]] i32 boltLivingTime() const
    {
        return m_boltLivingTime;
    }

    /**
     * @brief 获取闪电形状随机种子
     * 用于渲染器生成一致的闪电形状
     */
    [[nodiscard]] u64 boltVertex() const
    {
        return m_boltVertex;
    }

    /**
     * @brief 获取声音类别
     */
    [[nodiscard]] sound::SoundCategory getSoundCategory() const override
    {
        return sound::SoundCategory::Weather;
    }

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
    AreaEffectCloudEntity(ecs::EntityRegistry& registry);
    ~AreaEffectCloudEntity() override = default;

    void tick() override;

    [[nodiscard]] f32 width() const override;
    [[nodiscard]] f32 height() const override;

    [[nodiscard]] bool isPushable() const override
    {
        return false;
    }

    [[nodiscard]] bool canBeCollidedWith() const override
    {
        return false;
    }

    // 无战利品表，覆写基类方法返回空字符串
    [[nodiscard]] std::string getLootTableId() const override
    {
        return {};
    }

    // ========== 半径 ==========

    /**
     * @brief 设置效果半径
     */
    void setRadius(f32 radius);

    /**
     * @brief 获取效果半径
     */
    [[nodiscard]] f32 getRadius() const
    {
        return m_radius;
    }

    /**
     * @brief 设置每次应用效果时半径变化
     */
    void setRadiusOnUse(f32 radiusOnUse)
    {
        m_radiusOnUse = radiusOnUse;
    }

    /**
     * @brief 设置每tick半径变化
     */
    void setRadiusPerTick(f32 radiusPerTick)
    {
        m_radiusPerTick = radiusPerTick;
    }

    // ========== 持续时间 ==========

    /**
     * @brief 设置持续时间
     */
    void setDuration(i32 duration)
    {
        m_duration = duration;
    }

    [[nodiscard]] i32 getDuration() const
    {
        return m_duration;
    }

    /**
     * @brief 设置每次应用效果时持续时间变化
     */
    void setDurationOnUse(i32 durationOnUse)
    {
        m_durationOnUse = durationOnUse;
    }

    // ========== 等待时间 ==========

    /**
     * @brief 设置等待时间（应用效果前的延迟）
     */
    void setWaitTime(i32 waitTime)
    {
        m_waitTime = waitTime;
    }

    [[nodiscard]] i32 getWaitTime() const
    {
        return m_waitTime;
    }

    // ========== 重应用延迟 ==========

    /**
     * @brief 设置效果重应用延迟
     */
    void setReapplicationDelay(i32 delay)
    {
        m_reapplicationDelay = delay;
    }

    [[nodiscard]] i32 getReapplicationDelay() const
    {
        return m_reapplicationDelay;
    }

    // ========== 颜色 ==========

    /**
     * @brief 设置效果颜色（ARGB）
     */
    void setColor(u32 color)
    {
        m_color = color;
        m_colorSet = true;
    }

    [[nodiscard]] u32 getColor() const
    {
        return m_color;
    }

    /**
     * @brief 设置是否固定颜色
     */
    void setColorFixed(bool fixed)
    {
        m_colorSet = fixed;
    }

    // ========== 效果管理 ==========

    /**
     * @brief 添加效果实例
     */
    void addEffect(const effect::EffectInstance& effect);

    /**
     * @brief 获取所有效果
     */
    [[nodiscard]] const std::vector<effect::EffectInstance>& getEffects() const
    {
        return m_effects;
    }

    /**
     * @brief 清空所有效果
     */
    void clearEffects()
    {
        m_effects.clear();
    }

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
    [[nodiscard]] LivingEntity* getOwner() const
    {
        return m_owner;
    }

    /**
     * @brief 获取拥有者的 UUID
     * @return 拥有者 UUID 字符串（32 字符十六进制），如果无拥有者则为空字符串
     */
    [[nodiscard]] const std::string& ownerUuid() const
    {
        return m_ownerUuid;
    }

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
    void setParticleType(u32 particleType)
    {
        m_particleType = particleType;
    }

    [[nodiscard]] u32 getParticleType() const
    {
        return m_particleType;
    }

    // ========== 创建工厂 ==========

    /**
     * @brief 创建实体工厂方法
     * @param world 世界实例
     * @param registry ECS 实体注册表，透传给构造函数
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

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
    ArmorStandEntity(ecs::EntityRegistry& registry);
    ~ArmorStandEntity() override = default;

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @param registry ECS 实体注册表，透传给构造函数
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    void tick() override;

    [[nodiscard]] f32 width() const override;
    [[nodiscard]] f32 height() const override;

    [[nodiscard]] bool isPushable() const override
    {
        return !m_marker;
    }

    [[nodiscard]] bool canBeCollidedWith() const override
    {
        return !m_marker;
    }

    /**
     * @brief 检查实体是否不触发压力板/绊线
     * @return 标记模式返回 true，否则返回 false
     *
     * 标记模式的盔甲架不触发压力板和绊线。
     */
    [[nodiscard]] bool doesEntityNotTriggerPressurePlate() const override
    {
        return m_marker;
    }

    /**
     * @brief 判断盔甲架是否忽略此次爆炸
     *
     * 仅当爆炸影响方块类实体时（shouldAffectBlocklikeEntities）才受爆炸影响，
     * 且不可见盔甲架忽略爆炸。
     */
    [[nodiscard]] bool ignoreExplosion(const world::explosion::ExplosionImmunityContext& ctx) const override;

    /**
     * @brief 检查是否有重力
     */
    [[nodiscard]] bool hasGravity() const
    {
        return m_hasGravity;
    }

    void setGravity(bool gravity)
    {
        m_hasGravity = gravity;
    }

    /**
     * @brief 检查是否可见（非不可见、非标记模式）
     */
    [[nodiscard]] bool isVisible() const
    {
        return !isInvisible() && !m_marker;
    }

    /**
     * @brief 不可见状态（存放在 ArmorStandStateComponent）
     *
     * 不可见盔甲架在 shouldAffectBlocklikeEntities 为真时仍忽略爆炸（vanilla
     * ArmorStand.ignoreExplosion 用 Entity.isInvisible() 判定）。
     */
    [[nodiscard]] bool isInvisible() const
    {
        return armorStandState().m_invisible;
    }

    void setInvisible(bool invisible)
    {
        mutableArmorStandState().m_invisible = invisible;
        _syncClientFlags();
    }

    /**
     * @brief 检查是否为标记模式
     */
    [[nodiscard]] bool isMarker() const
    {
        return m_marker;
    }

    void setMarker(bool marker)
    {
        m_marker = marker;
    }

    /**
     * @brief 检查是否有底座
     */
    [[nodiscard]] bool hasBasePlate() const
    {
        return armorStandState().m_basePlate;
    }

    void setBasePlate(bool basePlate)
    {
        mutableArmorStandState().m_basePlate = basePlate;
        _syncClientFlags();
    }

    /**
     * @brief 盔甲架状态组件（只读）
     *
     * 承载不可见/小型/显示手臂/禁用槽位掩码/底座/六部位姿态。
     */
    [[nodiscard]] const ecs::ArmorStandStateComponent& armorStandState() const;

    /**
     * @brief 盔甲架状态组件（可变）
     */
    ecs::ArmorStandStateComponent& mutableArmorStandState();

    /**
     * @brief 设置原始禁用槽位掩码（读盘与 NBT 驱动场景）
     *
     * 不做任何位重写，保存后原样读回。掩码的位组语义见 isDisabled/swapItem。
     */
    void setDisabledSlots(i32 disabledSlots)
    {
        mutableArmorStandState().m_disabledSlots = disabledSlots;
        _syncClientFlags();
    }

    [[nodiscard]] i32 disabledSlots() const
    {
        return armorStandState().m_disabledSlots;
    }

    /**
     * @brief 检查是否显示手臂（为 false 时手部槽位不可交互）
     */
    [[nodiscard]] bool hasArms() const
    {
        return armorStandState().m_arms;
    }

    void setArms(bool arms)
    {
        mutableArmorStandState().m_arms = arms;
        _syncClientFlags();
    }

    /**
     * @brief 设置各部位姿态（单位：度，写入 ArmorStandStateComponent::m_pose）
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
    [[nodiscard]] bool isSmall() const
    {
        return armorStandState().m_small;
    }

    void setSmall(bool small)
    {
        mutableArmorStandState().m_small = small;
        _syncClientFlags();
    }

    /**
     * @brief 受伤入口（对齐 vanilla ArmorStand.hurtServer:266-318）
     *
     * 盔甲架伤害语义与普通生物不同：按伤害源标签分支处理——BYPASSES_INVULNERABILITY/IS_EXPLOSION
     * 直接销毁；IGNITES_ARMOR_STANDS 点燃 5 秒（已着火则受 0.15 伤害）；BURNS_ARMOR_STANDS（on_fire）
     * 受 4.0 伤害（health>0.5 时）；CAN_BREAK_ARMOR_STAND/ALWAYS_KILLS_ARMOR_STANDS 近战/箭破坏。
     * 详见 ArmorStandEntity.cpp hurt 实现注释。
     *
     * 注：vanilla ArmorStand 继承 LivingEntity（maxHealth=2）。Cubium ArmorStandEntity 继承 Entity 无
     * health 体系，此处自带 m_health 模拟 health 语义（默认 2.0）。销毁链路（破坏音效 / 木板碎屑粒子 /
     * 盔甲架本体掉落 / 穿戴装备逐个掉出）按 vanilla 补齐；装备读写经 EquipmentComponent 承载。
     */
    bool hurt(DamageSource& source, f32 amount) override;

    /**
     * @brief 当前生命值（自带 health，对齐 vanilla ArmorStand.MAX_HEALTH=2）
     */
    [[nodiscard]] f32 health() const
    {
        return m_health;
    }

    void setHealth(f32 health)
    {
        m_health = health;
    }

    [[nodiscard]] f32 maxHealth() const
    {
        return ARMOR_STAND_MAX_HEALTH;
    }

    /**
     * @brief 可交互槽位判定（对齐 vanilla ArmorStand.canUseSlot）
     *
     * 盔甲架不接受 Body（身体护甲）与 Saddle（鞍）槽位；其余槽位是否可交互由禁用槽位掩码决定
     * （见 isDisabled）。
     */
    [[nodiscard]] bool canUseSlot(EquipmentSlot slot) const;

    /**
     * @brief 读取指定装备槽位的物品（只读）
     */
    [[nodiscard]] const ItemStack& getEquipment(EquipmentSlot slot) const;

    /**
     * @brief 写入指定装备槽位的物品（新物品与槽内物品不同时播放装备音效）
     */
    void setEquipment(EquipmentSlot slot, const ItemStack& stack);

    /**
     * @brief 按命中点计算被点击的槽位（对齐 vanilla ArmorStand.getClickedSlot）
     *
     * 空手交互或槽位被禁用时的回退选择：按点击点相对实体高度的比例确定部位，并对每个候选
     * 槽位校验"槽内有物品"；全部不满足时回退主手，主手无物品而副手有物品时回退副手。
     *
     * @param hitPosition 命中点（相对实体脚的局部坐标，y 分量可以是任意实体坐标值）
     * @return 被点击的装备槽位
     */
    [[nodiscard]] EquipmentSlot getClickedSlot(const Vector3& hitPosition) const;

    /**
     * @brief 槽位是否被禁用（对齐 vanilla ArmorStand.isDisabled）
     *
     * 低位启用掩码覆盖 MainHand/OffHand/Feet/Legs/Chest/Head；手部槽位在未开启手臂（ShowArms=false）
     * 时恒禁用。Body/Saddle 槽位不受掩码控制，直接视为禁用。
     */
    [[nodiscard]] bool isDisabled(EquipmentSlot slot) const;

    /**
     * @brief 装备/替换/取出槽位物品（对齐 vanilla ArmorStand.swapItem）
     *
     * 交换后双方物品都通过参数回写：槽位内原物品写回 slotStack，玩家手持物品写回 heldStack。
     * 处理四种情形：取走物品、创造模式无限材料复制、数量为 1 的直接交换、数量大于 1 时
     * 仅在目标槽位为空的情况下取出 1 个放入。
     *
     * @param player 交互玩家（只读，用于创造模式判断）
     * @param slot 目标槽位
     * @param slotStack 输入/输出：槽位物品；成功时更新为之后留在槽位中的物品
     * @param heldStack 输入/输出：玩家手持物品；成功时更新为交互后玩家应手持的物品
     * @return 成功返回 true；取出与放入均被禁用槽位掩码锁定时返回 false 且不修改任何状态
     */
    [[nodiscard]] bool swapItem(const Player& player, EquipmentSlot slot, ItemStack& slotStack, ItemStack& heldStack);

    /**
     * @brief 带命中点的玩家交互（对齐 vanilla ArmorStand.interactAt）
     *
     * - 标记模式或手持命名牌：Pass
     * - 旁观者：Success（不消费物品）
     * - 空手：按命中点选槽，槽内有物品则取出
     * - 持物：按物品决定目标槽位，禁用则 Fail，手部槽位在未开启手臂时 Fail
     *
     * 服务端直接调用（不依赖 firstTick/level().isClientSide 门控），成功回写为 SuccessServer。
     */
    ActionResultType applyPlayerInteraction(Player& player, const Vector3& hitPosition, Hand hand) override;

    /**
     * @brief 把状态组件的全部字段推送到客户端数据参数（标志 + 六部位姿态）
     */
    void syncClientState()
    {
        _syncClientFlags();
        _syncClientPose();
    }

private:
    // 对齐 vanilla ArmorStand.MAX_HEALTH=2（ArmorStand.java）。Cubium ArmorStandEntity 继承 Entity
    // 无 LivingEntity 的 health 体系，自带 m_health 模拟 causeDamage 扣血 + kill 销毁语义。
    static constexpr f32 ARMOR_STAND_MAX_HEALTH = 2.0f;
    f32 m_health = ARMOR_STAND_MAX_HEALTH;

    /// 近战破坏节流窗口（tick）：两次受击间隔不超过该值时直接破坏（vanilla 常量 5）。
    static constexpr i64 ARMOR_STAND_HIT_COOLDOWN_TICKS = 5;
    /// 破坏粒子数量（vanilla showBreakingParticles 固定 10 个方块碎屑）。
    static constexpr i32 ARMOR_STAND_BREAK_PARTICLE_COUNT = 10;

    /// 槽位位组内"放入物品"禁用位（vanilla EquipmentSlot.getFilterBit(0) → id * 16 + 0）。
    static constexpr i32 ARMOR_STAND_SLOT_BIT_PLACE = 0;
    /// 槽位位组内"取出/替换物品"禁用位（vanilla EquipmentSlot.getFilterBit(8) → id * 16 + 8）。
    static constexpr i32 ARMOR_STAND_SLOT_BIT_REMOVE = 8;
    /// 槽位位组内"创建时默认禁用"位（vanilla EquipmentSlot.getFilterBit(16) → id * 16 + 16）。
    static constexpr i32 ARMOR_STAND_SLOT_BIT_DEFAULT = 16;

    /// 计算指定槽位在禁用掩码中的位偏移：槽位序号 * 16 + 组内偏移。
    [[nodiscard]] static i32 _disableBit(EquipmentSlot slot, i32 inGroupBit)
    {
        return static_cast<i32>(slot) * 16 + inGroupBit;
    }

    /// 计算禁用掩码中指定槽位"放入"位（组内 +0）对应的位掩码
    [[nodiscard]] static u32 _disabledPlaceMask(EquipmentSlot slot)
    {
        const i32 bitIndex = _disableBit(slot, ARMOR_STAND_SLOT_BIT_PLACE);
        return static_cast<u32>(1u << (bitIndex & 31)) << (bitIndex >> 5);
    }

    // 对齐 vanilla ArmorStand.lastHit（5 tick 破坏节流），读写世界游戏时间。
    // lastHit 从对象构造时刻起算为空（m_hasLastHit=false），首次受击必然通过节流。
    i64 m_lastHit = 0;
    bool m_hasLastHit = false;

    /**
     * @brief 扣血辅助（对齐 vanilla ArmorStand.causeDamage）
     *
     * 从 m_health 扣除伤害值，扣至 0.5 及以下时走 brokenByAnything + kill；
     * 否则仅更新生命值并触发 ENTITY_DAMAGE 游戏事件。
     * 与普通 LivingEntity 不同，盔甲架不经过护甲/附魔减伤链路（vanilla causeDamage 直接 setHealth）。
     */
    void causeDamage(f32 damage, const DamageSource& source);

    /**
     * @brief 销毁辅助（对齐 vanilla ArmorStand.kill）
     *
     * 以 KILLED 原因移除实体（经 onRemoval 播放破坏音效）并触发 ENTITY_DIE 游戏事件。
     */
    void kill();

    /**
     * @brief 任意方式破坏（对齐 vanilla ArmorStand.brokenByAnything）
     *
     * 播放破坏音效，随后遍历全部装备槽位逐个清空并把非空物品掉出（落点在方块位上方）。
     * 盔甲架本体掉落只发生在 brokenByPlayer（玩家破坏）。
     */
    void brokenByAnything(const DamageSource& source);

    /**
     * @brief 在方块位上方掉出一个物品（对齐 vanilla Block.popResource(level, blockPosition().above(), stack)）
     */
    void _popResourceAboveBlock(const ItemStack& stack);

    /**
     * @brief 装备变更音效（对齐 vanilla LivingEntity.onEquipItem 的音效分支）
     *
     * 仅当新旧物品不同且新物品确有护甲槽时播放，音效按护甲材质选取（皮革/锁链/铜/金/铁/钻石/
     * 下界合金/龟壳），非盔甲物品（如名称牌、木棍）不播放。
     */
    void _playEquipSound(EquipmentSlot slot, const ItemStack& previous, const ItemStack& current) const;

    /**
     * @brief 被玩家破坏（对齐 vanilla ArmorStand.brokenByPlayer）
     *
     * 先掉出带自定义名的盔甲架本体，再走 brokenByAnything。
     */
    void brokenByPlayer(const DamageSource& source);

    /**
     * @brief 播放破坏音效（对齐 vanilla ArmorStand.playBrokenSound）
     */
    void playBrokenSound();

    /**
     * @brief 显示破坏粒子（对齐 vanilla ArmorStand.showBreakingParticles）
     *
     * 广播 10 个携带橡木木板方块状态的碎屑粒子，位置抬升到碰撞箱 2/3 高度处。
     */
    void showBreakingParticles();

    void onRemoval(RemovalReason reason) override;

    /**
     * @brief 数据参数注册（向 EntityDataManager 登记客户端标志与各部位姿态参数）
     */
    void registerData() override;

    /**
     * @brief 把状态组件同步到客户端标志数据参数（不可见/小型/显示手臂/无底座）
     *
     * 对应 vanilla ArmorStand.setInvisible/setSmall/setShowArms/setNoBasePlate 对
     * DATA_CLIENT_FLAGS 的位写入，供客户端渲染读取。
     */
    void _syncClientFlags();

    /**
     * @brief 把姿态组件同步到客户端姿态数据参数（头/身/双臂/双腿三轴旋转）
     */
    void _syncClientPose();

    // 物理/交互相关状态（不可见、小型、手臂、禁用槽位、底座、姿态）已迁移至
    // ecs::ArmorStandStateComponent，经 armorStandState()/mutableArmorStandState() 访问。
    bool m_hasGravity = true;
    bool m_marker = false;

private:
    // 客户端标志位定义（对齐 vanilla ArmorStand DATA_CLIENT_FLAGS 的同一 bit 布局）。
    static constexpr i8 ARMOR_STAND_CLIENT_FLAG_SMALL = 1;
    static constexpr i8 ARMOR_STAND_CLIENT_FLAG_SHOW_ARMS = 4;
    static constexpr i8 ARMOR_STAND_CLIENT_FLAG_NO_BASE_PLATE = 8;
    static constexpr i8 ARMOR_STAND_CLIENT_FLAG_INVISIBLE = 16;

    /// 客户端标志（i8，位布局同上，经 DATA_ARMOR_STAND_FLAGS_PARAM 下发）
    static entity::DataParameter<i8> DATA_ARMOR_STAND_FLAGS_PARAM;
    /// 各部位姿态（Vector3f，三轴旋转，经 6 个姿态参数下发）
    static entity::DataParameter<entity::Vector3f> DATA_ARMOR_STAND_HEAD_POSE_PARAM;
    static entity::DataParameter<entity::Vector3f> DATA_ARMOR_STAND_BODY_POSE_PARAM;
    static entity::DataParameter<entity::Vector3f> DATA_ARMOR_STAND_LEFT_ARM_POSE_PARAM;
    static entity::DataParameter<entity::Vector3f> DATA_ARMOR_STAND_RIGHT_ARM_POSE_PARAM;
    static entity::DataParameter<entity::Vector3f> DATA_ARMOR_STAND_LEFT_LEG_POSE_PARAM;
    static entity::DataParameter<entity::Vector3f> DATA_ARMOR_STAND_RIGHT_LEG_POSE_PARAM;
};

} // namespace entity
} // namespace mc
