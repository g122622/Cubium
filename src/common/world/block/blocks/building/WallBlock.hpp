#pragma once

#include "../../Block.hpp"
#include "../../IWaterLoggable.hpp"
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
 * @brief 墙方块
 *
 * 支持与相邻墙/栅栏连接，并自动调整高度。
 * 实现 IWaterLoggable 接口支持含水功能。
 *
 * 状态属性：
 * - UP: 是否有顶部突出
 * - NORTH/WEST/EAST/SOUTH: 各方向连接高度 (NONE, LOW, TALL)
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.block.WallBlock
 */
class WallBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit WallBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~WallBlock() override = default;

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

    /**
     * @brief 判断方块是否为墙
     * @param state 方块状态
     * @return 如果是墙返回 true
     */
    [[nodiscard]] static bool isWall(const BlockState& state);

    // ========== IWaterLoggable 接口实现 ==========

    /**
     * @brief 获取流体状态
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    /**
     * @brief 检查方块是否含水
     */
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    // ========== 红石连接 ==========

    [[nodiscard]] bool canConnectRedstone(const BlockState& state, Direction side) const override;

    // ========== 旋转和镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

private:
    /**
     * @brief 计算连接状态
     * @param world 世界
     * @param pos 位置
     * @param state 当前状态
     * @return 更新后的状态
     */
    [[nodiscard]] BlockState calculateState(const IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 检查邻居是否连接到墙
     * @param state 邻居状态
     * @param neighborSide 邻居相对于墙的方向
     * @return 连接高度
     */
    [[nodiscard]] BlockStateProperties::WallHeight getWallHeight(
        const BlockState& state,
        Direction neighborSide) const;

    /**
     * @brief 判断方块是否为栅栏门
     * @param state 方块状态
     * @return 如果是栅栏门返回 true
     */
    [[nodiscard]] static bool isFenceGate(const BlockState& state);

    /**
     * @brief 获取形状索引
     * @param up 是否有顶部
     * @param north 北面高度
     * @param east 东面高度
     * @param south 南面高度
     * @param west 西面高度
     * @return 形状索引
     */
    [[nodiscard]] static size_t getShapeIndex(
        bool up,
        BlockStateProperties::WallHeight north,
        BlockStateProperties::WallHeight east,
        BlockStateProperties::WallHeight south,
        BlockStateProperties::WallHeight west);

    /// 基础墙形状（无顶部）
    CollisionShape m_baseShape;

    /// 墙柱形状（中间）
    CollisionShape m_pillarShape;

    /// 预计算的形状缓存
    std::array<CollisionShape, 162> m_shapes;  // 2(up) * 3(north) * 3(east) * 3(south) * 3(west)
};

} // namespace blocks
} // namespace mc
