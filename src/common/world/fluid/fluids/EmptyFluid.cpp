#include "EmptyFluid.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../FluidRegistry.hpp"

namespace mc {
namespace fluid {

EmptyFluid::EmptyFluid()
{
    // 空流体没有任何属性，创建空的状态容器
    auto container =
        StateContainer<Fluid, FluidState>::Builder(*this).create([this](const Fluid& fluid, auto values, u32 id) {
            return std::make_unique<FluidState>(fluid, std::move(values), id);
        });
    createFluidState(std::move(container));
    setDefaultState(stateContainer().baseState());
}

const BlockState* EmptyFluid::getBlockState(const FluidState& state) const
{
    (void)state;
    // 空流体对应的方块是空气
    if (VanillaBlocks::AIR != nullptr) {
        return &VanillaBlocks::AIR->defaultState();
    }
    return nullptr;
}

} // namespace fluid
} // namespace mc
