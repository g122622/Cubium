#include "server/network/play/ContainerActionHandler.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/container/AnvilContainer.hpp"
#include "common/entity/inventory/container/MerchantContainerMenu.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/interaction/ContainerManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <spdlog/spdlog.h>

namespace mc::server::net {

Player* ContainerActionHandler::_player(PlayerId playerId) const
{
    const auto* data = m_server.playerManager().getPlayer(playerId);
    if (data == nullptr || !data->loggedIn || !data->hasClientLoaded()) {
        return nullptr;
    }
    auto* world = m_server.getPlayerWorld(playerId);
    return world != nullptr ? m_server.playerEntityManager().getPlayerEntity(playerId, *world) : nullptr;
}

AbstractContainerMenu* ContainerActionHandler::_menu(PlayerId playerId, Player& player) const
{
    auto& containers = m_server.containerManager();
    auto* menu = containers.getOpenMenu(playerId);
    if (menu == nullptr) {
        menu = containers.getPlayerInventoryMenu(playerId);
    }
    if (menu != nullptr && !menu->stillValid(player)) {
        spdlog::warn("Container action: player {} interacted with an invalid menu", playerId);
        return nullptr;
    }
    return menu;
}

void ContainerActionHandler::handleRenameItemPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = _player(playerId);
    if (player == nullptr || player->isSpectator()) {
        return;
    }
    auto* anvil = dynamic_cast<AnvilContainer*>(_menu(playerId, *player));
    if (anvil == nullptr) {
        return;
    }
    const auto& request =
        std::get<mc::network::ir::play::RenameItem>(std::get<mc::network::ir::PlayPacket>(packet.packet));
    anvil->setItemName(request.name);
}

void ContainerActionHandler::handleSelectTradePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = _player(playerId);
    if (player == nullptr || player->isSpectator()) {
        return;
    }
    auto* menu = dynamic_cast<MerchantContainerMenu*>(_menu(playerId, *player));
    if (menu == nullptr) {
        return;
    }
    const auto& request =
        std::get<mc::network::ir::play::SelectTrade>(std::get<mc::network::ir::PlayPacket>(packet.packet));
    if (request.index < 0 || request.index >= static_cast<i32>(menu->getOffers().size())) {
        spdlog::warn("SelectTrade: player {} selected invalid offer {}", playerId, request.index);
        return;
    }
    menu->setSelectionHint(request.index);
    menu->tryMoveItems(request.index);
    menu->detectAndSendChanges();
}

void ContainerActionHandler::handleContainerButtonClickPacket(
    PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = _player(playerId);
    if (player == nullptr || player->isSpectator()) {
        return;
    }
    auto* menu = _menu(playerId, *player);
    const auto& request =
        std::get<mc::network::ir::play::ContainerButtonClick>(std::get<mc::network::ir::PlayPacket>(packet.packet));
    if (menu != nullptr && menu->getId() == request.containerId && menu->clickMenuButton(*player, request.buttonId)) {
        menu->detectAndSendChanges();
    }
}

} // namespace mc::server::net
