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

#include "ArgumentType.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {
namespace command {

/**
 * @brief 物品槽位索引
 *
 * 表示一个已解析的物品槽位。
 * 对于玩家背包，slotIndex 直接对应 PlayerInventory 的索引 (0-40)。
 * 对于实体装备，slotIndex 对应 MC 的装备槽位编号 (98-106)。
 * 对于容器方块，slotIndex 对应容器内的索引 (0-53)。
 *
 * 槽位命名参考 MC 1.21.11 的 SlotRanges：
 * - container.0 ~ container.53  -> 0-53
 * - hotbar.0 ~ hotbar.8          -> 0-8
 * - inventory.0 ~ inventory.26   -> 9-35 (主背包偏移)
 * - enderchest.0 ~ enderchest.26 -> 200-226
 * - villager.0 ~ villager.7      -> 300-307
 * - weapon / weapon.mainhand     -> 98
 * - weapon.offhand               -> 99
 * - armor.head                   -> 100
 * - armor.chest                  -> 101
 * - armor.legs                   -> 102
 * - armor.feet                   -> 103
 * - armor.body                   -> 105
 * - saddle                       -> 106
 * - horse.chest                  -> 499
 * - player.cursor                -> 499
 * - player.crafting.0 ~ 3        -> 500-503
 */
class ItemSlot {
public:
    ItemSlot() noexcept = default;
    explicit ItemSlot(i32 slotIndex) noexcept
        : m_slotIndex(slotIndex)
    {}

    /** @brief 获取槽位索引 */
    [[nodiscard]] i32 slotIndex() const noexcept { return m_slotIndex; }

    /** @brief 是否为有效的槽位 */
    [[nodiscard]] bool isValid() const noexcept { return m_slotIndex >= 0; }

    /**
     * @brief 判断该槽位是否为实体装备槽位
     *
     * 装备槽位编号范围 98-106，需要通过 LivingEntity::setEquipment 设置。
     */
    [[nodiscard]] bool isEquipmentSlot() const noexcept { return m_slotIndex >= 98 && m_slotIndex <= 106; }

    /**
     * @brief 判断该槽位是否为玩家背包槽位
     *
     * 玩家背包槽位范围 0-40，通过 PlayerInventory::setItem 设置。
     */
    [[nodiscard]] bool isPlayerInventorySlot() const noexcept { return m_slotIndex >= 0 && m_slotIndex <= 40; }

    /**
     * @brief 判断该槽位是否为末影箱槽位
     */
    [[nodiscard]] bool isEnderChestSlot() const noexcept { return m_slotIndex >= 200 && m_slotIndex <= 226; }

    /**
     * @brief 判断该槽位是否为马匹槽位
     *
     * 注意：马匹槽位 (500-514) 与合成槽位 (500-503) 编号范围重叠，
     * 需要根据命令上下文（目标实体类型）区分实际含义。
     * 当目标实体为马匹时，500-514 均为马匹槽位；
     * 当目标为玩家时，500-503 为合成槽位，504-514 无效。
     */
    [[nodiscard]] bool isHorseSlot() const noexcept { return m_slotIndex >= 500 && m_slotIndex <= 514; }

    /**
     * @brief 判断该槽位是否为马匹箱子槽位 (horse.chest = 499)
     */
    [[nodiscard]] bool isHorseChestSlot() const noexcept { return m_slotIndex == 499; }

    /**
     * @brief 判断该槽位是否为合成槽位
     *
     * 注意：合成槽位 (500-503) 与马匹槽位 (500-503) 编号范围重叠，
     * 需要根据命令上下文区分实际含义。
     * 当目标为玩家时，500-503 为合成槽位；当目标为马匹时，500-514 为马匹槽位。
     */
    [[nodiscard]] bool isCraftingSlot() const noexcept { return m_slotIndex >= 500 && m_slotIndex <= 503; }

    /**
     * @brief 判断该槽位是否为村民交易槽位
     */
    [[nodiscard]] bool isVillagerSlot() const noexcept { return m_slotIndex >= 300 && m_slotIndex <= 307; }

    /**
     * @brief 判断该槽位是否为玩家光标槽位 (player.cursor = 499)
     *
     * 注意：player.cursor (499) 与 horse.chest (499) 编号重叠，
     * 需根据命令上下文区分。
     */
    [[nodiscard]] bool isCursorSlot() const noexcept { return m_slotIndex == 499; }

    /**
     * @brief 获取装备槽位类型索引
     *
     * 将 MC 装备编号映射到 EquipmentSlot 枚举索引（编号与原版 SlotArgument 一致）：
     * 100=Feet, 101=Legs, 102=Chest, 103=Head——与原版 LivingEntity.getSlot 的
     * `case 100->Feet, 101->Legs, 102->Chest, 103->Head` 逐条对应。
     *
     * @return EquipmentSlot 索引 (0-6)，如果不是装备槽位返回 -1
     */
    [[nodiscard]] i32 toEquipmentSlotIndex() const noexcept
    {
        switch (m_slotIndex) {
            case 98:
                return 0; // MainHand
            case 99:
                return 1; // OffHand
            case 100:
                return 2; // Feet
            case 101:
                return 3; // Legs
            case 102:
                return 4; // Chest
            case 103:
                return 5; // Head
            case 105:
                return 6; // Body (EquipmentSlot::Body, 非玩家实体护甲槽位)
            case 106:
                return 7; // Saddle (EquipmentSlot::Saddle, 鞍槽/铜傀儡天线槽)
            default:
                return -1;
        }
    }

    /**
     * @brief 获取玩家背包槽位索引
     *
     * 对于装备槽位 (98-106)，将其映射到 PlayerInventory 的绝对索引：
     * 98 (weapon.mainhand) -> 玩家当前选中的快捷栏槽位
     * 99 (weapon.offhand)  -> 40 (OFFHAND)
     * 100 (armor.feet)     -> 39 (ARMOR_FEET)
     * 101 (armor.legs)     -> 38 (ARMOR_LEGS)
     * 102 (armor.chest)    -> 37 (ARMOR_CHEST)
     * 103 (armor.head)     -> 36 (ARMOR_HEAD)
     * 105 (armor.body)     -> -1 (玩家无 Body 槽位，仅非玩家实体使用)
     *
     * 对于非装备槽位 (0-40)，直接返回自身索引。
     *
     * @param selectedSlot 玩家当前选中的快捷栏槽位 (0-8)，仅对 weapon.mainhand 有意义
     * @return PlayerInventory 槽位索引 (0-40)，如果无法映射返回 -1
     */
    [[nodiscard]] i32 toPlayerInventorySlot(i32 selectedSlot = 0) const noexcept
    {
        if (m_slotIndex >= 0 && m_slotIndex <= 40) {
            return m_slotIndex;
        }
        switch (m_slotIndex) {
            case 98:
                return selectedSlot; // weapon.mainhand -> 当前手持快捷栏槽位
            case 99:
                return 40; // weapon.offhand -> OFFHAND
            case 100:
                return 39; // armor.feet -> ARMOR_FEET
            case 101:
                return 38; // armor.legs -> ARMOR_LEGS
            case 102:
                return 37; // armor.chest -> ARMOR_CHEST
            case 103:
                return 36; // armor.head -> ARMOR_HEAD
            case 105:
                return -1; // armor.body -> 玩家无 Body 槽位（仅非玩家实体使用）
            default:
                return -1;
        }
    }

    /**
     * @brief 获取末影箱内部槽位索引
     *
     * 将命令槽位索引 (200-226) 转换为 PlayerEnderChestInventory 内部索引 (0-26)。
     *
     * @return 末影箱内部槽位索引 (0-26)，如果不是末影箱槽位返回 -1
     */
    [[nodiscard]] i32 toEnderChestSlot() const noexcept
    {
        if (isEnderChestSlot()) {
            return m_slotIndex - 200; // PlayerEnderChestInventory::SLOT_INDEX_START
        }
        return -1;
    }

    /**
     * @brief 获取马匹内部槽位索引
     *
     * 将命令槽位索引 (500-514) 转换为 AbstractHorseEntity 内部槽位索引 (0-14)。
     * 马匹内部槽位布局：0=鞍, 1=马铠/地毯, 2+=箱子内容物（仅箱马）
     *
     * @return 马匹内部槽位索引 (0-14)，如果不是马匹槽位返回 -1
     */
    [[nodiscard]] i32 toHorseSlot() const noexcept
    {
        if (isHorseSlot()) {
            return m_slotIndex - 500;
        }
        return -1;
    }

    /**
     * @brief 获取村民交易槽位内部索引
     *
     * 将命令槽位索引 (300-307) 转换为村民内部槽位索引 (0-7)。
     *
     * @return 村民内部槽位索引 (0-7)，如果不是村民槽位返回 -1
     */
    [[nodiscard]] i32 toVillagerSlot() const noexcept
    {
        if (isVillagerSlot()) {
            return m_slotIndex - 300;
        }
        return -1;
    }

private:
    i32 m_slotIndex = -1;
};

/**
 * @brief 物品槽位参数类型
 *
 * 解析物品槽位名称字符串为 ItemSlot 对象。
 *
 * 支持的格式：
 * - "container.N"  (0-53)  通用容器槽位
 * - "hotbar.N"     (0-8)   快捷栏
 * - "inventory.N"  (0-26)  主背包（偏移到 9-35）
 * - "weapon" / "weapon.mainhand" (98)  主手
 * - "weapon.offhand"              (99)  副手
 * - "armor.head"    (100) 头盔
 * - "armor.chest"   (101) 胸甲
 * - "armor.legs"    (102) 护腿
 * - "armor.feet"    (103) 靴子
 * - "armor.body"    (105) 身体（非玩家护甲）
 * - "saddle"        (106) 鞍
 * - "enderchest.N"  (0-26) 末影箱（偏移到 200-226）
 * - "villager.N"    (0-7)  村民交易槽位（偏移到 300-307）
 * - "horse.N"       (0-14) 马匹槽位（偏移到 500-514）
 * - 纯数字          直接作为槽位索引
 *
 * 槽位编号定义与原版 SlotRanges 一致
 */
class ItemSlotArgumentType : public ArgumentType<ItemSlot> {
public:
    /**
     * @brief 解析物品栏槽位（原版 SlotArgument 语义）
     *
     * 原版只认下面这张名字表，槽位编号就是表里的数字；序号越界、前缀对不上、
     * 大小写不同都会报「Unknown slot」，并且报错位置是读完整个名字之后。
     * `contents` 在原版里是容器全体（0），纯数字与原版一致地不支持。
     */
    [[nodiscard]] ItemSlot parse(StringReader& reader) override
    {
        i32 start = reader.getCursor();
        std::string str = reader.readString();

        if (const auto it = s_slots.find(str); it != s_slots.end()) {
            return ItemSlot(it->second);
        }

        // 与 brigadier 一致：游标停留在已读到的位置，报错点名整个名字。
        throw CommandException(CommandErrorType::Unknown, "Unknown slot: " + str, reader.getCursor());
    }

    [[nodiscard]] std::string getTypeName() const noexcept override { return "item_slot"; }

    [[nodiscard]] std::vector<std::string> getExamples() const noexcept override
    {
        return {"container.5", "weapon.mainhand", "armor.head", "hotbar.0", "weapon", "0"};
    }

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<ItemSlotArgumentType> itemSlot() noexcept
    {
        return std::make_shared<ItemSlotArgumentType>();
    }

    // ========== 静态获取方法 ==========

    template <typename S>
    static ItemSlot getSlot(CommandContext<S>& context, const std::string& name)
    {
        return context.template getArgument<ItemSlot>(name);
    }

private:
    /// 与原版 SlotArgument 的 `case` 表逐条对应（名字 → 槽位号）。
    static const std::unordered_map<std::string, i32> s_slots;
};

inline const std::unordered_map<std::string, i32> ItemSlotArgumentType::s_slots = {
    {"contents", 0},
    {"weapon", 98},
    {"weapon.mainhand", 98},
    {"weapon.offhand", 99},
    // 头部/胸甲/护腿/靴子：100..103（原版枚举顺序，头=103）
    {"armor.head", 103},
    {"armor.chest", 102},
    {"armor.legs", 101},
    {"armor.feet", 100},
    {"armor.body", 105},
    {"saddle", 106},
    {"horse.chest", 499},
    {"player.cursor", 499},
    {"hotbar.0", 0},
    {"hotbar.1", 1},
    {"hotbar.2", 2},
    {"hotbar.3", 3},
    {"hotbar.4", 4},
    {"hotbar.5", 5},
    {"hotbar.6", 6},
    {"hotbar.7", 7},
    {"hotbar.8", 8},
    {"container.0", 0},
    {"container.1", 1},
    {"container.2", 2},
    {"container.3", 3},
    {"container.4", 4},
    {"container.5", 5},
    {"container.6", 6},
    {"container.7", 7},
    {"container.8", 8},
    {"container.9", 9},
    {"container.10", 10},
    {"container.11", 11},
    {"container.12", 12},
    {"container.13", 13},
    {"container.14", 14},
    {"container.15", 15},
    {"container.16", 16},
    {"container.17", 17},
    {"container.18", 18},
    {"container.19", 19},
    {"container.20", 20},
    {"container.21", 21},
    {"container.22", 22},
    {"container.23", 23},
    {"container.24", 24},
    {"container.25", 25},
    {"container.26", 26},
    {"container.27", 27},
    {"container.28", 28},
    {"container.29", 29},
    {"container.30", 30},
    {"container.31", 31},
    {"container.32", 32},
    {"container.33", 33},
    {"container.34", 34},
    {"container.35", 35},
    {"container.36", 36},
    {"container.37", 37},
    {"container.38", 38},
    {"container.39", 39},
    {"container.40", 40},
    {"container.41", 41},
    {"container.42", 42},
    {"container.43", 43},
    {"container.44", 44},
    {"container.45", 45},
    {"container.46", 46},
    {"container.47", 47},
    {"container.48", 48},
    {"container.49", 49},
    {"container.50", 50},
    {"container.51", 51},
    {"container.52", 52},
    {"container.53", 53},
    {"inventory.0", 9},
    {"inventory.1", 10},
    {"inventory.2", 11},
    {"inventory.3", 12},
    {"inventory.4", 13},
    {"inventory.5", 14},
    {"inventory.6", 15},
    {"inventory.7", 16},
    {"inventory.8", 17},
    {"inventory.9", 18},
    {"inventory.10", 19},
    {"inventory.11", 20},
    {"inventory.12", 21},
    {"inventory.13", 22},
    {"inventory.14", 23},
    {"inventory.15", 24},
    {"inventory.16", 25},
    {"inventory.17", 26},
    {"inventory.18", 27},
    {"inventory.19", 28},
    {"inventory.20", 29},
    {"inventory.21", 30},
    {"inventory.22", 31},
    {"inventory.23", 32},
    {"inventory.24", 33},
    {"inventory.25", 34},
    {"inventory.26", 35},
    {"enderchest.0", 200},
    {"enderchest.1", 201},
    {"enderchest.2", 202},
    {"enderchest.3", 203},
    {"enderchest.4", 204},
    {"enderchest.5", 205},
    {"enderchest.6", 206},
    {"enderchest.7", 207},
    {"enderchest.8", 208},
    {"enderchest.9", 209},
    {"enderchest.10", 210},
    {"enderchest.11", 211},
    {"enderchest.12", 212},
    {"enderchest.13", 213},
    {"enderchest.14", 214},
    {"enderchest.15", 215},
    {"enderchest.16", 216},
    {"enderchest.17", 217},
    {"enderchest.18", 218},
    {"enderchest.19", 219},
    {"enderchest.20", 220},
    {"enderchest.21", 221},
    {"enderchest.22", 222},
    {"enderchest.23", 223},
    {"enderchest.24", 224},
    {"enderchest.25", 225},
    {"enderchest.26", 226},
    {"villager.0", 300},
    {"villager.1", 301},
    {"villager.2", 302},
    {"villager.3", 303},
    {"villager.4", 304},
    {"villager.5", 305},
    {"villager.6", 306},
    {"villager.7", 307},
    {"horse.0", 500},
    {"horse.1", 501},
    {"horse.2", 502},
    {"horse.3", 503},
    {"horse.4", 504},
    {"horse.5", 505},
    {"horse.6", 506},
    {"horse.7", 507},
    {"horse.8", 508},
    {"horse.9", 509},
    {"horse.10", 510},
    {"horse.11", 511},
    {"horse.12", 512},
    {"horse.13", 513},
    {"horse.14", 514},
    {"player.crafting.0", 500},
    {"player.crafting.1", 501},
    {"player.crafting.2", 502},
    {"player.crafting.3", 503},
};

} // namespace command
} // namespace mc
