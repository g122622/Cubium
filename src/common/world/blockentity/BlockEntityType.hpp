#pragma once

#include "resource/ResourceLocation.hpp"
#include "world/block/BlockPos.hpp"
#include <memory>
#include <nlohmann/json.hpp>

namespace mc {

/**
 * @brief 方块实体类型枚举
 *
 * 定义所有已知的方块实体类型
 */
enum class BlockEntityType : u16 {
    Unknown = 0,

    // 存储类
    Chest,        ///< 箱子
    TrappedChest, ///< 陷阱箱
    EnderChest,   ///< 末影箱
    ShulkerBox,   ///< 潜影盒
    Barrel,       ///< 木桶

    // 工作类
    CraftingTable,    ///< 工作台
    Furnace,          ///< 熔炉
    BlastFurnace,     ///< 高炉
    Smoker,           ///< 烟熏炉
    BrewingStand,     ///< 酿造台
    Anvil,            ///< 铁砧
    Grindstone,       ///< 磨石
    Stonecutter,      ///< 切石机
    SmithingTable,    ///< 锻造台
    Loom,             ///< 织布机
    CartographyTable, ///< 制图台

    // 红石类
    Dispenser,        ///< 发射器
    Dropper,          ///< 投掷器
    Hopper,           ///< 漏斗
    Piston,           ///< 活塞
    Observer,         ///< 侦测器
    Comparator,       ///< 红石比较器
    DaylightDetector, ///< 阳光探测器

    // 标识类
    Sign,           ///< 告示牌
    Banner,         ///< 旗帜
    StructureBlock, ///< 结构方块
    JigsawBlock,    ///< 拼图方块

    // 其他
    Beacon,          ///< 信标
    Bed,             ///< 床
    Bell,            ///< 钟
    CommandBlock,    ///< 命令方块
    EnchantingTable, ///< 附魔台
    EndGateway,      ///< 末地折跃门
    EndPortal,       ///< 末地传送门
    MobSpawner,      ///< 刷怪笼
    Skull,           ///< 生物头颅
    Beehive,         ///< 蜂巢
    Campfire,        ///< 营火
    Conduit,         ///< 潮涌核心
    Lectern,         ///< 讲台
    Jukebox,         ///< 唱片机

    Count ///< 类型数量
};

/**
 * @brief 获取方块实体类型的资源位置ID
 * @param type 方块实体类型
 * @return 资源位置（如 "minecraft:crafting_table"）
 */
ResourceLocation blockEntityTypeToId(BlockEntityType type);

/**
 * @brief 从资源位置ID解析方块实体类型
 * @param id 资源位置ID
 * @return 方块实体类型，如果未知返回 Unknown
 */
BlockEntityType blockEntityTypeFromId(const ResourceLocation& id);

} // namespace mc
