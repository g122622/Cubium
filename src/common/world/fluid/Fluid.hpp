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

#include "../../core/Types.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "../../util/math/Vector3.hpp"
#include "../../util/property/StateContainer.hpp"
#include <memory>

namespace mc {

// 前向声明
class BlockState;
class BlockPos;
class CollisionShape;
class IWorld;
class IBlockReader;

namespace math {
class IRandom;
}

namespace fluid {

// ============================================================================
// 流体常量
// 参考: net.minecraft.world.level.material.FluidState (AMOUNT_FULL, AMOUNT_MAX)
// ============================================================================

/// 流体源头等级 (LEVEL_1_8 属性的最大值)
/// 源头方块的 level 为此值，流动方块的 level 为 1~(SOURCE_LEVEL-1)
/// 参考: FluidState.AMOUNT_FULL = 8
constexpr i32 SOURCE_LEVEL = 8;

/// 流体最大数量 (用于高度计算)
/// getOwnHeight() = getLevel() / (AMOUNT_MAX + 0.0F)
/// 当上方有同种流体时，level 可视为 9 以表示"视觉上填满"
/// 参考: FluidState.AMOUNT_MAX = 9
constexpr i32 MAX_AMOUNT = 9;

class Fluid;
class FluidState;

/**
 * @brief 流体状态
 *
 * 不可变的流体状态对象，包含流体的所有属性值。
 * 继承自StateHolder以支持O(1)的状态转换。
 *
 * 参考: net.minecraft.fluid.FluidState
 *
 * 用法示例:
 * @code
 * const FluidState& state = water.getDefaultState();
 * bool isSource = state.isSource();     // true
 * i32 level = state.getLevel();         // SOURCE_LEVEL (=8)
 * f32 height = state.getHeight();       // SOURCE_LEVEL/MAX_AMOUNT (~0.889)
 * @endcode
 */
class FluidState : public StateHolder<Fluid, FluidState> {
public:
    /**
     * @brief 默认构造函数（用于容器）
     *
     * 创建一个空的流体状态。主要用于 STL 容器支持。
     */
    FluidState()
        : StateHolder<Fluid, FluidState>(nullptr, {}, nullptr, nullptr, 0)
        , m_fluidId(0)
    {}

    /**
     * @brief 构造流体状态
     */
    FluidState(const Fluid& fluid,
        std::vector<size_t> valueIndices,
        const std::vector<StateHolder<Fluid, FluidState>::PropertyLayout>* propertyLayouts,
        const std::vector<FluidState*>* allStates,
        u32 stateId);

    // ========== 流体属性 ==========

    /**
     * @brief 是否为源头方块
     * @return true表示源头，false表示流动
     */
    [[nodiscard]] bool isSource() const;

    /**
     * @brief 获取流体等级 (1-SOURCE_LEVEL)
     *
     * - SOURCE_LEVEL (=8) = 源头
     * - 1-(SOURCE_LEVEL-1) = 流动，数值越大越接近源头
     *
     * @return 流体等级
     */
    [[nodiscard]] i32 getLevel() const;

    /**
     * @brief 是否正在下落
     *
     * 当流体从上方落下时为true。
     *
     * @return 是否下落
     */
    [[nodiscard]] bool isFalling() const;

    /**
     * @brief 获取流体高度（用于渲染）
     *
     * 计算公式: level / MAX_AMOUNT
     * 源头高度约为0.888... (8/9)
     *
     * @return 高度 (0.0-1.0)
     */
    [[nodiscard]] f32 getHeight() const;

    /**
     * @brief 获取实际高度（考虑下方流体）
     *
     * @param world 世界
     * @param pos 位置
     * @return 实际高度
     */
    [[nodiscard]] f32 getActualHeight(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 是否为空流体
     * @return true表示无流体
     */
    [[nodiscard]] bool isEmpty() const;

    // ========== 流体关联 ==========

    /**
     * @brief 获取流体实例
     * @return 流体引用
     */
    [[nodiscard]] const Fluid& getFluid() const;

    /**
     * @brief 获取对应的方块状态
     * @return 方块状态指针
     */
    [[nodiscard]] const BlockState* getBlockState() const;

    /**
     * @brief 获取流体ID
     * @return 流体ID
     */
    [[nodiscard]] u32 fluidId() const noexcept { return m_fluidId; }

    // ========== 流动方向 ==========

    /**
     * @brief 计算流动方向向量
     *
     * @param world 世界
     * @param pos 位置
     * @return 流动方向向量（归一化）
     */
    [[nodiscard]] Vector3 getFlow(IBlockReader& world, const BlockPos& pos) const;

    // ========== 碰撞 ==========

    /**
     * @brief 检查是否可以被指定流体替换
     *
     * @param world 世界
     * @param pos 位置
     * @param fluid 替换流体
     * @param dir 流入方向
     * @return 是否可替换
     */
    [[nodiscard]] bool canDisplace(IWorld& world, const BlockPos& pos, const Fluid& fluid, Direction dir) const;

protected:
    [[nodiscard]] std::string ownerName() const override;

private:
    friend class Fluid;
    friend class FluidRegistry;

    u32 m_fluidId = 0;
    mutable const BlockState* m_cachedBlockState = nullptr;
};

/**
 * @brief 流体基类
 *
 * 所有流体类型的基类。流体通过FluidRegistry注册，
 * 每个流体有一个或多个FluidState表示不同状态。
 *
 * 参考: net.minecraft.fluid.Fluid
 *
 * 用法示例:
 * @code
 * class WaterFluid : public FlowingFluid {
 * public:
 *     i32 getTickDelay() const override { return 5; }
 *     bool canSourcesMultiply() const override { return true; }
 *     // ...
 * };
 * @endcode
 */
class Fluid {
public:
    virtual ~Fluid() = default;

    // ========== 静态访问器 ==========

    /**
     * @brief 根据流体ID获取流体
     */
    [[nodiscard]] static Fluid* getFluid(u32 fluidId);

    /**
     * @brief 根据资源位置获取流体
     */
    [[nodiscard]] static Fluid* getFluid(const ResourceLocation& id);

    // ========== 流体属性 ==========

    /**
     * @brief 获取流体资源位置
     */
    [[nodiscard]] const ResourceLocation& fluidLocation() const noexcept { return m_fluidLocation; }

    /**
     * @brief 获取流体ID
     */
    [[nodiscard]] u32 fluidId() const noexcept { return m_fluidId; }

    /**
     * @brief 获取状态容器
     */
    [[nodiscard]] const StateContainer<Fluid, FluidState>& stateContainer() const noexcept { return *m_stateContainer; }

    /**
     * @brief 获取默认状态
     */
    [[nodiscard]] const FluidState& defaultState() const noexcept { return *m_defaultState; }

    // ========== 虚方法 ==========

    /**
     * @brief 是否为源头
     * @param state 流体状态
     */
    [[nodiscard]] virtual bool isSource(const FluidState& state) const = 0;

    /**
     * @brief 获取流体等级
     * @param state 流体状态
     */
    [[nodiscard]] virtual i32 getLevel(const FluidState& state) const = 0;

    /**
     * @brief 获取tick延迟（游戏刻）
     *
     * 水为5tick，岩浆在主世界为30tick，下界为10tick。
     */
    [[nodiscard]] virtual i32 getTickDelay() const = 0;

    /**
     * @brief 获取依赖世界的tick延迟（游戏刻）
     *
     * 默认返回无上下文版本；岩浆会根据维度覆盖。
     */
    [[nodiscard]] virtual i32 getTickDelay(IWorld& world) const
    {
        (void)world;
        return getTickDelay();
    }

    /**
     * @brief 是否可以形成无限源
     *
     * 水返回true（2个相邻源头可形成新源头），
     * 岩浆返回false。
     */
    [[nodiscard]] virtual bool canSourcesMultiply() const = 0;

    /**
     * @brief 获取对应的方块状态
     * @param state 流体状态
     */
    [[nodiscard]] virtual const BlockState* getBlockState(const FluidState& state) const = 0;

    /**
     * @brief 获取爆炸抗性
     */
    [[nodiscard]] virtual f32 getExplosionResistance() const = 0;

    /**
     * @brief 获取流动方向向量
     *
     * @param world 世界
     * @param pos 位置
     * @param state 流体状态
     * @return 流动方向（归一化）
     */
    [[nodiscard]] virtual Vector3 getFlow(IBlockReader& world, const BlockPos& pos, const FluidState& state) const;

    /**
     * @brief 执行tick
     *
     * @param world 世界
     * @param pos 位置
     * @param state 流体状态
     */
    virtual void tick(IWorld& world, const BlockPos& pos, FluidState& state);

    /**
     * @brief 随机tick
     *
     * @param world 世界
     * @param pos 位置
     * @param state 流体状态
     * @param random 随机数生成器
     */
    virtual void randomTick(IWorld& world, const BlockPos& pos, const FluidState& state, math::IRandom& random);

    /**
     * @brief 是否执行随机tick
     */
    [[nodiscard]] virtual bool ticksRandomly() const { return false; }

    /**
     * @brief 是否等效于指定流体
     *
     * @param other 其他流体
     * @return 是否等效
     */
    [[nodiscard]] virtual bool isEquivalentTo(const Fluid& other) const noexcept { return this == &other; }

    /**
     * @brief 检查流体是否在指定标签中
     *
     * @param tag 流体标签
     * @return 是否在标签中
     */
    [[nodiscard]] bool isIn(const class FluidTag& tag) const;

    /**
     * @brief 是否为空流体
     *
     * EmptyFluid重写返回true，其他流体返回false。
     */
    [[nodiscard]] virtual bool isEmpty() const { return false; }

    /**
     * @brief 检查是否可以被指定流体替换
     *
     * @param state 当前流体状态
     * @param world 世界
     * @param pos 位置
     * @param fluid 替换流体
     * @param dir 流入方向
     * @return 是否可替换
     */
    [[nodiscard]] virtual bool canDisplace(
        const FluidState& state, IWorld& world, const BlockPos& pos, const Fluid& fluid, Direction dir) const;

    /**
     * @brief 获取碰撞形状
     *
     * @param state 流体状态
     * @param world 世界
     * @param pos 位置
     * @return 碰撞形状
     */
    [[nodiscard]] virtual CollisionShape getShape(
        const FluidState& state, IBlockReader& world, const BlockPos& pos) const;

    /**
     * @brief 转换为字符串
     */
    [[nodiscard]] virtual std::string toString() const { return m_fluidLocation.toString(); }

protected:
    friend class FluidRegistry;
    friend class FluidState;

    /**
     * @brief 构造流体
     */
    Fluid() = default;

    /**
     * @brief 创建流体状态容器
     */
    void createFluidState(std::unique_ptr<StateContainer<Fluid, FluidState>> container);

    /**
     * @brief 设置默认状态
     */
    void setDefaultState(const FluidState& state);

    // 由FluidRegistry设置
    ResourceLocation m_fluidLocation;
    u32 m_fluidId = 0;

    // 由createFluidState设置
    std::unique_ptr<StateContainer<Fluid, FluidState>> m_stateContainer;
    const FluidState* m_defaultState = nullptr;
};

} // namespace fluid
} // namespace mc
