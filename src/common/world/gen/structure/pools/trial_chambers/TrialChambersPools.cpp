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

#include "common/util/assert/AssertMacros.hpp"
#include "common/world/gen/jigsaw/TemplatePoolRegistry.hpp"

namespace mc {
namespace world {
namespace gen {
namespace structure {
namespace pools {

void TrialChambersPools::registerAll(jigsaw::TemplatePoolRegistry& registry)
{
    MC_UNUSED(registry);

    // 试炼密室的模板池从数据包 JSON 文件中自动加载：
    // data/minecraft/worldgen/template_pool/trial_chambers/*.json
    // 因此不需要编程式注册模板池。

    // trial_chambers_copper_bulb_degradation 处理器列表同样由数据包 JSON 加载：
    // data/minecraft/worldgen/processor_list/trial_chambers_copper_bulb_degradation.json
    // ProcessorListLoader 会解析其中的 rule processor 和 protected_blocks processor，
    // 完整复刻 MC 1.21.11 试炼密室铜灯降级（加权概率 + lit=true 输出状态），
    // 因此无需任何编程式注册。
}

} // namespace pools
} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
