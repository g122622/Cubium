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
     * 将 MC 装备编号映射到 EquipmentSlot 枚举索引：
     * 98=MainHand(0), 99=OffHand(1), 100=Head(5), 101=Chest(4),
     * 102=Legs(3), 103=Feet(2), 105=Body(6)
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
                return 5; // Head
            case 101:
                return 4; // Chest
            case 102:
                return 3; // Legs
            case 103:
                return 2; // Feet
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
     * 100 (armor.head)     -> 36 (ARMOR_HEAD)
     * 101 (armor.chest)    -> 37 (ARMOR_CHEST)
     * 102 (armor.legs)     -> 38 (ARMOR_LEGS)
     * 103 (armor.feet)     -> 39 (ARMOR_FEET)
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
                return 36; // armor.head -> ARMOR_HEAD
            case 101:
                return 37; // armor.chest -> ARMOR_CHEST
            case 102:
                return 38; // armor.legs -> ARMOR_LEGS
            case 103:
                return 39; // armor.feet -> ARMOR_FEET
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
    [[nodiscard]] ItemSlot parse(StringReader& reader) override
    {
        i32 start = reader.getCursor();
        std::string str = reader.readString();

        // 尝试解析为纯数字
        if (_isPureNumber(str)) {
            i32 index = std::stoi(str);
            if (index < 0) {
                reader.setCursor(start);
                throw CommandException(
                    CommandErrorType::IntegerTooLow, "Slot index must be non-negative: " + str, start);
            }
            return ItemSlot(index);
        }

        // 查找命名槽位映射
        i32 slotIndex = _parseSlotName(str);
        if (slotIndex < 0) {
            reader.setCursor(start);
            throw CommandException(CommandErrorType::Unknown, "Unknown slot name: " + str, start);
        }

        return ItemSlot(slotIndex);
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
    /**
     * @brief 检查字符串是否为纯数字
     */
    [[nodiscard]] static bool _isPureNumber(const std::string& str) noexcept
    {
        if (str.empty()) return false;
        for (char c : str) {
            if (c < '0' || c > '9') return false;
        }
        return true;
    }

    /**
     * @brief 解析命名槽位字符串为索引
     * @return 槽位索引，如果无法识别返回 -1
     */
    [[nodiscard]] static i32 _parseSlotName(const std::string& str)
    {
        // 快捷栏: hotbar.0 ~ hotbar.8 -> 0-8
        if (str.starts_with("hotbar.")) {
            i32 offset = _parseTrailingInt(str, 7);
            if (offset >= 0 && offset <= 8) return offset;
            return -1;
        }

        // 容器槽位: container.0 ~ container.53 -> 0-53
        if (str.starts_with("container.")) {
            i32 offset = _parseTrailingInt(str, 10);
            if (offset >= 0 && offset <= 53) return offset;
            return -1;
        }

        // 主背包: inventory.0 ~ inventory.26 -> 9-35
        if (str.starts_with("inventory.")) {
            i32 offset = _parseTrailingInt(str, 10);
            if (offset >= 0 && offset <= 26) return 9 + offset;
            return -1;
        }

        // 末影箱: enderchest.0 ~ enderchest.26 -> 200-226
        if (str.starts_with("enderchest.")) {
            i32 offset = _parseTrailingInt(str, 11);
            if (offset >= 0 && offset <= 26) return 200 + offset;
            return -1;
        }

        // 村民交易: villager.0 ~ villager.7 -> 300-307
        if (str.starts_with("villager.")) {
            i32 offset = _parseTrailingInt(str, 9);
            if (offset >= 0 && offset <= 7) return 300 + offset;
            return -1;
        }

        // 马匹: horse.0 ~ horse.14 -> 500-514
        // 注意: horse.chest 必须在 horse.N 之前检查，避免被 horse. 前缀匹配误捕获
        if (str == "horse.chest") return 499;
        if (str.starts_with("horse.")) {
            i32 offset = _parseTrailingInt(str, 6);
            if (offset >= 0 && offset <= 14) return 500 + offset;
            return -1;
        }

        // 合成槽: player.crafting.0 ~ player.crafting.3 -> 500-503
        // 注意：player.crafting (500-503) 与 horse (500-514) 编号范围部分重叠，
        // 命令上下文决定实际含义：目标为玩家时为合成槽，目标为马匹时为马匹槽
        if (str.starts_with("player.crafting.")) {
            i32 offset = _parseTrailingInt(str, 16);
            if (offset >= 0 && offset <= 3) return 500 + offset;
            return -1;
        }

        // 玩家光标: player.cursor -> 499
        // 注意：player.cursor (499) 与 horse.chest (499) 编号重叠，
        // 命令上下文决定实际含义：目标为玩家时为光标，目标为马匹时为箱子
        if (str == "player.cursor") return 499;

        // 武器槽位
        if (str == "weapon" || str == "weapon.mainhand") return 98;
        if (str == "weapon.offhand") return 99;

        // 护甲槽位
        if (str == "armor.head") return 100;
        if (str == "armor.chest") return 101;
        if (str == "armor.legs") return 102;
        if (str == "armor.feet") return 103;
        if (str == "armor.body") return 105;

        // 鞍槽位
        if (str == "saddle") return 106;

        return -1;
    }

    /**
     * @brief 从字符串的指定位置开始解析整数后缀
     * @param str 完整字符串
     * @param startPos 数字开始的索引位置
     * @return 解析后的整数，如果失败返回 -1
     */
    [[nodiscard]] static i32 _parseTrailingInt(const std::string& str, std::size_t startPos)
    {
        if (startPos >= str.size()) return -1;
        try {
            std::size_t pos = 0;
            i32 value = std::stoi(str.substr(startPos), &pos);
            // 确保整个后缀都被解析为数字
            if (pos + startPos != str.size()) return -1;
            return value;
        }
        catch (...) {
            return -1;
        }
    }
};

} // namespace command
} // namespace mc
