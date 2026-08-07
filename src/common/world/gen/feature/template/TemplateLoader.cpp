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
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

namespace {

// 基岩版整型属性 → Java 属性 (key, value) 字符串。
// 已覆盖：
// - facing_direction (int 0-5: 0=down,1=up,2=north,3=south,4=east,5=west)：基岩通用 6 向 → Java facing
// - weirdo_direction (int 0-3: 0=east,1=west,2=south,3=north)：基岩 stairs 朝向 → Java stairs facing
// - rail_direction (int 0-9)：基岩 rail_direction → Java rail "shape"（一一对应）
//   0=north_south,1=east_west,2=ascending_east,3=ascending_west,4=ascending_north,5=ascending_south,
//   6=south_east,7=south_west,8=north_west,9=north_east
// - direction (int 0-3: 0=north,1=east,2=south,3=west,基岩 CommonDirection)：门/楼梯/按钮等通用朝向
//   → Java facing（vanilla HORIZONTAL_FACING 4 向，north/south/east/west）
// 返回 nullopt 表示该属性名不在已知映射表内，由调用方走默认数字字符串转换。
// TODO: 部分复合方块（如 lever 的门控/朝向组合属性）仍待补全。
std::optional<std::pair<std::string, std::string>> bedrockIntStateToJava(const std::string& bedrockKey, i32 intVal)
{
    if (bedrockKey == "facing_direction") {
        switch (intVal) {
            case 0:
                return std::make_pair(std::string("facing"), std::string("down"));
            case 1:
                return std::make_pair(std::string("facing"), std::string("up"));
            case 2:
                return std::make_pair(std::string("facing"), std::string("north"));
            case 3:
                return std::make_pair(std::string("facing"), std::string("south"));
            case 4:
                return std::make_pair(std::string("facing"), std::string("east"));
            case 5:
                return std::make_pair(std::string("facing"), std::string("west"));
            default:
                break;
        }
    } else if (bedrockKey == "weirdo_direction") {
        // 基岩 stairs 朝向 → Java stairs facing
        switch (intVal) {
            case 0:
                return std::make_pair(std::string("facing"), std::string("east"));
            case 1:
                return std::make_pair(std::string("facing"), std::string("west"));
            case 2:
                return std::make_pair(std::string("facing"), std::string("south"));
            case 3:
                return std::make_pair(std::string("facing"), std::string("north"));
            default:
                break;
        }
    } else if (bedrockKey == "rail_direction") {
        // 基岩 rail_direction 0-9 与 Java RailShape 字符串值一一对应（属性名 shape）
        static const char* kRailShapes[10] = {"north_south",
            "east_west",
            "ascending_east",
            "ascending_west",
            "ascending_north",
            "ascending_south",
            "south_east",
            "south_west",
            "north_west",
            "north_east"};
        if (intVal >= 0 && intVal <= 9) {
            return std::make_pair(std::string("shape"), std::string(kRailShapes[intVal]));
        }
    } else if (bedrockKey == "direction") {
        // 基岩 CommonDirection 0-3: 0=North,1=East,2=South,3=West → Java facing
        // 注意：vanilla 门/楼梯的 facing 表示"方块朝向哪一边"，与基岩 direction 语义一致直接映射。
        switch (intVal) {
            case 0:
                return std::make_pair(std::string("facing"), std::string("north"));
            case 1:
                return std::make_pair(std::string("facing"), std::string("east"));
            case 2:
                return std::make_pair(std::string("facing"), std::string("south"));
            case 3:
                return std::make_pair(std::string("facing"), std::string("west"));
            default:
                break;
        }
    } else if (bedrockKey == "height") {
        // 基岩 minecraft:snow_layer 的 height（Int 0-7，0=最薄1层）→ Java minecraft:snow 的 layers（1-8）。
        // 基岩 height=N 表示 N+1 层雪（与 Java layers 语义偏移 1）。映射后由 bedrockBlockNameToJava
        // 把 snow_layer 改名为 snow，此处仅做属性名+值转换。
        if (intVal >= 0 && intVal <= 7) {
            return std::make_pair(std::string("layers"), std::to_string(intVal + 1));
        }
    }
    return std::nullopt;
}

// 基岩版字节型属性 → Java 属性 (key, value) 字符串。
// 已覆盖：
// - upside_down_bit (byte 0/1)：基岩 stairs/slab 倒置标志 0=正放,1=倒置 → Java half (bottom/top)
//   （stairs 用 Half::Top/Bottom，slab 用 SlabType 但属性名不同，此处仅映射为 stairs 的 half；
//   slab 的 type 属性映射留待补全）
// - upper_block_bit (byte 0/1)：基岩门上下半 0=下半,1=上半 → Java half (lower/upper)（DoubleBlockHalf）
// - door_hinge_bit (byte 0/1)：基岩门铰链 0=左,1=右 → Java hinge (left/right)（DoorHinge）
// - open_bit (byte 0/1)：基岩门/栅栏门/活板门开合 → Java open (true/false)（属性名重命名 + 布尔值）
// - button_pressed_bit (byte 0/1)：基岩按钮按下 → Java powered (true/false)
// - rail_data_bit (byte 0/1)：基岩 golden_rail/activator_rail/detector_rail 充能位 → Java powered (true/false)
// - conditional_bit (byte 0/1)：基岩命令方块条件 → Java conditional (true/false)
// 返回 nullopt 表示该属性名不在已知映射表内，由调用方走默认布尔转换逻辑。
// TODO: 基岩字节属性到 Java 枚举的映射仅覆盖上述常见项，其他（如 top_slot_bit、direction 的 byte
// 变体等）需逐类补全。
std::optional<std::pair<std::string, std::string>> bedrockByteStateToJava(const std::string& bedrockKey, i8 byteVal)
{
    const bool b = (byteVal != 0);
    if (bedrockKey == "upside_down_bit") {
        // 0=正放(bottom), 1=倒置(top)
        return std::make_pair(std::string("half"), b ? std::string("top") : std::string("bottom"));
    }
    if (bedrockKey == "upper_block_bit") {
        // 门：0=下半(lower), 1=上半(upper)
        return std::make_pair(std::string("half"), b ? std::string("upper") : std::string("lower"));
    }
    if (bedrockKey == "door_hinge_bit") {
        // 门铰链：0=左(left), 1=右(right)
        return std::make_pair(std::string("hinge"), b ? std::string("right") : std::string("left"));
    }
    if (bedrockKey == "open_bit") {
        // 门/栅栏门/活板门开合：重命名为 open，值 0/1 → false/true
        return std::make_pair(std::string("open"), b ? std::string("true") : std::string("false"));
    }
    if (bedrockKey == "button_pressed_bit") {
        // 按钮/压力板按下：重命名为 powered，值 0/1 → false/true
        return std::make_pair(std::string("powered"), b ? std::string("true") : std::string("false"));
    }
    if (bedrockKey == "rail_data_bit") {
        // 基岩 golden_rail/activator_rail/detector_rail 的充能位 rail_data_bit（byte 0/1）
        // → Java powered（动力铁轨是否被红石充能）。重命名 + 布尔值转换。
        return std::make_pair(std::string("powered"), b ? std::string("true") : std::string("false"));
    }
    if (bedrockKey == "conditional_bit") {
        // 命令方块条件：重命名为 conditional，值 0/1 → false/true
        return std::make_pair(std::string("conditional"), b ? std::string("true") : std::string("false"));
    }
    return std::nullopt;
}

// 基岩版方块名 → Java 方块名映射（基岩用统一方块名 + 属性区分变体，Java 用独立方块名）。
// 返回 {javaBlockName, 属性重命名映射}。属性重命名映射：基岩属性名 → Java 属性名（值不变）。
// 已覆盖：
// - minecraft:log + old_log_type → minecraft:<type>_log（+ pillar_axis→axis）
// - minecraft:planks + wood_type → minecraft:<type>_planks
// - minecraft:fence + wood_type → minecraft:<type>_fence
// - minecraft:wood + wood_type(+stripped_bit) → minecraft:[stripped_]<type>_wood
// - minecraft:leaves + old_leaf_type → minecraft:<type>_leaves
// - minecraft:stone + stone_type → minecraft:stone/andesite/granite/diorite/polished_*
// - minecraft:dirt + dirt_type → minecraft:dirt/coarse_dirt
// - minecraft:sand + sand_type → minecraft:sand/red_sand
// - minecraft:double_plant + double_plant_type → minecraft:sunflower/lilac/tall_grass/...
// - minecraft:stained_glass_pane + color → minecraft:<color>_stained_glass_pane
// - 纯改名（无变体）：wooden_door→oak_door、golden_rail→powered_rail、lit_pumpkin→jack_o_lantern、
//   grass→short_grass、grass_path→dirt_path
// TODO: minecraft:sapling 等基岩独有方块仍待补全映射。
// 详见 GameTest 结构扫描结论（memory: gametest-stairs-orientation-filtered-clone-fix）。
struct BedrockBlockMapping {
    std::string javaName;                                         // 映射后的 Java 方块名（空表示无需改名）
    std::vector<std::pair<std::string, std::string>> propRenames; // 基岩属性名 → Java 属性名
};

// 从基岩 states Compound 中读取 string 状态值（不存在返回 default）
std::string bedrockStringState(const nbt::CompoundTag& states, const std::string& key, const std::string& def)
{
    auto it = states.value.find(key);
    if (it == states.value.end() || !it->second || it->second->id() != nbt::TagId::String) {
        return def;
    }
    return dynamic_cast<const nbt::StringTag&>(*it->second).value;
}

// 从基岩 states Compound 中读取 byte 状态值（不存在返回 default）
i8 bedrockByteState(const nbt::CompoundTag& states, const std::string& key, i8 def)
{
    auto it = states.value.find(key);
    if (it == states.value.end() || !it->second || it->second->id() != nbt::TagId::Byte) {
        return def;
    }
    return dynamic_cast<const nbt::ByteTag&>(*it->second).value;
}

BedrockBlockMapping bedrockBlockNameToJava(const std::string& bedrockName, const nbt::CompoundTag& bedrockStates)
{
    if (bedrockName == "minecraft:log") {
        // 基岩 minecraft:log + old_log_type=<oak|spruce|birch|jungle|acacia|dark_oak> + pillar_axis
        // → Java minecraft:<type>_log + axis（pillar_axis 值 y/x/z 与 Java axis 一致）
        std::string woodType = bedrockStringState(bedrockStates, "old_log_type", "oak");
        return {std::string("minecraft:") + woodType + "_log", {{"pillar_axis", "axis"}}};
    }
    if (bedrockName == "minecraft:planks") {
        // minecraft:planks + wood_type → minecraft:<type>_planks
        std::string woodType = bedrockStringState(bedrockStates, "wood_type", "oak");
        return {std::string("minecraft:") + woodType + "_planks", {}};
    }
    if (bedrockName == "minecraft:fence") {
        std::string woodType = bedrockStringState(bedrockStates, "wood_type", "oak");
        return {std::string("minecraft:") + woodType + "_fence", {}};
    }
    if (bedrockName == "minecraft:wood") {
        // minecraft:wood + wood_type + stripped_bit → minecraft:[stripped_]<type>_wood
        std::string woodType = bedrockStringState(bedrockStates, "wood_type", "oak");
        bool stripped = bedrockByteState(bedrockStates, "stripped_bit", 0) != 0;
        std::string javaName = stripped ? (std::string("minecraft:stripped_") + woodType + "_wood")
                                        : (std::string("minecraft:") + woodType + "_wood");
        // pillar_axis→axis（值 y/x/z 一致）
        return {javaName, {{"pillar_axis", "axis"}}};
    }
    if (bedrockName == "minecraft:leaves") {
        // minecraft:leaves + old_leaf_type → minecraft:<type>_leaves
        // （基岩 old_leaf_type: oak/spruce/birch/jungle，Java 同名 leaves 变体）
        std::string leafType = bedrockStringState(bedrockStates, "old_leaf_type", "oak");
        return {std::string("minecraft:") + leafType + "_leaves", {}};
    }
    if (bedrockName == "minecraft:stone") {
        // 基岩 stone + stone_type →
        // Java：stone/granite/diorite/andesite/polished_granite/polished_diorite/polished_andesite 基岩 stone_type:
        // stone=0/granite=1/granite_smooth=2/diorite=3/diorite_smooth=4/andesite=5/andesite_smooth=6 但 stone_type
        // 在结构里是字符串值（"stone"/"granite"/"diorite"/"andesite"/"granite_smooth"/...） Java
        // 方块名映射：granite_smooth→polished_granite, diorite_smooth→polished_diorite,
        // andesite_smooth→polished_andesite
        static const std::unordered_map<std::string, std::string> kStoneTypeMap = {
            {"stone", "stone"},
            {"granite", "granite"},
            {"granite_smooth", "polished_granite"},
            {"diorite", "diorite"},
            {"diorite_smooth", "polished_diorite"},
            {"andesite", "andesite"},
            {"andesite_smooth", "polished_andesite"},
        };
        std::string stoneType = bedrockStringState(bedrockStates, "stone_type", "stone");
        auto it = kStoneTypeMap.find(stoneType);
        return {std::string("minecraft:") + (it != kStoneTypeMap.end() ? it->second : "stone"), {}};
    }
    if (bedrockName == "minecraft:dirt") {
        // 基岩 dirt + dirt_type(normal/coarse) → minecraft:dirt / minecraft:coarse_dirt
        std::string dirtType = bedrockStringState(bedrockStates, "dirt_type", "normal");
        return {dirtType == "coarse" ? "minecraft:coarse_dirt" : "minecraft:dirt", {}};
    }
    if (bedrockName == "minecraft:sand") {
        // 基岩 sand + sand_type(normal/red) → minecraft:sand / minecraft:red_sand
        std::string sandType = bedrockStringState(bedrockStates, "sand_type", "normal");
        return {sandType == "red" ? "minecraft:red_sand" : "minecraft:sand", {}};
    }
    if (bedrockName == "minecraft:double_plant") {
        // 基岩 double_plant + double_plant_type → Java 独立方块名
        static const std::unordered_map<std::string, std::string> kDoublePlantMap = {
            {"sunflower", "sunflower"},
            {"syringa", "lilac"}, // 基岩 syringa = Java lilac
            {"grass", "tall_grass"},
            {"fern", "large_fern"},
            {"rose", "rose_bush"},
            {"paeonia", "peony"},
        };
        std::string plantType = bedrockStringState(bedrockStates, "double_plant_type", "sunflower");
        auto it = kDoublePlantMap.find(plantType);
        return {std::string("minecraft:") + (it != kDoublePlantMap.end() ? it->second : "sunflower"), {}};
    }
    if (bedrockName == "minecraft:stained_glass_pane") {
        // 基岩 stained_glass_pane + color → minecraft:<color>_stained_glass_pane
        std::string color = bedrockStringState(bedrockStates, "color", "white");
        return {std::string("minecraft:") + color + "_stained_glass_pane", {}};
    }
    if (bedrockName == "minecraft:snow") {
        // 基岩 minecraft:snow（无 height 属性）= 完整实心雪块（SnowBlock，full solid），
        // 与 Java minecraft:snow（SnowLayerBlock，LAYERS 1-8 分层雪，2px 薄板 .notSolid()）是不同方块。
        // 必须映射到 Java minecraft:snow_block（SimpleBlock 实心），否则铁轨等需刚性支撑的方块放在其上时
        // canSupportRigidBlock 返回 false（2px 雪层不可支撑），被 neighborChanged 误判无支撑而移除
        // （GameTest minibiomes 矿车在 snow 上的 r7 弯轨脱轨根因）。基岩 states 为空，无需属性映射。
        return {"minecraft:snow_block", {}};
    }
    if (bedrockName == "minecraft:snow_layer") {
        // 基岩 minecraft:snow_layer（TopSnowBlock，带 height Int 0-7）= Java minecraft:snow（SnowLayerBlock，
        // LAYERS 1-8）。属性 height→layers 由 bedrockIntStateToJava 做值转换（height+1→layers），
        // 此处仅声明属性名重命名。covered_bit 是基岩独有（草路径上的雪覆盖标记），Java 无对应属性，
        // applyPropertiesToState 会忽略未知属性。
        return {"minecraft:snow", {{"height", "layers"}}};
    }
    // 纯改名（无变体属性）
    static const std::unordered_map<std::string, std::string> kRenames = {
        {"minecraft:wooden_door", "minecraft:oak_door"},
        {"minecraft:golden_rail", "minecraft:powered_rail"},
        {"minecraft:lit_pumpkin", "minecraft:jack_o_lantern"},
        {"minecraft:grass", "minecraft:short_grass"},    // 1.21 改名
        {"minecraft:grass_path", "minecraft:dirt_path"}, // 1.17 改名
    };
    auto it = kRenames.find(bedrockName);
    if (it != kRenames.end()) {
        return {it->second, {}};
    }
    return {};
}

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

        // 解包空键嵌套层（见 _unwrapRootCompound 注释：NBT 库不跳过根 id+name，
        // Java 结构 .nbt 根 name 为空，root = {"": <真内容>}，须解包才能取到 size/blocks/palette）。
        return loadFromNbt(*_unwrapRootCompound(*root));
    }
    catch (...) {
        return nullptr;
    }
}

std::unique_ptr<Template> TemplateLoader::loadFromBedrockMcStructure(const std::vector<u8>& data)
{
    // 基岩版 .mcstructure 是未压缩的小端序 NBT（bedrock_disk 上下文），无需 gzip 解压。
    if (data.empty()) {
        spdlog::warn("GameTest: loadFromBedrockMcStructure empty data");
        return nullptr;
    }

    try {
        std::istringstream stream(std::string(data.begin(), data.end()));
        stream >> nbt::contexts::bedrock_disk;

        auto root = nbt::CompoundTag::read(stream);
        if (!root) {
            spdlog::warn("GameTest: .mcstructure CompoundTag::read returned null, size={}", data.size());
            return nullptr;
        }

        // 解包空键嵌套层（见 _unwrapRootCompound 注释：.mcstructure 根 name 为空，
        // root = {"": <真内容>}，须解包才能取到 size/structure 等键）。
        return _loadFromBedrockNbt(*_unwrapRootCompound(*root));
    }
    catch (const std::exception& e) {
        spdlog::warn("GameTest: loadFromBedrockMcStructure exception: {}", e.what());
        return nullptr;
    }
    catch (...) {
        spdlog::warn("GameTest: loadFromBedrockMcStructure unknown exception");
        return nullptr;
    }
}

const nbt::CompoundTag* TemplateLoader::_unwrapRootCompound(const nbt::CompoundTag& root) noexcept
{
    // NBT 库 read_compound_bin 不跳过根 id+name，把根 0x0A+name 当 body 第一个子项。
    // 结构文件根 name 恒为空，故 root = {"": <真内容>}。此处检测单空键 + Compound 值并解包。
    if (root.value.size() == 1) {
        auto it = root.value.find("");
        if (it != root.value.end() && it->second && it->second->id() == nbt::TagId::Compound) {
            return dynamic_cast<const nbt::CompoundTag*>(it->second.get());
        }
    }
    return &root;
}

std::unique_ptr<Template> TemplateLoader::_loadFromBedrockNbt(const nbt::CompoundTag& root)
{
    auto templ = std::make_unique<Template>();

    // size: List<Int> = [x, y, z]
    if (root.value.count("size") == 0) {
        spdlog::warn("GameTest: .mcstructure missing 'size' key");
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
        // 无 palette 则无法还原方块，返回空模板（仅 size）。
        spdlog::warn("GameTest: .mcstructure block palette empty");
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

    // structure_world_origin: 结构保存时所在的世界坐标（仅元信息，记录结构方块在世界中的原位）。
    // 放置结构到新位置时应忽略它——block_indices 的索引→坐标映射须从结构内相对坐标 (0,0,0) 起，
    // 由 Template::placeInWorld 叠加 placeOrigin 决定最终世界坐标。此前误把 structure_world_origin
    // 当作 BlockInfo.pos 的 origin 叠加，致 button 落到 (68,5,46)+结构内坐标 的错乱位置，
    // helper 按 origin+(rel) 取到的是 deepslate/air 而非 button。origin 此处仅用于把
    // block_entity_data 内的"保存时世界绝对坐标"换算回结构内相对坐标以匹配 BlockInfo。
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

        // 线性索引 -> 结构内相对坐标（X 外层、Z 内层，对齐基岩版存储顺序；不含 structure_world_origin）
        const size_t totalXZ = static_cast<size_t>(sizeY) * static_cast<size_t>(sizeZ);
        const i32 x = static_cast<i32>(i / totalXZ);
        const size_t remX = i % totalXZ;
        const i32 y = static_cast<i32>(remX / static_cast<size_t>(sizeZ));
        const i32 z = static_cast<i32>(remX % static_cast<size_t>(sizeZ));

        blockInfos.emplace_back(BlockPos(x, y, z), stateId);
    }

    // 解析 palette.default.block_position_data.<index>.block_entity_data（基岩版方块实体 NBT）
    // schema 见 https://wiki.bedrock.dev/nbt/mcstructure ：block_position_data 是 Compound，键为 block_indices
    // 数组的位置下标（十进制字符串），值为 { block_entity_data: Compound }。block_entity_data 内含 id（如
    // "CommandBlock"）+ 方块实体字段（Command/LPCommandMode 等）+ x/y/z。
    // 注意：block_entity_data 的 x/y/z 是结构保存时的"世界绝对坐标"（含 structure_world_origin），
    // 而 BlockInfo.pos 已统一为结构内相对坐标（不含 origin），故匹配时须用 bed.xyz - origin 换算。
    // 此前仅解析了 block_palette + block_indices（方块状态），方块实体数据全丢，导致命令方块 Command 字段为空。
    // 命中则 clone block_entity_data 到 BlockInfo.nbt，由 Template::placeInWorld 调 loadFromNBT 注入。
    if (structure.value.count("palette") != 0) {
        auto& palettesTag2 = *structure.value.at("palette");
        if (palettesTag2.id() == nbt::TagId::Compound) {
            const auto& palettesCompound2 = dynamic_cast<const nbt::CompoundTag&>(palettesTag2);
            if (!palettesCompound2.value.empty()) {
                auto firstIt2 = palettesCompound2.value.begin();
                if (firstIt2->second && firstIt2->second->id() == nbt::TagId::Compound) {
                    const auto& paletteEntry2 = dynamic_cast<const nbt::CompoundTag&>(*firstIt2->second);
                    if (paletteEntry2.value.count("block_position_data") != 0) {
                        auto& bpdTag = *paletteEntry2.value.at("block_position_data");
                        if (bpdTag.id() == nbt::TagId::Compound) {
                            const auto& bpd = dynamic_cast<const nbt::CompoundTag&>(bpdTag);
                            for (const auto& [idxKey, idxTagPtr] : bpd.value) {
                                if (idxTagPtr == nullptr || idxTagPtr->id() != nbt::TagId::Compound) {
                                    continue;
                                }
                                const auto& entry = dynamic_cast<const nbt::CompoundTag&>(*idxTagPtr);
                                if (entry.value.count("block_entity_data") == 0) {
                                    continue;
                                }
                                auto& bedTag = *entry.value.at("block_entity_data");
                                if (bedTag.id() != nbt::TagId::Compound) {
                                    continue;
                                }
                                const auto& bed = dynamic_cast<const nbt::CompoundTag&>(bedTag);

                                // 读 block_entity_data 的 x/y/z（保存时世界绝对坐标），减 origin 得结构内相对坐标
                                const nbt::Tag* xTag = bed.value.count("x") != 0 ? bed.value.at("x").get() : nullptr;
                                const nbt::Tag* yTag = bed.value.count("y") != 0 ? bed.value.at("y").get() : nullptr;
                                const nbt::Tag* zTag = bed.value.count("z") != 0 ? bed.value.at("z").get() : nullptr;
                                if (xTag == nullptr || yTag == nullptr || zTag == nullptr) {
                                    continue;
                                }
                                const i32 ex = dynamic_cast<const nbt::IntTag&>(*xTag).value;
                                const i32 ey = dynamic_cast<const nbt::IntTag&>(*yTag).value;
                                const i32 ez = dynamic_cast<const nbt::IntTag&>(*zTag).value;
                                const BlockPos relPos(ex - origin.x, ey - origin.y, ez - origin.z);

                                for (auto& bi : blockInfos) {
                                    if (bi.pos == relPos) {
                                        bi.nbt = _cloneNbt(&bed);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 若索引数与 size 体积不符，仍按已解析的方块构建（不强制断言，便于容错）
    templ->addPalette(Palette(std::move(blockInfos)));

    // TODO: 解析 structure.entities（基岩版 mob 实体 schema 与 Java 不同，GameTest 结构通常无 mob，暂不解析）

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

    // 基岩→Java 方块名/属性映射（如 minecraft:log + old_log_type → minecraft:oak_log + axis）
    BedrockBlockMapping mapping;
    if (paletteEntry.value.count("states") != 0) {
        auto& statesTag0 = *paletteEntry.value.at("states");
        if (statesTag0.id() == nbt::TagId::Compound) {
            mapping = bedrockBlockNameToJava(blockName, dynamic_cast<const nbt::CompoundTag&>(statesTag0));
        }
    }
    if (!mapping.javaName.empty()) {
        blockName = mapping.javaName;
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
                std::string javaKey = key; // 默认沿用基岩属性名，命中映射时覆盖
                // 方块名映射附带的属性重命名（如 pillar_axis → axis）
                for (const auto& [from, to] : mapping.propRenames) {
                    if (key == from) {
                        javaKey = to;
                        break;
                    }
                }
                switch (valueTag->id()) {
                    case nbt::TagId::String:
                        strVal = dynamic_cast<const nbt::StringTag&>(*valueTag).value;
                        break;
                    case nbt::TagId::Int: {
                        const i32 iv = dynamic_cast<const nbt::IntTag&>(*valueTag).value;
                        // 基岩方向属性（facing_direction/weirdo_direction 等）是 int，Java facing 取字符串值，
                        // 需先做属性名+值的映射；未命中映射则按原逻辑转数字字符串（适用于 axis 等少数属性）。
                        if (auto mapped = bedrockIntStateToJava(key, iv)) {
                            javaKey = mapped->first;
                            strVal = mapped->second;
                        } else {
                            strVal = std::to_string(iv);
                        }
                        break;
                    }
                    case nbt::TagId::Byte: {
                        // 基岩版字节属性：布尔型用 Byte (0/1) → Java "true"/"false"；
                        // 枚举型（如 upside_down_bit → half）需做属性名+值的映射。
                        const i8 bv = dynamic_cast<const nbt::ByteTag&>(*valueTag).value;
                        if (auto mapped = bedrockByteStateToJava(key, bv)) {
                            javaKey = mapped->first;
                            strVal = mapped->second;
                        } else {
                            strVal = (bv != 0) ? "true" : "false";
                        }
                        break;
                    }
                    default:
                        continue; // 跳过不支持的属性值类型
                }
                stringProps->value.emplace(javaKey, std::make_unique<nbt::StringTag>(strVal));
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
