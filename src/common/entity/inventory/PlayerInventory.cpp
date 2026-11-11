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
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/core/Item.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

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

    // 装填分两步，与原版 Inventory.add 的 addResource 一致：
    //
    //   1. getSlotWithRemainingSpace：先看选中槽、再看副手槽、最后从 0 开始按索引递增
    //      扫全部 36 个普通槽位，找**已有同类且未满**的槽；
    //   2. getFreeSlot：同样从 0 递增找**第一个空槽**。
    //
    // 两个细节决定玩家看到的结果：
    //
    //   - **扫描与找空槽都从索引 0 递增**，而 0..8 是快捷栏——所以捡东西会先塞满快捷栏
    //     再去主背包。曾经这里把副手排在快捷栏之前，捡到的东西会凭空出现在副手槽。
    //   - **副手槽只在第一步参与**（原版 getSlotWithRemainingSpace 显式查 getItem(40)），
    //     找空槽时不在候选里（getFreeSlot 只扫 items，长度 36）。
    //
    // 每次合并都受该槽的堆叠上限约束，超出部分继续找下一个槽；因此 100 个石头会铺成
    // 64 + 36 两个槽，而不是塞进一个槽里。
    const i32 maxStackLimit = getMaxStackSize();

    // 第一步：反复找能合并的槽，直到装完或再也找不到。
    //
    // 原版 addResource 只处理一个槽并把剩余量交回去，由上层循环再次调用；这里等价地
    // 写成一个循环。「只合并一个槽就转去填空槽」是错的：槽 1 有 50 个、加入 20 个时，
    // 槽 1 补满 64 后还剩 6 个，应先继续找下一个同类未满的槽（槽 3），而不是新开一个空槽。
    bool mergedAny = true;
    while (mergedAny && !stack.isEmpty()) {
        mergedAny = false;

        i32 mergeSlot = -1;
        if (_hasRemainingSpaceForItem(m_items[static_cast<size_t>(m_selectedSlot)], stack, maxStackLimit)) {
            mergeSlot = m_selectedSlot;
        } else if (_hasRemainingSpaceForItem(
                       m_items[static_cast<size_t>(InventorySlots::OFFHAND)], stack, maxStackLimit)) {
            mergeSlot = InventorySlots::OFFHAND;
        } else {
            for (i32 i = 0; i < InventorySlots::ARMOR_START; ++i) {
                if (_hasRemainingSpaceForItem(m_items[static_cast<size_t>(i)], stack, maxStackLimit)) {
                    mergeSlot = i;
                    break;
                }
            }
        }

        if (mergeSlot == -1) {
            break;
        }

        ItemStack& target = m_items[static_cast<size_t>(mergeSlot)];
        const i32 space = std::min(target.getMaxStackSize(), maxStackLimit) - target.getCount();
        const i32 toAdd = std::min(space, stack.getCount());
        target.grow(toAdd);
        stack.shrink(toAdd);
        mergedAny = toAdd > 0;
    }

    // 第二步：剩余的按空槽依次铺开，每个槽最多装到上限。
    while (!stack.isEmpty()) {
        i32 emptySlot = -1;
        for (i32 i = 0; i < InventorySlots::ARMOR_START; ++i) {
            if (m_items[static_cast<size_t>(i)].isEmpty()) {
                emptySlot = i;
                break;
            }
        }
        if (emptySlot == -1) {
            break;
        }

        const i32 capacity = std::min(stack.getMaxStackSize(), maxStackLimit);
        const i32 toPlace = std::min(capacity, stack.getCount());
        ItemStack placed = stack.split(toPlace);
        m_items[static_cast<size_t>(emptySlot)] = placed;
    }

    return stack.getCount();
}

bool PlayerInventory::_hasRemainingSpaceForItem(
    const ItemStack& target, const ItemStack& incoming, i32 maxStackLimit) const
{
    // 原版 hasRemainingSpaceForItem：能合并（同类、可堆叠）且还没装满。
    // 复用已有的 _canMergeStacks，避免在这里重复一套「同类」判定而与它漂移。
    return _canMergeStacks(target, incoming) && target.getCount() < std::min(target.getMaxStackSize(), maxStackLimit);
}

i32 PlayerInventory::addInRange(ItemStack& stack, i32 start, i32 end)
{
    if (stack.isEmpty() || start > end) {
        return 0;
    }

    // 先尝试合并
    for (i32 i = start; i <= end; ++i) {
        if (_canMergeStacks(m_items[static_cast<size_t>(i)], stack)) {
            i32 maxStack = std::min(m_items[static_cast<size_t>(i)].getMaxStackSize(), getMaxStackSize());
            i32 space = maxStack - m_items[static_cast<size_t>(i)].getCount();
            i32 toAdd = std::min(space, stack.getCount());
            m_items[static_cast<size_t>(i)].grow(toAdd);
            stack.shrink(toAdd);
            if (stack.isEmpty()) {
                return 0;
            }
        }
    }

    // 再找空槽位
    for (i32 i = start; i <= end; ++i) {
        if (m_items[static_cast<size_t>(i)].isEmpty()) {
            m_items[static_cast<size_t>(i)] = stack;
            stack = ItemStack::EMPTY;
            return 0;
        }
    }

    // 返回剩余未添加的数量（约定见头文件 @return，与 add 同语义修复）。
    return stack.getCount();
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
    // MC 1.16.5: PlayerInventory.damageArmor(DamageSource, f32)
    if (damage <= 0.0f) {
        return;
    }

    // 将伤害分摊到所有护甲上
    damage = damage / 4.0f;
    if (damage < 1.0f) {
        damage = 1.0f;
    }

    // 护甲槽位索引到 EquipmentSlot 的映射
    // 从 ARMOR_START(36) 起按索引递增遍历，对应顺序是 FEET→LEGS→CHEST→HEAD
    static constexpr EquipmentSlot armorEquipmentSlots[] = {
        EquipmentSlot::Feet,  // ARMOR_FEET  = 36
        EquipmentSlot::Legs,  // ARMOR_LEGS  = 37
        EquipmentSlot::Chest, // ARMOR_CHEST = 38
        EquipmentSlot::Head,  // ARMOR_HEAD  = 39
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

    // ========== 读取装备（MC 1.21.11 格式：equipment 复合标签）==========
    // 使用 EquipmentSlot 枚举名作为键（"offhand", "feet", "legs", "chest", "head"），
    // 空槽位在 equipment 标签中不存在。
    if (const auto* equipmentTag = tryGetCompound(tag, EQUIPMENT)) {
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
    // MC 1.21.11：Inventory 仅包含 Slot 0-35（快捷栏与主背包），装备经 equipment 字段读取。
    if (const auto* invList = tryGetList(tag, INVENTORY)) {
        if (invList->element_id() == nbt::TagId::Compound) {
            auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*invList);
            for (const auto& itemTag : compoundList.value) {
                // 读取槽位索引（NBT Slot 值，0-35）
                i8 nbtSlot = 0;
                if (auto slotOpt = tryGetByte(itemTag, "Slot")) {
                    nbtSlot = *slotOpt;
                } else {
                    continue;
                }

                if (nbtSlot < 0 || nbtSlot > InventorySlots::MAIN_END) {
                    continue; // 越界槽位，跳过
                }

                // 反序列化物品
                auto stackResult = ItemStack::fromNbt(itemTag);
                if (stackResult.success() && !stackResult.value().isEmpty()) {
                    inventory.m_items[static_cast<size_t>(nbtSlot)] = std::move(stackResult.value());
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
