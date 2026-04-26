#pragma once

#include "../../Block.hpp"
#include "../../Material.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/Direction.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 铁轨形状枚举
 *
 * 定义铁轨的连接形状。
 * 参考: net.minecraft.state.properties.RailShape
 */
enum class RailShape : u8 {
    NorthSouth = 0,     // 南北直轨
    EastWest = 1,       // 东西直轨
    AscendingEast = 2,  // 向东上升
    AscendingWest = 3,  // 向西上升
    AscendingNorth = 4, // 向北上升
    AscendingSouth = 5, // 向南上升
    SouthEast = 6,      // 东南弯轨
    SouthWest = 7,      // 西南弯轨
    NorthWest = 8,      // 西北弯轨
    NorthEast = 9       // 东北弯轨
};

/**
 * @brief 铁轨方块基类
 *
 * 铁轨是矿车行驶的基础：
 * - 自动连接到相邻铁轨
 * - 支持弯轨和斜轨
 * - 无碰撞箱（可以穿过）
 *
 * 参考: net.minecraft.block.AbstractRailBlock
 */
class AbstractRailBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param isPowered 是否为动力铁轨
     */
    AbstractRailBlock(const BlockProperties& properties, bool isPowered = false);

    // ========== 状态创建 ==========

    /**
     * @brief 获取放置状态
     */
    BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 更新 ==========

    /**
     * @brief 邻居更新
     */
    BlockState updatePostPlacement(
        const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 放置检测 ==========

    /**
     * @brief 检查是否可以放置
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const override;

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状（铁轨无碰撞）
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override {
        MC_UNUSED(state);
        static CollisionShape emptyShape = CollisionShape::empty();
        return emptyShape;
    }

    // ========== 红石 ==========

    /**
     * @brief 是否可以提供红石信号
     */
    [[nodiscard]] bool canProvidePower(const BlockState& state) const override {
        MC_UNUSED(state);
        return m_isPowered;
    }

    // ========== 属性访问 ==========

    /**
     * @brief 获取铁轨形状
     */
    [[nodiscard]] virtual RailShape getRailShape(const BlockState& state) const = 0;

    /**
     * @brief 设置铁轨形状
     */
    [[nodiscard]] virtual BlockState withRailShape(const BlockState& state, RailShape shape) const = 0;

    /**
     * @brief 是否为动力铁轨
     */
    [[nodiscard]] bool isPowered() const { return m_isPowered; }

    /**
     * @brief 检查状态是否有铁轨形状属性
     * 用于检测相邻方块是否为铁轨
     */
    [[nodiscard]] virtual bool hasRailShapeProperty(const BlockState& state) const = 0;

protected:
    /// 是否为动力铁轨
    bool m_isPowered;

    /// 各形状的碰撞箱
    std::array<CollisionShape, 10> m_shapes;

    /**
     * @brief 计算铁轨形状
     * @param world 世界
     * @param pos 位置
     * @param state 当前状态
     * @return 计算出的形状
     */
    [[nodiscard]] RailShape calculateRailShape(IWorld& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 检查相邻是否有铁轨
     */
    [[nodiscard]] bool isRailAt(IBlockReader& world, const BlockPos& pos) const;

    /**
     * @brief 检查相邻是否有可上升的铁轨
     */
    [[nodiscard]] bool canAscendTo(IBlockReader& world, const BlockPos& pos, Direction direction) const;
};

/**
 * @brief 铁轨形状属性
 */
class RailShapeProperty : public EnumProperty<RailShape> {
public:
    static std::unique_ptr<RailShapeProperty> create(const String& name);

private:
    RailShapeProperty(const String& name);
};

} // namespace blocks
} // namespace mc

// 枚举特征特化 - 必须在命名空间外
namespace mc {
template<>
struct EnumProperty<blocks::RailShape>::Traits {
    static String toString(const blocks::RailShape& value);
    static std::optional<blocks::RailShape> fromName(StringView name);
};
} // namespace mc
