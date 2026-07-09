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

#include "world/blockentity/core/LootableContainerBlockEntity.hpp"
#include "entity/attribute/Attributes.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/serialization/NbtHelper.hpp"
#include "item/loot/LootTable.hpp"
#include "item/loot/LootTableManager.hpp"
#include "item/loot/context/LootContext.hpp"
#include "item/loot/context/LootContextBuilder.hpp"
#include "item/loot/context/LootParameterSets.hpp"
#include "item/loot/context/LootParams.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"

namespace mc {
namespace blockentity {

// ========== 构造函数 ==========

LootableContainerBlockEntity::LootableContainerBlockEntity(BlockEntityType type, const BlockPos& pos)
    : LockableBlockEntity(type, pos)
{}

// ========== 打开权限检查 ==========

bool LootableContainerBlockEntity::canOpen(const Player* player, const ItemStack& heldItem) const
{
    // 先检查基类的锁定规则
    if (!LockableBlockEntity::canOpen(player, heldItem)) {
        return false;
    }

    // 当战利品表尚未填充时，观察者模式玩家不能打开容器
    if (m_hasLootTable && player != nullptr && player->isSpectator()) {
        return false;
    }

    return true;
}

// ========== 战利品表接口 ==========

void LootableContainerBlockEntity::setLootTable(const ResourceLocation& lootTable, i64 seed)
{
    m_hasLootTable = true;
    m_lootTable = lootTable;
    m_lootTableSeed = seed;
    m_lootFilled = false;
    setChanged();
}

// ========== 延迟填充战利品表 ==========

void LootableContainerBlockEntity::_unpackLootTable(Player* player) const
{
    // 仅在战利品表尚未填充时触发
    if (!m_hasLootTable || m_lootFilled) {
        return;
    }

    // 需要有世界引用来获取 LootTableManager
    if (m_world == nullptr) {
        return;
    }

    // 获取战利品表管理器
    const loot::LootTableManager* lootTableManager = m_world->lootTableManager();
    if (lootTableManager == nullptr) {
        // 在客户端或未初始化的服务端，无法填充
        return;
    }

    // const_cast 是安全的：填充战利品是缓存初始化行为，而非逻辑状态变更。
    // MC Java 的 RandomizableContainer.unpackLootTable 同样从 const/默认接口方法中修改内部状态。
    // m_lootFilled 已声明为 mutable，m_hasLootTable 和 m_lootTable 在填充后仅被清除，
    // 不会影响对象的逻辑一致性。
    auto& self = const_cast<LootableContainerBlockEntity&>(*this);
    self.fillWithLootFromTable(const_cast<loot::LootTableManager&>(*lootTableManager), player);
}

// ========== 容器访问重写 ==========

bool LootableContainerBlockEntity::isEmpty() const
{
    // 在检查物品之前，先触发战利品表填充
    // 参考: net.minecraft.RandomizableContainerBlockEntity.isEmpty()
    _unpackLootTable(nullptr);
    return LockableBlockEntity::isEmpty();
}

void LootableContainerBlockEntity::clearContainer()
{
    // 在清空容器之前，先触发战利品表填充
    // 参考: net.minecraft.RandomizableContainerBlockEntity.clearContent()
    _unpackLootTable(nullptr);
    LockableBlockEntity::clearContainer();
}

void LootableContainerBlockEntity::openContainer(Player* player)
{
    // 当战利品表尚未填充时，观察者模式玩家不能打开容器，防止观察者触发战利品生成
    if (m_hasLootTable && player != nullptr && player->isSpectator()) {
        return;
    }

    // 触发战利品表填充（带玩家上下文，包含幸运值等信息）
    // 参考: net.minecraft.RandomizableContainerBlockEntity.createMenu() 中 unpackLootTable(player)
    _unpackLootTable(player);

    LockableBlockEntity::openContainer(player);
}

// ========== 序列化 ==========

bool LootableContainerBlockEntity::load(const nlohmann::json& data)
{
    if (!LockableBlockEntity::load(data)) {
        return false;
    }

    // 加载战利品表
    if (data.contains("LootTable") && data["LootTable"].is_string()) {
        m_lootTable = ResourceLocation(data["LootTable"].get<std::string>());
        m_hasLootTable = true;
        m_lootFilled = false;
    }
    if (data.contains("LootTableSeed") && data["LootTableSeed"].is_number_integer()) {
        m_lootTableSeed = data["LootTableSeed"].get<i64>();
    }

    return true;
}

void LootableContainerBlockEntity::save(nlohmann::json& data) const
{
    LockableBlockEntity::save(data);

    // 保存战利品表（仅在未填充时保存）
    if (m_hasLootTable && !m_lootFilled) {
        data["LootTable"] = m_lootTable.toString();
        if (m_lootTableSeed != 0) {
            data["LootTableSeed"] = m_lootTableSeed;
        }
    }
}

// ============================================================================
// 序列化 - NBT（结构模板 / 客户端同步）
// ============================================================================

// 子类（ChestEntity/BarrelEntity/ShulkerBoxEntity/DispenserBlockEntity 等）通过
// 重写 loadFromNBT/saveToNBT，调用基类方法处理战利品表引用后，再调用基类 protected
// 辅助方法 saveItemsToNBT/loadItemsFromNBT 序列化容器物品列表（"Items" NBT 键）。
// LootTable/LootTableSeed 与 Items 互斥：未解包的战利品表存在时只持久化引用，
// 已解包或无战利品表时持久化实际物品（与 MC Java RandomizableContainer 一致）。

namespace {
/// NBT 键名
constexpr const char* LOOT_TABLE_TAG = "LootTable";
constexpr const char* LOOT_TABLE_SEED_TAG = "LootTableSeed";
constexpr const char* ITEMS_TAG = "Items";
} // namespace

bool LootableContainerBlockEntity::loadFromNBT(const nbt::CompoundTag& tag)
{
    if (!BlockEntity::loadFromNBT(tag)) {
        return false;
    }

    namespace nbt_helper = mc::entity::serialization::nbt_helper;

    // 战利品表：无论 LootTable 是否存在都读取种子，
    // 但种子仅在 hasLootTable 为 true 时才有意义。
    m_hasLootTable = false;
    m_lootTable = ResourceLocation();
    m_lootTableSeed = 0;
    m_lootFilled = false;

    auto lootTableOpt = nbt_helper::tryGetString(tag, LOOT_TABLE_TAG);
    if (lootTableOpt.has_value()) {
        m_lootTable = ResourceLocation(lootTableOpt.value());
        m_hasLootTable = true;
    }
    // 种子始终读取（缺失默认 0，表示使用随机种子填充）
    m_lootTableSeed = nbt_helper::tryGetLong(tag, LOOT_TABLE_SEED_TAG).value_or(0);

    return true;
}

void LootableContainerBlockEntity::saveToNBT(nbt::CompoundTag& tag) const
{
    BlockEntity::saveToNBT(tag);

    // 仅在已设置战利品表且尚未填充时保存
    // 已填充后战利品表引用清空，物品应由子类通过 saveItemsToNBT 持久化到 "Items" 键
    if (m_hasLootTable && !m_lootFilled) {
        tag.put(LOOT_TABLE_TAG, m_lootTable.toString());
        if (m_lootTableSeed != 0) {
            tag.put(LOOT_TABLE_SEED_TAG, m_lootTableSeed);
        }
    }
}

// ========== 容器物品 NBT 序列化辅助 ==========

void LootableContainerBlockEntity::saveItemsToNBT(nbt::CompoundTag& tag, const IInventory& inventory) const
{
    auto itemsList = std::make_unique<nbt::tags::compound_list_tag>();
    const i32 containerSize = inventory.getContainerSize();
    for (i32 slot = 0; slot < containerSize; ++slot) {
        const ItemStack& stack = inventory.getItem(slot);
        if (stack.isEmpty()) {
            continue;
        }
        nbt::tags::compound_tag itemTag;
        itemTag.put("Slot", static_cast<i8>(slot));
        stack.toNbt(itemTag);
        itemsList->value.push_back(std::move(itemTag));
    }
    tag.value.insert_or_assign(ITEMS_TAG, std::move(itemsList));
}

void LootableContainerBlockEntity::loadItemsFromNBT(const nbt::CompoundTag& tag, IInventory& inventory)
{
    namespace nbt_helper = mc::entity::serialization::nbt_helper;

    inventory.clear();

    const auto* listTag = nbt_helper::tryGetList(tag, ITEMS_TAG);
    if (listTag == nullptr || listTag->element_id() != nbt::TagId::Compound) {
        return;
    }

    const auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*listTag);
    const i32 containerSize = inventory.getContainerSize();
    for (const auto& itemTag : compoundList.value) {
        i8 slot = 0;
        if (auto slotOpt = nbt_helper::tryGetByte(itemTag, "Slot")) {
            slot = *slotOpt;
        }
        if (slot < 0 || slot >= containerSize) {
            continue;
        }
        auto stackResult = ItemStack::fromNbt(itemTag);
        if (!stackResult.success()) {
            continue;
        }
        inventory.setItem(slot, stackResult.value());
    }
}

// ========== 战利品填充 ==========

void LootableContainerBlockEntity::fillWithLoot(Player* player)
{
    // 如果没有战利品表或已填充，不执行
    if (!m_hasLootTable || m_lootFilled) {
        return;
    }

    // 需要有世界引用来获取 LootTableManager
    if (m_world == nullptr) {
        return;
    }

    // 获取战利品表管理器
    const loot::LootTableManager* lootTableManager = m_world->lootTableManager();
    if (lootTableManager == nullptr) {
        // 在客户端或未初始化的服务端，无法填充
        return;
    }

    // 调用实际的填充方法
    // 注：const_cast 是安全的，因为 fillWithLootFromTable 不修改管理器
    fillWithLootFromTable(const_cast<loot::LootTableManager&>(*lootTableManager), player);
}

bool LootableContainerBlockEntity::fillWithLootFromTable(loot::LootTableManager& lootTableManager, Player* player)
{
    // 如果没有战利品表或已填充，不执行
    if (!m_hasLootTable || m_lootFilled) {
        return false;
    }

    // 获取战利品表
    const loot::LootTable* table = lootTableManager.getTable(m_lootTable.toString());
    if (table == nullptr) {
        // 战利品表不存在，清除标记
        m_hasLootTable = false;
        m_lootFilled = true;
        return false;
    }

    // 需要有世界引用
    if (m_world == nullptr) {
        return false;
    }

    // 清除战利品表标记（在填充之前，防止递归）
    m_hasLootTable = false;
    m_lootFilled = true;

    // 创建随机数生成器
    math::Random rng;
    if (m_lootTableSeed != 0) {
        rng = math::Random(static_cast<u64>(m_lootTableSeed));
    }

    // 创建战利品上下文
    // 使用 CHEST 参数集，包含位置参数
    loot::LootContextBuilder builder(*m_world);

    // 设置种子
    if (m_lootTableSeed != 0) {
        builder.withSeed(static_cast<u64>(m_lootTableSeed));
    } else {
        builder.withRandom(rng);
    }

    // 设置位置参数
    BlockPos lootPos = m_pos;
    builder.withParameter(loot::LootParams::BLOCK_POS, &lootPos);

    // 设置玩家相关参数（如果有）
    if (player != nullptr) {
        // 设置玩家幸运值（来自属性系统，如幸运药水效果等）
        f32 luck = static_cast<f32>(player->getAttributeValue(entity::attribute::Attributes::LUCK, 0.0));
        builder.withLuck(luck);

        // 设置实体参数，使战利品条件可以引用玩家
        builder.withParameter(loot::LootParams::THIS_ENTITY, static_cast<Entity*>(player));
    }

    // 设置战利品表解析器（支持嵌套战利品表）
    builder.withLootTableResolver(
        [&lootTableManager](const std::string& id) -> const loot::LootTable* { return lootTableManager.getTable(id); });
    builder.withPredicateResolver([&lootTableManager](const std::string& id) -> const loot::LootCondition* {
        return lootTableManager.getPredicate(id);
    });

    auto context = builder.build(loot::LootParameterSets::chest());

    // 生成物品
    std::vector<ItemStack> items = table->generate(*context);

    // 填充到容器中
    IInventory* inventory = getInventory();
    if (inventory == nullptr) {
        return false;
    }

    i32 containerSize = inventory->getContainerSize();
    for (ItemStack stack : items) {
        if (stack.isEmpty()) {
            continue;
        }

        // 尝试找到可堆叠的槽位
        for (i32 slot = 0; slot < containerSize && !stack.isEmpty(); ++slot) {
            ItemStack existing = inventory->getItem(slot);
            if (!existing.isEmpty() && existing.canMergeWith(stack) &&
                existing.getCount() < existing.getMaxStackSize()) {
                // 尝试堆叠
                i32 space = existing.getMaxStackSize() - existing.getCount();
                i32 toAdd = std::min(space, stack.getCount());
                existing.grow(toAdd);
                inventory->setItem(slot, existing);
                stack.shrink(toAdd);
            }
        }

        // 剩余物品放入空槽位
        for (i32 slot = 0; slot < containerSize && !stack.isEmpty(); ++slot) {
            if (inventory->getItem(slot).isEmpty()) {
                inventory->setItem(slot, stack);
                break;
            }
        }
    }

    setChanged();
    return true;
}

} // namespace blockentity
} // namespace mc
