#include "ContainerPacketHandler.hpp"
#include "../../entity/entities/player/Player.hpp"
#include "../../entity/inventory/PlayerInventory.hpp"
#include "../../entity/inventory/Slot.hpp"
#include "../../entity/inventory/AbstractContainerMenu.hpp"
#include "../../entity/inventory/ContainerTypes.hpp"

namespace mc {

// ============================================================================
// ContainerPacketHandler 实现
// ============================================================================

bool ContainerPacketHandler::handleContainerClick(Player& player, const ContainerClickPacket& packet) {
    auto* menu = player.openContainerMenu();
    if (menu == nullptr || menu->getId() != packet.containerId()) {
        return false;
    }

    menu->setCarriedItem(packet.cursorItem());
    const ClickType clickType = ContainerTypes::toClickType(packet.action(), packet.button());
    menu->clicked(packet.slotIndex(), packet.button(), clickType, player);
    return true;
}

void ContainerPacketHandler::handleCloseContainer(Player& player, const CloseContainerPacket& packet) {
    auto* menu = player.openContainerMenu();
    if (menu == nullptr || menu->getId() != packet.containerId()) {
        return;
    }

    menu->removed(player);
    player.clearOpenContainerMenu();
}

void ContainerPacketHandler::handleHotbarSelect(Player& player, const HotbarSelectPacket& packet) {
    // 设置玩家选中的快捷栏槽位
    player.inventory().setSelectedSlot(packet.slot());
}

ContainerContentPacket ContainerPacketHandler::createContentPacket(const AbstractContainerMenu& menu) {
    std::vector<ItemStack> items;
    items.reserve(static_cast<size_t>(menu.getSlotCount()));

    for (i32 i = 0; i < menu.getSlotCount(); ++i) {
        const Slot* slot = menu.getSlot(i);
        if (slot != nullptr) {
            items.push_back(slot->getItem());
        } else {
            items.push_back(ItemStack::EMPTY);
        }
    }

    return ContainerContentPacket(menu.getId(), std::move(items));
}

ContainerSlotPacket ContainerPacketHandler::createSlotPacket(const AbstractContainerMenu& menu, i32 slotIndex) {
    const Slot* slot = menu.getSlot(slotIndex);
    ItemStack item = (slot != nullptr) ? slot->getItem() : ItemStack::EMPTY;
    return ContainerSlotPacket(menu.getId(), slotIndex, item);
}

OpenContainerPacket ContainerPacketHandler::createOpenContainerPacket(ContainerId containerId, u8 type,
                                                                       const String& title, i32 slotCount) {
    return OpenContainerPacket(containerId, type, title, slotCount);
}

RecipeListSyncPacket ContainerPacketHandler::createRecipeListPacket() {
    std::vector<RecipeSyncPacket> recipes;

    // 从 RecipeManager 获取所有配方
    const auto allRecipes = crafting::RecipeManager::instance().getAllRecipes();
    recipes.reserve(allRecipes.size());

    for (const auto* recipe : allRecipes) {
        if (recipe != nullptr) {
            // 配方ID和类型
            ResourceLocation id = recipe->getId();
            String typeStr = recipeTypeToString(recipe->getType());

            // TODO: 实现配方序列化
            // String recipeData = recipe->serialize();

            recipes.emplace_back(
                id,
                typeStr,
                ""  // 暂时为空，等待配方序列化实现
            );
        }
    }

    return RecipeListSyncPacket(std::move(recipes));
}

CraftResultPreviewPacket ContainerPacketHandler::createCraftResultPreview(ContainerId containerId,
                                                                           const AbstractContainerMenu& menu) {
    // 查找合成结果槽位
    // 对于 CraftingMenu，结果槽位是 RESULT_SLOT
    // 对于 InventoryCraftingMenu，结果槽位也是 RESULT_SLOT

    // 尝试获取结果槽位
    const i32 resultSlotIndex = menu.getResultSlotIndex();
    const Slot* resultSlot = (resultSlotIndex >= 0) ? menu.getSlot(resultSlotIndex) : nullptr;
    ItemStack resultItem = (resultSlot != nullptr) ? resultSlot->getItem() : ItemStack::EMPTY;

    // 尝试获取匹配的配方ID
    ResourceLocation recipeId;
    // TODO: 从结果槽位获取配方ID

    return CraftResultPreviewPacket(containerId, resultItem, recipeId);
}

// ============================================================================
// ContainerTypes 实现
// ============================================================================

namespace ContainerTypes {

i32 getSlotCount(ContainerType type) {
    switch (type) {
        case ContainerType::Player: return 46;      // 36背包 + 4护甲 + 5合成 + 1结果
        case ContainerType::Chest: return 27;       // 普通箱子
        case ContainerType::CraftingTable: return 10; // 9个格子 + 1个结果
        case ContainerType::Furnace: return 3;
        case ContainerType::Dispenser: return 9;
        case ContainerType::Enchantment: return 2;
        case ContainerType::Anvil: return 3;
        case ContainerType::BrewingStand: return 5;
        case ContainerType::Villager: return 3;
        case ContainerType::Beacon: return 1;
        case ContainerType::Hopper: return 5;
        case ContainerType::ShulkerBox: return 27;
        default: return 0;
    }
}

const char* getDefaultTitle(ContainerType type) {
    switch (type) {
        case ContainerType::Player: return "Inventory";
        case ContainerType::Chest: return "Chest";
        case ContainerType::CraftingTable: return "Crafting";
        case ContainerType::Furnace: return "Furnace";
        case ContainerType::Dispenser: return "Dispenser";
        case ContainerType::Enchantment: return "Enchanting";
        case ContainerType::Anvil: return "Anvil";
        case ContainerType::BrewingStand: return "Brewing Stand";
        case ContainerType::Villager: return "Villager";
        case ContainerType::Beacon: return "Beacon";
        case ContainerType::Hopper: return "Hopper";
        case ContainerType::ShulkerBox: return "Shulker Box";
        default: return "Container";
    }
}

u8 toNetworkType(ContainerType type) {
    return static_cast<u8>(type);
}

ClickType toClickType(ClickAction action, i32 button) {
    switch (action) {
        case ClickAction::Pick:
            return (button == 0) ? ClickType::Pick : ClickType::PickSome;
        case ClickAction::PickAll:
            return ClickType::PickAll;
        case ClickAction::Throw:
            return (button == 0) ? ClickType::Throw : ClickType::ThrowAll;
        case ClickAction::ThrowAll:
            return ClickType::ThrowAll;
        case ClickAction::Pickup:
            return ClickType::Pickup;
        case ClickAction::QuickMove:
            return ClickType::QuickMove;
        case ClickAction::Clone:
            return ClickType::Clone;
        case ClickAction::Spread:
            return ClickType::QuickCraft;
        case ClickAction::Swap:
            return ClickType::Swap;
        default:
            return ClickType::Pick;
    }
}

ClickAction toClickAction(ClickType clickType) {
    switch (clickType) {
        case ClickType::Pick:
        case ClickType::Place:
        case ClickType::PlaceSome:
        case ClickType::PlaceAll:
        case ClickType::PickSome:
            return ClickAction::Pick;
        case ClickType::PickAll:
            return ClickAction::PickAll;
        case ClickType::Throw:
            return ClickAction::Throw;
        case ClickType::ThrowAll:
            return ClickAction::ThrowAll;
        case ClickType::QuickMove:
            return ClickAction::QuickMove;
        case ClickType::QuickCraft:
            return ClickAction::Spread;
        case ClickType::Clone:
            return ClickAction::Clone;
        case ClickType::Pickup:
            return ClickAction::Pickup;
        case ClickType::Swap:
            return ClickAction::Swap;
        default:
            return ClickAction::Pick;
    }
}

} // namespace ContainerTypes

} // namespace mc
