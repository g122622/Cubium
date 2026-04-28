#pragma once

#include "../../Block.hpp"
#include "../../../redstone/RedstonePower.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../core/BlockRaycastResult.hpp"
#include "../../../../item/core/ActionResult.hpp"
#include <vector>

namespace mc {
namespace blocks {

/**
 * @brief 红石线连接类型
 *
 * 描述红石线在某个方向的连接状态。
 */
enum class RedstoneSide : u8 {
    None = 0,   ///< 无连接
    Side = 1,   ///< 水平连接
    Up = 2      ///< 向上连接（连接到高一格的方块侧面）
};

} // namespace blocks

// 特化 EnumProperty::Traits for RedstoneSide
template<>
struct EnumProperty<blocks::RedstoneSide>::Traits {
    static String toString(const blocks::RedstoneSide& value);
    static std::optional<blocks::RedstoneSide> fromName(StringView name);
};

namespace blocks {

/**
 * @brief 红石线方块
 *
 * 红石线是红石系统的核心组件，负责信号传输和衰减。
 *
 * ## 核心机制
 * - 信号强度：0-15，每传输一格衰减1
 * - 连接状态：四个方向独立计算
 * - 十字形连接：信号向四个方向传输
 * - T形/L形连接：根据相邻方块动态调整
 *
 * ## 容易踩的坑
 * - 向上/向下连接需要特殊处理
 * - 信号传播顺序影响性能
 * - 更新时需要防止无限递归
 *
 * 参考: net.minecraft.block.RedstoneWireBlock
 */
class RedstoneWireBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit RedstoneWireBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state, Direction facing,
        const BlockState& facingState, IWorld& world,
        const BlockPos& currentPos, const BlockPos& facingPos) override;

    void neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                        const BlockPos& neighborPos, bool isMoving) override;

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const override {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getWeakPower(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Direction side
    ) const override;

    /**
     * @brief 获取强信号强度
     *
     * MC Java 中红石线也输出强信号（委托给 getWeakPower）。
     * 这是因为红石线可以直接充能相邻的实体方块。
     *
     * @param state 方块状态
     * @param world 世界引用
     * @param pos 方块位置
     * @param side 方向
     * @return i32 强信号强度
     */
    [[nodiscard]] i32 getStrongPower(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Direction side
    ) const override;

    // ========== 红石线特有方法 ==========

    /**
     * @brief 更新信号强度和连接状态
     */
    bool updatePower(IWorld& world, const BlockPos& pos);

    /**
     * @brief 计算连接状态
     */
    [[nodiscard]] BlockState calculateConnections(IWorld& world,
                                                   const BlockPos& pos,
                                                   const BlockState& state) const;

    /**
     * @brief 检查指定方向的连接类型
     */
    [[nodiscard]] RedstoneSide getConnection(IWorld& world,
                                              const BlockPos& pos,
                                              Direction direction) const;

    /**
     * @brief 判断方块是否可以连接红石
     */
    [[nodiscard]] static bool canConnectTo(const BlockState& state);

    /**
     * @brief 判断方块是否可以在指定方向连接红石
     *
     * MC Java: canConnectTo 方法重载
     * - 红石线总是可以连接
     * - 中继器/比较器只有输出端朝向该方向时才连接
     * - 观察者只有输出端朝向该方向时才连接
     * - 其他方块通过 canProvidePower 或 canConnectRedstone 判断
     *
     * @param state 方块状态
     * @param side 连接方向（从红石线指向相邻方块的方向）
     * @return true 如果可以连接
     */
    [[nodiscard]] static bool canConnectTo(const BlockState& state, Direction side);

    /**
     * @brief 检查方块是否是实体方块
     */
    [[nodiscard]] static bool isNormalCube(const BlockState& state);

    /**
     * @brief 获取当前信号强度
     */
    [[nodiscard]] static i32 getPower(const BlockState& state);

    /**
     * @brief 设置信号强度
     */
    [[nodiscard]] static BlockState withPower(BlockState state, i32 power);

    /**
     * @brief 获取红石线连接属性
     */
    [[nodiscard]] static const EnumProperty<RedstoneSide>& NORTH_PROP();
    [[nodiscard]] static const EnumProperty<RedstoneSide>& EAST_PROP();
    [[nodiscard]] static const EnumProperty<RedstoneSide>& SOUTH_PROP();
    [[nodiscard]] static const EnumProperty<RedstoneSide>& WEST_PROP();

    // ========== Block 接口实现 ==========

    /**
     * @brief 右键交互 - 切换十字/点状连接
     *
     * MC Java: 右键点击红石线可以在十字和点状连接之间切换。
     * 这个功能用于控制红石信号的传播方向。
     */
    [[nodiscard]] ActionResultType onBlockActivated(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

private:
    /**
     * @brief 计算输入信号强度
     */
    [[nodiscard]] i32 calculateInputPower(IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 获取相邻红石线的信号强度
     */
    [[nodiscard]] i32 getWirePower(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 通知相邻红石组件更新
     */
    void notifyWireNeighbors(IWorld& world, const BlockPos& pos);

    /// 临时变量：防止递归调用时检测自己的信号输出
    mutable bool m_canProvidePower = true;
};

} // namespace blocks
} // namespace mc
