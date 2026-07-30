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
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/entity/interfaces/IAngerable.hpp"
#include "common/entity/interfaces/IFlyingAnimal.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>

namespace mc {

// Forward declarations
class LivingEntity;
class BlockState;

namespace blockentity {
class BeehiveBlockEntity;
}

namespace entity::ai::goal {
class BeePollinateGoal;
class BeeFindHiveGoal;
class BeeFindFlowerGoal;
} // namespace entity::ai::goal

/**
 * @brief 蜜蜂实体
 *
 * 生活在蜂巢和蜂箱中的小型飞行生物。
 *
 * 特性：
 * - 授粉：采集花朵后获得花粉，可用于加速农作物生长
 * - 蜂巢记忆：记住蜂巢位置并返回
 * - 攻击：被攻击后会群起攻击，螫刺后死亡
 * - 飞行：可以飞行
 * - 群体：会召唤其他蜜蜂一起攻击
 */
class BeeEntity : public AnimalEntity, public entity::IFlyingAnimal, public entity::IAngerable {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    BeeEntity(EntityInstanceId id);
    ~BeeEntity() override = default;

    // 禁止拷贝
    BeeEntity(const BeeEntity&) = delete;
    BeeEntity& operator=(const BeeEntity&) = delete;

    // 允许移动
    BeeEntity(BeeEntity&&) = delete;
    BeeEntity& operator=(BeeEntity&&) = delete;

    /**
     * @brief 创建蜜蜂实体
     * @param world 世界实例
     * @return 新的蜜蜂实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 花朵吸引判定 ==========

    /**
     * @brief 判定给定方块状态是否吸引蜜蜂
     *
     * 用于眼眸花等方块在 onEntityCollision 中判定蜜蜂是否应被施加效果，
     * 也用于蜜蜂 AI 寻找授粉目标时过滤候选方块。
     *
     * 判定规则（与 MC 1.21.11 Bee.attractsBees 一致）：
     * 1. 方块必须在 BlockTags::BEE_ATTRACTIVE 标签中
     *    （闭合眼眸花不在该标签中，因此不吸引蜜蜂）
     * 2. 含水（waterlogged=true）的花朵不吸引蜜蜂
     * 3. 向日葵仅上半部分（DoubleBlockHalf::Upper）吸引蜜蜂
     *
     * 参考: net.minecraft.world.entity.animal.bee.Bee#attractsBees
     *
     * @param state 待判定的方块状态
     * @return 是否吸引蜜蜂
     */
    [[nodiscard]] static bool attractsBees(const BlockState& state);

    // ========== 花粉状态 ==========

    /**
     * @brief 是否携带花粉
     * 从 DataParameter 读取，支持网络同步
     */
    [[nodiscard]] bool hasNectar() const;

    /**
     * @brief 设置花粉状态
     * 通过 DataParameter 同步到客户端
     */
    void setHasNectar(bool nectar);

    /**
     * @brief 是否有螫刺
     * 蜜蜂失去螫刺后无法攻击
     * 从 DataParameter 读取，支持网络同步
     */
    [[nodiscard]] bool hasStung() const;

    /**
     * @brief 设置螫刺状态
     * 通过 DataParameter 同步到客户端
     */
    void setHasStung(bool stung);

    // ========== 蜂巢系统 ==========

    /**
     * @brief 设置蜂巢位置
     * @param pos 蜂巢位置
     */
    void setHivePos(const BlockPos& pos);

    /**
     * @brief 获取蜂巢位置
     */
    [[nodiscard]] const BlockPos& getHivePos() const { return m_hivePos; }

    /**
     * @brief 是否有蜂巢
     */
    [[nodiscard]] bool hasHive() const { return m_hasHive; }

    /**
     * @brief 设置是否有蜂巢
     */
    void setHasHive(bool hasHive) { m_hasHive = hasHive; }

    /**
     * @brief 是否正在返回蜂巢
     */
    [[nodiscard]] bool isReturningToHive() const { return m_returningToHive; }

    /**
     * @brief 设置返回蜂巢状态
     */
    void setReturningToHive(bool returning) { m_returningToHive = returning; }

    /**
     * @brief 检查蜂巢是否有效
     * @return 蜂巢位置非空、距离在48格内、且存在蜂巢方块实体
     */
    [[nodiscard]] bool isHiveValid() const;

    /**
     * @brief 获取蜂巢方块实体
     * @return 蜂巢方块实体指针，如果无效返回nullptr
     */
    [[nodiscard]] blockentity::BeehiveBlockEntity* getBeehiveBlockEntity() const;

    /**
     * @brief 蜜蜂是否想要进入蜂巢
     * @return 满足进入蜂巢的所有条件时返回true
     *
     * 条件：stayOutOfHiveCountdown <= 0 && !isPollinating && !hasStung
     *       && getAttackTarget() == nullptr
     *       && (hasNectar || isTiredOfLookingForNectar || isRainingOrNight)
     *       && !isHiveNearFire()
     *
     * 其中 isRainingOrNight 在雨天/雷暴/夜间为 true。
     */
    [[nodiscard]] bool wantsToEnterHive() const;

    /**
     * @brief 检查蜂巢附近是否有火
     * @return 蜂巢3x3x3范围内是否有火
     */
    [[nodiscard]] bool isHiveNearFire() const;

    /**
     * @brief 忘记蜂巢位置
     * 清空蜂巢位置并设置200 tick冷却
     */
    void dropHive();

    /**
     * @brief 设置不进入蜂巢的倒计时
     */
    void setStayOutOfHiveCountdown(i32 countdown) { m_stayOutOfHiveCountdown = countdown; }

    /**
     * @brief 获取不进入蜂巢的倒计时
     */
    [[nodiscard]] i32 getStayOutOfHiveCountdown() const { return m_stayOutOfHiveCountdown; }

    /**
     * @brief 设置寻找新蜂巢冷却
     */
    void setHiveLocateCooldown(i32 cooldown) { m_remainingCooldownBeforeLocatingNewHive = cooldown; }

    /**
     * @brief 获取寻找新蜂巢冷却
     */
    [[nodiscard]] i32 getHiveLocateCooldown() const { return m_remainingCooldownBeforeLocatingNewHive; }

    /**
     * @brief 设置寻找新花朵冷却
     */
    void setFlowerCooldown(i32 cooldown) { m_remainingCooldownBeforeLocatingNewFlower = cooldown; }

    /**
     * @brief 获取寻找新花朵冷却
     */
    [[nodiscard]] i32 getFlowerCooldown() const { return m_remainingCooldownBeforeLocatingNewFlower; }

    // ========== 飞行系统 (IFlyingAnimal接口) ==========

    /**
     * @brief 是否正在飞行 (IFlyingAnimal接口实现)
     */
    [[nodiscard]] bool isFlying() const override { return m_flying; }

    /**
     * @brief 设置飞行状态 (IFlyingAnimal接口实现)
     */
    void setFlying(bool flying) override { m_flying = flying; }

    /**
     * @brief 获取飞行目标位置
     */
    [[nodiscard]] BlockPos getFlowerPos() const { return m_flowerPos; }

    /**
     * @brief 设置花朵位置
     */
    void setFlowerPos(const BlockPos& pos);

    /**
     * @brief 清除花朵位置
     */
    void clearFlowerPos()
    {
        m_flowerPos = BlockPos::zero();
        m_hasFlower = false;
    }

    /**
     * @brief 是否有花朵目标
     */
    [[nodiscard]] bool hasFlower() const { return m_hasFlower; }

    // ========== 繁殖 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     * 蜜蜂使用花朵繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.3f; }

    // ========== 生命周期 ==========

    void tick() override;

    // ========== 数据同步 ==========

    /**
     * @brief 注册同步数据参数
     * 注册 DATA_FLAGS 和 ANGER_TIME 数据参数
     */
    void registerData() override;

    // ========== IAngerable 接口实现 ==========

    /**
     * @brief 设置攻击目标 (IAngerable)
     */
    void setAttackTarget(LivingEntity* target) override { MobEntity::setAttackTarget(target); }

    /**
     * @brief 获取攻击目标 (IAngerable)
     */
    [[nodiscard]] LivingEntity* getAttackTarget() const override
    {
        return const_cast<BeeEntity*>(this)->MobEntity::attackTarget();
    }

    /**
     * @brief 设置复仇目标 (IAngerable)
     * 设置愤怒目标和愤怒时间
     */
    void setRevengeTarget(LivingEntity* target) override;

    /**
     * @brief 获取复仇目标 (IAngerable)
     */
    [[nodiscard]] LivingEntity* getRevengeTarget() const override;

    /**
     * @brief 获取复仇计时器 (IAngerable)
     */
    [[nodiscard]] i32 getRevengeTimer() const override { return m_revengeTimer; }

    /**
     * @brief 是否愤怒 (IAngerable)
     */
    [[nodiscard]] bool isAngry() const override { return getAngerTime() > 0; }

    /**
     * @brief 设置愤怒状态 (IAngerable)
     * 设置愤怒时间为默认值
     */
    void setAngry(bool angry) override;

    /**
     * @brief 获取愤怒时间 (IAngerable)
     * 从 DataParameter 读取
     */
    [[nodiscard]] i32 getAngerTime() const override;

    /**
     * @brief 设置愤怒时间 (IAngerable)
     * 通过 DataParameter 同步
     */
    void setAngerTime(i32 time) override;

    /**
     * @brief 更新愤怒计时器 (IAngerable)
     *
     * 每tick调用，递减愤怒时间
     */
    void updateAnger() override;

    /**
     * @brief 是否正在攻击
     */
    [[nodiscard]] bool isAttacking() const { return m_attacking; }

    /**
     * @brief 设置攻击状态
     */
    void setAttacking(bool attacking) { m_attacking = attacking; }

    // ========== 标记 ==========

    /**
     * @brief 设置攻击目标标记
     * 用于通知其他蜜蜂一起攻击
     */
    void setTargetPlayer(u64 playerId) { m_targetPlayerId = playerId; }

    /**
     * @brief 获取攻击目标玩家ID
     */
    [[nodiscard]] u64 getTargetPlayer() const { return m_targetPlayerId; }

    /**
     * @brief 获取水下计时器
     * 用于追踪蜜蜂在水中的时间
     */
    [[nodiscard]] i32 getUnderWaterTimer() const { return m_underWaterTimer; }

    // ========== 导航辅助 ==========

    /**
     * @brief 随机朝向目标位置寻路飞行
     *
     * 对应 MC 1.21.11 Bee.pathfindRandomlyTowards()。
     * 不直接飞向目标，而是在目标方向 PI/10 弧度锥形范围内选择随机空中航点，
     * 产生蜜蜂特有的漂移飞行效果。
     *
     * 算法：
     * 1. 计算到目标的曼哈顿距离，近距离时缩小搜索范围
     * 2. 根据Y轴高度差计算垂直偏移（上2格→+4，下2格→-4）
     * 3. 使用 AirRandomPos.getPosTowards 生成目标方向18度锥形内的随机空中位置
     * 4. 设置导航器寻路并降低寻路开销
     *
     * @param targetPos 目标方块位置（蜂巢或花朵）
     * @return 是否成功生成并开始导航到航点
     */
    [[nodiscard]] bool pathfindRandomlyTowards(const BlockPos& targetPos);

    /**
     * @brief 直接朝向目标位置寻路
     *
     * 对应 MC 1.21.11 BeeGoToHiveGoal.pathfindDirectlyTowards()。
     * 近距离（16格内）使用精确导航，设置高寻路开销以确保路径可达。
     *
     * @param targetPos 目标方块位置
     * @return 是否找到可达路径
     */
    [[nodiscard]] bool pathfindDirectlyTowards(const BlockPos& targetPos);

    // ========== 授粉系统 ==========

    /**
     * @brief 检查是否正在授粉
     */
    [[nodiscard]] bool isPollinating() const { return m_pollinating; }

    /**
     * @brief 设置授粉状态
     */
    void setPollinating(bool pollinating) { m_pollinating = pollinating; }

    /**
     * @brief 重置离巢无花粉计时
     */
    void resetTicksWithoutNectar() { m_ticksWithoutNectarSinceExitingHive = 0; }

    /**
     * @brief 获取离巢无花粉计时
     */
    [[nodiscard]] i32 getTicksWithoutNectar() const { return m_ticksWithoutNectarSinceExitingHive; }

    /**
     * @brief 蜜蜂是否厌倦寻找花蜜
     *
     * 离巢后超过 3600 tick（3分钟）仍未获得花蜜时返回 true。
     */
    [[nodiscard]] bool isTiredOfLookingForNectar() const { return m_ticksWithoutNectarSinceExitingHive > 3600; }

    /**
     * @brief 增加授粉作物计数
     */
    void addCropCounter() { ++m_cropsGrownSincePollination; }

    /**
     * @brief 重置授粉作物计数
     */
    void resetCropCounter() { m_cropsGrownSincePollination = 0; }

    /**
     * @brief 获取授粉作物计数
     */
    [[nodiscard]] i32 getCropsGrownSincePollination() const { return m_cropsGrownSincePollination; }

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    /**
     * @brief 获取环境音效
     *
     * 蜜蜂无环境音（飞行音由客户端 BeeFlyingSoundInstance 循环处理），
     * 对齐原版 Bee.getAmbientSound 返回 null，避免默认拼接出不存在的 entity.bee.ambient。
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

private:
    // ========== MC 1.16.5 数据参数 ==========
    // DATA_FLAGS (i8): 位标志
    //   - bit 2 (0x02): NEAR_TARGET - 接近攻击目标
    //   - bit 3 (0x04): HAS_STUNG - 已螫刺
    //   - bit 4 (0x08): HAS_NECTAR - 携带花蜜
    static entity::DataParameter<i8> DATA_FLAGS_PARAM;
    // ANGER_TIME (i32): 愤怒时间（ticks）
    static entity::DataParameter<i32> ANGER_TIME_PARAM;

protected:
    /// 本类继承链标识（parent = AnimalEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

private:
    // 数据参数标志位
    static constexpr i8 FLAG_NEAR_TARGET = 2; // bit 1: 接近攻击目标
    static constexpr i8 FLAG_HAS_STUNG = 4;   // bit 2: 已螫刺
    static constexpr i8 FLAG_HAS_NECTAR = 8;  // bit 3: 携带花蜜

    // ========== 花粉状态 ==========
    bool m_hasNectar = false;
    bool m_hasStung = false;

    // ========== 蜂巢系统 ==========
    BlockPos m_hivePos;
    bool m_hasHive = false;
    bool m_returningToHive = false;

    // ========== 花朵位置 ==========
    BlockPos m_flowerPos;
    bool m_hasFlower = false;

    // ========== 愤怒系统（m_attackTarget 使用 MobEntity::m_attackTarget，不重复声明） ==========
    std::optional<u64> m_revengeTargetId;
    i32 m_revengeTimer = 0;
    i32 m_angerTime = 0; // 本地缓存，从 DataParameter 同步
    bool m_attacking = false;
    u64 m_targetPlayerId = 0;

    // ========== 飞行状态 ==========
    bool m_flying = false;

    // ========== 计时器 ==========
    i32 m_underWaterTimer = 0;
    i32 m_timeSinceSting = 0; ///< 蛰刺后经过的 tick 数

    // ========== MC 1.16.5 计时器和计数器 ==========
    i32 m_ticksWithoutNectarSinceExitingHive = 0;       ///< 离巢后无花粉的tick数
    i32 m_cropsGrownSincePollination = 0;               ///< 授粉后促生长的作物数
    i32 m_stayOutOfHiveCountdown = 0;                   ///< 不进入蜂巢的倒计时
    i32 m_remainingCooldownBeforeLocatingNewHive = 0;   ///< 寻找新蜂巢冷却
    i32 m_remainingCooldownBeforeLocatingNewFlower = 0; ///< 寻找新花朵冷却
    bool m_pollinating = false;                         ///< 是否正在授粉

    // ========== 常量 ==========
    static constexpr i32 MAX_ANGER_TIME = 1200; // 60秒
    static constexpr i32 STING_DAMAGE = 2;      // 螫刺伤害

    // ========== 私有辅助方法 ==========

    /**
     * @brief 获取数据参数标志位
     * @param flag 标志位掩码
     * @return 标志位是否设置
     */
    [[nodiscard]] bool _getBeeFlag(i8 flag) const;

    /**
     * @brief 设置数据参数标志位
     * @param flag 标志位掩码
     * @param value 是否设置
     */
    void _setBeeFlag(i8 flag, bool value);
};

} // namespace mc
