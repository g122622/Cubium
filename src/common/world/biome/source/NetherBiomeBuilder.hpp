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

#pragma once

#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/climate/ParameterList.hpp"

namespace mc {
namespace world {
namespace biome {
namespace source {

/**
 * @brief 下界生物群系参数构建器
 *
 * 下界使用简化的气候参数映射，仅 temperature 和 humidity 有效，
 * 其他参数（continentalness, erosion, depth, weirdness）均为全范围。
 * 通过 offset 参数微调优先级。
 */
class NetherBiomeBuilder {
public:
    /**
     * @brief 构建下界生物群系参数列表
     * @return 下界 ParameterList<BiomeId>
     */
    [[nodiscard]] static climate::ParameterList<BiomeId> buildParameterList();
};

} // namespace source
} // namespace biome
} // namespace world
} // namespace mc
