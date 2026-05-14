#pragma once

#include "../../../util/math/Vector3.hpp"
#include "../Fluid.hpp"

namespace mc {
namespace fluid {

/**
 * @brief 空流体
 *
 * 表示无流体的状态。单例模式。
 *
 * 参考: net.minecraft.fluid.EmptyFluid
 */
class EmptyFluid : public Fluid {
public:
    EmptyFluid();

    // ========== Fluid接口实现 ==========

    [[nodiscard]] bool isSource(const FluidState& state) const override { return false; }

    [[nodiscard]] i32 getLevel(const FluidState& state) const override { return 0; }

    [[nodiscard]] i32 getTickDelay() const override { return 0; }

    [[nodiscard]] bool canSourcesMultiply() const override { return false; }

    [[nodiscard]] const BlockState* getBlockState(const FluidState& state) const override;

    [[nodiscard]] f32 getExplosionResistance() const override { return 0.0f; }

    [[nodiscard]] Vector3 getFlow(IBlockReader& world, const BlockPos& pos, const FluidState& state) const override
    {
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    void tick(IWorld& world, const BlockPos& pos, FluidState& state) override
    {
        // 空流体不执行tick
        (void)world;
        (void)pos;
        (void)state;
    }

    [[nodiscard]] bool canDisplace(
        const FluidState& state, IWorld& world, const BlockPos& pos, const Fluid& fluid, Direction dir) const override
    {
        (void)state;
        (void)world;
        (void)pos;
        (void)fluid;
        (void)dir;
        return true;
    }

    [[nodiscard]] bool ticksRandomly() const override { return false; }

    [[nodiscard]] bool isEmpty() const override { return true; }
};

} // namespace fluid
} // namespace mc
