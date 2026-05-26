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

#pragma once

#include "common/core/Types.hpp"
#include <unordered_map>

namespace mc::world::storage::reader::bedrock {

/**
 * @brief 基岩版生物群系 ID 映射器
 *
 * 基岩版和 Java 版的生物群系 ID 编号不完全一致。
 * 此类将基岩版生物群系 ID 映射到项目内部 BiomeId（与 Java 版一致）。
 */
class BedrockBiomeMapper {
public:
    BedrockBiomeMapper();

    /**
     * @brief 将基岩版生物群系 ID 映射到内部 BiomeId
     * @param bedrockBiomeId 基岩版生物群系 ID
     * @param dimension 维度 ID（当前用于保留接口一致性）
     * @return 内部 BiomeId
     */
    BiomeId mapBiome(i32 bedrockBiomeId, DimensionId dimension = 0);

private:
    void initializeMappings();

    /// 基岩版 ID → Java 版 ID 映射表
    std::unordered_map<i32, BiomeId> m_bedrockToJava;
};

} // namespace mc::world::storage::reader::bedrock
