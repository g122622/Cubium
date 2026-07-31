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

#include "../../../interfaces/IRangedAttackMob.hpp"
#include "ZombieEntity.hpp"
#include <memory>

namespace mc {

// 前向声明
class LivingEntity;

/**
 * @brief 溺尸实体
 *
 * 在水中生成的僵尸变种。可以手持三叉戟进行远程攻击，
 * 也可以在水中游泳、在夜间登陆海滩攻击玩家。
 *
 * 特性：
 * - 水中生成：在海洋和河流中生成
 * - 水中生活：可以在水中呼吸
 * - 三叉戟：有概率手持三叉戟，进行远程投掷攻击
 * - 日间避阳：白天会主动寻找水源
 * - 夜间登陆：夜间会游到水面并登陆海滩
 * - 溺水转化：玩家溺水后可能转化为溺尸
 */
class DrownedEntity : public ZombieEntity, public entity::IRangedAttackMob {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    DrownedEntity(EntityInstanceId id);

    ~DrownedEntity() override = default;

    // 禁止拷贝
    DrownedEntity(const DrownedEntity&) = delete;
    DrownedEntity& operator=(const DrownedEntity&) = delete;

    // 允许移动
    DrownedEntity(DrownedEntity&&) = delete;
    DrownedEntity& operator=(DrownedEntity&&) = delete;

    /// 本类继承链标识（parent = ZombieEntity::classInfo()）。见 Entity::classInfo()。
    // 透传层无自身同步字段，classInfo 仅作父链遍历节点。
    static const entity::EntityClassInfo& classInfo();

    /**
     * @brief 创建溺尸实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 水中生活 ==========

    /**
     * @brief 是否在水中
     */
    [[nodiscard]] bool isInWater() const override;

    /**
     * @brief 是否可以游泳
     */
    [[nodiscard]] bool canSwim() const noexcept { return true; }

    /**
     * @brief 溺尸可以在液体中生成
     *
     * 溺尸是水中生物，增援生成时允许在水中生成。
     */
    [[nodiscard]] bool canSpawnInLiquids() const override { return true; }

    /**
     * @brief 溺尸不需要溺水转化（已是溺尸状态）
     *
     * 防止溺尸反复触发溺水转化逻辑。
     */
    [[nodiscard]] bool shouldDrown() const override { return false; }

    // ========== 游泳状态同步 ==========

    /**
     * @brief 更新游泳标志位
     *
     * 对应 MC 1.21.11 Drowned.updateSwimming()：
     *   if (!level.isClientSide) setSwimming(isEffectiveAi() && isUnderWater() && wantsToSwim());
     *
     * 仅在服务端调用：当溺尸在水中（眼与身体都在水中）、且 wantsToSwim() 为真、且未骑乘
     * 其他实体时，置位 Swimming 标志。该标志通过 DATA_FLAGS_PARAM 同步到客户端，进而驱动
     * isVisuallySwimming() 与 swimAmount 渐入渐出，最终触发 DrownedModel 的游泳手臂/腿部覆盖动画。
     *
     * isEffectiveAi() 在原版用于排除 NoAi 实体；Cubium 不支持 NoAi，且本方法仅在服务端
     * tick 路径调用，因此等价于恒真，这里通过 isRiding() 排除骑乘状态以匹配
     * isVisuallySwimming 的视觉约束。
     */
    void updateSwimming();

    /**
     * @brief 重写视觉游泳判定
     *
     * 对应 MC 1.21.11 Drowned.isVisuallySwimming()：
     *   return isSwimming() && !isPassenger();
     *
     * 与基类 LivingEntity 不同，溺尸的视觉游泳完全由 Swimming 标志位驱动
     * （基类还考虑 Pose::Swimming 与 FALL_FLYING 姿态），且要求未骑乘其他实体。
     * 该返回值驱动 updateSwimAmount 的渐入渐出。
     */
    [[nodiscard]] bool isVisuallySwimming() const override;

    // ========== 装备 ==========

    /**
     * @brief 是否手持三叉戟
     */
    [[nodiscard]] bool hasTrident() const { return m_hasTrident; }

    /**
     * @brief 设置手持三叉戟
     */
    void setHasTrident(bool trident) { m_hasTrident = trident; }

    // ========== 阳光燃烧 ==========

    /**
     * @brief 溺尸不在阳光下燃烧（如果在水中）
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override;

    // ========== 目标过滤 ==========

    /**
     * @brief 判断目标是否有效
     *
     * 溺尸只在非白天或目标在水中时才视为有效攻击目标。
     * 这控制了溺尸何时主动攻击玩家等目标。
     *
     * @param target 潜在攻击目标
     * @return 如果目标有效返回 true
     */
    [[nodiscard]] bool okTarget(const LivingEntity* target) const;

    /**
     * @brief 溺尸是否想要游泳
     *
     * 当 searchingForLand 为 true 或当前攻击目标在水中时返回 true。
     * 用于 DrownedMoveControl 判断是否应用水中移动逻辑。
     */
    [[nodiscard]] bool wantsToSwim() const;

    // ========== 陆地搜索状态 ==========

    /**
     * @brief 是否正在搜索陆地
     */
    [[nodiscard]] bool isSearchingForLand() const { return m_searchingForLand; }

    /**
     * @brief 设置搜索陆地状态
     */
    void setSearchingForLand(bool searching) { m_searchingForLand = searching; }

    // ========== 远程攻击 (IRangedAttackMob) ==========

    /**
     * @brief 使用三叉戟进行远程攻击
     * @param target 攻击目标
     * @param charge 蓄力程度 (0.0 - 1.0)
     */
    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;

    // ========== 生命周期 ==========

    void tick() override;

    // ========== 生成初始化 ==========

    /**
     * @brief 完成溺尸的生成初始化
     *
     * 在父类（僵尸）生成初始化之后，随机决定是否手持三叉戟：
     * 约 10% 概率装主手武器，其中 10/16 为三叉戟（综合约 6.25%）。
     * 仅在生成初始化路径中决定，未经过生成初始化的溺尸默认不持有三叉戟。
     *
     * @param world 世界引用
     * @param difficulty 区域难度实例
     * @param spawnReason 生成原因
     */
    void finalizeSpawn(IWorld& world,
        const entity::combat::DifficultyInstance& difficulty,
        world::spawn::SpawnReason spawnReason) override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_hasTrident = false;
    bool m_searchingForLand = false;

    /// 三叉戟投掷速度
    static constexpr f32 TRIDENT_VELOCITY = 1.6f;
};

} // namespace mc
