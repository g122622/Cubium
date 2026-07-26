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

#include "FluidRegistry.hpp"
#include "Fluids.hpp"
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
    _registerFluidInternal(emptyFluid.get(), ResourceLocation("minecraft:empty"), EMPTY_ID);
    m_fluids.push_back(std::move(emptyFluid));

    // 注册水源头 (ID = 1)
    auto waterSource = std::make_unique<WaterSourceFluid>();
    _registerFluidInternal(waterSource.get(), ResourceLocation("minecraft:water"), WATER_ID);
    m_fluids.push_back(std::move(waterSource));

    // 注册流动水 (ID = 2)
    auto flowingWater = std::make_unique<WaterFlowingFluid>();
    _registerFluidInternal(flowingWater.get(), ResourceLocation("minecraft:flowing_water"), FLOWING_WATER_ID);
    m_fluids.push_back(std::move(flowingWater));

    // 注册岩浆源头 (ID = 3)
    auto lavaSource = std::make_unique<LavaSourceFluid>();
    _registerFluidInternal(lavaSource.get(), ResourceLocation("minecraft:lava"), LAVA_ID);
    m_fluids.push_back(std::move(lavaSource));

    // 注册流动岩浆 (ID = 4)
    auto flowingLava = std::make_unique<LavaFlowingFluid>();
    _registerFluidInternal(flowingLava.get(), ResourceLocation("minecraft:flowing_lava"), FLOWING_LAVA_ID);
    m_fluids.push_back(std::move(flowingLava));

    m_initialized = true;

    // 注册表填充完成后，刷新 Fluids 命名空间下的内置访问器指针缓存
    // （EMPTY/WATER/...）。把这一步收敛进 initialize()，使得"注册表已初始化"
    // 与"Fluids::EMPTY() 可用"成为同一个不变量——调用方无需再单独调
    // Fluids::initialize()，从而避免拿注册表却忘了刷缓存导致访问器返回空指针。
    Fluids::initialize();
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

void FluidRegistry::_registerFluidInternal(Fluid* fluid, const ResourceLocation& id, u32 fluidId)
{
    // 设置流体属性
    fluid->m_fluidLocation = id;
    fluid->m_fluidId = fluidId;

    // 注册到ID映射
    m_fluidsByNumericId[fluidId] = fluid;
    m_fluidsById[id] = fluid;
}

} // namespace mc::fluid
