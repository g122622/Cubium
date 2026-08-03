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

#include "PointOfInterestType.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include <unordered_map>

namespace mc {
namespace world {
namespace village {
namespace poi {

namespace {

// 方块ID到POI类型的映射表（在initialize()中填充）
std::unordered_map<u32, PointOfInterestType> s_blockToPOI;
bool s_initialized = false;

// 初始化方块ID到POI类型的映射
void initializeBlockToPOIMap()
{
    if (s_initialized) {
        return;
    }
    s_initialized = true;

    auto& registry = BlockRegistry::instance();

    // 工作站映射
    auto addWorkstation = [&](const char* blockName, PointOfInterestType poiType) {
        Block* block = registry.getBlock(ResourceLocation(blockName));
        if (block) {
            s_blockToPOI[block->blockId()] = poiType;
        }
    };

    // 工作站
    addWorkstation("minecraft:smoker", PointOfInterestType::Smoker);
    addWorkstation("minecraft:blast_furnace", PointOfInterestType::BlastFurnace);
    addWorkstation("minecraft:cartography_table", PointOfInterestType::CartographyTable);
    addWorkstation("minecraft:brewing_stand", PointOfInterestType::BrewingStand);
    addWorkstation("minecraft:composter", PointOfInterestType::Composter);
    addWorkstation("minecraft:barrel", PointOfInterestType::Barrel);
    addWorkstation("minecraft:fletching_table", PointOfInterestType::FletchingTable);
    addWorkstation("minecraft:cauldron", PointOfInterestType::Cauldron);
    addWorkstation("minecraft:lectern", PointOfInterestType::Lectern);
    addWorkstation("minecraft:stonecutter", PointOfInterestType::Stonecutter);
    addWorkstation("minecraft:smithing_table", PointOfInterestType::SmithingTable);
    addWorkstation("minecraft:loom", PointOfInterestType::Loom);

    // 钟
    addWorkstation("minecraft:bell", PointOfInterestType::Bell);

    // 下界传送门
    addWorkstation("minecraft:nether_portal", PointOfInterestType::NetherPortal);

    // 磁石
    addWorkstation("minecraft:lodestone", PointOfInterestType::Lodestone);

    // 避雷针
    addWorkstation("minecraft:lightning_rod", PointOfInterestType::LightningRod);

    // 床（所有16种颜色）
    addWorkstation("minecraft:red_bed", PointOfInterestType::BedRed);
    addWorkstation("minecraft:black_bed", PointOfInterestType::BedBlack);
    addWorkstation("minecraft:blue_bed", PointOfInterestType::BedBlue);
    addWorkstation("minecraft:brown_bed", PointOfInterestType::BedBrown);
    addWorkstation("minecraft:cyan_bed", PointOfInterestType::BedCyan);
    addWorkstation("minecraft:gray_bed", PointOfInterestType::BedGray);
    addWorkstation("minecraft:green_bed", PointOfInterestType::BedGreen);
    addWorkstation("minecraft:light_blue_bed", PointOfInterestType::BedLightBlue);
    addWorkstation("minecraft:light_gray_bed", PointOfInterestType::BedLightGray);
    addWorkstation("minecraft:lime_bed", PointOfInterestType::BedLime);
    addWorkstation("minecraft:magenta_bed", PointOfInterestType::BedMagenta);
    addWorkstation("minecraft:orange_bed", PointOfInterestType::BedOrange);
    addWorkstation("minecraft:pink_bed", PointOfInterestType::BedPink);
    addWorkstation("minecraft:purple_bed", PointOfInterestType::BedPurple);
    addWorkstation("minecraft:white_bed", PointOfInterestType::BedWhite);
    addWorkstation("minecraft:yellow_bed", PointOfInterestType::BedYellow);
}

} // namespace

const char* POITypeHelper::getName(PointOfInterestType type)
{
    switch (type) {
        case PointOfInterestType::BedRed:
            return "bed_red";
        case PointOfInterestType::BedBlack:
            return "bed_black";
        case PointOfInterestType::BedBlue:
            return "bed_blue";
        case PointOfInterestType::BedBrown:
            return "bed_brown";
        case PointOfInterestType::BedCyan:
            return "bed_cyan";
        case PointOfInterestType::BedGray:
            return "bed_gray";
        case PointOfInterestType::BedGreen:
            return "bed_green";
        case PointOfInterestType::BedLightBlue:
            return "bed_light_blue";
        case PointOfInterestType::BedLightGray:
            return "bed_light_gray";
        case PointOfInterestType::BedLime:
            return "bed_lime";
        case PointOfInterestType::BedMagenta:
            return "bed_magenta";
        case PointOfInterestType::BedOrange:
            return "bed_orange";
        case PointOfInterestType::BedPink:
            return "bed_pink";
        case PointOfInterestType::BedPurple:
            return "bed_purple";
        case PointOfInterestType::BedWhite:
            return "bed_white";
        case PointOfInterestType::BedYellow:
            return "bed_yellow";
        case PointOfInterestType::Smoker:
            return "smoker";
        case PointOfInterestType::BlastFurnace:
            return "blast_furnace";
        case PointOfInterestType::CartographyTable:
            return "cartography_table";
        case PointOfInterestType::BrewingStand:
            return "brewing_stand";
        case PointOfInterestType::Composter:
            return "composter";
        case PointOfInterestType::Barrel:
            return "barrel";
        case PointOfInterestType::FletchingTable:
            return "fletching_table";
        case PointOfInterestType::Cauldron:
            return "cauldron";
        case PointOfInterestType::Lectern:
            return "lectern";
        case PointOfInterestType::Stonecutter:
            return "stonecutter";
        case PointOfInterestType::SmithingTable:
            return "smithing_table";
        case PointOfInterestType::Loom:
            return "loom";
        case PointOfInterestType::Bell:
            return "bell";
        case PointOfInterestType::NetherPortal:
            return "nether_portal";
        case PointOfInterestType::Lodestone:
            return "lodestone";
        case PointOfInterestType::LightningRod:
            return "lightning_rod";
        default:
            return "none";
    }
}

bool POITypeHelper::isBed(PointOfInterestType type)
{
    return type >= PointOfInterestType::BedRed && type <= PointOfInterestType::BedYellow;
}

bool POITypeHelper::isWorkstation(PointOfInterestType type)
{
    return type >= PointOfInterestType::Smoker && type <= PointOfInterestType::Loom;
}

PointOfInterestType POITypeHelper::getProfessionForWorkstation(PointOfInterestType type)
{
    if (!isWorkstation(type)) {
        return PointOfInterestType::None;
    }

    // 工作站与职业对应关系
    // 注意：这里返回的是工作站类型，实际职业映射在VillagerProfession中处理
    return type;
}

PointOfInterestType POITypeHelper::fromBlockId(u32 blockId)
{
    // 确保映射表已初始化
    initializeBlockToPOIMap();

    auto it = s_blockToPOI.find(blockId);
    if (it != s_blockToPOI.end()) {
        return it->second;
    }
    return PointOfInterestType::None;
}

i32 POITypeHelper::getMaxTickets(PointOfInterestType type)
{
    // 大多数POI只能被一个村民占用
    // 钟可以被多个村民共享（用于聚集）
    if (type == PointOfInterestType::Bell) {
        return 32; // 钟可以同时被多个村民使用
    }
    return 1;
}

f32 POITypeHelper::getSearchRange(PointOfInterestType type)
{
    // 村民搜索POI的范围
    // 床位和工作站：48格
    // 钟：64格（聚集点）
    // 其他：16格
    if (isBed(type) || isWorkstation(type)) {
        return 48.0f;
    }
    if (type == PointOfInterestType::Bell) {
        return 64.0f;
    }
    return 16.0f;
}

} // namespace poi
} // namespace village
} // namespace world
} // namespace mc
