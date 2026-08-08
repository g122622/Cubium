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

#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <string>

namespace mc {

// Forward declarations
class Block;
class BlockState;
class Player;
class LivingEntity;

namespace entity {

/**
 * @brief 下落方块实体
 *
 * 沙子、砾石、铁砧等方块下落时创建的实体。
 * 铁砧下落时会伤害实体，并有概率损坏（降级到下一级铁砧或完全摧毁）。
 *
 * 网络同步：对齐 MC 1.21.11 FallingBlockEntity。
 * - DATA_START_POS（BlockPos，id8，默认 ZERO）：唯一的 SynchedEntityData 字段。
 * - BlockState 不走同步数据，而是经 AddEntity 包 data 字段下发 stateId
 *   （见 getSpawnData()），客户端 spawn 时即拿到下落方块的方块状态。
 */
class FallingBlockEntity : public Entity {
public:
    FallingBlockEntity(ecs::EntityRegistry& registry);
    ~FallingBlockEntity() override = default;

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @param registry ECS 实体注册表
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    void tick() override;

    [[nodiscard]] f32 width() const override { return 0.98f; }
    [[nodiscard]] f32 height() const override { return 0.98f; }
    [[nodiscard]] bool isPushable() const { return false; }
    [[nodiscard]] bool canBeCollidedWith() const override { return false; }

    // 无战利品表，覆写基类方法返回空字符串
    [[nodiscard]] std::string getLootTableId() const override { return {}; }

    /**
     * @brief 处理下落方块实体受到伤害
     *
     * 下落方块不可被伤害，但当来源非无敌时标记 hurtMarked 以同步速度。
     * 对应 MC Java 的 FallingBlockEntity.hurtServer()。
     */
    bool hurt(DamageSource& source, f32 amount) override;

    /**
     * @brief 取 AddEntity.data 字段值（下落方块的 BlockState stateId）
     *
     * 对齐 MC 1.21.11 FallingBlockEntity.getEntityData()：BlockState 不走
     * SynchedEntityData，而是经 AddEntity 包 data 字段下发。客户端 spawn 时
     * 据此解析 BlockState 用于渲染。
     */
    [[nodiscard]] i32 getSpawnData() const override;

    /**
     * @brief 设置方块ID
     *
     * 更新本地 m_blockId（落地恢复用）。BlockState 经 AddEntity.data 下发，见 getSpawnData()。
     */
    void setBlockId(u32 blockId);

    [[nodiscard]] u32 getBlockId() const { return m_blockId; }

    /**
     * @brief 设置下落时的方块状态
     *
     * 保存原始方块状态（包含属性如朝向等），用于落地时恢复。
     * BlockState 经 AddEntity.data 下发，见 getSpawnData()。
     */
    void setFallingState(const BlockState* state);

    [[nodiscard]] const BlockState* getFallingState() const { return m_fallingState; }

    /**
     * @brief 获取下落起始位置参数 ID（供客户端 ClientEntity 读取）
     *
     * 对应 MC 1.21.11 FallingBlockEntity.DATA_START_POS（BlockPos，id8）。
     */
    [[nodiscard]] static u16 getStartPosParamId() { return DATA_START_POS_PARAM.id(); }

    /**
     * @brief 设置是否在落地时造成伤害
     */
    void setHurtEntities(bool hurt) { m_hurtEntities = hurt; }
    [[nodiscard]] bool shouldHurtEntities() const { return m_hurtEntities; }

    /**
     * @brief 设置每格下落伤害系数
     *
     * 默认为 2.0f（铁砧专用值），其他下落方块使用 HURT_AMOUNT 常量。
     */
    void setFallDamagePerDistance(f32 damage) { m_fallDamagePerDistance = damage; }
    [[nodiscard]] f32 getFallDamagePerDistance() const { return m_fallDamagePerDistance; }

    /**
     * @brief 设置最大伤害值
     *
     * 默认为 40（铁砧专用值），其他下落方块使用 MAX_HURT_AMOUNT 常量。
     */
    void setFallDamageMax(i32 maxDamage) { m_fallDamageMax = maxDamage; }
    [[nodiscard]] i32 getFallDamageMax() const { return m_fallDamageMax; }

    /**
     * @brief 设置下落起始位置（用于计算伤害）
     */
    void setFallStartPos(f64 y) { m_fallStartY = y; }

    /**
     * @brief 检查是否应该放置方块
     */
    [[nodiscard]] bool shouldPlaceBlock() const { return m_placeBlock; }

    /**
     * @brief 设置是否应该掉落物品
     *
     * 默认为 true。铁砧完全损坏时设置为 false。
     */
    void setShouldDropItem(bool drop) { m_shouldDropItem = drop; }
    [[nodiscard]] bool shouldDropItem() const { return m_shouldDropItem; }

    /**
     * @brief 设置是否不放置方块
     *
     * 当铁砧损坏时会设置为 true，此时只调用 onBroken 回调。
     */
    void setDontSetBlock(bool dontSet) { m_dontSetBlock = dontSet; }
    [[nodiscard]] bool dontSetBlock() const { return m_dontSetBlock; }

    /**
     * @brief 设置是否取消掉落物品
     *
     * 当铁砧在最大损坏状态下损坏时设置为 true（铁砧完全摧毁，不掉落物品）。
     */
    void setCancelDrop(bool cancel) { m_cancelDrop = cancel; }
    [[nodiscard]] bool cancelDrop() const { return m_cancelDrop; }

    /**
     * @brief 设置下落伤害类型
     *
     * 默认为 FallingBlock。钟乳石掉落时应设置为 FallingStalactite。
     */
    void setFallDamageType(DamageType type) { m_fallDamageType = type; }
    [[nodiscard]] DamageType getFallDamageType() const { return m_fallDamageType; }

protected:
    /**
     * @brief 注册网络同步数据参数
     *
     * 注册 DATA_START_POS（BlockPos，id8）到 EntityDataManager，由 EntityTracker 自动广播到客户端。
     *
     * 必须在构造函数中显式调用（参考 EndermanEntity 模式），因为基类构造函数
     * 中的虚函数调用不会派发到派生类。
     */
    void registerData() override;

private:
    void _handleLanding();

    /**
     * @brief 尝试放置方块
     *
     * @param world 世界指针
     * @param landingPos 落地位置
     * @param fallingState 下落的方块状态
     * @param hitState 落地点的方块状态
     * @return 是否成功放置
     */
    bool _tryPlaceBlock(
        IWorld* world, const BlockPos& landingPos, const BlockState* fallingState, const BlockState* hitState);

    /**
     * @brief 掉落物品
     *
     * @param world 世界指针
     * @param pos 掉落位置
     */
    void _dropItem(IWorld* world, const BlockPos& pos);

    /**
     * @brief 伤害碰撞箱内的实体，并处理铁砧损坏逻辑
     *
     * @param world 世界指针
     */
    void _hurtEntities(IWorld* world);

    u32 m_blockId = 0;                                      ///< 方块ID
    const BlockState* m_fallingState = nullptr;             ///< 下落时的方块状态（含属性，如朝向）
    bool m_hurtEntities = false;                            ///< 是否伤害实体（铁砧=true）
    bool m_placeBlock = true;                               ///< 是否应该放置方块
    bool m_shouldDropItem = true;                           ///< 是否应该掉落物品
    bool m_dontSetBlock = false;                            ///< 是否不放置方块（铁砧损坏时）
    bool m_cancelDrop = false;                              ///< 是否取消掉落物品（铁砧完全摧毁时）
    f64 m_fallStartY = 0.0;                                 ///< 下落起始Y坐标
    f32 m_fallDamagePerDistance = 2.0f;                     ///< 每格下落伤害系数
    i32 m_fallDamageMax = 40;                               ///< 最大伤害值
    DamageType m_fallDamageType = DamageType::FallingBlock; ///< 下落伤害类型
    i32 m_fallTime = 0;                                     ///< 下落时间（tick）
    static constexpr f32 HURT_AMOUNT = 2.0f;                ///< 默认每格下落伤害系数
    static constexpr i32 MAX_HURT_AMOUNT = 40;              ///< 默认最大伤害值
    static constexpr i32 MAX_FALL_TIME = 600;               ///< 最大下落时间（30秒）

    /**
     * @brief 下落起始位置同步参数
     *
     * 对应 MC 1.21.11 FallingBlockEntity.DATA_START_POS（BlockPos，id8，默认 BlockPos.ZERO）。
     * vanilla FallingBlock 的 BlockState 不走 SynchedEntityData，而是通过 AddEntity 包的
     * data 字段下发（blockState registry id），见 Entity::getSpawnData() 与 EntityTracker。
     * 本项目同样把 BlockState 经 AddEntity.data 下发，同步字段仅保留 DATA_START_POS
     * 以对齐 vanilla 字段布局（避免真 Java 客户端 field 8 类型校验崩溃）。
     */
    static ::mc::entity::DataParameter<::mc::Vector3i> DATA_START_POS_PARAM;

protected:
    /// 本类继承链标识（parent = Entity::classInfo()）。见 Entity::classInfo()。
    static const EntityClassInfo& classInfo();
};

/**
 * @brief TNT实体
 *
 * 被激活的TNT方块，倒计时后爆炸。
 *
 * 网络同步：
 * - DATA_FUSE_PARAM：引信剩余 tick（i32），对应 MC 1.21.11 PrimedTnt.DATA_FUSE_ID（id8）。
 * - DATA_BLOCK_STATE_PARAM：TNT 方块状态（BlockStateValue → BLOCK_STATE 序列化器 id14，id9），
 *   对应 MC 1.21.11 PrimedTnt.DATA_BLOCK_STATE_ID。承载 BlockState 的 stateId，默认为 TNT
 *   方块默认状态。客户端据此渲染 TNT 外观。
 */
class TNTEntity : public Entity {
public:
    TNTEntity(ecs::EntityRegistry& registry);
    explicit TNTEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~TNTEntity() override = default;

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @param registry ECS 实体注册表
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    void tick() override;

    [[nodiscard]] f32 width() const override { return 0.98f; }
    [[nodiscard]] f32 height() const override { return 0.98f; }
    [[nodiscard]] bool isPushable() const { return false; }
    [[nodiscard]] bool canBeCollidedWith() const override { return false; }

    // 无战利品表，覆写基类方法返回空字符串
    [[nodiscard]] std::string getLootTableId() const override { return {}; }

    /**
     * @brief 获取爆炸倒计时
     *
     * 读取 DataParameter DATA_FUSE_PARAM 的值，与 MC 1.21.11 PrimedTnt.getFuse() 一致。
     */
    [[nodiscard]] i32 getFuse() const;

    /**
     * @brief 设置引信时间
     *
     * 同时写入 DataParameter DATA_FUSE_PARAM 以同步到客户端，
     * 对应 MC 1.21.11 PrimedTnt.setFuse(int)。
     */
    void setFuse(i32 fuse);

    /**
     * @brief 设置爆炸半径
     */
    void setExplosionRadius(f32 radius) { m_explosionRadius = radius; }
    [[nodiscard]] f32 getExplosionRadius() const { return m_explosionRadius; }

    /**
     * @brief 点燃TNT
     *
     * 设置引信时间为默认值（80 ticks）并开始倒计时。
     */
    void ignite();

    /**
     * @brief 点燃TNT并指定引信时间
     *
     * @param fuseTicks 引信时间（ticks）
     */
    void ignite(i32 fuseTicks);

    /**
     * @brief 设置点燃者
     * @param igniter 点燃TNT的实体（可为nullptr）
     *
     * 用于追踪爆炸责任的归属。
     */
    void setOwner(LivingEntity* igniter) { m_owner = igniter; }
    [[nodiscard]] LivingEntity* getOwner() const { return m_owner; }

    /**
     * @brief 爆炸
     */
    void explode();

    /**
     * @brief 检查是否已点燃
     */
    [[nodiscard]] bool isPrimed() const { return m_fuse > 0; }

    /**
     * @brief 获取引信参数 ID（供客户端 ClientEntity 读取）
     */
    [[nodiscard]] static u16 getFuseParamId() { return DATA_FUSE_PARAM.id(); }

    /**
     * @brief 获取 TNT 方块状态参数 ID（供客户端 ClientEntity 读取）
     *
     * 对应 MC 1.21.11 PrimedTnt.DATA_BLOCK_STATE_ID（BLOCK_STATE 序列化器 id14，id9）。
     */
    [[nodiscard]] static u16 getBlockStateParamId() { return DATA_BLOCK_STATE_PARAM.id(); }

protected:
    /**
     * @brief 注册网络同步数据参数
     *
     * 注册 DATA_FUSE_PARAM（Int，id8）和 DATA_BLOCK_STATE_PARAM（BlockStateValue→BLOCK_STATE id14，id9）
     * 到 EntityDataManager，由 EntityTracker 自动广播到客户端。
     *
     * 必须在构造函数中显式调用（参考 EndermanEntity 模式），因为基类构造函数
     * 中的虚函数调用不会派发到派生类。
     */
    void registerData() override;

private:
    i32 m_fuse = 0;
    f32 m_explosionRadius = 4.0f;
    bool m_exploded = false;
    LivingEntity* m_owner = nullptr;
    static constexpr i32 DEFAULT_FUSE = 80; // 4秒（80 ticks）

    /**
     * @brief 引信同步参数
     *
     * 对应 MC 1.21.11 PrimedTnt.DATA_FUSE_ID。
     * 存储 i32 引信剩余 tick。由 setFuse 写入，由 EntityTracker 自动广播。
     * 客户端 ClientEntity::syncMetadataFromDataManager 读取后缓存到 m_tntFuse，
     * 供 TNTRenderer 计算闪烁动画。
     */
    static ::mc::entity::DataParameter<i32> DATA_FUSE_PARAM;

    /**
     * @brief TNT 方块状态同步参数
     *
     * 对应 MC 1.21.11 PrimedTnt.DATA_BLOCK_STATE_ID（BLOCK_STATE 序列化器 id14，id9）。
     * 承载 BlockState 的 stateId（BlockStateValue），默认为 TNT 方块默认状态。
     * 客户端 ClientEntity 读取后通过 BlockRegistry::getBlockState 解析为 BlockState*
     * 并缓存到镜像字段，供 TNTRenderer 渲染 TNT 方块模型。
     */
    static ::mc::entity::DataParameter<::mc::entity::BlockStateValue> DATA_BLOCK_STATE_PARAM;

protected:
    /// 本类继承链标识（parent = Entity::classInfo()）。见 Entity::classInfo()。
    static const EntityClassInfo& classInfo();
};

/**
 * @brief 监守者警告追踪器
 *
 * 追踪玩家在深暗之域中被幽匿尖啸体警告的等级和冷却。
 *
 * 核心机制：
 * - 警告等级范围为 0-4，每次触发时递增 1
 * - 递增后有 200 tick (10秒) 冷却，冷却中不可再次递增
 * - 每 12000 tick (10分钟) 未触发新警告，警告等级自动降 1
 * - 附近所有玩家的追踪器数据通过 copyData() 保持同步
 */
class WardenWarningEffect {
public:
    WardenWarningEffect() = default;
    ~WardenWarningEffect() = default;

    /// 每tick调用：递增计时器、冷却递减、超时自动降级
    void tick();

    /// 重置所有状态为初始值（由 /warden_spawn_tracker clear 命令调用）
    void reset();

    [[nodiscard]] i32 getWarningLevel() const { return m_warningLevel; }
    void setWarningLevel(i32 level);

    /// 递增警告等级（仅在非冷却期间生效，同时重置递减计时器和设置冷却）
    void increaseWarning();

    /// 递减警告等级（由 tick() 自动调用，不暴露给外部）
    void decreaseWarning();

    /// 检查是否处于冷却中（冷却期间不可递增警告等级）
    [[nodiscard]] bool onCooldown() const { return m_cooldownTicks > 0; }

    /// 从另一个追踪器复制数据（同步附近玩家警告等级）
    void copyData(const WardenWarningEffect& other);

    [[nodiscard]] BlockPos getSourcePos() const { return m_sourcePos; }
    void setSourcePos(BlockPos pos) { m_sourcePos = pos; }

    [[nodiscard]] f32 getWarningRadius() const { return m_warningRadius; }

    // ========== 常量 ==========

    static constexpr i32 MAX_WARNING_LEVEL = 4;
    static constexpr f32 PLAYER_SEARCH_RADIUS = 16.0f;
    static constexpr i32 WARNING_CHECK_DIAMETER = 48;
    static constexpr i32 DECREASE_WARNING_LEVEL_EVERY_INTERVAL = 12000; // 10 分钟
    static constexpr i32 WARNING_LEVEL_INCREASE_COOLDOWN = 200;         // 10 秒

private:
    i32 m_warningLevel = 0;
    i32 m_cooldownTicks = 0;
    i32 m_ticksSinceLastWarning = 0;
    BlockPos m_sourcePos;
    f32 m_warningRadius = 10.0f;

    // 保留旧常量名作为别名，确保向后兼容
    static constexpr i32 MAX_WARNING = MAX_WARNING_LEVEL;
    static constexpr i32 DECREASE_INTERVAL = DECREASE_WARNING_LEVEL_EVERY_INTERVAL;
};

// 注意: EvokerFangsEntity 和 EyeOfEnderEntity 已移至
// src/common/entity/entities/projectile/OtherProjectiles.hpp 以避免重复定义

} // namespace entity
} // namespace mc
