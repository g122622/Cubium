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

#include <unordered_map>

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../Material.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 花盆方块
 *
 * 花盆是一种装饰性方块，可容纳一种植物内容物：
 * - 空花盆：玩家右键点击可放入手持的可盆栽植物（花、树苗、蕨、蘑菇、仙人掌等）
 * - 已有内容物的花盆：玩家空手右键可取出内容物，花盆变回空花盆
 *
 * 每个具体的盆栽植物（如 potted_poppy、potted_oak_sapling）都是独立的 FlowerPotBlock 实例，
 * 通过构造时传入不同的"内容物方块"来区分。空花盆的内容物为 nullptr。
 *
 * 参考: net.minecraft.world.level.block.FlowerPotBlock
 *
 * 与 MC Java 1.21.11 的差异：
 * - MC 中空花盆的 potted 字段为 Blocks.AIR；本项目使用 nullptr 表示空花盆
 * - MC 中 canSurvive 默认返回 true，花盆可以在任何位置存在（包括悬空）；
 *   本项目 isValidPosition 同样默认返回 true，行为一致
 * - MC 中 updateShape 检查 DOWN && !canSurvive，由于 canSurvive 始终为 true，
 *   花盆不会因下方方块移除而自动掉落；本项目 updatePostPlacement 保持同样行为
 *
 * 眼眸花（potted_open_eyeblossom / potted_closed_eyeblossom）特殊逻辑：
 * - 响应随机刻，根据 EnvironmentAttributes.EYEBLOSSOM_OPEN 环境属性在开/合状态间切换，
 *   生成 TrailParticle 转换粒子并播放长切换音效
 * - 本项目 EnvironmentAttributes 系统尚未完整实现，使用 EyeblossomEnvironment
 *   工具函数近似查询 EYEBLOSSOM_OPEN（详见 EyeblossomEnvironment.hpp）
 * - 与地栽眼眸花不同，花盆版不连锁触发周围 3×2×3 范围内的同种方块
 */
class FlowerPotBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param potted 内容物方块指针；空花盆传 nullptr
     */
    FlowerPotBlock(const BlockProperties& properties, const Block* potted = nullptr);

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状（花盆没有完整碰撞）
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 放置检测 ==========

    /**
     * @brief 检查是否可以放置
     *
     * 匹配 MC Java 1.21.11: FlowerPotBlock 不重写 canSurvive，
     * 默认返回 true，花盆可以放置在任何位置（包括悬空）。
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 邻居更新
     *
     * 匹配 MC Java 1.21.11: updateShape 检查 DOWN && !canSurvive。
     * 由于 canSurvive 默认返回 true，花盆不会因下方方块变化而破坏。
     */
    BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 交互 ==========

    /**
     * @brief 玩家右键点击
     *
     * - 空花盆 + 玩家手持可盆栽植物的 BlockItem → 放入植物，花盆变为对应 potted_* 方块
     * - 已有内容物的花盆 + 空手 → 取出内容物，花盆变回空花盆，物品掉落或入背包
     * - 已有内容物的花盆 + 玩家手持可盆栽植物 → 消费物品但不执行动作（与 MC Java 一致）
     * - 空花盆 + 空手 → 消费动作（无操作）
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 中键选取 ==========

    /**
     * @brief 获取中键选取物品
     *
     * 已盆栽的花盆返回内容物对应的物品（匹配 MC Java getCloneItemStack）；
     * 空花盆返回默认（flower_pot 物品）。
     */
    [[nodiscard]] ItemStack getCloneItemStack(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

    // ========== 随机刻（眼眸花特殊逻辑） ==========

    /**
     * @brief 是否响应随机刻
     *
     * 仅 potted_open_eyeblossom / potted_closed_eyeblossom 返回 true。
     * 匹配 MC Java 1.21.11: isRandomlyTicking
     */
    [[nodiscard]] bool ticksRandomly() const noexcept override;

    /**
     * @brief 随机刻处理
     *
     * 眼眸花盆栽根据 EnvironmentAttributes.EYEBLOSSOM_OPEN 环境属性在开/合状态间切换。
     * 切换时：
     * 1. 替换为对应的眼眸花盆栽方块（potted_open_eyeblossom <-> potted_closed_eyeblossom）
     * 2. 生成 TrailParticle 转换粒子（复用 EyeblossomBlock::spawnTransformParticle）
     * 3. 播放 longSwitchSound 长切换音效
     *
     * 与地栽眼眸花不同，花盆版不连锁触发周围 3×2×3 范围内的同种方块。
     *
     * 参考: net.minecraft.world.level.block.FlowerPotBlock#randomTick
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    // ========== 内容物 ==========

    /**
     * @brief 获取内容物方块指针（空花盆返回 nullptr）
     */
    [[nodiscard]] const Block* getPotted() const { return m_potted; }

    /**
     * @brief 是否为空花盆
     */
    [[nodiscard]] bool isEmpty() const { return m_potted == nullptr; }

    // ========== 静态映射表 ==========

    /**
     * @brief 根据内容物方块查找对应的花盆方块
     *
     * 用于交互时反查"哪个 FlowerPotBlock 对应这个内容物方块"。
     * 若该方块不是任何花盆的内容物，返回 nullptr。
     *
     * @param content 内容物方块
     * @return 对应的 FlowerPotBlock，找不到返回 nullptr
     */
    [[nodiscard]] static const FlowerPotBlock* getByContent(const Block& content);

private:
    /// 内容物方块指针（空花盆为 nullptr）
    const Block* m_potted;
    /// 花盆形状
    CollisionShape m_shape;
    /// 碰撞形状
    CollisionShape m_collisionShape;

    /// 内容物方块 -> 花盆方块 的反查映射表
    /// 在每个 FlowerPotBlock 构造时填充
    /// 注意：空花盆不参与此表（nullptr 不能作为 unordered_map 键）
    static std::unordered_map<const Block*, const FlowerPotBlock*> s_pottedByContent;
};

} // namespace blocks
} // namespace mc
