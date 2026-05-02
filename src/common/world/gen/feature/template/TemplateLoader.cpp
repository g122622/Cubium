#include "TemplateLoader.hpp"
#include "../../../../resource/IResourcePack.hpp"
#include "../../../../util/CompressionUtils.hpp"
#include "../../../block/Block.hpp"
#include "../../../block/BlockRegistry.hpp"
#include <sstream>
#include <cstring>
#include <unordered_map>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

namespace {

const BlockState* applyPropertiesToState(
    const Block& block,
    const BlockState* defaultState,
    const nbt::CompoundTag& propsCompound)
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

        const String& value = dynamic_cast<const nbt::StringTag&>(*valueTag).value;
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
            const auto it = candidate->values().find(prop);
            if (it == candidate->values().end() || it->second != index) {
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

std::unique_ptr<Template> TemplateLoader::loadFromNbt(const nbt::CompoundTag& nbt) {
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

    // 读取方块调色板
    // MC 1.16.5: 支持两种格式
    // 1. palette: 单一调色板列表
    // 2. palettes: 多个调色板列表的列表（用于结构变体）
    // 参考 Template.read -> readPalletesAndBlocks
    std::vector<u32> palette;
    bool hasPalette = false;

    if (nbt.value.count("palettes") != 0) {
        // 多调色板格式：palettes 是一个列表的列表
        auto& palettesTag = *nbt.value.at("palettes");
        if (palettesTag.id() == nbt::TagId::List) {
            auto& palettesList = dynamic_cast<const nbt::ListTag&>(palettesTag);
            if (palettesList.size() > 0) {
                // MC 1.16.5 默认使用第一个 palette
                // 完整实现应该根据结构配置选择不同的 palette
                auto& firstPaletteTag = *palettesList[0];
                if (firstPaletteTag.id() == nbt::TagId::List) {
                    auto& firstPalette = dynamic_cast<const nbt::ListTag&>(firstPaletteTag);
                    palette.reserve(firstPalette.size());
                    for (size_t i = 0; i < firstPalette.size(); ++i) {
                        auto& entry = dynamic_cast<const nbt::CompoundTag&>(*firstPalette[i]);
                        palette.push_back(parseBlockStateId(entry));
                    }
                    hasPalette = true;
                }
            }
        }
    }

    // 如果没有 palettes，尝试单 palette 格式
    if (!hasPalette && nbt.value.count("palette") != 0) {
        auto& paletteTag = *nbt.value.at("palette");
        if (paletteTag.id() == nbt::TagId::List) {
            auto& paletteList = dynamic_cast<const nbt::ListTag&>(paletteTag);
            palette.reserve(paletteList.size());
            for (size_t i = 0; i < paletteList.size(); ++i) {
                auto& entry = dynamic_cast<const nbt::CompoundTag&>(*paletteList[i]);
                palette.push_back(parseBlockStateId(entry));
            }
            hasPalette = true;
        }
    }

    // 读取方块: blocks: [...]
    if (nbt.value.count("blocks") != 0) {
        auto& blocksTag = *nbt.value.at("blocks");
        if (blocksTag.id() == nbt::TagId::List) {
            auto& blocksList = dynamic_cast<const nbt::ListTag&>(blocksTag);
            for (size_t i = 0; i < blocksList.size(); ++i) {
                auto& blockEntry = dynamic_cast<const nbt::CompoundTag&>(*blocksList[i]);

                // 读取位置: pos: [x, y, z]
                BlockPos pos;
                if (blockEntry.value.count("pos") != 0) {
                    auto& posTag = *blockEntry.value.at("pos");
                    if (posTag.id() == nbt::TagId::List) {
                        pos = readBlockPos(dynamic_cast<const nbt::ListTag&>(posTag));
                    }
                }

                // 读取状态索引: state: int
                u32 stateId = 0;
                if (blockEntry.value.count("state") != 0) {
                    stateId = static_cast<u32>(
                        dynamic_cast<const nbt::IntTag&>(*blockEntry.value.at("state")).value);
                    if (stateId < palette.size()) {
                        stateId = palette[stateId];
                    }
                }

                BlockInfo blockInfo(pos, stateId);

                // 读取 NBT 数据: nbt: {...}
                if (blockEntry.value.count("nbt") != 0) {
                    auto& nbtTag = *blockEntry.value.at("nbt");
                    if (nbtTag.id() == nbt::TagId::Compound) {
                        const nbt::CompoundTag* nbtPtr = dynamic_cast<const nbt::CompoundTag*>(&nbtTag);
                        blockInfo.nbt = cloneNbt(nbtPtr);

                        // 检查是否是 Jigsaw 方块
                        auto jigsawInfo = parseJigsawBlock(blockInfo.nbt.get(), pos);
                        if (!jigsawInfo.name.empty()) {
                            templ->addJigsawBlock(jigsawInfo);
                        }
                    }
                }

                templ->addBlock(blockInfo);
            }
        }
    }

    // 读取实体: entities: [...]
    // MC 1.16.5: Template.readEntities
    // 实体有两个位置：
    // - pos: Double 列表（精确位置）
    // - blockPos: Int 列表（方块坐标）
    if (nbt.value.count("entities") != 0) {
        auto& entitiesTag = *nbt.value.at("entities");
        if (entitiesTag.id() == nbt::TagId::List) {
            auto& entitiesList = dynamic_cast<const nbt::ListTag&>(entitiesTag);
            for (size_t i = 0; i < entitiesList.size(); ++i) {
                auto& entityEntry = dynamic_cast<const nbt::CompoundTag&>(*entitiesList[i]);

                TemplateEntityInfo entityInfo;

                // 读取实体类型
                if (entityEntry.value.count("id") != 0) {
                    entityInfo.typeId = dynamic_cast<const nbt::StringTag&>(
                        *entityEntry.value.at("id")).value;
                }

                // 读取精确位置 pos: [double, double, double]
                // MC 1.16.5: Template.readDoubles
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
                // MC 1.16.5: Template.readInts
                if (entityEntry.value.count("blockPos") != 0) {
                    auto& blockPosTag = *entityEntry.value.at("blockPos");
                    if (blockPosTag.id() == nbt::TagId::List) {
                        entityInfo.blockPos = readBlockPos(dynamic_cast<const nbt::ListTag&>(blockPosTag));
                    }
                }

                // 读取 NBT
                if (entityEntry.value.count("nbt") != 0) {
                    auto& nbtTag = *entityEntry.value.at("nbt");
                    if (nbtTag.id() == nbt::TagId::Compound) {
                        const nbt::CompoundTag* nbtPtr = dynamic_cast<const nbt::CompoundTag*>(&nbtTag);
                        entityInfo.nbt = cloneNbt(nbtPtr);
                    }
                }

                templ->addEntity(entityInfo);
            }
        }
    }

    return templ;
}

std::unique_ptr<Template> TemplateLoader::loadFromResourcePack(
    const IResourcePack& pack,
    const ResourceLocation& location)
{
    // 构建资源路径: assets/<namespace>/structures/<path>.nbt
    // 参考 MC 1.16.5 TemplateManager.func_227458_a_
    const std::string basePath = "assets/" + location.namespace_() + "/structures/" + location.path();
    std::string path = basePath + ".nbt";

    // 尝试读取资源
    auto result = pack.readResource(path);
    if (!result.success() || result.value().empty()) {
        // 尝试不带 .nbt 后缀
        result = pack.readResource(basePath);
        if (!result.success() || result.value().empty()) {
            return nullptr;
        }
    }

    return loadFromCompressedNbt(result.value());
}

std::unique_ptr<Template> TemplateLoader::loadFromCompressedNbt(const std::vector<u8>& data) {
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
    } catch (...) {
        return nullptr;
    }
}

BlockPos TemplateLoader::readBlockPos(const nbt::ListTag& list) {
    i32 x = 0, y = 0, z = 0;

    if (list.size() >= 3) {
        x = dynamic_cast<const nbt::IntTag&>(*list[0]).value;
        y = dynamic_cast<const nbt::IntTag&>(*list[1]).value;
        z = dynamic_cast<const nbt::IntTag&>(*list[2]).value;
    }

    return BlockPos(x, y, z);
}

std::unique_ptr<nbt::CompoundTag> TemplateLoader::cloneNbt(const nbt::CompoundTag* source) {
    if (!source) {
        return nullptr;
    }
    return std::make_unique<nbt::CompoundTag>(*source);
}

u32 TemplateLoader::parseBlockStateId(const nbt::CompoundTag& paletteEntry) {
    // NBT 格式:
    // Name: "minecraft:stone"
    // Properties: { ... } (可选)

    std::string blockName;
    if (paletteEntry.value.count("Name") != 0) {
        blockName = dynamic_cast<const nbt::StringTag&>(
            *paletteEntry.value.at("Name")).value;
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

TemplateJigsawBlockInfo TemplateLoader::parseJigsawBlock(
    const nbt::CompoundTag* nbt,
    const BlockPos& pos)
{
    TemplateJigsawBlockInfo info;
    info.pos = pos;

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
        info.targetPool = dynamic_cast<const nbt::StringTag&>(
            *nbt->value.at("target_pool")).value;
    }

    // 读取目标连接点名称
    if (nbt->value.count("target_name") != 0) {
        info.targetName = dynamic_cast<const nbt::StringTag&>(
            *nbt->value.at("target_name")).value;
    }

    // 读取连接类型
    if (nbt->value.count("joint") != 0) {
        std::string joint = dynamic_cast<const nbt::StringTag&>(*nbt->value.at("joint")).value;
        info.jointType = (joint == "aligned") ? 1 : 0;
    }

    return info;
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
