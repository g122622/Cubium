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
#include <string>
#include <unordered_map>

namespace mc::world::storage::reader::java {

/// 默认生物群系 ID（未知时回退）
constexpr BiomeId UnknownBiome = 0;

/**
 * @brief Java 版生物群系名称→内部 BiomeId 映射器
 *
 * 将 Java 版的生物群系名称（如 "minecraft:plains"）
 * 映射到项目内部的 BiomeId。
 *
 * Java 1.16.5 的生物群系数值 ID 与项目内部 ID 一致，
 * 因此名称映射主要作为备用和未来兼容。
 */
class JavaBiomeMapper {
public:
    JavaBiomeMapper();

    /**
     * @brief 从 Java 版生物群系名称映射到内部 BiomeId
     * @param biomeName Java 版生物群系名称（如 "minecraft:plains"）
     * @return 内部 BiomeId，未识别返回 0（海洋）
     */
    BiomeId mapBiome(const std::string& biomeName);

    /**
     * @brief 从 Java 版数值 ID 映射到内部 BiomeId
     * @param numericId Java 版生物群系数值 ID
     * @return 内部 BiomeId
     */
    BiomeId mapBiome(i32 numericId);

private:
    void _initializeMappings();

    std::unordered_map<std::string, BiomeId> m_nameToId;
};

} // namespace mc::world::storage::reader::java
