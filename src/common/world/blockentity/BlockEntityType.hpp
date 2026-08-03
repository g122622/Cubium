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
    Furnace,      ///< 熔炉
    BlastFurnace, ///< 高炉
    Smoker,       ///< 烟熏炉
    BrewingStand, ///< 酿造台

    // 红石类
    Dispenser,        ///< 发射器
    Dropper,          ///< 投掷器
    Hopper,           ///< 漏斗
    Piston,           ///< 活塞
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
    Shelf,           ///< 书架（Shelf，1.21.4+ 木质变体）

    // 试炼密室
    TrialSpawner, ///< 试炼刷怪笼
    Vault,        ///< 宝库
    Crafter,      ///< 自动合成器

    // 幽匿
    SculkSensor,   ///< 幽匿感测体
    SculkShrieker, ///< 幽匿尖啸体

    // 交互
    DecoratedPot, ///< 饰纹陶罐

    // 考古
    BrushableBlock, ///< 可疑沙/可疑沙砾（可刷方块）

    // 1.21.11 铜傀儡雕像
    CopperGolemStatue, ///< 铜傀儡雕像

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
