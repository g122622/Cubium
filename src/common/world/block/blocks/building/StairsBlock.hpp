#pragma once

#include "../../Block.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"
#include <array>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 楼梯方块
 *
 * 支持内角/外角自动检测和连接。
 *
 * 状态属性：
 * - FACING: 楼梯朝向 (NORTH, SOUTH, EAST, WEST) - 楼梯上升的方向
 * - HALF: 上半/下半 (TOP, BOTTOM) - 楼梯是正放还是倒放
 * - SHAPE: 楼梯形状 (STRAIGHT, INNER_LEFT, INNER_RIGHT, OUTER_LEFT, OUTER_RIGHT)
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.block.StairsBlock
 */
class StairsBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param baseState 源方块状态（用于继承属性）
     * @param properties 方块属性
     */
    StairsBlock(const BlockState& baseState, const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~StairsBlock() override = default;

    // ========== 状态容器 ==========

    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

    // ========== 放置和更新 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos
    ) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getOcclusionShape(const BlockState& state) const override;

    // ========== 旋转和镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 其他 ==========

    /**
     * @brief 检查是否为楼梯
     * @param state 方块状态
     * @return 如果是楼梯返回true
     */
    [[nodiscard]] static bool isStairs(const BlockState& state);

private:
    /**
     * @brief 计算楼梯形状
     * @param state 当前状态
     * @param world 世界
     * @param pos 位置
     * @return 楼梯形状
     */
    [[nodiscard]] BlockStateProperties::StairsShape calculateShape(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos) const;

    /**
     * @brief 检查邻居是否为楼梯
     * @param world 世界
     * @param pos 位置
     * @param facing 检查方向
     * @return 如果是楼梯返回其形状，否则返回nullopt
     */
    [[nodiscard]] std::optional<BlockStateProperties::StairsShape> neighborIsStairs(
        IWorld& world,
        const BlockPos& pos,
        Direction facing) const;

    /**
     * @brief 获取形状索引
     * @param facing 朝向
     * @param half 上下半
     * @param shape 形状
     * @return 形状索引 (0-39)
     */
    [[nodiscard]] static size_t getShapeIndex(
        Direction facing,
        BlockStateProperties::DoubleBlockHalf half,
        BlockStateProperties::StairsShape shape);

    /// 源方块状态（用于继承属性如硬度、抗性等）
    const BlockState* m_baseState;

    /// 预计算的形状缓存 (4 facing * 2 half * 5 shape = 40种)
    std::array<CollisionShape, 40> m_shapes;

    /// 完整方块形状（用于双层台阶）
    CollisionShape m_fullCubeShape;
};

} // namespace blocks
} // namespace mc
