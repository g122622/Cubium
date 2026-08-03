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

#include "Fluids.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"

namespace mc {
namespace fluid {

namespace {

// ============================================================================
// 内置流体指针缓存
// 这些指针在 Fluids::initialize() 中初始化，指向 FluidRegistry 中注册的流体实例
// ============================================================================

Fluid* g_empty = nullptr;
Fluid* g_water = nullptr;
Fluid* g_flowingWater = nullptr;
Fluid* g_lava = nullptr;
Fluid* g_flowingLava = nullptr;
bool g_initialized = false;

} // namespace

// ============================================================================
// Fluids 命名空间实现
// ============================================================================

void Fluids::initialize()
{
    // 防止重复初始化
    if (g_initialized) {
        return;
    }

    // 从注册表中获取内置流体实例
    auto& registry = FluidRegistry::instance();

    g_empty = registry.getFluid(FluidRegistry::EMPTY_ID);
    g_water = registry.getFluid(FluidRegistry::WATER_ID);
    g_flowingWater = registry.getFluid(FluidRegistry::FLOWING_WATER_ID);
    g_lava = registry.getFluid(FluidRegistry::LAVA_ID);
    g_flowingLava = registry.getFluid(FluidRegistry::FLOWING_LAVA_ID);

    g_initialized = true;
}

Fluid* Fluids::EMPTY()
{
    return g_empty;
}

Fluid* Fluids::WATER()
{
    return g_water;
}

Fluid* Fluids::FLOWING_WATER()
{
    return g_flowingWater;
}

Fluid* Fluids::LAVA()
{
    return g_lava;
}

Fluid* Fluids::FLOWING_LAVA()
{
    return g_flowingLava;
}

} // namespace fluid
} // namespace mc
