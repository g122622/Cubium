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

#include "TemplateLoader.hpp"
#include "../../../../resource/pack/IResourcePack.hpp"
#include "../../../../util/CompressionUtils.hpp"
#include "../../../block/Block.hpp"
#include "../../../block/BlockRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/property/IProperty.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

namespace {

const BlockState* applyPropertiesToState(
    const Block& block, const BlockState* defaultState, const nbt::CompoundTag& propsCompound)
{
    if (!defaultState) {
        return nullptr;
    }

    const auto& container = block.stateContainer();
    std::unordered_map<const IProperty*, size_t> wanted;

    for (const auto& [key, valueTag] : propsCompound.value) {
        if (!valueTag || valueTag->id() != nbt::TagId::String) {
            continue;
        }

        const IProperty* prop = container.getProperty(key);
        if (!prop) {
            continue;
        }

        const std::string& value = dynamic_cast<const nbt::StringTag&>(*valueTag).value;
        auto parsedValue = prop->parseValue(value);
        if (!parsedValue) {
            continue;
        }

        wanted[prop] = *parsedValue;
    }

    if (wanted.empty()) {
        return defaultState;
    }

    for (const auto& candidate : container.validStates()) {
        if (!candidate) {
            continue;
        }

        bool matches = true;
        for (const auto& [prop, index] : wanted) {
            const auto valueIndex = candidate->getValueIndex(*prop);
            if (!valueIndex.has_value() || *valueIndex != index) {
                matches = false;
                break;
            }
        }

        if (matches) {
            return candidate.get();
        }
    }

    return defaultState;
}

} // anonymous namespace

std::unique_ptr<Template> TemplateLoader::loadFromNbt(const nbt::CompoundTag& nbt)
{
    auto templ = std::make_unique<Template>();

    // 读取大小: size: [x, y, z]
    if (nbt.value.count("size") == 0) {
        return templ;
    }

    auto& sizeTag = *nbt.value.at("size");
    if (sizeTag.id() != nbt::TagId::List) {
        return templ;
    }

    auto& sizeList = dynamic_cast<const nbt::ListTag&>(sizeTag);
    if (sizeList.size() < 3) {
        return templ;
    }

    auto sizeX = dynamic_cast<const nbt::IntTag&>(*sizeList[0]).value;
    auto sizeY = dynamic_cast<const nbt::IntTag&>(*sizeList[1]).value;
    auto sizeZ = dynamic_cast<const nbt::IntTag&>(*sizeList[2]).value;
    templ->setSize(BlockPos(sizeX, sizeY, sizeZ));

    // 支持两种格式：
    // 1. palette: 单一调色板列表
    // 2. palettes: 多个调色板列表的列表（用于结构变体）
    // blocks 引用调色板中的方块状态索引

    // 读取所有调色板
    std::vector<std::vector<u32>> palettes;

    if (nbt.value.count("palettes") != 0) {
        // 多调色板格式：palettes 是一个列表的列表
        auto& palettesTag = *nbt.value.at("palettes");
        if (palettesTag.id() == nbt::TagId::List) {
            auto& palettesList = dynamic_cast<const nbt::ListTag&>(palettesTag);
            palettes.resize(palettesList.size());

            for (size_t paletteIdx = 0; paletteIdx < palettesList.size(); ++paletteIdx) {
                const auto& paletteTagPtr = palettesList[paletteIdx];
                const nbt::Tag* paletteTag = paletteTagPtr.get();
                if (paletteTag == nullptr || paletteTag->id() != nbt::TagId::List) {
                    continue;
                }
                const auto& paletteList = dynamic_cast<const nbt::ListTag&>(*paletteTag);
                auto& palette = palettes[paletteIdx];
                palette.reserve(paletteList.size());

                for (size_t i = 0; i < paletteList.size(); ++i) {
                    const auto& entryTagPtr = paletteList[i];
                    const nbt::Tag* entryTag = entryTagPtr.get();
                    MC_ASSERT_RELEASE(entryTag != nullptr);
                    const auto& entry = dynamic_cast<const nbt::CompoundTag&>(*entryTag);
                    palette.push_back(_parseBlockStateId(entry));
                }
            }
        }
    }

    // 如果没有 palettes，尝试单 palette 格式
    if (palettes.empty() && nbt.value.count("palette") != 0) {
        auto& paletteTag = *nbt.value.at("palette");
        if (paletteTag.id() == nbt::TagId::List) {
            auto& paletteList = dynamic_cast<const nbt::ListTag&>(paletteTag);
            palettes.resize(1);
            auto& palette = palettes[0];
            palette.reserve(paletteList.size());

            for (size_t i = 0; i < paletteList.size(); ++i) {
                const auto& entryTagPtr = paletteList[i];
                const nbt::Tag* entryTag = entryTagPtr.get();
                MC_ASSERT_RELEASE(entryTag != nullptr);
                const auto& entry = dynamic_cast<const nbt::CompoundTag&>(*entryTag);
                palette.push_back(_parseBlockStateId(entry));
            }
        }
    }

    // 读取方块并分配到各个调色板
    // blocks 数组中的 state 索引引用调色板中的方块状态
    // 每个 Palette 包含完整的方块列表
    if (nbt.value.count("blocks") != 0 && !palettes.empty()) {
        auto& blocksTag = *nbt.value.at("blocks");
        if (blocksTag.id() == nbt::TagId::List) {
            auto& blocksList = dynamic_cast<const nbt::ListTag&>(blocksTag);

            // 首先读取所有方块信息（位置和 NBT）
            struct RawBlockInfo {
                BlockPos pos;
                u32 stateIndex;
                std::unique_ptr<nbt::CompoundTag> nbt;
            };
            std::vector<RawBlockInfo> rawBlocks;
            rawBlocks.reserve(blocksList.size());

            for (size_t i = 0; i < blocksList.size(); ++i) {
                const auto& blockEntryTagPtr = blocksList[i];
                const nbt::Tag* blockEntryTag = blockEntryTagPtr.get();
                MC_ASSERT_RELEASE(blockEntryTag != nullptr);
                const auto& blockEntry = dynamic_cast<const nbt::CompoundTag&>(*blockEntryTag);

                RawBlockInfo rawInfo;

                // 读取位置: pos: [x, y, z]
                if (blockEntry.value.count("pos") != 0) {
                    auto& posTag = *blockEntry.value.at("pos");
                    if (posTag.id() == nbt::TagId::List) {
                        rawInfo.pos = _readBlockPos(dynamic_cast<const nbt::ListTag&>(posTag));
                    }
                }

                // 读取状态索引: state: int
                if (blockEntry.value.count("state") != 0) {
                    rawInfo.stateIndex =
                        static_cast<u32>(dynamic_cast<const nbt::IntTag&>(*blockEntry.value.at("state")).value);
                }

                // 读取 NBT 数据: nbt: {...}
                if (blockEntry.value.count("nbt") != 0) {
                    auto& nbtTag = *blockEntry.value.at("nbt");
                    if (nbtTag.id() == nbt::TagId::Compound) {
                        const nbt::CompoundTag* nbtPtr = dynamic_cast<const nbt::CompoundTag*>(&nbtTag);
                        rawInfo.nbt = _cloneNbt(nbtPtr);
                    }
                }

                rawBlocks.push_back(std::move(rawInfo));
            }

            // 为每个调色板创建 Palette 对象
            // 使用第一个调色板来确定 Jigsaw 方块的 orientation
            const auto& firstPalette = palettes[0];
            for (size_t paletteIdx = 0; paletteIdx < palettes.size(); ++paletteIdx) {
                const auto& palette = palettes[paletteIdx];
                std::vector<BlockInfo> blockInfos;
                blockInfos.reserve(rawBlocks.size());

                for (const auto& rawInfo : rawBlocks) {
                    u32 stateId = 0; // 默认空气
                    if (rawInfo.stateIndex < palette.size()) {
                        stateId = palette[rawInfo.stateIndex];
                    }

                    BlockInfo blockInfo(rawInfo.pos, stateId);
                    if (rawInfo.nbt) {
                        blockInfo.nbt = _cloneNbt(rawInfo.nbt.get());
                    }
                    blockInfos.push_back(std::move(blockInfo));

                    // 仅在第一个调色板时处理 Jigsaw 方块
                    if (paletteIdx == 0 && rawInfo.nbt) {
                        // 获取第一个调色板中的方块状态ID
                        u32 firstPaletteStateId = 0;
                        if (rawInfo.stateIndex < firstPalette.size()) {
                            firstPaletteStateId = firstPalette[rawInfo.stateIndex];
                        }

                        // 检查是否是 Jigsaw 方块
                        auto jigsawInfo = _parseJigsawBlock(rawInfo.nbt.get(), rawInfo.pos, firstPaletteStateId);
                        if (!jigsawInfo.name.empty()) {
                            templ->addJigsawBlock(jigsawInfo);
                        }
                    }
                }

                templ->addPalette(Palette(std::move(blockInfos)));
            }
        }
    }

    // 读取实体: entities: [...]
    // 实体有两个位置：
    // - pos: Double 列表（精确位置）
    // - blockPos: Int 列表（方块坐标）
    if (nbt.value.count("entities") != 0) {
        auto& entitiesTag = *nbt.value.at("entities");
        if (entitiesTag.id() == nbt::TagId::List) {
            auto& entitiesList = dynamic_cast<const nbt::ListTag&>(entitiesTag);
            for (size_t i = 0; i < entitiesList.size(); ++i) {
                const auto& entityEntryTagPtr = entitiesList[i];
                const nbt::Tag* entityEntryTag = entityEntryTagPtr.get();
                MC_ASSERT_RELEASE(entityEntryTag != nullptr);
                const auto& entityEntry = dynamic_cast<const nbt::CompoundTag&>(*entityEntryTag);

                TemplateEntityInfo entityInfo;

                // 读取实体类型
                if (entityEntry.value.count("id") != 0) {
                    entityInfo.typeId = dynamic_cast<const nbt::StringTag&>(*entityEntry.value.at("id")).value;
                }

                // 读取精确位置 pos: [double, double, double]
                if (entityEntry.value.count("pos") != 0) {
                    auto& posTag = *entityEntry.value.at("pos");
                    if (posTag.id() == nbt::TagId::List) {
                        auto& posList = dynamic_cast<const nbt::ListTag&>(posTag);
                        if (posList.size() >= 3) {
                            // pos 是 Double 列表
                            entityInfo.posx = dynamic_cast<const nbt::DoubleTag&>(*posList[0]).value;
                            entityInfo.posy = dynamic_cast<const nbt::DoubleTag&>(*posList[1]).value;
                            entityInfo.posz = dynamic_cast<const nbt::DoubleTag&>(*posList[2]).value;
                        }
                    }
                }

                // 读取方块坐标 blockPos: [int, int, int]
                if (entityEntry.value.count("blockPos") != 0) {
                    auto& blockPosTag = *entityEntry.value.at("blockPos");
                    if (blockPosTag.id() == nbt::TagId::List) {
                        entityInfo.blockPos = _readBlockPos(dynamic_cast<const nbt::ListTag&>(blockPosTag));
                    }
                }

                // 读取 NBT
                if (entityEntry.value.count("nbt") != 0) {
                    auto& nbtTag = *entityEntry.value.at("nbt");
                    if (nbtTag.id() == nbt::TagId::Compound) {
                        const nbt::CompoundTag* nbtPtr = dynamic_cast<const nbt::CompoundTag*>(&nbtTag);
                        entityInfo.nbt = _cloneNbt(nbtPtr);
                    }
                }

                templ->addEntity(entityInfo);
            }
        }
    }

    return templ;
}

std::unique_ptr<Template> TemplateLoader::loadFromResourcePack(
    const IResourcePack& pack, const ResourceLocation& location)
{
    // 构建资源路径: <namespace>/structure/<path>.nbt
    const std::string basePath = location.namespace_() + "/structure/" + location.path();
    std::string path = basePath + ".nbt";

    // 尝试读取资源
    // 结构模板位于数据包路径 data/<namespace>/structure/<path>.nbt
    auto result = pack.readResource(resource::PackType::ServerData, path);
    if (!result.success() || result.value().empty()) {
        // 尝试不带 .nbt 后缀
        result = pack.readResource(resource::PackType::ServerData, basePath);
        if (!result.success() || result.value().empty()) {
            return nullptr;
        }
    }

    return loadFromCompressedNbt(result.value());
}

std::unique_ptr<Template> TemplateLoader::loadFromCompressedNbt(const std::vector<u8>& data)
{
    // 解压 gzip 数据
    std::vector<u8> decompressed = util::decompressGzip(data);
    if (decompressed.empty()) {
        return nullptr;
    }

    // 解析 NBT
    try {
        std::istringstream stream(std::string(decompressed.begin(), decompressed.end()));
        stream >> nbt::contexts::java;

        // 读取根复合标签
        auto root = nbt::CompoundTag::read(stream);
        if (!root) {
            return nullptr;
        }

        return loadFromNbt(*root);
    }
    catch (...) {
        return nullptr;
    }
}

std::unique_ptr<Template> TemplateLoader::loadFromBedrockMcStructure(const std::vector<u8>& data)
{
    // 基岩版 .mcstructure 是未压缩的小端序 NBT（bedrock_disk 上下文），无需 gzip 解压
    if (data.empty()) {
        return nullptr;
    }

    try {
        std::istringstream stream(std::string(data.begin(), data.end()));
        stream >> nbt::contexts::bedrock_disk;

        auto root = nbt::CompoundTag::read(stream);
        if (!root) {
            return nullptr;
        }

        return _loadFromBedrockNbt(*root);
    }
    catch (...) {
        return nullptr;
    }
}

std::unique_ptr<Template> TemplateLoader::_loadFromBedrockNbt(const nbt::CompoundTag& root)
{
    auto templ = std::make_unique<Template>();

    // size: List<Int> = [x, y, z]
    if (root.value.count("size") == 0) {
        return templ;
    }
    auto& sizeTag = *root.value.at("size");
    if (sizeTag.id() != nbt::TagId::List) {
        return templ;
    }
    auto& sizeList = dynamic_cast<const nbt::ListTag&>(sizeTag);
    if (sizeList.size() < 3) {
        return templ;
    }
    const i32 sizeX = dynamic_cast<const nbt::IntTag&>(*sizeList[0]).value;
    const i32 sizeY = dynamic_cast<const nbt::IntTag&>(*sizeList[1]).value;
    const i32 sizeZ = dynamic_cast<const nbt::IntTag&>(*sizeList[2]).value;
    templ->setSize(BlockPos(sizeX, sizeY, sizeZ));

    // structure: Compound { block_indices, palette{ default{ block_palette } }, entities }
    if (root.value.count("structure") == 0) {
        return templ;
    }
    auto& structureTag = *root.value.at("structure");
    if (structureTag.id() != nbt::TagId::Compound) {
        return templ;
    }
    const auto& structure = dynamic_cast<const nbt::CompoundTag&>(structureTag);

    // 解析 palette.default.block_palette -> 方块状态 ID 数组
    // palette 是 Compound，键为 palette 名（默认 "default"），值为 { block_palette, block_position_data }
    std::vector<u32> blockPaletteStates;
    if (structure.value.count("palette") != 0) {
        auto& palettesTag = *structure.value.at("palette");
        if (palettesTag.id() == nbt::TagId::Compound) {
            const auto& palettesCompound = dynamic_cast<const nbt::CompoundTag&>(palettesTag);
            // 取第一个 palette（通常只有一个 "default"）
            if (!palettesCompound.value.empty()) {
                auto firstIt = palettesCompound.value.begin();
                if (firstIt->second && firstIt->second->id() == nbt::TagId::Compound) {
                    const auto& paletteEntry = dynamic_cast<const nbt::CompoundTag&>(*firstIt->second);
                    if (paletteEntry.value.count("block_palette") != 0) {
                        auto& blockPaletteTag = *paletteEntry.value.at("block_palette");
                        if (blockPaletteTag.id() == nbt::TagId::List) {
                            const auto& blockPaletteList = dynamic_cast<const nbt::ListTag&>(blockPaletteTag);
                            blockPaletteStates.reserve(blockPaletteList.size());
                            for (size_t i = 0; i < blockPaletteList.size(); ++i) {
                                const auto& entryTagPtr = blockPaletteList[i];
                                const nbt::Tag* entryTag = entryTagPtr.get();
                                MC_ASSERT_RELEASE(entryTag != nullptr);
                                const auto& entry = dynamic_cast<const nbt::CompoundTag&>(*entryTag);
                                blockPaletteStates.push_back(_parseBedrockBlockStateId(entry));
                            }
                        }
                    }
                }
            }
        }
    }

    if (blockPaletteStates.empty()) {
        // 无 palette 则无法还原方块，返回空模板（仅 size）
        return templ;
    }

    // block_indices: List<List<Int>>，每层一个索引数组（对应一个 palette 变体）
    // 基岩版结构通常只有 1 层；多层时取第一层（多 palette 变体语义在项目 Template 中以 addPalette 表达，
    // 但 GameTest 结构无需变体，取首层即可）
    if (structure.value.count("block_indices") == 0) {
        return templ;
    }
    auto& blockIndicesTag = *structure.value.at("block_indices");
    if (blockIndicesTag.id() != nbt::TagId::List) {
        return templ;
    }
    const auto& blockIndicesList = dynamic_cast<const nbt::ListTag&>(blockIndicesTag);
    if (blockIndicesList.size() == 0) {
        return templ;
    }

    // 取第一层索引数组
    const auto& firstLayerTagPtr = blockIndicesList[0];
    const nbt::Tag* firstLayerTag = firstLayerTagPtr.get();
    if (firstLayerTag == nullptr || firstLayerTag->id() != nbt::TagId::List) {
        return templ;
    }
    const auto& indices = dynamic_cast<const nbt::ListTag&>(*firstLayerTag);

    // 结构原点偏移 structure_world_origin: List<Int> = [x, y, z]（可选，默认 0）
    BlockPos origin(0, 0, 0);
    if (root.value.count("structure_world_origin") != 0) {
        auto& originTag = *root.value.at("structure_world_origin");
        if (originTag.id() == nbt::TagId::List) {
            origin = _readBlockPos(dynamic_cast<const nbt::ListTag&>(originTag));
        }
    }

    // 基岩版 block_indices 按尺寸 [x, y, z] 的顺序线性存储（X 最外层、Z 最内层）
    // 索引值 -1 表示该位置为空气（基岩版用 -1 标记 air，不进 palette）
    std::vector<BlockInfo> blockInfos;
    blockInfos.reserve(indices.size());
    const size_t expectedCount = static_cast<size_t>(sizeX) * static_cast<size_t>(sizeY) * static_cast<size_t>(sizeZ);
    for (size_t i = 0; i < indices.size(); ++i) {
        const auto& idxTagPtr = indices[i];
        const nbt::Tag* idxTag = idxTagPtr.get();
        if (idxTag == nullptr) {
            continue;
        }
        const i32 idx = dynamic_cast<const nbt::IntTag&>(*idxTag).value;

        u32 stateId = 0; // 默认空气
        if (idx >= 0 && static_cast<size_t>(idx) < blockPaletteStates.size()) {
            stateId = blockPaletteStates[static_cast<size_t>(idx)];
        }

        // 线性索引 -> 三维坐标（X 外层、Z 内层，对齐基岩版存储顺序）
        const size_t totalXZ = static_cast<size_t>(sizeY) * static_cast<size_t>(sizeZ);
        const i32 x = origin.x + static_cast<i32>(i / totalXZ);
        const size_t remX = i % totalXZ;
        const i32 y = origin.y + static_cast<i32>(remX / static_cast<size_t>(sizeZ));
        const i32 z = origin.z + static_cast<i32>(remX % static_cast<size_t>(sizeZ));

        blockInfos.emplace_back(BlockPos(x, y, z), stateId);
    }

    // 若索引数与 size 体积不符，仍按已解析的方块构建（不强制断言，便于容错）
    templ->addPalette(Palette(std::move(blockInfos)));

    // TODO: 解析 structure.entities（基岩版实体 schema 与 Java 不同，GameTest 结构通常无实体，暂不解析）

    return templ;
}

u32 TemplateLoader::_parseBedrockBlockStateId(const nbt::CompoundTag& paletteEntry)
{
    // 基岩版 block_palette 项: { name: String, states: Compound, version: Int }
    // 字段名为小写 name / states（区别于 Java 的 Name / Properties）
    std::string blockName;
    if (paletteEntry.value.count("name") != 0) {
        blockName = dynamic_cast<const nbt::StringTag&>(*paletteEntry.value.at("name")).value;
    }

    if (blockName.empty()) {
        return 0; // 空气
    }

    auto& registry = BlockRegistry::instance();
    auto* block = registry.getBlock(ResourceLocation(blockName));
    if (!block) {
        return 0; // 未知方块，返回空气
    }

    const BlockState* state = &block->defaultState();

    // 基岩版 states 是 Compound，键为属性名、值为属性值标签（String/Int/Byte 等）
    // 复用 Java 版的 applyPropertiesToState 需要字符串值，故先把非 String 标签转字符串
    if (paletteEntry.value.count("states") != 0) {
        auto& statesTag = *paletteEntry.value.at("states");
        if (statesTag.id() == nbt::TagId::Compound) {
            const auto& statesCompound = dynamic_cast<const nbt::CompoundTag&>(statesTag);
            // 构造一个全 String 值的临时 Compound 供 applyPropertiesToState 消费
            auto stringProps = std::make_unique<nbt::CompoundTag>();
            for (const auto& [key, valueTag] : statesCompound.value) {
                if (!valueTag) {
                    continue;
                }
                std::string strVal;
                switch (valueTag->id()) {
                    case nbt::TagId::String:
                        strVal = dynamic_cast<const nbt::StringTag&>(*valueTag).value;
                        break;
                    case nbt::TagId::Int:
                        strVal = std::to_string(dynamic_cast<const nbt::IntTag&>(*valueTag).value);
                        break;
                    case nbt::TagId::Byte: {
                        // 基岩版布尔属性用 Byte (0/1)，Java 用 "true"/"false" 字符串
                        const i8 bv = dynamic_cast<const nbt::ByteTag&>(*valueTag).value;
                        strVal = (bv != 0) ? "true" : "false";
                        break;
                    }
                    default:
                        continue; // 跳过不支持的属性值类型
                }
                stringProps->value.emplace(key, std::make_unique<nbt::StringTag>(strVal));
            }
            if (!stringProps->value.empty()) {
                state = applyPropertiesToState(*block, state, *stringProps);
            }
        }
    }

    return state->stateId();
}

BlockPos TemplateLoader::_readBlockPos(const nbt::ListTag& list)
{
    i32 x = 0, y = 0, z = 0;

    if (list.size() >= 3) {
        x = dynamic_cast<const nbt::IntTag&>(*list[0]).value;
        y = dynamic_cast<const nbt::IntTag&>(*list[1]).value;
        z = dynamic_cast<const nbt::IntTag&>(*list[2]).value;
    }

    return BlockPos(x, y, z);
}

std::unique_ptr<nbt::CompoundTag> TemplateLoader::_cloneNbt(const nbt::CompoundTag* source) noexcept
{
    if (!source) {
        return nullptr;
    }
    return std::make_unique<nbt::CompoundTag>(*source);
}

u32 TemplateLoader::_parseBlockStateId(const nbt::CompoundTag& paletteEntry)
{
    // NBT 格式:
    // Name: "minecraft:stone"
    // Properties: { ... } (可选)

    std::string blockName;
    if (paletteEntry.value.count("Name") != 0) {
        blockName = dynamic_cast<const nbt::StringTag&>(*paletteEntry.value.at("Name")).value;
    }

    if (blockName.empty()) {
        return 0; // 空气
    }

    // 从 BlockRegistry 获取方块
    auto& registry = BlockRegistry::instance();
    auto* block = registry.getBlock(ResourceLocation(blockName));
    if (!block) {
        return 0; // 未知方块，返回空气
    }

    // 获取默认状态
    const BlockState* state = &block->defaultState();

    // 如果有属性，尝试应用
    if (paletteEntry.value.count("Properties") != 0) {
        auto& propsTag = *paletteEntry.value.at("Properties");
        if (propsTag.id() == nbt::TagId::Compound) {
            auto& propsCompound = dynamic_cast<const nbt::CompoundTag&>(propsTag);
            state = applyPropertiesToState(*block, state, propsCompound);
        }
    }

    return state->stateId();
}

TemplateJigsawBlockInfo TemplateLoader::_parseJigsawBlock(
    const nbt::CompoundTag* nbt, const BlockPos& pos, u32 blockStateId)
{
    TemplateJigsawBlockInfo info;
    info.pos = pos;
    info.blockStateId = blockStateId;

    if (!nbt) {
        return info;
    }

    // Jigsaw 方块的 NBT 结构:
    // id: "minecraft:jigsaw"
    // name: "minecraft:bottom" (连接点名称)
    // target_pool: "minecraft:village/street" (目标模板池)
    // target_name: "minecraft:empty" 或具体名称
    // joint: "rollable" 或 "aligned"

    // 检查是否是 Jigsaw 方块
    if (nbt->value.count("id") != 0) {
        std::string id = dynamic_cast<const nbt::StringTag&>(*nbt->value.at("id")).value;
        if (id != "minecraft:jigsaw" && id != "jigsaw") {
            return info; // 不是 Jigsaw 方块
        }
    } else {
        return info;
    }

    // 读取连接点名称
    if (nbt->value.count("name") != 0) {
        info.name = dynamic_cast<const nbt::StringTag&>(*nbt->value.at("name")).value;
    }

    // 读取目标模板池
    if (nbt->value.count("target_pool") != 0) {
        info.targetPool = dynamic_cast<const nbt::StringTag&>(*nbt->value.at("target_pool")).value;
    }

    // 读取目标连接点名称
    if (nbt->value.count("target_name") != 0) {
        info.targetName = dynamic_cast<const nbt::StringTag&>(*nbt->value.at("target_name")).value;
    }

    // 读取连接类型
    if (nbt->value.count("joint") != 0) {
        std::string joint = dynamic_cast<const nbt::StringTag&>(*nbt->value.at("joint")).value;
        info.jointType = (joint == "aligned") ? 1 : 0;
    }

    // 读取优先级（对应 MC 1.21 JigsawBlockEntity NBT 的 placement_priority/selection_priority，默认 0）
    // placement_priority: 该连接点生成的子拼图块在组装队列中的出队优先级（降序）
    // selection_priority: 同一拼图块内连接点的处理顺序（降序）
    if (nbt->value.count("placement_priority") != 0) {
        info.placementPriority = dynamic_cast<const nbt::IntTag&>(*nbt->value.at("placement_priority")).value;
    }
    if (nbt->value.count("selection_priority") != 0) {
        info.selectionPriority = dynamic_cast<const nbt::IntTag&>(*nbt->value.at("selection_priority")).value;
    }

    return info;
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
