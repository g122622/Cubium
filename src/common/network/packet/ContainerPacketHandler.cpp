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

OpenContainerPacket ContainerPacketHandler::createOpenContainerPacket(ContainerId containerId, i32 type,
                                                                       const std::string& title) {
    return OpenContainerPacket(containerId, type, title);
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
            std::string typeStr = recipeTypeToString(recipe->getType());

            // TODO: 实现配方序列化
            // std::string recipeData = recipe->serialize();

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
        case ContainerType::Generic9x1: return 9;        // 1行箱子
        case ContainerType::Generic9x2: return 18;       // 2行箱子
        case ContainerType::Generic9x3: return 27;       // 3行箱子（普通大箱子）
        case ContainerType::Generic9x4: return 36;       // 4行箱子
        case ContainerType::Generic9x5: return 45;       // 5行箱子
        case ContainerType::Generic9x6: return 54;       // 6行箱子（最大箱子）
        case ContainerType::Generic3x3: return 9;        // 发射器/投掷器
        case ContainerType::Anvil: return 3;             // 铁砧（2输入+1输出）
        case ContainerType::Beacon: return 1;            // 信标
        case ContainerType::BlastFurnace: return 3;      // 高炉（输入+燃料+输出）
        case ContainerType::BrewingStand: return 5;      // 酿造台（1燃料+3药水+1材料）
        case ContainerType::Crafting: return 10;         // 工作台（9格网格+1输出）
        case ContainerType::Enchantment: return 2;       // 附魔台（1输入+1青金石）
        case ContainerType::Furnace: return 3;           // 熔炉（输入+燃料+输出）
        case ContainerType::Grindstone: return 2;        // 砂轮（输入+输出）
        case ContainerType::Hopper: return 5;            // 漏斗
        case ContainerType::Lectern: return 1;           // 讲台
        case ContainerType::Loom: return 4;              // 织布机（3输入+1输出）
        case ContainerType::Merchant: return 3;          // 村民交易（2输入+1输出）
        case ContainerType::ShulkerBox: return 27;       // 潜影盒（与3行箱子相同）
        case ContainerType::Smithing: return 3;          // 锻造台（2输入+1输出）
        case ContainerType::Smoker: return 3;            // 烟熏炉（输入+燃料+输出）
        case ContainerType::Cartography: return 3;       // 制图台（2输入+1输出）
        case ContainerType::Stonecutter: return 2;       // 切石机（1输入+1输出）
        case ContainerType::Player: return 46;           // 玩家背包（36背包+4护甲+5合成+1结果）
        default: return 0;
    }
}

const char* getDefaultTitle(ContainerType type) {
    switch (type) {
        case ContainerType::Generic9x1: return "Chest";
        case ContainerType::Generic9x2: return "Chest";
        case ContainerType::Generic9x3: return "Chest";
        case ContainerType::Generic9x4: return "Chest";
        case ContainerType::Generic9x5: return "Chest";
        case ContainerType::Generic9x6: return "Chest";
        case ContainerType::Generic3x3: return "Dispenser";
        case ContainerType::Anvil: return "Anvil";
        case ContainerType::Beacon: return "Beacon";
        case ContainerType::BlastFurnace: return "Blast Furnace";
        case ContainerType::BrewingStand: return "Brewing Stand";
        case ContainerType::Crafting: return "Crafting";
        case ContainerType::Enchantment: return "Enchanting";
        case ContainerType::Furnace: return "Furnace";
        case ContainerType::Grindstone: return "Grindstone";
        case ContainerType::Hopper: return "Hopper";
        case ContainerType::Lectern: return "Lectern";
        case ContainerType::Loom: return "Loom";
        case ContainerType::Merchant: return "Villager";
        case ContainerType::ShulkerBox: return "Shulker Box";
        case ContainerType::Smithing: return "Smithing Table";
        case ContainerType::Smoker: return "Smoker";
        case ContainerType::Cartography: return "Cartography Table";
        case ContainerType::Stonecutter: return "Stonecutter";
        case ContainerType::Player: return "Inventory";
        default: return "Container";
    }
}

u8 toNetworkType(ContainerType type) {
    return static_cast<u8>(type);
}

ClickType toClickType(ClickAction action, i32 button) {
    switch (action) {
        case ClickAction::Pickup:
            // PICKUP: button 0 = 左键拾取/放置, button 1 = 右键拾取一半/放置一个
            return (button == 0) ? ClickType::Pick : ClickType::PickSome;
        case ClickAction::QuickMove:
            return ClickType::QuickMove;
        case ClickAction::Swap:
            return ClickType::Swap;
        case ClickAction::Clone:
            return ClickType::Clone;
        case ClickAction::Throw:
            return (button == 0) ? ClickType::Throw : ClickType::ThrowAll;
        case ClickAction::QuickCraft:
            return ClickType::QuickCraft;
        case ClickAction::PickupAll:
            return ClickType::PickAll;
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
            return ClickAction::Pickup;
        case ClickType::PickAll:
            return ClickAction::PickupAll;
        case ClickType::Throw:
        case ClickType::ThrowAll:
            return ClickAction::Throw;
        case ClickType::QuickMove:
            return ClickAction::QuickMove;
        case ClickType::QuickCraft:
            return ClickAction::QuickCraft;
        case ClickType::Clone:
            return ClickAction::Clone;
        case ClickType::Pickup:
            return ClickAction::Pickup;
        case ClickType::Swap:
            return ClickAction::Swap;
        default:
            return ClickAction::Pickup;
    }
}

} // namespace ContainerTypes

} // namespace mc
