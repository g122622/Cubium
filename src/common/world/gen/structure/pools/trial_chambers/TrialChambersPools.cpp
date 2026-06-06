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

#include "TrialChambersPools.hpp"

#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/gen/feature/template/CopperBulbDegradationProcessor.hpp"
#include "common/world/gen/structure/pools/ProcessorLists.hpp"

namespace mc {
namespace world {
namespace gen {
namespace structure {
namespace pools {

void TrialChambersPools::registerAll(jigsaw::JigsawPatternRegistry& registry)
{
    MC_UNUSED(registry);

    // 试炼密室的模板池从数据包 JSON 文件中自动加载：
    // data/minecraft/worldgen/template_pool/trial_chambers/*.json
    // 因此不需要编程式注册模板池。

    // 注册铜灯降级处理器到处理器列表
    // 此处理器在试炼密室的多个模板中被引用
    // TODO(trial_chambers): 当 ProcessorLists 支持自定义处理器名称注册时，
    // 将 "minecraft:trial_chambers_copper_bulb_degradation" 注册为 CopperBulbDegradationProcessor

    // 注册空模板池（如果数据包未加载时作为后备）
    // 正常情况下数据包加载后会覆盖这些空池
}

} // namespace pools
} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
