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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN AN EVENT SHALL THE
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
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/IWaterLoggable.hpp"
#include "common/world/block/Material.hpp"
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace mc {
namespace blocks {

/**
 * @brief 铁轨形状枚举
 *
 * 定义铁轨的连接形状。
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
 * @brief 检查铁轨形状是否为上升/斜坡形状
 * @param shape 铁轨形状
 * @return 是否为上升形状
 */
[[nodiscard]] constexpr bool isAscending(RailShape shape)
{
    return shape == RailShape::AscendingEast || shape == RailShape::AscendingWest ||
        shape == RailShape::AscendingNorth || shape == RailShape::AscendingSouth;
}

/**
 * @brief 铁轨方块基类
 *
 * 铁轨是矿车行驶的基础：
 * - 自动连接到相邻铁轨（包括上下Y层级）
 * - 支持弯轨和斜轨
 * - 通过 RailState 计算连接形状
 * - 普通铁轨支持三连接道岔（红石切换弯轨方向）
 * - 无碰撞箱（可以穿过）
 * - 实现 IWaterLoggable 接口，支持含水放置
 */
class AbstractRailBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param isStraight 是否为直线铁轨（动力铁轨/探测铁轨/激活铁轨不支持弯轨）
     * @param isPowered 是否为动力铁轨类型（影响canProvidePower返回值）
     */
    AbstractRailBlock(const BlockProperties& properties, bool isStraight = false, bool isPowered = false);

    // ========== 状态创建 ==========

    /**
     * @brief 获取放置状态
     */
    BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 更新 ==========

    /**
     * @brief 邻居更新
     */
    BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 方块放置后的处理
     *
     * 铁轨放置后立即重新计算连接形状。
     * getStateForPlacement 只根据玩家朝向返回初始形状，
     * 真正的邻居连接计算在此处触发。
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 邻居变化通知
     *
     * 检查铁轨是否仍有支撑，并在邻居变化时重新计算铁轨形状。
     */
    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    // ========== 放置检测 ==========

    /**
     * @brief 检查是否可以放置
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状（铁轨无碰撞）
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override
    {
        MC_UNUSED(state);
        static CollisionShape emptyShape = CollisionShape::empty();
        return emptyShape;
    }

    // ========== 红石 ==========

    /**
     * @brief 是否可以提供红石信号
     */
    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
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
     * @brief 是否为直线铁轨（不支持弯轨）
     *
     * 普通铁轨返回false（支持弯轨），动力铁轨/探测铁轨/激活铁轨返回true。
     */
    [[nodiscard]] bool isStraight() const noexcept { return m_isStraight; }

    /**
     * @brief 是否为动力铁轨类型（可提供红石信号）
     *
     * 动力铁轨和探测铁轨返回true，普通铁轨和激活铁轨返回false。
     * 注意：这不代表铁轨当前是否被充能，仅表示铁轨类型。
     */
    [[nodiscard]] bool isPoweredType() const noexcept { return m_isPowered; }

    /**
     * @brief 检查状态是否有铁轨形状属性
     * 用于检测相邻方块是否为铁轨
     */
    [[nodiscard]] virtual bool hasRailShapeProperty(const BlockState& state) const = 0;

    // ========== 铁轨形状更新 ==========

    /**
     * @brief 重新计算铁轨方向
     *
     * 创建 RailState 并调用 place() 来计算铁轨形状。
     * 这是铁轨形状计算的主要入口方法。
     *
     * @param world 世界
     * @param pos 铁轨位置
     * @param state 当前方块状态
     * @param updateBlock 是否更新世界中的方块状态
     * @return 更新后的方块状态
     */
    [[nodiscard]] BlockState updateDir(IWorld& world, const BlockPos& pos, const BlockState& state, bool updateBlock);

    /**
     * @brief 铁轨状态更新
     *
     * 在邻居变化时调用 updateDir 重新计算形状，然后根据需要传播更新。
     * 子类可以重写此方法添加额外行为（如 RailBlock 的三连接红石道岔）。
     *
     * @param world 世界
     * @param pos 铁轨位置
     * @param state 当前方块状态
     * @param neighborBlock 触发更新的邻居方块
     */
    virtual void updateState(IWorld& world, const BlockPos& pos, const BlockState& state, Block& neighborBlock);

    /**
     * @brief 检查铁轨是否应该被移除（斜坡支撑检测）
     *
     * 斜坡铁轨需要在其上升方向上方有支撑方块。
     *
     * @param state 铁轨状态
     * @param world 世界
     * @param pos 铁轨位置
     * @return 如果铁轨缺少支撑应被移除则返回true
     */
    [[nodiscard]] static bool shouldBeRemoved(const BlockState& state, IBlockReader& world, const BlockPos& pos);

    // ========== IWaterLoggable 接口实现 ==========

    /**
     * @brief 检查方块是否含水
     *
     * 读取 WATERLOGGED 属性值。
     *
     * @param state 方块状态
     * @return 是否含水
     */
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.hasProperty(BlockStateProperties::WATERLOGGED()) && state.get(BlockStateProperties::WATERLOGGED());
    }

    /**
     * @brief 获取流体状态
     *
     * 如果方块含水，返回水的流体状态；否则返回默认（空）流体状态。
     *
     * @param state 方块状态
     * @return 流体状态指针
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

protected:
    /// 是否为直线铁轨（不支持弯轨）
    bool m_isStraight;

    /// 是否为动力铁轨（可提供红石信号）
    bool m_isPowered;

    /// 各形状的碰撞箱
    std::array<CollisionShape, 10> m_shapes;
};

/**
 * @brief 铁轨形状属性
 */
class RailShapeProperty : public EnumProperty<RailShape> {
public:
    static std::unique_ptr<RailShapeProperty> create(const std::string& name);

    /**
     * @brief 创建仅含 6 个直线/斜坡形状的属性
     *
     * vanilla 动力/探测/激活铁轨的 shape 只有 6 个值（不含 4 个弯轨），
     * 普通铁轨才允许 10 个值。用此工厂构建矿车铁轨的状态容器以对齐 vanilla。
     */
    static std::unique_ptr<RailShapeProperty> createStraight(const std::string& name);

private:
    RailShapeProperty(const std::string& name, std::vector<RailShape> values);
    RailShapeProperty(const std::string& name);
};

} // namespace blocks
} // namespace mc

// 枚举特征特化 - 必须在命名空间外
namespace mc {
template <>
struct EnumProperty<blocks::RailShape>::Traits {
    static std::string toString(const blocks::RailShape& value);
    static std::optional<blocks::RailShape> fromName(std::string_view name);
};
} // namespace mc
