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

// ============================================================================
// 注意：本文件不在 CMakeLists.txt 编译列表中，仅供参考。
// 实际实现在 Template.hpp/cpp 中。
// 修改逻辑时，务必修改 Template.hpp/cpp，而非仅修改本文件。
// ============================================================================

#include "JigsawReplacementStructureProcessor.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/template/BlockInfo.hpp"
#include "common/world/gen/feature/template/PlacementSettings.hpp"
#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

JigsawReplacementStructureProcessor::JigsawReplacementStructureProcessor() {}

std::optional<ProcessedBlockInfo> JigsawReplacementStructureProcessor::process(const BlockPos& /*seedPos*/,
    const BlockPos& /*pos*/,
    const BlockInfo& /*rawBlockInfo*/,
    const BlockInfo& blockInfo,
    const PlacementSettings& /*settings*/)
{
    // 检查方块是否为 Jigsaw 方块
    // 如果是，读取 NBT 中的 final_state 字段并解析为新的方块状态
    const BlockState* state = BlockRegistry::instance().getBlockState(blockInfo.blockStateId);
    if (!state) {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 检查是否是 Jigsaw 方块
    const Block& block = state->getBlock();
    ResourceLocation blockId = block.blockLocation();
    if (blockId.toString() != "minecraft:jigsaw") {
        // 不是 Jigsaw 方块，保持原样
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    // 是 Jigsaw 方块，读取 final_state
    if (!blockInfo.nbt) {
        // 没有 NBT，返回空气（跳过）
        return std::nullopt;
    }

    auto it = blockInfo.nbt->value.find("final_state");
    if (it == blockInfo.nbt->value.end() || !it->second) {
        // 没有 final_state，返回空气（跳过）
        return std::nullopt;
    }

    if (it->second->id() != nbt::TagId::String) {
        return std::nullopt;
    }

    const std::string& finalStateStr = dynamic_cast<const nbt::StringTag&>(*it->second).value;

    // 解析 final_state 字符串
    // 格式: "minecraft:stone[properties]" 或 "minecraft:stone"
    u32 newStateId = parseBlockStateString(finalStateStr);

    // 检查是否是 structure_void
    const BlockState* newState = BlockRegistry::instance().getBlockState(newStateId);
    if (newState) {
        ResourceLocation newBlockId = newState->getBlock().blockLocation();
        if (newBlockId.toString() == "minecraft:structure_void") {
            // structure_void 表示跳过此方块
            return std::nullopt;
        }
    }

    // 返回新的方块状态（无 NBT）
    ProcessedBlockInfo result;
    result.pos = blockInfo.pos;
    result.blockStateId = newStateId;
    // Jigsaw 方块被替换后不保留 NBT
    return result;
}

u32 JigsawReplacementStructureProcessor::parseBlockStateString(const std::string& stateStr)
{
    // 解析方块状态字符串
    // 格式: "minecraft:stone[axis=y,facing=north]" 或 "minecraft:stone"
    size_t bracketPos = stateStr.find('[');
    std::string blockName;
    std::unordered_map<std::string, std::string> properties;

    if (bracketPos == std::string::npos) {
        // 没有属性
        blockName = stateStr;
    } else {
        // 有属性
        blockName = stateStr.substr(0, bracketPos);
        std::string propsStr = stateStr.substr(bracketPos + 1);
        if (!propsStr.empty() && propsStr.back() == ']') {
            propsStr.pop_back();
        }

        // 解析属性
        size_t start = 0;
        size_t end = propsStr.find(',');
        while (start < propsStr.size()) {
            std::string prop =
                (end == std::string::npos) ? propsStr.substr(start) : propsStr.substr(start, end - start);
            size_t eqPos = prop.find('=');
            if (eqPos != std::string::npos) {
                std::string key = prop.substr(0, eqPos);
                std::string value = prop.substr(eqPos + 1);
                properties[key] = value;
            }
            if (end == std::string::npos) break;
            start = end + 1;
            end = propsStr.find(',', start);
        }
    }

    // 获取方块
    auto& registry = BlockRegistry::instance();
    Block* block = registry.getBlock(ResourceLocation(blockName));
    if (!block) {
        return 0; // 空气
    }

    // 获取默认状态
    const BlockState* state = &block->defaultState();

    // 应用属性
    if (!properties.empty()) {
        const auto& container = block->stateContainer();
        std::unordered_map<const IProperty*, size_t> wanted;

        for (const auto& [key, value] : properties) {
            const IProperty* prop = container.getProperty(key);
            if (!prop) continue;

            auto parsedValue = prop->parseValue(value);
            if (!parsedValue) continue;

            wanted[prop] = *parsedValue;
        }

        // 查找匹配的状态
        if (!wanted.empty()) {
            for (const auto& candidate : container.validStates()) {
                if (!candidate) continue;

                bool matches = true;
                for (const auto& [prop, index] : wanted) {
                    const auto valueIndex = candidate->getValueIndex(*prop);
                    if (!valueIndex.has_value() || *valueIndex != index) {
                        matches = false;
                        break;
                    }
                }

                if (matches) {
                    state = candidate.get();
                    break;
                }
            }
        }
    }

    return state ? state->stateId() : 0;
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
