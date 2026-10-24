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

#include "ZombieEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include <memory>

namespace mc {

/**
 * @brief 尸壳实体
 *
 * 在沙漠中生成的僵尸变种。
 *
 * 特性：
 * - 沙漠生成：在沙漠生物群系生成
 * - 不燃烧：在阳光下不燃烧
 * - 饥饿攻击：攻击会使玩家饥饿
 */
class HuskEntity : public ZombieEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    HuskEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    ~HuskEntity() override = default;

    // 禁止拷贝
    HuskEntity(const HuskEntity&) = delete;
    HuskEntity& operator=(const HuskEntity&) = delete;

    // 允许移动
    HuskEntity(HuskEntity&&) = delete;
    HuskEntity& operator=(HuskEntity&&) = delete;

    /// 本类继承链标识（parent = ZombieEntity::classInfo()）。见 Entity::classInfo()。
    // 透传层无自身同步字段，classInfo 仅作父链遍历节点。
    static const entity::EntityClassInfo& classInfo();

    /**
     * @brief 创建尸壳实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 阳光燃烧 ==========

    /**
     * @brief 尸壳不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override
    {
        return false;
    }

    // ========== 饥饿攻击 ==========

    /**
     * @brief 尸壳空手近战攻击目标时施加饥饿效果
     *
     * 对齐 MC 1.21.11 Husk.doHurtTarget（Husk.java:57-65）：父类基础攻击命中 + 主手为空 +
     * 目标是 LivingEntity 时，取区域难度 effectiveDifficulty，对目标施加饥饿效果
     * 140 * (i32)effectiveDifficulty ticks（Normal 难度≈280 ticks=14 秒，等级 I）。
     * 持物品的尸壳不施加饥饿（对齐 vanilla getMainHandItem().isEmpty() 门控）。
     *
     * 由 MeleeAttackGoal::_attackTarget 委托调用（对齐 vanilla MeleeAttackGoal 调 doHurtTarget）。
     */
    bool attackEntityAsMob(LivingEntity& target) override;

    // ========== 骆驼骑兵生成 ==========

    /// 自然生成的尸壳召唤骆驼尸壳 + 干枯骷髅骑兵的概率（对齐 vanilla Husk.finalizeSpawn:0.1F）
    static constexpr f32 CAMEL_HUSK_JOCKEY_CHANCE = 0.1f;

    /**
     * @brief 生成初始化：含骆驼尸壳骑兵分支
     *
     * 对齐 vanilla Husk.finalizeSpawn（Husk.java:90-122）：父类初始化后，若为自然生成且
     * 本组尚未尝试过、且当前位置可容纳骆驼尸壳（无碰撞），则 10% 概率：
     * 主手换铁长矛 → 生成骆驼尸壳并让本尸壳骑上 → 生成干枯骷髅作为骆驼的第二个乘客。
     *
     * 组数据（vanilla Husk.HuskGroupData.triedToSpawnCamelHusk，保证同组只尝试一次）在
     * Cubium 的 finalizeSpawn 签名中没有 SpawnGroupData 参数，故以"每次自然生成都尝试"
     * 近似；该偏差只影响同一刷怪组内出现骑兵的组数上限，不影响骑兵本身的生成逻辑。
     */
    void finalizeSpawn(mc::IWorld& world,
        const mc::entity::combat::DifficultyInstance& difficulty,
        mc::world::spawn::SpawnReason spawnReason) override;

protected:
    void registerAttributes() override;

    /// 执行骆驼尸壳骑兵生成（失败时静默返回，对齐 vanilla 各判空分支）
    void _trySpawnCamelHuskJockey(mc::IWorld& world,
        const mc::entity::combat::DifficultyInstance& difficulty,
        mc::world::spawn::SpawnReason spawnReason);
};

} // namespace mc
