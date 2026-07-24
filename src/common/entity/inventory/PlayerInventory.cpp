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

#include "PlayerInventory.hpp"
#include "../../item/core/Item.hpp"
#include "../../util/nbt/Nbt.hpp"
#include "../damage/DamageSource.hpp"
#include "../entities/player/Player.hpp"
#include "../serialization/EntityNbtKeys.hpp"
#include "../serialization/NbtHelper.hpp"
#include <algorithm>
#include <cmath>

namespace mc {

// ============================================================================
// PlayerInventory
// ============================================================================

PlayerInventory::PlayerInventory(Player* player)
    : m_player(player)
    , m_selectedSlot(0)
{
    // 初始化所有槽位为空
    for (auto& item : m_items) {
        item = ItemStack::EMPTY;
    }
}

bool PlayerInventory::isEmpty() const
{
    for (const auto& item : m_items) {
        if (!item.isEmpty()) {
            return false;
        }
    }
    return true;
}

ItemStack PlayerInventory::getItem(i32 slot) const
{
    if (slot < 0 || slot >= TOTAL_SIZE) {
        return ItemStack::EMPTY;
    }
    return m_items[static_cast<size_t>(slot)];
}

void PlayerInventory::setItem(i32 slot, const ItemStack& stack)
{
    if (slot < 0 || slot >= TOTAL_SIZE) {
        return;
    }
    ItemStack oldItem = m_items[static_cast<size_t>(slot)];
    m_items[static_cast<size_t>(slot)] = stack;
    m_timesChanged++;

    // 触发变更回调
    if (m_changeCallback) {
        m_changeCallback(slot, oldItem, stack);
    }
}

ItemStack PlayerInventory::removeItem(i32 slot, i32 count)
{
    if (slot < 0 || slot >= TOTAL_SIZE) {
        return ItemStack::EMPTY;
    }

    ItemStack& stack = m_items[static_cast<size_t>(slot)];
    if (stack.isEmpty()) {
        return ItemStack::EMPTY;
    }

    if (count >= stack.getCount()) {
        // 移除整个堆
        ItemStack result = stack;
        m_items[static_cast<size_t>(slot)] = ItemStack::EMPTY;
        m_timesChanged++;
        return result;
    }

    // 移除部分
    ItemStack result = stack.split(count);
    m_timesChanged++;
    return result;
}

ItemStack PlayerInventory::removeItemNoUpdate(i32 slot)
{
    if (slot < 0 || slot >= TOTAL_SIZE) {
        return ItemStack::EMPTY;
    }

    ItemStack result = m_items[static_cast<size_t>(slot)];
    m_items[static_cast<size_t>(slot)] = ItemStack::EMPTY;
    return result;
}

void PlayerInventory::clear()
{
    for (auto& item : m_items) {
        item = ItemStack::EMPTY;
    }
    m_timesChanged++;
}

void PlayerInventory::setChanged()
{
    m_timesChanged++;
}

// ============================================================================
// 快捷栏操作
// ============================================================================

void PlayerInventory::setSelectedSlot(i32 slot)
{
    m_selectedSlot = std::clamp(slot, 0, static_cast<i32>(HOTBAR_SIZE - 1));
}

ItemStack PlayerInventory::getSelectedStack() const
{
    if (m_selectedSlot < 0 || m_selectedSlot >= HOTBAR_SIZE) {
        return ItemStack::EMPTY;
    }
    return m_items[static_cast<size_t>(m_selectedSlot)];
}

i32 PlayerInventory::getBestHotbarSlot() const
{
    // MC 1.16.5: 从当前选中槽位开始循环查找
    // 首先寻找空槽位
    for (i32 i = 0; i < HOTBAR_SIZE; ++i) {
        i32 slot = (m_selectedSlot + i) % HOTBAR_SIZE;
        if (m_items[static_cast<size_t>(slot)].isEmpty()) {
            return slot;
        }
    }

    // 如果没有空槽，找非附魔物品槽位
    for (i32 i = 0; i < HOTBAR_SIZE; ++i) {
        i32 slot = (m_selectedSlot + i) % HOTBAR_SIZE;
        if (!m_items[static_cast<size_t>(slot)].isEmpty() && !m_items[static_cast<size_t>(slot)].hasEnchantments()) {
            return slot;
        }
    }

    // 都没有则返回当前选中槽位
    return m_selectedSlot;
}

// ============================================================================
// 物品添加
// ============================================================================

i32 PlayerInventory::add(ItemStack& stack)
{
    if (stack.isEmpty()) {
        return 0;
    }

    i32 originalCount = stack.getCount();

    // MC 1.16.5行为: 损坏的物品直接放入第一个空槽位
    // 普通物品: 优先尝试合并到现有堆叠
    // 顺序: 当前选中槽位 → 副手槽 → 快捷栏 → 主背包

    // 1. 首先尝试合并到当前选中槽位
    if (_canMergeStacks(m_items[static_cast<size_t>(m_selectedSlot)], stack)) {
        i32 maxStack = std::min(m_items[static_cast<size_t>(m_selectedSlot)].getMaxStackSize(), getMaxStackSize());
        i32 space = maxStack - m_items[static_cast<size_t>(m_selectedSlot)].getCount();
        i32 toAdd = std::min(space, stack.getCount());
        m_items[static_cast<size_t>(m_selectedSlot)].grow(toAdd);
        stack.shrink(toAdd);
        if (stack.isEmpty()) {
            return originalCount;
        }
    }

    // 2. 尝试合并到副手槽
    if (_canMergeStacks(m_items[static_cast<size_t>(InventorySlots::OFFHAND)], stack)) {
        i32 maxStack =
            std::min(m_items[static_cast<size_t>(InventorySlots::OFFHAND)].getMaxStackSize(), getMaxStackSize());
        i32 space = maxStack - m_items[static_cast<size_t>(InventorySlots::OFFHAND)].getCount();
        i32 toAdd = std::min(space, stack.getCount());
        m_items[static_cast<size_t>(InventorySlots::OFFHAND)].grow(toAdd);
        stack.shrink(toAdd);
        if (stack.isEmpty()) {
            return originalCount;
        }
    }

    // 3. 尝试合并到快捷栏（排除当前选中槽位）
    for (i32 i = 0; i < HOTBAR_SIZE; ++i) {
        if (i == m_selectedSlot) continue;
        if (_canMergeStacks(m_items[static_cast<size_t>(i)], stack)) {
            i32 maxStack = std::min(m_items[static_cast<size_t>(i)].getMaxStackSize(), getMaxStackSize());
            i32 space = maxStack - m_items[static_cast<size_t>(i)].getCount();
            i32 toAdd = std::min(space, stack.getCount());
            m_items[static_cast<size_t>(i)].grow(toAdd);
            stack.shrink(toAdd);
            if (stack.isEmpty()) {
                return originalCount;
            }
        }
    }

    // 4. 尝试合并到主背包
    for (i32 i = InventorySlots::MAIN_START; i <= InventorySlots::MAIN_END; ++i) {
        if (_canMergeStacks(m_items[static_cast<size_t>(i)], stack)) {
            i32 maxStack = std::min(m_items[static_cast<size_t>(i)].getMaxStackSize(), getMaxStackSize());
            i32 space = maxStack - m_items[static_cast<size_t>(i)].getCount();
            i32 toAdd = std::min(space, stack.getCount());
            m_items[static_cast<size_t>(i)].grow(toAdd);
            stack.shrink(toAdd);
            if (stack.isEmpty()) {
                return originalCount;
            }
        }
    }

    // 5. 寻找空槽位（同样按优先顺序）
    i32 emptySlot = -1;

    // 当前选中槽位
    if (m_items[static_cast<size_t>(m_selectedSlot)].isEmpty()) {
        emptySlot = m_selectedSlot;
    }

    // 副手槽
    if (emptySlot == -1 && m_items[static_cast<size_t>(InventorySlots::OFFHAND)].isEmpty()) {
        emptySlot = InventorySlots::OFFHAND;
    }

    // 快捷栏（排除当前选中槽位）
    if (emptySlot == -1) {
        for (i32 i = 0; i < HOTBAR_SIZE; ++i) {
            if (i != m_selectedSlot && m_items[static_cast<size_t>(i)].isEmpty()) {
                emptySlot = i;
                break;
            }
        }
    }

    // 主背包
    if (emptySlot == -1) {
        for (i32 i = InventorySlots::MAIN_START; i <= InventorySlots::MAIN_END; ++i) {
            if (m_items[static_cast<size_t>(i)].isEmpty()) {
                emptySlot = i;
                break;
            }
        }
    }

    if (emptySlot != -1) {
        m_items[static_cast<size_t>(emptySlot)] = stack;
        stack = ItemStack::EMPTY;
        return originalCount;
    }

    // 返回已添加的数量
    return originalCount - stack.getCount();
}

i32 PlayerInventory::addInRange(ItemStack& stack, i32 start, i32 end)
{
    if (stack.isEmpty() || start > end) {
        return 0;
    }

    i32 originalCount = stack.getCount();

    // 先尝试合并
    for (i32 i = start; i <= end; ++i) {
        if (_canMergeStacks(m_items[static_cast<size_t>(i)], stack)) {
            i32 maxStack = std::min(m_items[static_cast<size_t>(i)].getMaxStackSize(), getMaxStackSize());
            i32 space = maxStack - m_items[static_cast<size_t>(i)].getCount();
            i32 toAdd = std::min(space, stack.getCount());
            m_items[static_cast<size_t>(i)].grow(toAdd);
            stack.shrink(toAdd);
            if (stack.isEmpty()) {
                return originalCount;
            }
        }
    }

    // 再找空槽位
    for (i32 i = start; i <= end; ++i) {
        if (m_items[static_cast<size_t>(i)].isEmpty()) {
            m_items[static_cast<size_t>(i)] = stack;
            stack = ItemStack::EMPTY;
            return originalCount;
        }
    }

    return originalCount - stack.getCount();
}

ItemStack PlayerInventory::addItemCopy(const ItemStack& stack) const
{
    PlayerInventory tempCopy(nullptr);
    for (i32 i = 0; i < TOTAL_SIZE; ++i) {
        tempCopy.m_items[static_cast<size_t>(i)] = m_items[static_cast<size_t>(i)];
    }

    ItemStack copy = stack.copy();
    tempCopy.add(copy);
    return copy; // 返回剩余的物品
}

// ============================================================================
// 物品查找
// ============================================================================

i32 PlayerInventory::getFirstEmptySlot() const
{
    // 先检查快捷栏
    for (i32 i = 0; i < HOTBAR_SIZE; ++i) {
        if (m_items[static_cast<size_t>(i)].isEmpty()) {
            return i;
        }
    }

    // 再检查主背包
    for (i32 i = InventorySlots::MAIN_START; i <= InventorySlots::MAIN_END; ++i) {
        if (m_items[static_cast<size_t>(i)].isEmpty()) {
            return i;
        }
    }

    return -1;
}

i32 PlayerInventory::findSlot(const Item& item) const
{
    for (i32 i = 0; i < TOTAL_SIZE; ++i) {
        if (m_items[static_cast<size_t>(i)].getItem() == &item) {
            return i;
        }
    }
    return -1;
}

i32 PlayerInventory::findSlotMatching(const ItemStack& stack) const
{
    if (stack.isEmpty()) {
        return -1;
    }

    for (i32 i = 0; i < TOTAL_SIZE; ++i) {
        if (!m_items[static_cast<size_t>(i)].isEmpty() && _stacksEqualExact(stack, m_items[static_cast<size_t>(i)])) {
            return i;
        }
    }
    return -1;
}

i32 PlayerInventory::findSlotMatchingInRange(const ItemStack& stack, i32 start, i32 end) const
{
    if (stack.isEmpty()) {
        return -1;
    }

    for (i32 i = start; i <= end; ++i) {
        if (i >= 0 && i < TOTAL_SIZE && !m_items[static_cast<size_t>(i)].isEmpty() &&
            _stacksEqualExact(stack, m_items[static_cast<size_t>(i)])) {
            return i;
        }
    }
    return -1;
}

// ============================================================================
// 槽位操作
// ============================================================================

void PlayerInventory::swapSlots(i32 slot1, i32 slot2)
{
    if (slot1 < 0 || slot1 >= TOTAL_SIZE || slot2 < 0 || slot2 >= TOTAL_SIZE) {
        return;
    }

    ItemStack temp = m_items[static_cast<size_t>(slot1)];
    m_items[static_cast<size_t>(slot1)] = m_items[static_cast<size_t>(slot2)];
    m_items[static_cast<size_t>(slot2)] = temp;
    m_timesChanged++;
}

ItemStack PlayerInventory::placeItem(i32 slot, ItemStack stack)
{
    if (slot < 0 || slot >= TOTAL_SIZE || stack.isEmpty()) {
        return stack;
    }

    ItemStack& existing = m_items[static_cast<size_t>(slot)];

    if (existing.isEmpty()) {
        // 槽位为空，直接放入
        m_items[static_cast<size_t>(slot)] = stack;
        return ItemStack::EMPTY;
    }

    if (_stacksEqualExact(existing, stack)) {
        // 相同物品，尝试合并
        i32 maxStack = std::min(existing.getMaxStackSize(), getMaxStackSize());
        i32 space = maxStack - existing.getCount();

        if (space > 0) {
            i32 toAdd = std::min(space, stack.getCount());
            existing.grow(toAdd);
            stack.shrink(toAdd);
            m_timesChanged++;

            if (stack.isEmpty()) {
                return ItemStack::EMPTY;
            }
        }
        return stack;
    }

    // 不同物品，交换
    ItemStack result = existing;
    m_items[static_cast<size_t>(slot)] = stack;
    m_timesChanged++;
    return result;
}

// ============================================================================
// 统计
// ============================================================================

i32 PlayerInventory::countItem(const Item& item) const
{
    i32 total = 0;
    for (const auto& stack : m_items) {
        if (stack.getItem() == &item) {
            total += stack.getCount();
        }
    }
    return total;
}

bool PlayerInventory::hasItem(const Item& item) const
{
    for (const auto& stack : m_items) {
        if (stack.getItem() == &item) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// 私有方法
// ============================================================================

bool PlayerInventory::_canMergeStacks(const ItemStack& stack1, const ItemStack& stack2) const
{
    if (stack1.isEmpty() || stack2.isEmpty()) {
        return false;
    }

    // MC 1.16.5: 必须是可堆叠的物品
    if (!stack1.isStackable()) {
        return false;
    }

    // 检查是否可以合并
    if (!_stacksEqualExact(stack1, stack2)) {
        return false;
    }

    // 检查堆叠数是否已达上限
    i32 maxStack = std::min(stack1.getMaxStackSize(), getMaxStackSize());
    return stack1.getCount() < maxStack;
}

bool PlayerInventory::_stacksEqualExact(const ItemStack& stack1, const ItemStack& stack2) const
{
    if (stack1.isEmpty() && stack2.isEmpty()) {
        return true;
    }
    if (stack1.isEmpty() || stack2.isEmpty()) {
        return false;
    }

    // MC 1.16.5: 检查物品类型相同
    if (!stack1.isSameItem(stack2)) {
        return false;
    }

    // 检查耐久度
    if (stack1.getDamage() != stack2.getDamage()) {
        return false;
    }

    // 检查自定义名称
    if (stack1.getCustomName() != stack2.getCustomName()) {
        return false;
    }

    // 检查附魔 - 使用EnchantmentContainer的比较
    const auto& ench1 = stack1.getEnchantments();
    const auto& ench2 = stack2.getEnchantments();
    if (ench1 != ench2) {
        return false;
    }

    // 检查自定义NBT数据
    if (stack1.hasTag() != stack2.hasTag()) {
        return false;
    }
    if (stack1.hasTag() && stack2.hasTag()) {
        const auto* tag1 = stack1.getTag();
        const auto* tag2 = stack2.getTag();
        if (tag1 && tag2 && *tag1 != *tag2) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// IInventory 接口扩展
// ============================================================================

bool PlayerInventory::isUsableByPlayer(const Player& player) const
{
    // MC 1.16.5: 玩家必须存活且在64格范围内
    if (m_player == nullptr) {
        return false;
    }

    // 检查是否是同一个玩家
    if (m_player != &player) {
        return false;
    }

    // 检查玩家是否存活
    if (!player.isAlive()) {
        return false;
    }

    return true;
}

// ============================================================================
// Tick 更新
// ============================================================================

void PlayerInventory::tick()
{
    if (m_player == nullptr) {
        return;
    }

    IWorld* world = m_player->world();
    if (world == nullptr) {
        return;
    }

    // MC 1.16.5: 调用所有物品的 inventoryTick
    for (i32 i = 0; i < TOTAL_SIZE; ++i) {
        if (!m_items[static_cast<size_t>(i)].isEmpty()) {
            m_items[static_cast<size_t>(i)].inventoryTick(*world, *m_player, i, i == m_selectedSlot);
        }
    }

    // MC 1.16.5: 调用护甲的 onArmorTick
    for (i32 i = InventorySlots::ARMOR_START; i <= InventorySlots::ARMOR_END; ++i) {
        if (!m_items[static_cast<size_t>(i)].isEmpty()) {
            m_items[static_cast<size_t>(i)].onArmorTick(*world, *m_player);
        }
    }
}

// ============================================================================
// 物品掉落
// ============================================================================

void PlayerInventory::dropAllItems()
{
    // MC 1.16.5: 遍历所有槽位并掉落物品
    if (m_player == nullptr) {
        return;
    }

    for (auto& item : m_items) {
        if (!item.isEmpty()) {
            m_player->dropItem(item, true, false);
        }
    }
}

void PlayerInventory::deleteStack(const ItemStack& stack)
{
    // MC 1.16.5: 按引用删除物品堆
    for (auto& item : m_items) {
        if (&item == &stack) {
            item = ItemStack::EMPTY;
            break;
        }
    }
}

bool PlayerInventory::placeItemBackInInventory(
    ItemStack& stack, const std::function<void(const ItemStack&, bool)>& dropCallback)
{
    if (stack.isEmpty()) {
        return true;
    }

    // MC 1.16.5: 尝试放回背包
    while (!stack.isEmpty()) {
        // 首先尝试合并到现有槽位
        i32 slot = findSlotMatching(stack);
        if (slot != -1) {
            ItemStack& existing = m_items[static_cast<size_t>(slot)];
            i32 maxStack = std::min(existing.getMaxStackSize(), getMaxStackSize());
            i32 space = maxStack - existing.getCount();
            if (space > 0) {
                i32 toAdd = std::min(space, stack.getCount());
                existing.grow(toAdd);
                stack.shrink(toAdd);
                m_timesChanged++;
                continue;
            }
        }

        // 然后找空槽位
        i32 emptySlot = getFirstEmptySlot();
        if (emptySlot != -1) {
            m_items[static_cast<size_t>(emptySlot)] = stack;
            stack = ItemStack::EMPTY;
            m_timesChanged++;
            return true;
        }

        // 没有空槽位，丢弃剩余物品
        if (dropCallback) {
            dropCallback(stack, false);
        }
        stack = ItemStack::EMPTY;
        return false;
    }

    return true;
}

// ============================================================================
// 护甲操作
// ============================================================================

void PlayerInventory::damageArmor(DamageSource& source, f32 damage)
{
    // MC 1.16.5: PlayerInventory.damageArmor(DamageSource, float)
    if (damage <= 0.0f) {
        return;
    }

    // 将伤害分摊到所有护甲上
    damage = damage / 4.0f;
    if (damage < 1.0f) {
        damage = 1.0f;
    }

    // 护甲槽位索引到 EquipmentSlot 的映射
    // ARMOR_HEAD(36)→Head(5), ARMOR_CHEST(37)→Chest(4), ARMOR_LEGS(38)→Legs(3), ARMOR_FEET(39)→Feet(2)
    static constexpr EquipmentSlot armorEquipmentSlots[] = {
        EquipmentSlot::Head,  // ARMOR_HEAD  = 36
        EquipmentSlot::Chest, // ARMOR_CHEST = 37
        EquipmentSlot::Legs,  // ARMOR_LEGS  = 38
        EquipmentSlot::Feet,  // ARMOR_FEET  = 39
    };

    for (i32 i = InventorySlots::ARMOR_START; i <= InventorySlots::ARMOR_END; ++i) {
        ItemStack& armor = m_items[static_cast<size_t>(i)];
        if (armor.isEmpty()) {
            continue;
        }

        // MC 1.16.5: 火焰伤害不损坏可燃烧的护甲
        if (source.isFire() && armor.getItem()->isBurnable()) {
            continue;
        }

        // MC 1.16.5: 只有 ArmorItem 和 ElytraItem 才会损坏
        if (armor.getItem()->isArmor() && armor.isDamageable()) {
            i32 damageAmount = static_cast<i32>(damage);
            EquipmentSlot slot = armorEquipmentSlots[static_cast<size_t>(i - InventorySlots::ARMOR_START)];

            LivingEntity::hurtAndBreak(armor, damageAmount, m_player, slot);
        }
    }
}

// ============================================================================
// 复制和比较
// ============================================================================

void PlayerInventory::copyInventory(const PlayerInventory& other)
{
    for (i32 i = 0; i < TOTAL_SIZE; ++i) {
        m_items[static_cast<size_t>(i)] = other.m_items[static_cast<size_t>(i)];
    }
    m_selectedSlot = other.m_selectedSlot;
    m_timesChanged++;
}

f32 PlayerInventory::getDestroySpeed(const BlockState& blockState) const
{
    // MC 1.16.5: 获取当前手持物品的挖掘速度
    const ItemStack& selected = getSelectedStack();
    if (selected.isEmpty()) {
        return 1.0f;
    }

    return selected.getDestroySpeed(blockState);
}

// ============================================================================
// NBT 序列化
// ============================================================================

void PlayerInventory::toNbt(nbt::tags::compound_tag& tag) const
{
    using namespace mc::entity::serialization::nbt_keys;

    // ========== Inventory 列表（仅快捷栏和主背包，Slot 0-35）==========
    // MC 1.21.11 新格式：护甲和副手不再存储在 Inventory 列表中，
    // 而是通过 LivingEntity 的 "equipment" 复合标签以 EquipmentSlot 枚举名独立存储。
    // 参考: net.minecraft.world.entity.player.Inventory.save()
    // Player 的 equipment 标签由 LivingEntity::addAdditionalSaveData() 写入，
    // PlayerInventory 仅负责 Inventory 列表和 SelectedItemSlot。
    auto inventoryList = std::make_unique<nbt::tags::compound_list_tag>();
    for (i32 i = 0; i < HOTBAR_SIZE + MAIN_SIZE; ++i) { // 0-35
        const ItemStack& stack = m_items[static_cast<size_t>(i)];
        if (stack.isEmpty()) {
            continue;
        }
        nbt::tags::compound_tag itemTag;
        itemTag.put("Slot", static_cast<i8>(i));
        stack.toNbt(itemTag);
        inventoryList->value.push_back(std::move(itemTag));
    }
    tag.value.emplace(INVENTORY, std::move(inventoryList));

    // 写入当前选中的快捷栏槽位
    tag.put(SELECTED_ITEM_SLOT, m_selectedSlot);
}

Result<PlayerInventory> PlayerInventory::fromNbt(const nbt::tags::compound_tag& tag)
{
    using namespace mc::entity::serialization::nbt_helper;
    using namespace mc::entity::serialization::nbt_keys;

    PlayerInventory inventory(nullptr);

    // ========== 读取装备（MC 1.21.11 新格式：equipment 复合标签）==========
    // 新格式使用 EquipmentSlot 枚举名作为键（"offhand", "feet", "legs", "chest", "head"）
    // 空槽位在 equipment 标签中不存在
    // 参考: net.minecraft.world.entity.LivingEntity.readAdditionalSaveData()
    bool hasEquipment = false;
    if (const auto* equipmentTag = tryGetCompound(tag, EQUIPMENT)) {
        hasEquipment = true;
        // EquipmentSlot 名称到内部背包索引的映射
        static constexpr struct {
            const char* name;
            i32 internalSlot;
        } slotMapping[] = {
            {"offhand", InventorySlots::OFFHAND},
            {"feet", InventorySlots::ARMOR_FEET},
            {"legs", InventorySlots::ARMOR_LEGS},
            {"chest", InventorySlots::ARMOR_CHEST},
            {"head", InventorySlots::ARMOR_HEAD},
        };

        for (const auto& [name, internalSlot] : slotMapping) {
            if (const auto* itemCompound = tryGetCompound(*equipmentTag, name)) {
                auto stackResult = ItemStack::fromNbt(*itemCompound);
                if (stackResult.success() && !stackResult.value().isEmpty()) {
                    inventory.m_items[static_cast<size_t>(internalSlot)] = std::move(stackResult.value());
                }
            }
        }
    }

    // ========== 读取背包物品列表 ==========
    // 新格式（MC 1.21.11）：Inventory 仅包含 Slot 0-35，装备通过 equipment 字段读取
    // 旧格式：Inventory 包含所有槽位（0-40），护甲使用 Slot 100-103，副手使用 Slot -106
    // 为了向后兼容，当 equipment 字段不存在时，仍从 Inventory 列表中读取护甲和副手
    if (const auto* invList = tryGetList(tag, INVENTORY)) {
        if (invList->element_id() == nbt::TagId::Compound) {
            auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*invList);
            for (const auto& itemTag : compoundList.value) {
                // 读取槽位索引（NBT Slot 值）
                i8 nbtSlot = 0;
                if (auto slotOpt = tryGetByte(itemTag, "Slot")) {
                    nbtSlot = *slotOpt;
                } else {
                    continue;
                }

                // 将 NBT Slot 值转换为内部索引
                i32 internalSlot = InventorySlots::fromNbtSlot(static_cast<i32>(nbtSlot));
                if (internalSlot < 0) {
                    continue; // 无效槽位，跳过
                }

                // 如果已有 equipment 字段，跳过护甲和副手槽位（它们已从 equipment 读取）
                // 新格式的 Inventory 列表中 Slot 值仅为 0-35，不会出现护甲/副手的旧编号
                if (hasEquipment && internalSlot >= InventorySlots::ARMOR_START) {
                    continue;
                }

                // 反序列化物品
                auto stackResult = ItemStack::fromNbt(itemTag);
                if (stackResult.success() && !stackResult.value().isEmpty()) {
                    inventory.m_items[static_cast<size_t>(internalSlot)] = std::move(stackResult.value());
                }
            }
        }
    }

    // 读取当前选中的快捷栏槽位
    if (auto slotOpt = tryGetInt(tag, SELECTED_ITEM_SLOT)) {
        inventory.m_selectedSlot = std::clamp(*slotOpt, 0, HOTBAR_SIZE - 1);
    }

    return Result<PlayerInventory>(std::move(inventory));
}

} // namespace mc
