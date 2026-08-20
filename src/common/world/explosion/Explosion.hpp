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

#include "ExplosionContext.hpp"
#include "ExplosionMode.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc {

// 前向声明
class IWorld;
class Entity;
class LivingEntity;
class Player;
class BlockState;
class AxisAlignedBB;
class ItemStack;

namespace entity {
class TNTEntity;
class ProjectileEntity;
} // namespace entity

namespace loot {
class LootTableManager;
}

namespace world::explosion {

/**
 * @brief 爆炸类
 *
 * 实现完整的爆炸行为，包括：
 * - 方块破坏（射线追踪算法）
 * - 实体伤害和击退
 * - 物品掉落
 * - 粒子和音效
 *
 * 使用方法：
 * @code
 * Explosion explosion(world, position, 4.0f, ExplosionMode::Break, false, tntEntity);
 * explosion.explode();
 * @endcode
 */
class Explosion {
public:
    /**
     * @brief 构造爆炸对象
     *
     * @param world 世界引用
     * @param position 爆炸中心位置
     * @param radius 爆炸半径
     * @param mode 爆炸模式
     * @param causesFire 是否生成火焰
     * @param source 爆炸源实体（可能为空）
     * @param damageSource 自定义伤害来源（可能为空，默认使用爆炸伤害）
     * @param lootTableManager 掉落表管理器（可能为空，为空时不掉落物品）
     */
    Explosion(IWorld& world,
        const Vector3& position,
        f32 radius,
        ExplosionMode mode = ExplosionMode::Destroy,
        bool causesFire = false,
        Entity* source = nullptr,
        std::unique_ptr<DamageSource> damageSource = nullptr,
        const loot::LootTableManager* lootTableManager = nullptr);

    /**
     * @brief 构造带自定义爆炸上下文的爆炸对象
     *
     * 允许调用者传入自定义的 ExplosionContext，以控制爆炸对方块的行为。
     * 例如蓝色凋灵之首使用 WitherSkullExplosionContext 来穿透高抗性方块。
     *
     * @param world 世界引用
     * @param position 爆炸中心位置
     * @param radius 爆炸半径
     * @param mode 爆炸模式
     * @param causesFire 是否生成火焰
     * @param source 爆炸源实体（可能为空）
     * @param damageSource 自定义伤害来源（可能为空）
     * @param lootTableManager 掉落表管理器（可能为空）
     * @param context 自定义爆炸上下文（必须非空）
     */
    Explosion(IWorld& world,
        const Vector3& position,
        f32 radius,
        ExplosionMode mode,
        bool causesFire,
        Entity* source,
        std::unique_ptr<DamageSource> damageSource,
        const loot::LootTableManager* lootTableManager,
        std::unique_ptr<ExplosionContext> context);

    /**
     * @brief 执行爆炸
     *
     * 执行完整的爆炸流程：
     * 1. 计算受影响的方块
     * 2. 计算受影响的实体（伤害和击退）
     * 3. 破坏方块并生成掉落物
     * 4. 应用击退
     * 5. 生成粒子和音效
     */
    void explode();

    // ========== 结果查询 ==========

    /**
     * @brief 获取受影响的方块位置列表
     * @return 受影响方块的坐标列表
     */
    [[nodiscard]] const std::vector<BlockPos>& affectedBlocks() const noexcept { return m_affectedBlocks; }

    /**
     * @brief 获取玩家击退映射
     * @return 玩家ID到击退向量的映射
     */
    [[nodiscard]] const std::unordered_map<u64, Vector3>& playerKnockback() const noexcept { return m_playerKnockback; }

    /**
     * @brief 获取爆炸半径
     */
    [[nodiscard]] f32 radius() const noexcept { return m_radius; }

    /**
     * @brief 获取爆炸中心位置
     */
    [[nodiscard]] const Vector3& position() const noexcept { return m_position; }

    /**
     * @brief 获取实体伤害/影响范围半径（= radius * 2）
     *
     * 既是实体搜索范围，也是伤害公式中距离比例的分母，与 vanilla 一致。
     */
    [[nodiscard]] f32 damageRadius() const noexcept { return m_radius * game::explosion::ENTITY_RANGE_MULTIPLIER; }

    /**
     * @brief 获取爆炸模式
     */
    [[nodiscard]] ExplosionMode mode() const noexcept { return m_mode; }

    /**
     * @brief 是否生成火焰
     */
    [[nodiscard]] bool causesFire() const noexcept { return m_causesFire; }

    /**
     * @brief 获取爆炸源实体
     */
    [[nodiscard]] Entity* source() const noexcept { return m_source; }

    /**
     * @brief 获取爆炸的间接源实体
     *
     * 追溯爆炸链中的间接源实体：
     * - 如果直接源是 LivingEntity，返回它本身
     * - 如果直接源是 ProjectileEntity，返回其发射者（如果是 LivingEntity）
     * - 如果直接源是 TNTEntity，返回其点燃者（如果是 LivingEntity）
     * - 其他情况返回 nullptr
     *
     * 用于连锁爆炸场景中正确归属伤害来源。
     *
     * @return 间接源实体，如果无法确定返回 nullptr
     */
    [[nodiscard]] LivingEntity* getIndirectSourceEntity() const;

private:
    // ========== 第一阶段：计算 ==========

    /**
     * @brief 计算受影响的方块
     *
     * 使用射线追踪算法：
     * - 从爆炸中心发射 16x16x16 立方体表面的射线（1352条）
     * - 每条射线步进 0.3 格
     * - 根据方块爆炸抗性消耗射线强度
     */
    void _calculateAffectedBlocks();

    /**
     * @brief 计算受影响的实体
     *
     * - 获取半径 * 2 范围内的所有实体
     * - 计算每个实体的伤害和击退
     */
    void _calculateAffectedEntities();

    // ========== 第二阶段：执行 ==========

    /**
     * @brief 破坏方块
     *
     * 根据 ExplosionMode：
     * - None: 不破坏方块
     * - Break: 破坏方块但不掉落
     * - Destroy: 破坏方块并掉落物品
     */
    void _destroyBlocks();

    /**
     * @brief 应用击退效果
     */
    void _applyKnockback();

    /**
     * @brief 生成粒子效果
     */
    void _spawnParticles();

    /**
     * @brief 播放爆炸音效
     */
    void _playSound();

    // ========== 辅助方法 ==========

    /**
     * @brief 计算方块密度（视线检测）
     *
     * 用于计算实体的遮挡系数。
     *
     * @param entityBox 实体碰撞箱
     * @return 阻挡密度 (0.0 - 1.0)，1.0 表示完全无遮挡
     */
    [[nodiscard]] f32 _getBlockDensity(const AxisAlignedBB& entityBox);

    /**
     * @brief 判断本次爆炸是否影响"方块类实体"（掉落物/盔甲架/悬挂实体/载具）
     *
     * mobGriefing 开启时恒为 true；否则取决于爆炸是否破坏方块（ExplosionMode 非 None）。
     * 简化前提：风弹/风爆附魔不走 Explosion 类，故 m_source 永非风弹源，等价 vanilla
     * shouldAffectBlocklikeEntities() 退化为 mobGriefing ? true : (mode != None)。
     */
    [[nodiscard]] bool _shouldAffectBlocklikeEntities() const;

    /**
     * @brief 获取指定位置的爆炸抗性
     *
     * @param pos 方块位置
     * @return 爆炸抗性值，如果为空气返回 std::nullopt
     */
    [[nodiscard]] std::optional<f32> _getExplosionResistance(const BlockPos& pos);

    /**
     * @brief 生成火焰
     *
     * 在被破坏的方块位置随机生成火焰（如果 causesFire 为 true）。
     */
    void _spawnFire();

    /**
     * @brief 生成方块掉落物
     *
     * 在 DESTROY 模式下为可掉落的方块生成 ItemEntity。
     *
     * @param pos 方块位置
     * @param state 方块状态
     * @return 生成的掉落物列表
     */
    [[nodiscard]] std::vector<ItemStack> _generateBlockDrops(const BlockPos& pos, const BlockState& state);

    /**
     * @brief 在世界中生成物品实体
     *
     * 将掉落物列表转换为 ItemEntity 并在世界中生成。
     *
     * @param pos 方块位置
     * @param drops 掉落物列表
     */
    void _spawnItemEntities(const BlockPos& pos, const std::vector<ItemStack>& drops);

private:
    IWorld& m_world;
    Vector3 m_position;
    f32 m_radius;
    ExplosionMode m_mode;
    bool m_causesFire;

    Entity* m_source;
    std::unique_ptr<DamageSource> m_damageSource;
    std::unique_ptr<ExplosionContext> m_context;
    const loot::LootTableManager* m_lootTableManager;

    std::vector<BlockPos> m_affectedBlocks;
    std::unordered_map<u64, Vector3> m_playerKnockback;
    math::Random m_random;
};

} // namespace world::explosion
} // namespace mc
