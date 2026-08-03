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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"

#include <cstddef>
#include <unordered_map>

namespace mc::world::biome {

/// vanilla 1.21.11 biome registry id 的总数（RegistryDataBuilder 同步的 66 条）。
constexpr u32 kJavaBiomeRegistryCount = 66;

/**
 * @brief 内部 BiomeId ↔ Java biome registry id 双向映射
 *
 * 项目内部 BiomeId 是 1.16.5 数值 id（BiomeIds.hpp，0-185），1.21.11 wire 里 biome
 * PalettedContainer 的 palette 值是 Java biome registry id（RegistryDataBuilder.cpp:81
 * 同步的 66 条 worldgen/biome 条目顺序，registry id = 条目下标）。两套编号无关。
 *
 * Biome 的 m_name 是 1.16.5 旧名字符串（如 mountains/stone_shore），部分在 1.18 被重命名
 * （→ windswept_hills/stony_shore 等），部分 1.16.x 变体（desert_hills 等）在 1.21.11
 * vanilla 已删除无对应。initialize() 时：
 * 1. 用 RegistryDataBuilder 同源的 66 条 vanilla biome name 列表建 name→registry id 表；
 * 2. 遍历 BiomeRegistry::allBiomes()，对每个 biome 取 m_name，先经 1.18 旧名→新名别名表
 *    归一化，再查 66 条列表得 registry id；查不到（1.16.x 已删变体）兜底 plains(38)+warn；
 * 3. 建 BiomeId↔registry id 双向表。
 *
 * 须在 BiomeRegistry::initialize() 之后调用。
 */
class JavaBiomeRegistryIdMap {
public:
    static JavaBiomeRegistryIdMap& instance();

    JavaBiomeRegistryIdMap() = default;
    ~JavaBiomeRegistryIdMap() = default;
    JavaBiomeRegistryIdMap(const JavaBiomeRegistryIdMap&) = delete;
    JavaBiomeRegistryIdMap& operator=(const JavaBiomeRegistryIdMap&) = delete;

    /// 构建双向映射。须在 BiomeRegistry::initialize() 之后调用。可重复调用。
    [[nodiscard]] Result<void> initialize();

    /// 内部 BiomeId → Java biome registry id；未建立或查不到返回 plains(38) 并记 warn。
    [[nodiscard]] u32 toJavaRegistryId(BiomeId internalBiomeId) const;

    /// Java biome registry id → 内部 BiomeId；查不到返回 plains 内部 id 并记 warn。
    [[nodiscard]] BiomeId fromJavaRegistryId(u32 javaRegistryId) const;

    /// 是否已建立映射。
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

    /// 已匹配的 biome 对数（诊断用）。
    [[nodiscard]] size_t matchedCount() const noexcept { return m_toJava.size(); }

private:
    bool m_initialized = false;
    /// 内部 BiomeId → Java registry id
    std::unordered_map<BiomeId, u32> m_toJava;
    /// Java registry id → 内部 BiomeId
    std::unordered_map<u32, BiomeId> m_fromJava;
};

} // namespace mc::world::biome
