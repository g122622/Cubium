#pragma once

#include "../FlowingFluid.hpp"
#include "../../../util/math/Vector3.hpp"

namespace mc {
namespace fluid {

/**
 * @brief 水流体基类
 *
 * 参考MC 1.16.5 WaterFluid
 *
 * 特性：
 * - tick延迟: 5 tick
 * - 每格衰减: 1级
 * - 斜坡搜索距离: 4格
 * - 可以形成无限源（2+相邻源头）
 */
class WaterFluid : public FlowingFluid {
public:
    // ========== FlowingFluid接口实现 ==========

    using FlowingFluid::getTickDelay;

    [[nodiscard]] i32 getTickDelay() const override {
        return 5;
    }

    [[nodiscard]] bool canDisplace(const FluidState& state, IWorld& world,
                                   const BlockPos& pos, const Fluid& fluid,
                                   Direction dir) const override;

    [[nodiscard]] i32 getLevelDecrease(IWorld& world) const override {
        (void)world;
        return 1;
    }

    [[nodiscard]] i32 getSpreadDistance(IWorld& world) const override {
        (void)world;
        return 4;
    }

    [[nodiscard]] bool canSourcesMultiply() const override { return true; }

    [[nodiscard]] f32 getExplosionResistance() const override { return 100.0f; }

    [[nodiscard]] const BlockState* getBlockState(const FluidState& state) const override;

    /**
     * @brief 检查是否等效于指定流体
     *
     * 水和流动水视为等效。
     *
     * @param other 其他流体
     * @return 是否等效
     */
    [[nodiscard]] bool isEquivalentTo(const Fluid& other) const override;

protected:
    void beforeReplacingBlock(IWorld& world, const BlockPos& pos,
                              const BlockState* state) override;
};

/**
 * @brief 水源头
 *
 * 源头水方块，level=8，没有LEVEL属性。
 */
class WaterSourceFluid : public WaterFluid {
public:
    WaterSourceFluid();

    [[nodiscard]] bool isSource(const FluidState& state) const override {
        (void)state;
        return true;
    }

    [[nodiscard]] i32 getLevel(const FluidState& state) const override {
        (void)state;
        return 8;
    }

    [[nodiscard]] FlowingFluid& getFlowing() override;
    [[nodiscard]] FlowingFluid& getStill() override { return *this; }

private:
    mutable FlowingFluid* m_flowingCache = nullptr;
};

/**
 * @brief 流动水
 *
 * 流动水方块，有LEVEL属性(1-8)和FALLING属性。
 */
class WaterFlowingFluid : public WaterFluid {
public:
    WaterFlowingFluid();

    [[nodiscard]] bool isSource(const FluidState& state) const override {
        (void)state;
        return false;
    }

    [[nodiscard]] i32 getLevel(const FluidState& state) const override;

    [[nodiscard]] FlowingFluid& getFlowing() override { return *this; }
    [[nodiscard]] FlowingFluid& getStill() override;

    /**
     * @brief 检查是否等效于指定流体
     *
     * 水和流动水视为等效。
     *
     * @param fluid 其他流体
     * @return 是否等效
     */
    [[nodiscard]] bool isEquivalentTo(const Fluid& fluid) const override;

private:
    mutable FlowingFluid* m_stillCache = nullptr;
};

} // namespace fluid
} // namespace mc
