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
 */

#include "NetherBiomeBuilder.hpp"

namespace mc {
namespace world {
namespace biome {
namespace source {

using namespace climate;

ParameterList<BiomeId> NetherBiomeBuilder::buildParameterList()
{
    ParameterList<BiomeId> list;
    const Parameter fullRange = Parameter::fullRange();

    // 下界使用简化的气候参数映射
    // 仅 temperature 和 humidity 有效，其他参数为全范围
    // offset 用于微调优先级

    // 下界荒地
    list.add(pointParameters(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), Biomes::NetherWastes);

    // 灵魂沙峡谷
    list.add(pointParameters(0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), Biomes::SoulSandValley);

    // 绯红森林
    list.add(pointParameters(0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), Biomes::CrimsonForest);

    // 诡异森林
    list.add(pointParameters(0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.375f), Biomes::WarpedForest);

    // 玄武岩三角洲
    list.add(pointParameters(-0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.175f), Biomes::BasaltDeltas);

    return list;
}

} // namespace source
} // namespace biome
} // namespace world
} // namespace mc
