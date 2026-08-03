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

#include "Pools.hpp"
#include "ProcessorLists.hpp"
#include "bastion/BastionPools.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "pillager_outpost/PillagerOutpostPools.hpp"
#include "trial_chambers/TrialChambersPools.hpp"
#include "village/VillagePools.hpp"
#include <memory>
#include <utility>

namespace mc {
namespace world {
namespace gen {
namespace structure {
namespace pools {

// 初始化标志
static bool s_initialized = false;

void Pools::initialize()
{
    if (s_initialized) {
        return;
    }

    // 1. 初始化处理器列表
    ProcessorLists::initialize();

    // 2. 获取模板池注册表
    auto& registry = TemplatePoolRegistry::instance();

    // 3. 注册空模板池
    registerEmptyPool(registry);

    // 4. 注册村庄模板池
    VillagePools::registerAll(registry);

    // 5. 注册掠夺者前哨站模板池
    PillagerOutpostPools::registerAll(registry);

    // 6. 注册堡垒遗迹模板池
    BastionPools::registerAll(registry);

    // 7. 注册试炼密室模板池和处理器
    TrialChambersPools::registerAll(registry);

    s_initialized = true;
}

bool Pools::isInitialized()
{
    return s_initialized;
}

void Pools::registerEmptyPool(TemplatePoolRegistry& registry)
{
    // 空模板池，用于终止 Jigsaw 链
    auto emptyPool = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "empty"), ResourceLocation("minecraft", "empty") // fallback 指向自己
    );

    registry.registerPool(std::move(emptyPool));
}

} // namespace pools
} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
