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

namespace mc {
namespace blocks {

/**
 * @brief 花盆方块
 *
 * 花盆是一种装饰性方块，可容纳一种植物内容物：
 * - 空花盆：玩家右键点击可放入手持的可盆栽植物（花、树苗、蕨、蘑菇、仙人掌等）
 * - 已有内容物的花盆：玩家空手右键可取出内容物，花盆变回空花盆
 * - 下方方块被移除时自动掉落
 *
 * 每个具体的盆栽植物（如 potted_poppy、potted_oak_sapling）都是独立的 FlowerPotBlock 实例，
 * 通过构造时传入不同的"内容物方块"来区分。空花盆的内容物为 nullptr。
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
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 邻居更新
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
     */
    [[nodiscard]] ActionResultType onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

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
