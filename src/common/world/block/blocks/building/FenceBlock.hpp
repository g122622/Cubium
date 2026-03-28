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
 * @brief 栅栏方块
 *
 * 支持与相邻栅栏/墙连接。
 *
 * 状态属性：
 * - NORTH/WEST/EAST/SOUTH: 各方向是否连接
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.block.FenceBlock
 */
class FenceBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit FenceBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~FenceBlock() override = default;

    // ========== 放置和更新 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getOcclusionShape(const BlockState& state) const override;

    // ========== 旋转和镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

private:
    /**
     * @brief 检查是否可以连接到邻居
     * @param state 邻居状态
     * @param isNeighborSolid 邻居是否为固体
     * @return 如果可以连接返回true
     */
    [[nodiscard]] bool canConnect(const BlockState& state, bool isNeighborSolid) const;

    /**
     * @brief 获取形状索引
     * @param north 北面是否连接
     * @param east 东面是否连接
     * @param south 南面是否连接
     * @param west 西面是否连接
     * @return 形状索引 (0-15)
     */
    [[nodiscard]] static size_t getShapeIndex(bool north, bool east, bool south, bool west);

    /// 预计算的形状缓存（16种组合）
    std::array<CollisionShape, 16> m_shapes;
};

} // namespace blocks
} // namespace mc
