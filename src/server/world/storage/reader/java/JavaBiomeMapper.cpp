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

#include "JavaBiomeMapper.hpp"
#include "common/core/Types.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/JavaBiomeRegistryIdMap.hpp"
#include <spdlog/spdlog.h>

namespace mc::world::storage::reader::java {

BiomeId JavaBiomeMapper::mapBiome(const std::string& biomeName)
{
    const auto& registryMap = biome::JavaBiomeRegistryIdMap::instance();
    if (!registryMap.isInitialized()) {
        spdlog::warn("JavaBiomeMapper: JavaBiomeRegistryIdMap 未初始化，无法按名解析群系 '{}'；"
                     "请确保在 BiomeRegistry::initialize() 之后调用其 initialize()",
            biomeName);
        return UnknownBiome;
    }

    if (const auto id = registryMap.biomeIdByName(biomeName); id.has_value()) {
        return *id;
    }

    // Java 侧存在但项目注册表没有对应群系（或该存档来自更新的版本）。
    // 此处刻意逐条告警而非静默兜底——静默回退曾让整片群系错位长期无人察觉。
    spdlog::warn("JavaBiomeMapper: 群系 '{}' 在内部注册表中没有对应项，回退为默认群系", biomeName);
    return UnknownBiome;
}

BiomeId JavaBiomeMapper::mapBiome(i32 numericId)
{
    // Java 1.16.5 及更早：群系数值 ID 与项目内部 ID 一致
    if (numericId >= 0 && numericId <= 255) {
        return static_cast<BiomeId>(numericId);
    }
    return UnknownBiome;
}

} // namespace mc::world::storage::reader::java
