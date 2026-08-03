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

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/FallingBlock.hpp"

#include <array>
#include <cstddef>

namespace mc {

class BlockState;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 铁砧方块
 *
 * 铁砧是可下落的水平方向方块，落地时会对碰撞箱内的实体造成伤害，
 * 并有概率损坏（anvil → chipped_anvil → damaged_anvil → 消失）。
 *
 * 三个铁砧变体（anvil、chipped_anvil、damaged_anvil）均为 AnvilBlock 实例，
 * 通过不同的 Block 注册来区分。
 *
 * 参考: net.minecraft.world.level.block.AnvilBlock
 */
class AnvilBlock : public FallingBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit AnvilBlock(const BlockProperties& properties);

    ~AnvilBlock() override = default;

    // ========== FallingBlock 回调覆盖 ==========

    /**
     * @brief 开始下落时设置伤害参数
     *
     * 铁砧下落时设置 hurtEntities=true，每格伤害 2.0，最大伤害 40。
     */
    void onStartFalling(IWorld& world, const BlockPos& pos, entity::FallingBlockEntity& entity) override;

    /**
     * @brief 落地成功放置时播放音效
     *
     * 播放铁砧落地音效 (WorldEvents::ANVIL_LAND_SOUND = 1031)。
     */
    void onEndFalling(IWorld& world,
        const BlockPos& pos,
        const BlockState& fallingState,
        const BlockState& hitState,
        entity::FallingBlockEntity& entity) override;

    /**
     * @brief 破碎时播放音效
     *
     * 当铁砧无法放置时播放破坏音效 (WorldEvents::ANVIL_DESTROYED_SOUND = 1029)。
     */
    void onBroken(IWorld& world, const BlockPos& pos, entity::FallingBlockEntity& entity) override;

    // ========== 方块行为覆盖 ==========

    /**
     * @brief 获取放置时的方块状态
     *
     * 根据玩家水平朝向的顺时针旋转90度设置 HORIZONTAL_FACING，
     * 与 MC 原版一致：铁砧的正面朝向玩家右手边。
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 旋转方块状态
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 镜像方块状态
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    /**
     * @brief 获取方块的视觉形状
     *
     * 铁砧形状根据朝向轴（X轴或Z轴）返回不同的预计算形状。
     * MC 原版铁砧形状是 X/Z 不对称的：顶部沿朝向方向延伸至满16像素宽，
     * 垂直于朝向方向仅10像素宽。中段和窄颈也是 X/Z 不对称的。
     *
     * 形状由四个部分组成（以Z轴/北南朝向为例）：
     * - 底座：X/Z=2~14, Y=0~4 (12×12)
     * - 中段：X=4~12, Z=3~13, Y=4~5 (8×10)
     * - 窄颈：X=6~10, Z=4~12, Y=5~10 (4×8)
     * - 顶面：X=3~13, Z=0~16, Y=10~16 (10×16)
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取方块的碰撞形状
     *
     * 与视觉形状相同。
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 玩家右键交互时打开铁砧容器
     *
     * 在服务端打开铁砧修复界面（ContainerType::Anvil），并触发交互统计。
     * 铁砧不需要方块实体，容器通过世界位置直接访问。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    /**
     * @brief 铁砧不允许作为寻路路径
     *
     * 铁砧的材质虽然阻挡移动（默认 allowsMovement 返回 false），
     * 但这里显式覆盖以确保行为与 MC 原版一致。
     */
    [[nodiscard]] bool allowsMovement(const BlockState& state, IBlockReader& world, const BlockPos& pos) const override
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return false;
    }

    // ========== 静态工具方法 ==========

    /**
     * @brief 铁砧损坏状态转换
     *
     * anvil → chipped_anvil → damaged_anvil → nullptr（完全损坏）
     * 保留朝向属性。
     *
     * @param state 当前铁砧方块状态
     * @return 损坏后的方块状态指针，如果已完全损坏则返回 nullptr
     */
    [[nodiscard]] static const BlockState* damageAnvil(const BlockState& state);

private:
    /**
     * @brief 根据朝向轴获取形状索引
     *
     * North/South 映射到 Z 轴形状（index 0），East/West 映射到 X 轴形状（index 1）。
     */
    [[nodiscard]] static size_t _getAxisIndex(Direction facing);

    /// 预计算的形状，按轴索引：[0]=Z轴(North/South), [1]=X轴(East/West)
    std::array<CollisionShape, 2> m_shapesByAxis;
};

} // namespace blocks
} // namespace mc
