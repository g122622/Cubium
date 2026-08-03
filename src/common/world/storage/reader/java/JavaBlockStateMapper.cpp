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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE OR IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "JavaBlockStateMapper.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include <string>
#include <vector>

namespace mc::world::storage::reader::java {

JavaBlockStateMapper::JavaBlockStateMapper() = default;

u32 JavaBlockStateMapper::mapBlockState(const PaletteEntry& entry)
{
    std::string cacheKey = _buildCacheKey(entry);

    auto it = m_cache.find(cacheKey);
    if (it != m_cache.end()) {
        return it->second;
    }

    // 通过 ResourceLocation 查找 Block
    ResourceLocation location(entry.blockName);
    Block* block = BlockRegistry::instance().getBlock(location);

    if (!block) {
        // 未知方块，映射为空气
        m_cache[cacheKey] = 0;
        return 0;
    }

    // 数据迁移：旧版 Java 世界中 minecraft:cauldron 有 level 属性（0-3），
    // level >= 1 时应映射为 minecraft:water_cauldron
    if (entry.blockName == "minecraft:cauldron") {
        auto levelIt = entry.properties.find("level");
        if (levelIt != entry.properties.end() && levelIt->second != "0") {
            ResourceLocation waterCauldronLocation("minecraft:water_cauldron");
            Block* waterCauldronBlock = BlockRegistry::instance().getBlock(waterCauldronLocation);
            if (waterCauldronBlock) {
                block = waterCauldronBlock;
            }
        }
    }

    // 获取默认状态
    const BlockState* state = &block->defaultState();

    // 逐步叠加属性，确保多属性方块状态能正确组合。
    if (!entry.properties.empty()) {
        const auto& container = block->stateContainer();

        for (const auto& [propName, propValue] : entry.properties) {
            const IProperty* prop = container.getProperty(propName);
            if (!prop) {
                continue;
            }

            auto parsedValue = prop->parseValue(propValue);
            if (!parsedValue.has_value()) {
                continue;
            }

            const auto currentValueIndex = state->getValueIndex(*prop);
            if (!currentValueIndex.has_value()) {
                continue;
            }

            if (currentValueIndex.value() == parsedValue.value()) {
                continue;
            }

            const BlockState* matchedState = nullptr;
            for (const auto& validState : container.validStates()) {
                if (validState->getValueIndex(*prop) != parsedValue) {
                    continue;
                }

                bool otherPropertiesMatch = true;
                for (const auto& [otherName, otherValue] : entry.properties) {
                    if (otherName == propName) {
                        continue;
                    }

                    const IProperty* otherProp = container.getProperty(otherName);
                    if (!otherProp) {
                        continue;
                    }

                    const auto desiredOtherValue = otherProp->parseValue(otherValue);
                    if (!desiredOtherValue.has_value()) {
                        continue;
                    }

                    if (validState->getValueIndex(*otherProp) != desiredOtherValue) {
                        otherPropertiesMatch = false;
                        break;
                    }
                }

                if (otherPropertiesMatch) {
                    matchedState = validState.get();
                    break;
                }
            }

            if (matchedState != nullptr) {
                state = matchedState;
            }
        }
    }

    u32 stateId = state->stateId();
    m_cache[cacheKey] = stateId;
    return stateId;
}

std::vector<u32> JavaBlockStateMapper::mapPalette(const std::vector<PaletteEntry>& entries)
{
    std::vector<u32> result;
    result.reserve(entries.size());
    for (const auto& entry : entries) {
        result.push_back(mapBlockState(entry));
    }
    return result;
}

std::string JavaBlockStateMapper::_buildCacheKey(const PaletteEntry& entry) const noexcept
{
    std::string key = entry.blockName;
    for (const auto& [k, v] : entry.properties) {
        key += ',';
        key += k;
        key += '=';
        key += v;
    }
    return key;
}

} // namespace mc::world::storage::reader::java
