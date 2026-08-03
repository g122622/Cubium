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
 * LIABILITY, CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"
#include <memory>

namespace mc {

namespace fluid {
class Fluid;
} // namespace fluid

class World;
class BlockItemUseContext;
class Player;
class BlockRaycastResult;
class ItemStack;

namespace blocks {

/**
 * @brief 空炼药锅方块
 *
 * 空炼药锅方块，不持有水位。
 * 当接收到水时（降水、水桶、水瓶、滴石滴水等），替换为 WaterCauldronBlock (LayeredCauldronBlock)。
 * 当接收到岩浆时（岩浆桶、滴石岩浆滴），替换为 LavaCauldronBlock。
 *
 * 状态属性：无（空炼药锅没有水位）
 *
 * 交互（空炼药锅）：
 * - 水桶 → 替换为 WaterCauldronBlock（水位3）
 * - 岩浆桶 → 替换为 LavaCauldronBlock
 * - 细雪桶 → 替换为 PowderSnowCauldronBlock（水位3）
 * - 水瓶（水瓶药水）→ 替换为 WaterCauldronBlock（水位1）
 * - 空桶 / 玻璃瓶 / 其他 → Pass（空炼药锅无法取水/清洗）
 *
 * 降水处理：
 * - 雨天：5% 概率替换为 WaterCauldronBlock（水位1）
 * - 雪天：10% 概率替换为 PowderSnowCauldronBlock（水位1）
 *
 * 滴石填充：
 * - 水滴 → 替换为 WaterCauldronBlock（水位1）
 * - 岩浆滴 → 替换为 LavaCauldronBlock
 */
class CauldronBlock : public Block {
public:
    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit CauldronBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~CauldronBlock() override = default;

    // ========== 放置和更新 ==========

    /**
     * @brief 邻居方块更新（无操作）
     */
    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    /**
     * @brief 降水处理
     *
     * 雨天：5% 概率替换为 WaterCauldronBlock（水位1）
     * 雪天：10% 概率替换为 PowderSnowCauldronBlock（水位1）
     */
    void handlePrecipitation(
        IWorld& world, const BlockPos& pos, world::biome::BiomeClimate::Precipitation precipitation) override;

    // ========== 交互 ==========

    /**
     * @brief 玩家右键点击
     *
     * 空炼药锅的交互仅限于往空锅中添加内容：
     * - 水桶 → 替换为 WaterCauldronBlock（水位3）
     * - 岩浆桶 → 替换为 LavaCauldronBlock
     * - 水瓶 → 替换为 WaterCauldronBlock（水位1）
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 形状 ==========

    /**
     * @brief 获取渲染形状（炼药锅外部形状）
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状（与渲染形状相同）
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 获取实体内部碰撞形状
     *
     * 空炼药锅返回完整方块形状。
     */
    [[nodiscard]] const CollisionShape& getEntityInsideCollisionShape(const BlockState& state) const override;

    /**
     * @brief 空炼药锅需要形状遮挡检测
     *
     * 炼药锅内部为空心结构，需要使用形状进行光照遮挡计算，
     * 否则光线会穿过炼药锅壁导致不正确的光照效果。
     */
    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    // ========== 静态工具方法 ==========

    /**
     * @brief 空炼药锅始终为空
     */
    [[nodiscard]] static bool isEmpty(const BlockState& state)
    {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 空炼药锅永远不满
     */
    [[nodiscard]] static bool isFull(const BlockState& state)
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 滴石填充 ==========

    /**
     * @brief 方块 tick 处理（滴石滴水）
     *
     * 重新验证上方钟乳石尖端，检查流体类型并执行填充。
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 判断是否可以接收滴石滴水
     *
     * 空炼药锅可以接收任何流体（水和岩浆）的滴石滴水。
     *
     * @param fluid 流体类型
     * @return 始终返回 true
     */
    [[nodiscard]] static bool canReceiveStalactiteDrip(const fluid::Fluid& fluid);

    /**
     * @brief 接收滴石滴水填充
     *
     * 根据流体类型处理炼药锅填充：
     * - 水滴 → 替换为 WaterCauldronBlock（水位1）
     * - 岩浆滴 → 替换为 LavaCauldronBlock
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前方块状态
     * @param fluid 流体类型
     */
    static void receiveStalactiteDrip(
        IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::Fluid& fluid);

private:
    /**
     * @brief 处理水桶交互（空炼药锅：装水/装岩浆）
     */
    ActionResultType _handleBucketInteraction(
        IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem);

    /**
     * @brief 处理水瓶交互（空炼药锅：水瓶倒入）
     */
    ActionResultType _handleBottleInteraction(
        IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem);

    /// 炼药锅外部形状（与 LayeredCauldronBlock/LavaCauldronBlock 共享相同的几何形状）
    CollisionShape m_outerShape;
};

} // namespace blocks
} // namespace mc
