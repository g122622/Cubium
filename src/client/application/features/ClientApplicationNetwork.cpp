#include "../ClientApplication.hpp"

#include "client/command/ClientCommandManager.hpp"
#include "client/renderer/trident/block/BreakProgressManager.hpp"
#include "client/skin/ClientSkinManager.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/network/SkinPackets.hpp"
#include "client/sound/AudioService.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "client/ui/minecraft/widgets/ChatWidget.hpp"
#include "client/ui/screen/AbstractContainerScreen.hpp"
#include "client/ui/screen/ChestScreen.hpp"
#include "client/ui/screen/CraftingScreen.hpp"
#include "client/ui/screen/FurnaceScreen.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "client/world/player/ClientPlayerPredictor.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/ExperiencePackets.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/BlockRegistry.hpp"

#include <algorithm>
#include <memory>
#include <vector>

namespace mc::client {

namespace {

template<typename Menu>
AbstractContainerScreen<Menu>* asContainerScreen(IScreen* screen)
{
    return dynamic_cast<AbstractContainerScreen<Menu>*>(screen);
}

template<typename Menu>
void applyContainerContent(AbstractContainerScreen<Menu>* screen, ContainerId containerId, const std::vector<ItemStack>& items)
{
    if (!screen) {
        return;
    }

    Menu* menu = screen->getMenu();
    if (!menu || menu->getId() != containerId) {
        return;
    }

    const i32 slotCount = std::min(menu->getSlotCount(), static_cast<i32>(items.size()));
    for (i32 slotIndex = 0; slotIndex < slotCount; ++slotIndex) {
        if (Slot* slot = menu->getSlot(slotIndex)) {
            slot->set(items[static_cast<size_t>(slotIndex)]);
        }
    }
}

template<typename Menu>
bool applyContainerSlot(AbstractContainerScreen<Menu>* screen, ContainerId containerId, i32 slotIndex, const ItemStack& item)
{
    if (!screen) {
        return false;
    }

    Menu* menu = screen->getMenu();
    if (!menu || menu->getId() != containerId) {
        return false;
    }

    Slot* slot = menu->getSlot(slotIndex);
    if (!slot) {
        return false;
    }

    slot->set(item);
    return true;
}

std::function<void(ContainerId, i32, i32, ClickAction, const ItemStack&)> makeContainerClickSender(NetworkClient* networkClient)
{
    return [networkClient](ContainerId containerId, i32 slotIndex, i32 button, ClickAction action, const ItemStack& cursorItem) {
        if (networkClient) {
            networkClient->sendContainerClick(ContainerClickPacket(containerId, slotIndex, button, action, cursorItem));
        }
    };
}

std::function<void(ContainerId)> makeContainerCloseSender(NetworkClient* networkClient)
{
    return [networkClient](ContainerId containerId) {
        if (networkClient) {
            networkClient->sendCloseContainer(containerId);
        }
    };
}

} // namespace

void ClientApplication::setupNetworkCallbacks()
{
    MC_TRACE_EVENT("client.initialization", "SetupNetworkCallbacks");

    if (!m_networkClient) return;

    NetworkClientCallbacks callbacks;

    callbacks.onLoginSuccess = [this](PlayerId playerId, EntityId entityId, const String& username) {
        spdlog::info("Login successful: playerId={}, entityId={}, username={}", playerId, entityId, username);

        // 设置本地玩家身份
        m_localIdentity.setIdentity(playerId, entityId);

        // 创建本地玩家实体
        auto& entityManager = m_world.entityManager();
        ClientEntity* playerEntity = entityManager.spawnLocalPlayer(entityId, playerId, username);
        if (playerEntity) {
            spdlog::info("Local player entity created: entityId={}", entityId);
        }

        // 初始化玩家预测器
        m_predictor = std::make_unique<ClientPlayerPredictor>();

        if (m_player) {
            m_player->setPlayerId(playerId);
        }
        m_knownPlayerNames[playerId] = username;
    };

    callbacks.onLoginFailed = [this](const String& reason) {
        spdlog::error("Login failed: {}", reason);

        m_knownPlayerNames.clear();
        if (m_commandManager) {
            m_commandManager->clear();
        }
        stop();
    };

    callbacks.onDisconnected = [this](const String& reason) {
        spdlog::info("Disconnected from server: {}", reason);

        // 清除本地玩家身份
        m_localIdentity.clear();

        // 清除本地玩家实体
        m_world.entityManager().clearLocalPlayer();

        // 清除预测器
        m_predictor.reset();

        m_knownPlayerNames.clear();
        if (m_commandManager) {
            m_commandManager->clear();
        }
        m_hasServerTimeSync = false;
        if (m_integratedServer) {
            m_integratedServer->stop();
            m_integratedServer.reset();
        }
        stop();
    };

    callbacks.onCommandTree = [this](const String& treeJson) {
        if (!m_commandManager) {
            m_commandManager = std::make_unique<command::ClientCommandManager>();
        }
        auto result = m_commandManager->applyCommandTreeJson(treeJson);
        if (result.failed()) {
            spdlog::warn("Failed to apply command tree: {}", result.error().toString());
            return;
        }
        m_commandManager->setPlayerNameProvider([this]() {
            return collectPlayerCompletionCandidates();
        });
        m_commandManager->setEntityNameProvider([this]() {
            return collectEntityCompletionCandidates();
        });
    };

    callbacks.onTeleport = [this](f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId) {
        MC_UNUSED(teleportId);
        if (!m_player) {
            return;
        }

        // 传送时重置预测器
        if (m_predictor) {
            m_predictor->reset(Vector3(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z)), yaw, pitch);
        }

        m_player->setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
        m_player->setRotation(yaw, pitch);
        m_lastSentX = static_cast<f32>(x);
        m_lastSentY = static_cast<f32>(y);
        m_lastSentZ = static_cast<f32>(z);
        m_lastSentYaw = yaw;
        m_lastSentPitch = pitch;
    };

    callbacks.onChunkData = [this](ChunkCoord x, ChunkCoord z, DimensionId dimension, const std::vector<u8>& data) {
        MC_UNUSED(dimension);
        m_world.onChunkData(x, z, std::vector<u8>(data));
    };

    callbacks.onChunkUnload = [this](ChunkCoord x, ChunkCoord z, DimensionId dimension) {
        MC_UNUSED(dimension);
        m_world.onChunkUnload(x, z);
    };

    callbacks.onPlayerSpawn = [this](PlayerId playerId, const String& username, f64 x, f64 y, f64 z) {
        m_knownPlayerNames[playerId] = username;

        // 使用 LocalPlayerIdentity 判断是否是本地玩家
        if (m_localIdentity.isLocalPlayer(playerId)) {
            // 本地玩家：设置位置
            if (m_player) {
                m_player->setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
            }
            return;
        }

        // 远程玩家：创建或更新实体
        auto& entityManager = m_world.entityManager();

        // 远程玩家使用 playerId 作为 entityId（服务端在 EntityTracker 中这样处理）
        const EntityId entityId = static_cast<EntityId>(playerId);
        ClientEntity* entity = entityManager.spawnEntity(entityId, mc::entity::EntityTypes::PLAYER);
        if (!entity) {
            entity = entityManager.getEntity(entityId);
        }

        if (entity) {
            entity->setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
        }
    };

    callbacks.onPlayerDespawn = [this](PlayerId playerId) {
        m_knownPlayerNames.erase(playerId);

        // 使用 LocalPlayerIdentity 判断是否是本地玩家
        if (m_localIdentity.isLocalPlayer(playerId)) {
            return;
        }

        // 远程玩家：移除实体
        m_world.entityManager().removeEntity(static_cast<EntityId>(playerId));
    };

    callbacks.onBlockUpdate = [this](i32 x, i32 y, i32 z, u32 blockStateId) {
        m_world.setBlock(x, y, z, BlockRegistry::instance().getBlockState(blockStateId));
    };

    callbacks.onChatMessage = [this](const String& message, PlayerId senderId) {
        if (m_kageroEngine) {
            auto* chatWidget = static_cast<ui::minecraft::widgets::ChatWidget*>(m_kageroEngine->getLayer(m_chatLayerId));
            if (chatWidget) {
                const auto it = m_knownPlayerNames.find(senderId);
                const String senderName = (senderId != 0 && it != m_knownPlayerNames.end())
                    ? it->second
                    : String();
                if (!senderName.empty()) {
                    chatWidget->addMessage(senderName + ": " + message, 0xFFFFFFFF);
                } else {
                    chatWidget->addMessage(message, 0xFFFFFFFF);
                }
            }
        }
    };

    callbacks.onPlayerMove = [this](PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch) {
        // 使用 LocalPlayerIdentity 判断是否是本地玩家
        if (m_localIdentity.isLocalPlayer(playerId)) {
            if (m_player) {
                m_player->setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
                m_player->setRotation(yaw, pitch);
            }
            return;
        }

        ClientEntity* entity = m_world.entityManager().getEntity(static_cast<EntityId>(playerId));
        if (!entity) {
            return;
        }

        // 使用目标位置进行平滑插值
        entity->setTargetPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
        entity->setTargetRotation(yaw, pitch);
    };

    callbacks.onTimeUpdate = [this](i64 gameTime, i64 dayTime, bool daylightCycleEnabled) {
        m_world.onTimeUpdate(gameTime, dayTime, daylightCycleEnabled);
    };

    callbacks.onPlayerInventory = [this](i32 selectedSlot, const std::vector<ItemStack>& items) {
        if (!m_player) {
            return;
        }

        auto& inventory = m_player->inventory();
        inventory.setSelectedSlot(selectedSlot);
        for (i32 slotIndex = 0; slotIndex < static_cast<i32>(items.size()) && slotIndex < inventory.getContainerSize(); ++slotIndex) {
            inventory.setItem(slotIndex, items[static_cast<size_t>(slotIndex)]);
        }

        if (auto* inventoryScreen = asContainerScreen<mc::InventoryCraftingMenu>(ScreenManager::instance().getCurrentScreen())) {
            applyContainerContent(inventoryScreen, mc::inventory::PLAYER_CONTAINER_ID, items);
        }
    };

    callbacks.onOpenContainer = [this](const OpenContainerPacket& packet) {
        if (!m_player) {
            return;
        }

        if (ScreenManager::instance().hasScreen()) {
            ScreenManager::instance().closeAll();
        }

        const ContainerType type = static_cast<ContainerType>(packet.type());
        std::unique_ptr<IScreen> screen;

        switch (type) {
            case ContainerType::Player:
                screen = std::make_unique<InventoryCraftingScreen>(
                    std::make_unique<mc::InventoryCraftingMenu>(packet.containerId(), &m_player->inventory()),
                    makeContainerClickSender(m_networkClient.get()),
                    makeContainerCloseSender(m_networkClient.get()));
                break;

            case ContainerType::CraftingTable:
                screen = std::make_unique<CraftingScreen>(
                    std::make_unique<mc::CraftingMenu>(packet.containerId(), &m_player->inventory(), nullptr),
                    makeContainerClickSender(m_networkClient.get()),
                    makeContainerCloseSender(m_networkClient.get()));
                break;

            case ContainerType::Chest: {
                const i32 rows = std::max(1, packet.slotCount() / mc::blockentity::ChestContainer::SLOTS_PER_ROW);
                screen = std::make_unique<ChestScreen>(
                    packet.containerId(),
                    &m_player->inventory(),
                    rows,
                    makeContainerClickSender(m_networkClient.get()),
                    makeContainerCloseSender(m_networkClient.get()));
                break;
            }

            case ContainerType::Furnace:
                screen = std::make_unique<FurnaceScreen>(
                    packet.containerId(),
                    &m_player->inventory(),
                    makeContainerClickSender(m_networkClient.get()),
                    makeContainerCloseSender(m_networkClient.get()));
                break;

            default:
                spdlog::warn("Ignored unsupported container type {}", static_cast<i32>(type));
                break;
        }

        if (!screen) {
            return;
        }

        if (m_renderer && m_renderer->isGuiRendererInitialized()) {
            if (auto* inventoryContainerScreen = dynamic_cast<AbstractContainerScreen<mc::InventoryCraftingMenu>*>(screen.get())) {
                inventoryContainerScreen->setRenderers(
                    &m_renderer->guiRenderer(),
                    m_guiTextureManager.get(),
                    m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
                inventoryContainerScreen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
            } else if (auto* craftingContainerScreen = dynamic_cast<AbstractContainerScreen<mc::CraftingMenu>*>(screen.get())) {
                craftingContainerScreen->setRenderers(
                    &m_renderer->guiRenderer(),
                    m_guiTextureManager.get(),
                    m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
                craftingContainerScreen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
            } else if (auto* chestContainerScreen = dynamic_cast<AbstractContainerScreen<mc::blockentity::ChestContainer>*>(screen.get())) {
                chestContainerScreen->setRenderers(
                    &m_renderer->guiRenderer(),
                    m_guiTextureManager.get(),
                    m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
                chestContainerScreen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
            } else if (auto* furnaceContainerScreen = dynamic_cast<AbstractContainerScreen<mc::blockentity::FurnaceContainer>*>(screen.get())) {
                furnaceContainerScreen->setRenderers(
                    &m_renderer->guiRenderer(),
                    m_guiTextureManager.get(),
                    m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
                furnaceContainerScreen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
            }
        }

        ScreenManager::instance().openScreen(std::move(screen));
    };

    callbacks.onContainerContent = [this](const ContainerContentPacket& packet) {
        if (!m_player) {
            return;
        }

        IScreen* currentScreen = ScreenManager::instance().getCurrentScreen();
        if (auto* inventoryScreen = asContainerScreen<mc::InventoryCraftingMenu>(currentScreen)) {
            applyContainerContent(inventoryScreen, packet.containerId(), packet.items());
            return;
        }

        if (auto* craftingScreen = asContainerScreen<mc::CraftingMenu>(currentScreen)) {
            applyContainerContent(craftingScreen, packet.containerId(), packet.items());
            return;
        }

        if (auto* chestScreen = asContainerScreen<mc::blockentity::ChestContainer>(currentScreen)) {
            applyContainerContent(chestScreen, packet.containerId(), packet.items());
            return;
        }

        if (auto* furnaceScreen = asContainerScreen<mc::blockentity::FurnaceContainer>(currentScreen)) {
            applyContainerContent(furnaceScreen, packet.containerId(), packet.items());
        }
    };

    callbacks.onContainerSlot = [this](const ContainerSlotPacket& packet) {
        if (!m_player) {
            return;
        }

        IScreen* currentScreen = ScreenManager::instance().getCurrentScreen();
        if (auto* inventoryScreen = asContainerScreen<mc::InventoryCraftingMenu>(currentScreen)) {
            if (applyContainerSlot(inventoryScreen, packet.containerId(), packet.slotIndex(), packet.item())) {
                return;
            }
        }

        if (auto* craftingScreen = asContainerScreen<mc::CraftingMenu>(currentScreen)) {
            if (applyContainerSlot(craftingScreen, packet.containerId(), packet.slotIndex(), packet.item())) {
                return;
            }
        }

        if (auto* chestScreen = asContainerScreen<mc::blockentity::ChestContainer>(currentScreen)) {
            if (applyContainerSlot(chestScreen, packet.containerId(), packet.slotIndex(), packet.item())) {
                return;
            }
        }

        if (auto* furnaceScreen = asContainerScreen<mc::blockentity::FurnaceContainer>(currentScreen)) {
            (void)applyContainerSlot(furnaceScreen, packet.containerId(), packet.slotIndex(), packet.item());
        }
    };

    callbacks.onCloseContainer = [this](ContainerId containerId) {
        IScreen* currentScreen = ScreenManager::instance().getCurrentScreen();
        if (!currentScreen) {
            return;
        }

        auto matches = [containerId](auto* screen) {
            return screen && screen->getMenu() && screen->getMenu()->getId() == containerId;
        };

        if (matches(asContainerScreen<mc::InventoryCraftingMenu>(currentScreen)) ||
            matches(asContainerScreen<mc::CraftingMenu>(currentScreen)) ||
            matches(asContainerScreen<mc::blockentity::ChestContainer>(currentScreen)) ||
            matches(asContainerScreen<mc::blockentity::FurnaceContainer>(currentScreen))) {
            ScreenManager::instance().closeScreen();
        }
    };

    callbacks.onSpawnEntity = [this](u32 entityId, const String& typeId, f32 x, f32 y, f32 z, f32 yaw, f32 pitch, const ItemStack* itemStack) {
        auto& entityManager = m_world.entityManager();
        ClientEntity* entity = entityManager.spawnEntity(static_cast<EntityId>(entityId), typeId);
        if (!entity) {
            entity = entityManager.getEntity(static_cast<EntityId>(entityId));
        }

        if (!entity) {
            return;
        }

        entity->setPosition(x, y, z);
        entity->setRotation(yaw, pitch);
        entity->setVelocity(0.0f, 0.0f, 0.0f);

        if (typeId == mc::entity::EntityTypes::ITEM && itemStack) {
            entity->setItemStack(*itemStack);
        }
    };

    callbacks.onSpawnMob = [this](u32 entityId, const String& typeId, f32 x, f32 y, f32 z, f32 yaw, f32 pitch, f32 headYaw) {
        auto& entityManager = m_world.entityManager();
        ClientEntity* entity = entityManager.spawnEntity(static_cast<EntityId>(entityId), typeId);
        if (!entity) {
            entity = entityManager.getEntity(static_cast<EntityId>(entityId));
        }

        if (!entity) {
            return;
        }

        // 出生时使用立即设置（不是插值）
        entity->setPosition(x, y, z);
        entity->setRotation(yaw, pitch);
        entity->setHeadRotation(headYaw);
    };

    callbacks.onEntityDestroy = [this](const std::vector<u32>& entityIds) {
        auto& entityManager = m_world.entityManager();
        for (u32 entityId : entityIds) {
            // 使用 LocalPlayerIdentity 判断是否是本地玩家实体
            if (m_localIdentity.isLocalPlayerEntity(static_cast<EntityId>(entityId))) {
                continue;
            }
            entityManager.removeEntity(static_cast<EntityId>(entityId));
        }
    };

    callbacks.onEntityMove = [this](u32 entityId, f32 deltaX, f32 deltaY, f32 deltaZ) {
        // 使用 LocalPlayerIdentity 判断是否是本地玩家实体
        const EntityId eid = static_cast<EntityId>(entityId);
        if (m_localIdentity.isLocalPlayerEntity(eid)) {
            if (m_player) {
                const Vector3 position = m_player->position();
                m_player->setPosition(position.x + deltaX, position.y + deltaY, position.z + deltaZ);
            }
            return;
        }

        ClientEntity* entity = m_world.entityManager().getEntity(eid);
        if (!entity) {
            return;
        }

        const Vector3 position = entity->targetPosition();
        entity->setTargetPosition(position.x + deltaX, position.y + deltaY, position.z + deltaZ);
    };

    callbacks.onEntityTeleport = [this](u32 entityId, f32 x, f32 y, f32 z, f32 yaw, f32 pitch) {
        // 使用 LocalPlayerIdentity 判断是否是本地玩家实体
        const EntityId eid = static_cast<EntityId>(entityId);
        if (m_localIdentity.isLocalPlayerEntity(eid)) {
            if (m_player) {
                // 本地玩家：交给预测器处理服务端确认
                if (m_predictor) {
                    m_predictor->receiveServerPosition(Vector3(x, y, z), yaw, pitch);
                } else {
                    // 如果预测器未初始化，直接设置位置
                    m_player->setPosition(x, y, z);
                    m_player->setRotation(yaw, pitch);
                }
            }
            return;
        }

        ClientEntity* entity = m_world.entityManager().getEntity(eid);
        if (!entity) {
            return;
        }

        // 传送使用立即设置（不是插值）
        entity->setPosition(x, y, z);
        entity->setRotation(yaw, pitch);
    };

    callbacks.onEntityVelocity = [this](u32 entityId, i16 vx, i16 vy, i16 vz) {
        const f32 scale = 1.0f / 8000.0f;
        // 使用 LocalPlayerIdentity 判断是否是本地玩家实体
        const EntityId eid = static_cast<EntityId>(entityId);
        if (m_localIdentity.isLocalPlayerEntity(eid)) {
            if (m_player) {
                m_player->setVelocity(static_cast<f32>(vx) * scale,
                                      static_cast<f32>(vy) * scale,
                                      static_cast<f32>(vz) * scale);
            }
            return;
        }

        ClientEntity* entity = m_world.entityManager().getEntity(eid);
        if (!entity) {
            return;
        }

        entity->setVelocity(static_cast<f32>(vx) * scale,
                            static_cast<f32>(vy) * scale,
                            static_cast<f32>(vz) * scale);
    };

    callbacks.onEntityMetadata = [this](u32 entityId, const std::vector<u8>& metadata) {
        // 使用 LocalPlayerIdentity 判断是否是本地玩家实体
        const EntityId eid = static_cast<EntityId>(entityId);
        if (m_localIdentity.isLocalPlayerEntity(eid)) {
            return;
        }

        ClientEntity* entity = m_world.entityManager().getEntity(eid);
        if (!entity) {
            return;
        }

        entity->setMetadata(metadata);
    };

    callbacks.onEntityAnimation = [this](u32 entityId, u8 animation) {
        MC_UNUSED(entityId);
        MC_UNUSED(animation);
    };

    callbacks.onEntityHeadLook = [this](u32 entityId, f32 headYaw) {
        // 使用 LocalPlayerIdentity 判断是否是本地玩家实体
        const EntityId eid = static_cast<EntityId>(entityId);
        if (m_localIdentity.isLocalPlayerEntity(eid)) {
            return;
        }

        ClientEntity* entity = m_world.entityManager().getEntity(eid);
        if (!entity) {
            return;
        }

        // 使用目标头部旋转进行平滑插值
        entity->setTargetHeadRotation(headYaw);
    };

    callbacks.onEntityStatus = [this](u32 entityId, u8 status) {
        MC_UNUSED(entityId);
        MC_UNUSED(status);
    };

    callbacks.onGameModeChange = [this](GameMode gameMode) {
        if (!m_player) {
            return;
        }

        spdlog::info("Game mode changed to: {}", static_cast<i32>(gameMode));
        m_player->setGameMode(gameMode);

        closeInventoryScreenIfModeMismatch();
    };

    callbacks.onRainStrengthChange = [this](f32 rainStrength) {
        m_world.onRainStrengthChange(rainStrength);
    };

    callbacks.onThunderStrengthChange = [this](f32 thunderStrength) {
        m_world.onThunderStrengthChange(thunderStrength);
    };

    callbacks.onBeginRaining = [this]() {
        m_world.onBeginRaining();
    };

    callbacks.onEndRaining = [this]() {
        m_world.onEndRaining();
    };

    callbacks.onPlayerAbilities = [this](bool invulnerable, bool flying, bool canFly, bool creativeMode, f32 flySpeed, f32 walkSpeed) {
        spdlog::debug("Player abilities updated: invulnerable={}, flying={}, canFly={}, creativeMode={}",
                      invulnerable, flying, canFly, creativeMode);
        if (m_player) {
            PlayerAbilities& abilities = m_player->abilities();
            abilities.invulnerable = invulnerable;
            abilities.flying = flying;
            abilities.canFly = canFly;
            abilities.creativeMode = creativeMode;
            abilities.flySpeed = flySpeed;
            abilities.walkSpeed = walkSpeed;
        }

        closeInventoryScreenIfModeMismatch();
    };

    callbacks.onLightUpdate = [this](i32 chunkX, i32 chunkZ, i32 sectionY,
                                      const std::vector<u8>& skyLight,
                                      const std::vector<u8>& blockLight,
                                      bool trustEdges) {
        m_world.onLightUpdate(chunkX, chunkZ, sectionY, skyLight, blockLight, trustEdges);
    };

    callbacks.onBlockBreakAnim = [this](EntityId breakerEntityId, i32 x, i32 y, i32 z, i8 stage) {
        using namespace mc::client::renderer::trident::block;
        auto& manager = BreakProgressManager::instance();

        BlockPos pos(x, y, z);
        u64 currentTick = static_cast<u64>(m_world.gameTime());

        if (stage < 0) {
            manager.removeRemoteProgress(breakerEntityId);
        } else {
            manager.updateRemoteProgress(breakerEntityId, pos, stage, currentTick);
        }
    };

    callbacks.onPlaySound = [this](const ResourceLocation& soundEventId,
                                   mc::sound::SoundCategory category,
                                   f32 x,
                                   f32 y,
                                   f32 z,
                                   f32 volume,
                                   f32 pitch) {
        if (!m_audioService) {
            spdlog::warn("Received sound event '{}' but audio service is not initialized", soundEventId.toString());
            return;
        }

        auto sound = sound::SoundInstance::createLocated(
            soundEventId,
            category,
            x,
            y,
            z,
            volume,
            pitch);

        m_audioService->play(std::make_unique<sound::SoundInstance>(std::move(sound)));
    };

    callbacks.onStopSound = [this](const Optional<ResourceLocation>& soundEventId,
                                   const Optional<mc::sound::SoundCategory>& category) {
        if (!m_audioService) {
            return;
        }

        if (!soundEventId.has_value() && !category.has_value()) {
            m_audioService->stopAll();
            return;
        }

        if (soundEventId.has_value()) {
            m_audioService->stop(*soundEventId);
            return;
        }

        if (category.has_value()) {
            m_audioService->stop(*category);
        }
    };

    callbacks.onSetExperience = [this](f32 progress, i32 totalXp, i32 level) {
        if (!m_player) {
            return;
        }

        m_player->setExperience(level, progress, totalXp);
    };

    callbacks.onSpawnExperienceOrb = [this](u32 entityId, f64 x, f64 y, f64 z, i16 xpValue) {
        auto& entityManager = m_world.entityManager();
        ClientEntity* entity = entityManager.spawnEntity(static_cast<EntityId>(entityId), mc::entity::EntityTypes::EXPERIENCE_ORB);
        if (!entity) {
            entity = entityManager.getEntity(static_cast<EntityId>(entityId));
        }

        if (!entity) {
            return;
        }

        entity->setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
        entity->setXpValue(static_cast<i32>(xpValue));
    };

    // 玩家列表回调 - 皮肤系统集成
    callbacks.onPlayerListAdd = [this](const std::vector<::mc::skin::PlayerListEntry>& entries) {
        if (!m_skinManager) {
            return;
        }

        for (const auto& entry : entries) {
            // 创建玩家档案并注册皮肤
            ::mc::skin::GameProfile profile(entry.uuid, entry.name);
            for (const auto& prop : entry.properties) {
                profile.addProperty(prop);
            }

            m_skinManager->registerPlayerSkin(profile);
            spdlog::debug("PlayerList: Registered skin for {} ({})", entry.name,
                         profile.uuidToString());
        }
    };

    callbacks.onPlayerListRemove = [this](const std::vector<std::array<u8, 16>>& uuids) {
        for (const auto& uuid : uuids) {
            m_skinManager->skinManager().removePlayerInfo(uuid);
            spdlog::debug("PlayerList: Removed player skin");
        }
    };

    callbacks.onPlayerListUpdateGameMode = [this](const ::mc::skin::PlayerListEntry& entry) {
        MC_UNUSED(entry);
        // 游戏模式更新 - 暂时不需要特殊处理
    };

    callbacks.onPlayerListUpdateLatency = [this](const std::array<u8, 16>& uuid, i32 ping) {
        MC_UNUSED(uuid);
        MC_UNUSED(ping);
        // 延迟更新 - 暂时不需要特殊处理
    };

    callbacks.onPlayerListUpdateDisplayName = [this](const std::array<u8, 16>& uuid, const Optional<String>& displayName) {
        MC_UNUSED(uuid);
        MC_UNUSED(displayName);
        // 显示名更新 - 暂时不需要特殊处理
    };

    m_networkClient->setCallbacks(callbacks);
}

std::vector<String> ClientApplication::collectPlayerCompletionCandidates() const
{
    std::vector<String> candidates;
    candidates.reserve(m_knownPlayerNames.size() + 1);

    for (const auto& [playerId, playerName] : m_knownPlayerNames) {
        MC_UNUSED(playerId);
        if (!playerName.empty()) {
            candidates.push_back(playerName);
        }
    }

    if (m_player) {
        const auto& username = m_player->username();
        if (!username.empty()) {
            candidates.push_back(username);
        }
    }

    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
    return candidates;
}

std::vector<String> ClientApplication::collectEntityCompletionCandidates() const
{
    return collectPlayerCompletionCandidates();
}

void ClientApplication::handleChatCommand(const String& input)
{
    if (input.empty()) {
        return;
    }

    auto* chatWidget = m_kageroEngine ?
        static_cast<ui::minecraft::widgets::ChatWidget*>(m_kageroEngine->getLayer(m_chatLayerId)) : nullptr;

    if (chatWidget) {
        chatWidget->addMessage(input, 0xFFFFFFFF);
    }

    if (input[0] == '/') {
        String command = input.substr(1);

        spdlog::info("Chat command received: {}", std::string(command.begin(), command.end()));

        if (m_networkClient && m_networkClient->isLoggedIn()) {
            m_networkClient->sendChatMessage(input);
        } else if (chatWidget) {
            chatWidget->addSystemMessage("Command executed locally (not connected to server)");
        }
    } else {
        if (m_networkClient && m_networkClient->isLoggedIn()) {
            m_networkClient->sendChatMessage(input);
        } else if (chatWidget) {
            chatWidget->addSystemMessage("Message sent locally (not connected to server)");
        }
    }
}

} // namespace mc::client
