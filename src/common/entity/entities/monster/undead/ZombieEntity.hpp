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

#include "../MonsterEntity.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <memory>
#include <optional>

namespace mc {

// 前向声明
namespace entity::ai::goal {
class BreakDoorGoal;
}

using entity::ai::goal::BreakDoorGoal;

/**
 * @brief 僵尸实体
 *
 * 最常见的亡灵怪物。
 *
 * 特性：
 * - 攻击：近战攻击玩家和村民
 * - 燃烧：在阳光下燃烧
 * - 增援：被攻击时有概率召唤增援
 * - 感染：杀死村民会将其转化为僵尸村民
 * - 变种：可转化为溺尸
 * - 破门：可以破坏木门
 */
class ZombieEntity : public MonsterEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    ZombieEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~ZombieEntity() override = default;

    /// 本类继承链标识（parent = MonsterEntity::classInfo()）。见 Entity::classInfo()。
    // vanilla 1.21.11 Zombie 在 Mob(id15) 之后注册三字段：DATA_BABY_ID(Boolean,id16)、
    // DATA_SPECIAL_TYPE_ID(Int,id17)、DATA_DROWNED_CONVERSION_ID(Boolean,id18)。本项目保留
    // 此层并补齐 registerData+ClassRegisterGuard 对齐 vanilla 字段 id，见
    // entity-sync-alignment-decisions-2026-07。
    static const entity::EntityClassInfo& classInfo();

    // 禁止拷贝
    ZombieEntity(const ZombieEntity&) = delete;
    ZombieEntity& operator=(const ZombieEntity&) = delete;

    // 允许移动
    ZombieEntity(ZombieEntity&&) = delete;
    ZombieEntity& operator=(ZombieEntity&&) = delete;

    /**
     * @brief 创建僵尸实体
     * @param world 世界实例
     * @return 新的僵尸实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 声音 ==========

    /**
     * @brief 获取环境音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取脚步声音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getStepSound() const;

    // ========== 脚步声 ==========

    /**
     * @brief 播放脚步声
     */
    void playStepSound(const BlockPos& pos, const BlockState* blockState) override;

    // ========== 破门能力 ==========

    /**
     * @brief 是否可以破门
     */
    [[nodiscard]] bool canBreakDoors() const { return m_canBreakDoors; }

    /**
     * @brief 设置破门能力
     */
    void setBreakDoorsAbility(bool canBreak);

    // ========== 溺水转化 ==========

    /**
     * @brief 是否正在转化为溺尸
     */
    [[nodiscard]] bool isConverting() const { return m_converting; }

    /**
     * @brief 获取转化时间
     */
    [[nodiscard]] i32 getConversionTime() const { return m_conversionTime; }

    /**
     * @brief 开始溺水转化
     */
    void startDrowning(i32 conversionTime);

    /**
     * @brief 是否应该溺水转化
     *
     * 僵尸和尸壳返回 true（在水中会转化为溺尸），
     * 僵尸村民和溺尸返回 false。
     * 注意：此方法仅用于溺水转化逻辑，不用于增援生成检查。
     * 增援生成中的液体检查使用 canSpawnInLiquids()。
     */
    [[nodiscard]] virtual bool shouldDrown() const { return true; }

    /**
     * @brief 转化为溺尸
     *
     * 将当前僵尸转化为溺尸：
     * 1. 创建新的 DrownedEntity
     * 2. 复制位置、旋转、生命值、装备、婴儿状态、自定义名称、持久化状态
     * 3. 清空原僵尸装备（防止死亡掉落）
     * 4. 播放转化音效和事件
     * 5. 移除原僵尸
     */
    virtual void convertToDrowned();

    // ========== 婴儿状态 ==========

    /**
     * @brief 是否是婴儿僵尸
     */
    [[nodiscard]] bool isBaby() const { return m_isBaby; }

    /**
     * @brief 设置婴儿状态
     */
    void setBaby(bool baby);

    // ========== 同步字段 id 访问器（测试/诊断用，抗字段 id 偏移） ==========

    [[nodiscard]] static u16 getBabyParamId() { return DATA_BABY_PARAM.id(); }
    [[nodiscard]] static u16 getSpecialTypeParamId() { return DATA_SPECIAL_TYPE_PARAM.id(); }
    [[nodiscard]] static u16 getDrownedConversionParamId() { return DATA_DROWNED_CONVERSION_PARAM.id(); }

    // ========== 增援系统 ==========

    /**
     * @brief 是否可以召唤增援
     */
    [[nodiscard]] bool canSummonReinforcements() const;

    /**
     * @brief 尝试召唤增援
     *
     * 检查难度、概率等前置条件后，尝试在附近生成增援僵尸。
     * 此方法是增援逻辑的唯一入口，hurt() 内部调用此方法。
     *
     * @param explicitTarget 可选的显式攻击目标。如果为 nullptr，
     *                       则使用当前攻击目标（attackTarget()）。
     *                       hurt() 中会传入伤害来源实体作为备选目标。
     */
    void trySummonReinforcements(LivingEntity* explicitTarget = nullptr);

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return m_isBaby ? 0.93f : 1.74f; }

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return m_isBaby ? 0.3f : 0.6f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return m_isBaby ? 0.975f : 1.95f; }

    // ========== 伤害处理 ==========

    /**
     * @brief 受到伤害时的处理（包含增援逻辑）
     */
    bool hurt(DamageSource& source, f32 amount) override;

    /**
     * @brief 作为生物攻击实体
     *
     * 重写以实现燃烧传递逻辑。
     * 燃烧的僵尸攻击时，有概率点燃目标。
     *
     * @param target 目标生物
     * @return 是否攻击成功
     */
    bool attackEntityAsMob(LivingEntity& target) override;

    // ========== 生命周期 ==========

    void tick() override;

    // ========== 生成初始化 ==========

    /**
     * @brief 完成僵尸的生成初始化
     *
     * 重写 MobEntity::finalizeSpawn() 以实现僵尸特有的初始化：
     * - 设置拾取物品能力 (0.55 * specialMultiplier)
     * - 设置破门能力 (specialMultiplier * 0.1)
     * - 调用父类的装备填充和附魔逻辑
     * - 僵尸特有主手武器（铁剑/铁锹，Hard 难度下概率 5%，其他难度 1%）
     * - 万圣节南瓜头（10月31日，25% 概率）
     * - 属性修饰符（随机增援概率、击退抗性、跟随范围、领袖僵尸判定）
     *
     * @param world 世界引用
     * @param difficulty 区域难度实例
     * @param spawnReason 生成原因
     */
    void finalizeSpawn(mc::IWorld& world,
        const mc::entity::combat::DifficultyInstance& difficulty,
        mc::world::spawn::SpawnReason spawnReason) override;

    /**
     * @brief 根据难度填充僵尸的默认装备
     *
     * 重写以添加僵尸特有的主手武器：
     * - Hard 难度: 5% 概率生成铁剑/铁锹
     * - 其他难度: 1% 概率生成铁剑/铁锹
     *
     * @param random 随机数生成器
     * @param difficulty 区域难度实例
     */
    void populateDefaultEquipmentSlots(
        math::Random& random, const mc::entity::combat::DifficultyInstance& difficulty) override;

    // ========== NBT 序列化 ==========

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

protected:
    // ========== 数据同步 ==========

    /**
     * @brief 注册同步数据参数
     *
     * 重写 MonsterEntity::registerData，注册 vanilla 1.21.11 Zombie 三字段：
     * DATA_BABY(Boolean,id16)、DATA_SPECIAL_TYPE(Int,id17)、DATA_DROWNED_CONVERSION(Boolean,id18)。
     * 派生类构造函数必须显式调用 registerData()，参考 MobEntity/WolfEntity 模式。
     */
    void registerData() override;

    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 破门能力
    bool m_canBreakDoors = false;
    BreakDoorGoal* m_breakDoorGoal = nullptr;

    // 溺水转化
    bool m_converting = false;
    i32 m_conversionTime = 0;
    i32 m_inWaterTime = 0; // 在水中的时间

    // 婴儿状态
    bool m_isBaby = false;

    // ========== 同步数据参数（vanilla 1.21.11 Zombie 三字段，见 registerData） ==========
    static entity::DataParameter<bool> DATA_BABY_PARAM;               // id16，幼体状态同步镜像
    static entity::DataParameter<i32> DATA_SPECIAL_TYPE_PARAM;        // id17，特殊僵尸变体类型（占位 0）
    static entity::DataParameter<bool> DATA_DROWNED_CONVERSION_PARAM; // id18，溺水转化中标志

    // 常量
    static constexpr i32 CONVERSION_DURATION = 300;     // 15秒转化时间 (300 ticks)
    static constexpr i32 IN_WATER_TIME_THRESHOLD = 600; // 水下30秒开始转化
    static constexpr f32 BABY_SPEED_BOOST = 0.5f;       // 婴儿速度加成 50%

    // 增援系统常量（对应 MC 1.21.11 Zombie.REINFORCEMENT_ATTEMPTS/RANGE_MAX/RANGE_MIN）
    static constexpr i32 REINFORCEMENT_ATTEMPTS = 50;         // 增援尝试次数上限
    static constexpr i32 REINFORCEMENT_RANGE_MIN = 7;         // 增援最小距离（偏移量 = nextInt(7,40) * nextInt(-1,1)）
    static constexpr i32 REINFORCEMENT_RANGE_MAX = 40;        // 增援最大距离
    static constexpr f64 REINFORCEMENT_CALLEE_CHARGE = -0.05; // 被召唤增援僵尸的增援概率衰减

    /**
     * @brief 更新溺水转化
     */
    void _updateDrowning();

    /**
     * @brief 根据难度设置属性修饰符
     *
     * 在 finalizeSpawn 末尾调用，设置：
     * - 随机增援概率基础值（0.0~0.1）
     * - 击退抗性随机加成（0~0.05）
     * - 跟随范围随机加成（条件性，>1.0时生效）
     * - 领袖僵尸判定（增援+生命+破门）
     *
     * @param rng 共享的随机数生成器（由调用方传入，避免同tick多次调用getRandom导致种子重复）
     * @param specialMultiplier 区域难度特殊乘数
     */
    void _handleAttributes(math::Random& rng, f32 specialMultiplier);

    /**
     * @brief 尝试在附近生成增援僵尸
     *
     * 对应 MC 1.21.11 Zombie.hurtServer() 中的增援生成逻辑：
     * 1. 在 50 次尝试内随机选择有效生成位置
     * 2. 位置范围：距离 [7, 40] 方块，各轴独立随机正负
     * 3. 验证：使用 EntitySpawnPlacementRegistry 检查生成位置有效性
     *    （对应 MC 的 SpawnPlacements.isSpawnPositionOk + checkSpawnRules）
     * 4. 附近无存活玩家（7格内）
     * 5. 无实体碰撞、无方块碰撞
     * 6. 不在液体中（溺尸除外，使用 canSpawnInLiquids() 判断）
     * 7. 生成同类型僵尸，设置攻击目标，调用 finalizeSpawn
     * 8. 召唤者获得 reinforcement_caller_charge 修饰符 (-0.05)
     * 9. 被召唤者获得 reinforcement_callee_charge 修饰符 (-0.05)
     *
     * @param world 世界引用
     * @param target 增援僵尸的攻击目标
     */
    void _trySpawnReinforcement(IWorld& world, LivingEntity& target);
};

} // namespace mc
