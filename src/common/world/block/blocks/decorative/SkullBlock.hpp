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
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/Material.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 头颅种类
 *
 * 对应 MC Java 1.21.11 SkullBlock.Types，决定音符盒乐器与放置时的默认方块类型。
 */
enum class SkullType : u8 {
    Skeleton = 0,       ///< 骷髅头颅
    WitherSkeleton = 1, ///< 凋灵骷髅头颅
    Player = 2,         ///< 玩家头颅
    Zombie = 3,         ///< 僵尸头
    Creeper = 4,        ///< 苦力怕头
    Piglin = 5,         ///< 猪灵头
    Dragon = 6          ///< 龙首
};

/**
 * @brief 头颅方块抽象基类
 *
 * 持有 POWERED 属性（被红石信号激活时龙首/猪灵头做张嘴动画，由方块实体 tick 驱动）。
 * 子类：SkullBlock（站立）、WallSkullBlock（贴墙）。
 *
 * 参考: net.minecraft.world.level.block.AbstractSkullBlock
 */
class AbstractSkullBlock : public Block {
public:
    /**
     * @brief 构造头颅方块
     * @param type 头颅种类
     * @param properties 方块属性
     */
    AbstractSkullBlock(SkullType type, const BlockProperties& properties);

    ~AbstractSkullBlock() override = default;

    /**
     * @brief 获取头颅种类
     */
    [[nodiscard]] SkullType getSkullType() const noexcept { return m_type; }

    /**
     * @brief 头颅方块持有方块实体（存储玩家档案/自定义名称/音符盒音色）
     */
    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 邻居方块更新：红石信号变化时翻转 POWERED
     */
    void neighborChanged(IWorld& world,
        const BlockPos& pos,
        Block& neighborBlock,
        const BlockPos& neighborPos,
        bool isMoving) override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

protected:
    SkullType m_type;
};

/**
 * @brief 站立头颅方块
 *
 * 放置在地面上的头颅，拥有 16 向旋转属性（ROTATION_0_15）。
 *
 * 参考: net.minecraft.world.level.block.SkullBlock
 */
class SkullBlock : public AbstractSkullBlock {
public:
    SkullBlock(SkullType type, const BlockProperties& properties);

    ~SkullBlock() override = default;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

private:
    CollisionShape m_shape;
    CollisionShape m_piglinShape;
};

/**
 * @brief 贴墙头颅方块
 *
 * 附着在墙面的头颅，拥有水平朝向属性（HORIZONTAL_FACING）。
 *
 * 参考: net.minecraft.world.level.block.WallSkullBlock
 */
class WallSkullBlock : public AbstractSkullBlock {
public:
    WallSkullBlock(SkullType type, const BlockProperties& properties);

    ~WallSkullBlock() override = default;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    [[nodiscard]] bool isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

protected:
    /// 获取指定朝向的碰撞形状
    [[nodiscard]] const CollisionShape& _shapeForFacing(Direction facing) const;

    CollisionShape m_shapes[4]; // North, South, West, East
};

/**
 * @brief 玩家头颅方块
 *
 * 站立玩家头颅，放置时从物品的 SkullOwner 组件读取档案写入方块实体。
 *
 * 参考: net.minecraft.world.level.block.PlayerHeadBlock
 */
class PlayerHeadBlock : public SkullBlock {
public:
    explicit PlayerHeadBlock(const BlockProperties& properties);

    void onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack) override;
};

/**
 * @brief 贴墙玩家头颅方块
 */
class PlayerWallHeadBlock : public WallSkullBlock {
public:
    explicit PlayerWallHeadBlock(const BlockProperties& properties);

    void onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack) override;
};

/**
 * @brief 凋灵骷髅头颅方块
 *
 * 放置时检测凋灵生成结构（4 个灵魂沙/灵魂土 T 形 + 3 个凋灵骷髅头颅）。
 *
 * 参考: net.minecraft.world.level.block.WitherSkullBlock
 */
class WitherSkullBlock : public SkullBlock {
public:
    explicit WitherSkullBlock(const BlockProperties& properties);

    void onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack) override;

    /**
     * @brief 检测并生成凋灵
     *
     * 对应 MC Java 1.21.11 WitherSkullBlock.checkSpawn(Level, BlockPos)。
     */
    static void checkSpawn(IWorld& world, const BlockPos& pos);
};

/**
 * @brief 贴墙凋灵骷髅头颅方块
 */
class WitherWallSkullBlock : public WallSkullBlock {
public:
    explicit WitherWallSkullBlock(const BlockProperties& properties);

    void onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack) override;
};

/**
 * @brief 贴墙猪灵头方块
 *
 * 猪灵头的贴墙碰撞箱略大（10 像素宽），单独重写形状。
 *
 * 参考: net.minecraft.world.level.block.PiglinWallSkullBlock
 */
class PiglinWallSkullBlock : public WallSkullBlock {
public:
    explicit PiglinWallSkullBlock(const BlockProperties& properties);

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

private:
    CollisionShape m_piglinShapes[4]; // North, South, West, East
};

} // namespace blocks
} // namespace mc
