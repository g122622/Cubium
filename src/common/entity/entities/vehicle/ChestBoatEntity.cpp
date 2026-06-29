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

#include "ChestBoatEntity.hpp"

#include "common/entity/ai/util/PiglinAi.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/AbstractContainerMenu.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/entity/inventory/container/ChestContainer.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/redstone/RedstoneHelper.hpp"

#include <cmath>

namespace mc {
namespace entity {

// ============================================================================
// 常量
// ============================================================================

namespace {
/// 玩家与箱子船的有效交互距离（格）的平方
constexpr f64 INTERACTION_RANGE_SQ = 64.0; // 8格的平方
} // namespace

// ============================================================================
// 工厂方法
// ============================================================================

std::unique_ptr<Entity> ChestBoatEntity::create(IWorld* /*world*/)
{
    return std::make_unique<ChestBoatEntity>();
}

ChestBoatEntity::ChestBoatEntity(Type type)
    : BoatEntity(type)
    , m_inventory(std::make_unique<blockentity::SimpleInventory>(CONTAINER_SIZE))
{
    // 箱子船始终设置 hasChest 标志
    setHasChest(true);

    // 设置容器变更回调（用于红stone比较器更新等）
    m_inventory->setOnChanged([this]() {
        // 容器内容变更时的回调（当前为空，后续可添加比较器更新逻辑）
    });
}

// ============================================================================
// Entity 接口重写
// ============================================================================

ActionResultType ChestBoatEntity::processInitialInteract(Player& player, Hand hand)
{
    // 先尝试基类交互（乘坐）
    // BoatEntity::processInitialInteract 会在玩家不蹲下且船可控时让玩家乘坐
    ActionResultType result = BoatEntity::processInitialInteract(player, hand);
    if (result != ActionResultType::Pass) {
        return result;
    }

    // 到达这里说明玩家蹲下或船失控或船已满
    // 如果玩家本可以乘坐但基类仍返回了 Pass，说明是船失控等边缘情况
    // 此时不应打开容器，而是继续传递 Pass
    if (canAddPassenger(player) && !player.isSneaking()) {
        return ActionResultType::Pass;
    }

    // 玩家蹲下或船已满时，打开容器菜单
    if (!player.openContainer(*this)) {
        return ActionResultType::Fail;
    }

    // 4. 触发容器打开游戏事件和猪灵愤怒
    if (m_world && !m_world->isClientSide()) {
        m_world->gameEvent(gameevent::GameEvents::CONTAINER_OPEN,
            BlockPos(static_cast<i32>(std::floor(x())),
                static_cast<i32>(std::floor(y())),
                static_cast<i32>(std::floor(z()))),
            gameevent::GameEvent::Context::of(&player));
        entity::PiglinAi::angerNearbyPiglins(*m_world, player, true);
    }

    return ActionResultType::Success;
}

void ChestBoatEntity::dropItem()
{
    IWorld* worldPtr = world();
    if (!worldPtr || worldPtr->isClientSide()) {
        return;
    }

    // 先掉落容器内容
    dropInventoryContents();

    // 掉落船物品本身（箱子船物品）
    const Item* boatItem = getBoatItem();
    if (boatItem == nullptr) {
        return;
    }

    // 检查游戏规则 doEntityDrops
    if (!worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS)) {
        return;
    }

    ItemStack stack(*boatItem, 1);
    if (hasCustomName()) {
        stack.setCustomName(customNameText());
    }

    math::Random& rng = worldPtr->getRandom();
    ItemDropHelper::spawnItemEntity(worldPtr, stack, x(), y(), z(), rng, ItemDropHelper::DEFAULT_PICKUP_DELAY);
}

void ChestBoatEntity::remove()
{
    // 在服务端且实体被销毁时，掉落容器内容
    if (m_world && !m_world->isClientSide()) {
        dropInventoryContents();
    }

    BoatEntity::remove();
}

f64 ChestBoatEntity::getMountedYOffset() const
{
    // 箱子船的乘客Y偏移
    return -0.1;
}

const Item* ChestBoatEntity::getBoatItem() const
{
    // 重写基类方法，始终返回箱子船物品
    switch (getBoatType()) {
        case Type::OAK:
            return Items::OAK_CHEST_BOAT;
        case Type::SPRUCE:
            return Items::SPRUCE_CHEST_BOAT;
        case Type::BIRCH:
            return Items::BIRCH_CHEST_BOAT;
        case Type::JUNGLE:
            return Items::JUNGLE_CHEST_BOAT;
        case Type::ACACIA:
            return Items::ACACIA_CHEST_BOAT;
        case Type::DARK_OAK:
            return Items::DARK_OAK_CHEST_BOAT;
        case Type::MANGROVE:
            return Items::MANGROVE_CHEST_BOAT;
        case Type::CHERRY:
            return Items::CHERRY_CHEST_BOAT;
        case Type::PALE_OAK:
            return Items::PALE_OAK_CHEST_BOAT;
        case Type::BAMBOO:
            return Items::BAMBOO_CHEST_RAFT;
        default:
            return Items::OAK_CHEST_BOAT;
    }
}

i32 ChestBoatEntity::getComparatorOutput() const
{
    return world::redstone::RedstoneHelper::calcRedstoneFromInventory(*m_inventory);
}

void ChestBoatEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    using namespace mc::entity::serialization;

    BoatEntity::addAdditionalSaveData(tag);

    // 保存容器物品栏
    if (!m_lootTable.empty()) {
        // 如果有战利品表，只保存战利品表引用（不保存物品）
        tag.put(nbt_keys::LOOT_TABLE, m_lootTable);
        tag.put(nbt_keys::LOOT_TABLE_SEED, m_lootTableSeed);
    } else {
        // 没有战利品表，保存物品
        auto itemsList = std::make_unique<nbt::tags::compound_list_tag>();
        for (i32 i = 0; i < CONTAINER_SIZE; ++i) {
            const ItemStack& stack = m_inventory->getItem(i);
            if (!stack.isEmpty()) {
                nbt::tags::compound_tag itemTag;
                itemTag.put("Slot", static_cast<i8>(i));
                stack.toNbt(itemTag);
                itemsList->value.push_back(std::move(itemTag));
            }
        }
        tag.value.insert_or_assign(nbt_keys::ITEMS, std::move(itemsList));
    }
}

Result<void> ChestBoatEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace mc::entity::serialization;

    auto result = BoatEntity::readAdditionalSaveData(tag);
    if (!result.success()) {
        return result;
    }

    // 清空容器
    m_inventory->clear();
    m_lootTable.clear();
    m_lootTableSeed = 0L;

    // 读取战利品表
    if (auto lootTableOpt = nbt_helper::tryGetString(tag, nbt_keys::LOOT_TABLE)) {
        m_lootTable = *lootTableOpt;
        if (auto seedOpt = nbt_helper::tryGetLong(tag, nbt_keys::LOOT_TABLE_SEED)) {
            m_lootTableSeed = *seedOpt;
        }
    } else {
        // 读取物品
        if (auto itemsList = nbt_helper::tryGetList(tag, nbt_keys::ITEMS)) {
            if (itemsList->element_id() == nbt::TagId::Compound) {
                auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*itemsList);
                for (const auto& itemTag : compoundList.value) {
                    i8 slot = 0;
                    if (auto slotOpt = nbt_helper::tryGetByte(itemTag, "Slot")) {
                        slot = *slotOpt;
                    }
                    if (slot >= 0 && slot < CONTAINER_SIZE) {
                        auto stackResult = ItemStack::fromNbt(itemTag);
                        if (stackResult.success()) {
                            m_inventory->setItem(slot, stackResult.value());
                        }
                    }
                }
            }
        }
    }

    return Result<void>();
}

// ============================================================================
// INamedContainerProvider 接口实现
// ============================================================================

std::unique_ptr<AbstractContainerMenu> ChestBoatEntity::createMenu(i32 containerId, Player& player)
{
    // 如果有战利品表且玩家是旁观者，不允许打开
    // TODO: 当战利品表系统完善后，在此解包战利品表

    // 创建3行9列的箱子容器菜单
    auto menu = std::make_unique<blockentity::ChestContainer>(ContainerId(containerId),
        &player.inventory(),
        m_inventory.get(),
        blockentity::ChestContainer::SINGLE_CHEST_ROWS);

    return menu;
}

std::string ChestBoatEntity::getDisplayName() const
{
    return "container.chestBoat";
}

// ============================================================================
// IInventory 代理方法
// ============================================================================

i32 ChestBoatEntity::getContainerSize() const
{
    return m_inventory->getContainerSize();
}

bool ChestBoatEntity::isInventoryEmpty() const
{
    return m_inventory->isEmpty();
}

ItemStack ChestBoatEntity::getInventoryItem(i32 slot) const
{
    return m_inventory->getItem(slot);
}

void ChestBoatEntity::setInventoryItem(i32 slot, const ItemStack& stack)
{
    m_inventory->setItem(slot, stack);
}

ItemStack ChestBoatEntity::removeInventoryItem(i32 slot, i32 count)
{
    return m_inventory->removeItem(slot, count);
}

void ChestBoatEntity::clearInventory()
{
    m_inventory->clear();
}

IInventory* ChestBoatEntity::getInventory()
{
    return m_inventory.get();
}

bool ChestBoatEntity::stillValid(const Player& player) const
{
    // 玩家距离实体8格以内为有效
    if (isRemoved()) {
        return false;
    }
    f32 distSq = player.distanceSqTo(*this);
    return static_cast<f64>(distSq) <= INTERACTION_RANGE_SQ;
}

// ============================================================================
// 私有方法
// ============================================================================

void ChestBoatEntity::dropInventoryContents()
{
    IWorld* worldPtr = world();
    if (!worldPtr || worldPtr->isClientSide()) {
        return;
    }

    // 受 doEntityDrops 游戏规则控制
    if (!worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS)) {
        return;
    }

    if (!m_inventory) {
        return;
    }

    math::Random& rng = worldPtr->getRandom();
    for (i32 i = 0; i < CONTAINER_SIZE; ++i) {
        ItemStack stack = m_inventory->getItem(i);
        if (!stack.isEmpty()) {
            ItemDropHelper::spawnItemEntity(worldPtr, stack, x(), y(), z(), rng, ItemDropHelper::DEFAULT_PICKUP_DELAY);
        }
    }
    m_inventory->clear();
}

} // namespace entity
} // namespace mc
