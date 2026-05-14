#include "FluidRegistry.hpp"
#include "fluids/EmptyFluid.hpp"
#include "fluids/LavaFluid.hpp"
#include "fluids/WaterFluid.hpp"
#include <algorithm>

namespace mc::fluid {

FluidRegistry& FluidRegistry::instance()
{
    static FluidRegistry instance;
    return instance;
}

void FluidRegistry::initialize()
{
    if (m_initialized) {
        return;
    }

    // 注册内置流体
    // EmptyFluid在构造时自动注册，ID为0
    auto emptyFluid = std::make_unique<EmptyFluid>();
    registerFluidInternal(emptyFluid.get(), ResourceLocation("minecraft:empty"), EMPTY_ID);
    m_fluids.push_back(std::move(emptyFluid));

    // 注册水源头 (ID = 1)
    auto waterSource = std::make_unique<WaterSourceFluid>();
    registerFluidInternal(waterSource.get(), ResourceLocation("minecraft:water"), WATER_ID);
    m_fluids.push_back(std::move(waterSource));

    // 注册流动水 (ID = 2)
    auto flowingWater = std::make_unique<WaterFlowingFluid>();
    registerFluidInternal(flowingWater.get(), ResourceLocation("minecraft:flowing_water"), FLOWING_WATER_ID);
    m_fluids.push_back(std::move(flowingWater));

    // 注册岩浆源头 (ID = 3)
    auto lavaSource = std::make_unique<LavaSourceFluid>();
    registerFluidInternal(lavaSource.get(), ResourceLocation("minecraft:lava"), LAVA_ID);
    m_fluids.push_back(std::move(lavaSource));

    // 注册流动岩浆 (ID = 4)
    auto flowingLava = std::make_unique<LavaFlowingFluid>();
    registerFluidInternal(flowingLava.get(), ResourceLocation("minecraft:flowing_lava"), FLOWING_LAVA_ID);
    m_fluids.push_back(std::move(flowingLava));

    m_initialized = true;
}

Fluid* FluidRegistry::getFluid(u32 fluidId) const
{
    auto it = m_fluidsByNumericId.find(fluidId);
    return it != m_fluidsByNumericId.end() ? it->second : nullptr;
}

Fluid* FluidRegistry::getFluid(const ResourceLocation& id) const
{
    auto it = m_fluidsById.find(id);
    return it != m_fluidsById.end() ? it->second : nullptr;
}

FluidState* FluidRegistry::getFluidState(u32 stateId) const
{
    auto it = m_statesById.find(stateId);
    return it != m_statesById.end() ? it->second : nullptr;
}

void FluidRegistry::forEachFluid(std::function<void(Fluid&)> callback)
{
    for (auto& fluid : m_fluids) {
        callback(*fluid);
    }
}

void FluidRegistry::forEachFluidState(std::function<void(const FluidState&)> callback)
{
    // 遍历所有流体的所有状态
    for (const auto& fluid : m_fluids) {
        const auto& container = fluid->stateContainer();
        for (size_t i = 0; i < container.stateCount(); ++i) {
            callback(*container.getStateById(static_cast<u32>(i)));
        }
    }
}

void FluidRegistry::registerFluidInternal(Fluid* fluid, const ResourceLocation& id, u32 fluidId)
{
    // 设置流体属性
    fluid->m_fluidLocation = id;
    fluid->m_fluidId = fluidId;

    // 注册到ID映射
    m_fluidsByNumericId[fluidId] = fluid;
    m_fluidsById[id] = fluid;

    // 注册所有状态
    if (fluid->m_stateContainer) {
        auto& container = *fluid->m_stateContainer;
        for (size_t i = 0; i < container.stateCount(); ++i) {
            // 注意：StateHolder的stateId在StateContainer创建时已分配
            // 这里我们需要更新statesById映射
            const FluidState* state = container.getStateById(static_cast<u32>(i));
            u32 stateId = state->stateId();
            m_statesById[stateId] = const_cast<FluidState*>(state);
        }
    }
}

} // namespace mc::fluid
