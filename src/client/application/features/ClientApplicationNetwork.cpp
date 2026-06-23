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

#include "client/application/ClientApplication.hpp"

#include "client/command/ClientCommandManager.hpp"
#include "client/renderer/trident/block/BreakProgressManager.hpp"
#include "client/renderer/trident/chunk/ChunkRenderer.hpp"
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/particle/ParticleManager.hpp"
#include "client/renderer/trident/particle/ParticleRegistry.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/skin/ClientSkinManager.hpp"
#include "client/sound/AudioService.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "client/ui/minecraft/widgets/ChatWidget.hpp"
#include "client/ui/minecraft/widgets/TitleWidget.hpp"
#include "client/ui/screen/AbstractContainerScreen.hpp"
#include "client/ui/screen/CartographyScreen.hpp"
#include "client/ui/screen/ChestScreen.hpp"
#include "client/ui/screen/CraftingScreen.hpp"
#include "client/ui/screen/FurnaceScreen.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "client/world/player/ClientPlayerPredictor.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/ExperiencePackets.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/network/SkinPackets.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockSoundType.hpp"
#include "common/world/block/blocks/cave/PointedDripstoneBlock.hpp"
#include "common/world/block/registry/CaveBlocks.hpp"
#include "common/world/block/registry/MudBlocks.hpp"
#include "common/world/fluid/FluidTags.hpp"

#include <algorithm>
#include <memory>
#include <vector>

namespace mc::client {

namespace {

template <typename Menu>
AbstractContainerScreen<Menu>* asContainerScreen(IScreen* screen)
{
    return dynamic_cast<AbstractContainerScreen<Menu>*>(screen);
}

template <typename Menu>
void applyContainerContent(
    AbstractContainerScreen<Menu>* screen, ContainerId containerId, const std::vector<ItemStack>& items)
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

template <typename Menu>
bool applyContainerSlot(
    AbstractContainerScreen<Menu>* screen, ContainerId containerId, i32 slotIndex, const ItemStack& item)
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

std::function<void(ContainerId, i32, i32, i16, ClickAction, const ItemStack&)> makeContainerClickSender(
    NetworkClient* networkClient)
{
    return [networkClient](ContainerId containerId,
               i32 slotIndex,
               i32 button,
               i16 transactionId,
               ClickAction action,
               const ItemStack& cursorItem) {
        if (networkClient) {
            networkClient->sendContainerClick(
                ContainerClickPacket(containerId, slotIndex, button, transactionId, action, cursorItem));
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

    callbacks.onLoginSuccess = [this](PlayerId playerId, EntityId entityId, const std::string& username) {
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

    callbacks.onLoginFailed = [this](const std::string& reason) {
        spdlog::error("Login failed: {}", reason);

        m_knownPlayerNames.clear();
        if (m_commandManager) {
            m_commandManager->clear();
        }
        stop();
    };

    callbacks.onDisconnected = [this](const std::string& reason) {
        spdlog::info("Disconnected from server: {}", reason);

        // 如果正在离开世界，这是预期的断开，继续返回主菜单流程
        if (m_stateMachine.isLeavingWorld()) {
            spdlog::info("[Network] Disconnection during world leave - continuing to main menu");
            return;
        }

        // 如果正在关闭，忽略断开事件
        if (m_stateMachine.isShuttingDown()) {
            spdlog::info("[Network] Disconnection during shutdown - ignoring");
            return;
        }

        // 非预期的断开连接：清理并返回主菜单
        spdlog::error("[Network] Unexpected disconnection - returning to main menu");

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

        // 销毁游戏会话
        destroyGameSession();

        // 强制返回主菜单状态
        m_stateMachine.forceState(ClientAppState::MainMenu);
        showMainMenu();
    };

    callbacks.onCommandTree = [this](const std::string& treeJson) {
        if (!m_commandManager) {
            m_commandManager = std::make_unique<command::ClientCommandManager>();
        }
        auto result = m_commandManager->applyCommandTreeJson(treeJson);
        if (result.failed()) {
            spdlog::error("Failed to apply command tree: {}", result.error().toString());
            return;
        }
        m_commandManager->setPlayerNameProvider([this]() { return collectPlayerCompletionCandidates(); });
        m_commandManager->setEntityNameProvider([this]() { return collectEntityCompletionCandidates(); });

        // 更新 ChatWidget 的 commandManager 指针
        if (m_kageroEngine && m_chatLayerId != 0) {
            auto* chatWidget =
                static_cast<ui::minecraft::widgets::ChatWidget*>(m_kageroEngine->getLayer(m_chatLayerId));
            if (chatWidget) {
                chatWidget->setCommandManager(m_commandManager.get());
                spdlog::info("[ClientApplication] Updated ChatWidget commandManager after command tree sync");
            }
        }
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
        m_world.onChunkData(x, z, dimension, std::vector<u8>(data));
    };

    callbacks.onChunkUnload = [this](ChunkCoord x, ChunkCoord z, DimensionId dimension) {
        m_world.onChunkUnload(x, z, dimension);
    };

    callbacks.onPlayerSpawn = [this](PlayerId playerId, const std::string& username, f64 x, f64 y, f64 z) {
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
        m_world.setBlockState(x, y, z, BlockRegistry::instance().getBlockState(blockStateId));
    };

    callbacks.onChatMessage = [this](const std::string& message, PlayerId senderId) {
        if (m_kageroEngine) {
            auto* chatWidget =
                static_cast<ui::minecraft::widgets::ChatWidget*>(m_kageroEngine->getLayer(m_chatLayerId));
            if (chatWidget) {
                // senderId == 0 表示系统消息，非零表示玩家聊天消息
                // 与 MC Java 一致：系统消息使用灰色文本，玩家消息使用白色文本
                if (senderId == 0) {
                    chatWidget->addMessage(message, chat::ChatMessageType::System);
                } else {
                    const auto it = m_knownPlayerNames.find(senderId);
                    const std::string senderName = (it != m_knownPlayerNames.end()) ? it->second : std::string();
                    if (!senderName.empty()) {
                        chatWidget->addMessage(senderName + ": " + message, chat::ChatMessageType::Chat);
                    } else {
                        chatWidget->addMessage(message, chat::ChatMessageType::Chat);
                    }
                }
            }
        }
    };

    callbacks.onPlayerMove = [this](PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch) {
        // 使用 LocalPlayerIdentity 判断是否是本地玩家
        if (m_localIdentity.isLocalPlayer(playerId)) {
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
        for (i32 slotIndex = 0; slotIndex < static_cast<i32>(items.size()) && slotIndex < inventory.getContainerSize();
            ++slotIndex) {
            inventory.setItem(slotIndex, items[static_cast<size_t>(slotIndex)]);
        }

        if (auto* inventoryScreen =
                asContainerScreen<mc::InventoryCraftingMenu>(ScreenManager::instance().getCurrentScreen())) {
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

            case ContainerType::Crafting:
                screen = std::make_unique<CraftingScreen>(
                    std::make_unique<mc::CraftingMenu>(packet.containerId(), &m_player->inventory(), nullptr),
                    makeContainerClickSender(m_networkClient.get()),
                    makeContainerCloseSender(m_networkClient.get()));
                break;

            case ContainerType::Generic9x1:
            case ContainerType::Generic9x2:
            case ContainerType::Generic9x3:
            case ContainerType::Generic9x4:
            case ContainerType::Generic9x5:
            case ContainerType::Generic9x6:
            case ContainerType::ShulkerBox: {
                // 根据容器类型计算行数
                const ContainerType containerType = static_cast<ContainerType>(packet.type());
                i32 rows = 3; // 默认3行
                switch (containerType) {
                    case ContainerType::Generic9x1:
                        rows = 1;
                        break;
                    case ContainerType::Generic9x2:
                        rows = 2;
                        break;
                    case ContainerType::Generic9x3:
                        rows = 3;
                        break;
                    case ContainerType::Generic9x4:
                        rows = 4;
                        break;
                    case ContainerType::Generic9x5:
                        rows = 5;
                        break;
                    case ContainerType::Generic9x6:
                        rows = 6;
                        break;
                    case ContainerType::ShulkerBox:
                        rows = 3;
                        break;
                    default:
                        rows = 3;
                        break;
                }
                screen = std::make_unique<ChestScreen>(packet.containerId(),
                    &m_player->inventory(),
                    rows,
                    makeContainerClickSender(m_networkClient.get()),
                    makeContainerCloseSender(m_networkClient.get()));
                break;
            }

            case ContainerType::Furnace:
            case ContainerType::BlastFurnace:
            case ContainerType::Smoker:
                screen = std::make_unique<FurnaceScreen>(packet.containerId(),
                    &m_player->inventory(),
                    makeContainerClickSender(m_networkClient.get()),
                    makeContainerCloseSender(m_networkClient.get()));
                break;

            case ContainerType::Cartography:
                screen = std::make_unique<CartographyScreen>(packet.containerId(),
                    &m_player->inventory(),
                    makeContainerClickSender(m_networkClient.get()),
                    makeContainerCloseSender(m_networkClient.get()));
                break;

            default:
                spdlog::error("Ignored unsupported container type {}", static_cast<i32>(type));
                break;
        }

        if (!screen) {
            return;
        }

        if (m_renderer && m_renderer->isGuiRendererInitialized()) {
            if (auto* inventoryContainerScreen =
                    dynamic_cast<AbstractContainerScreen<mc::InventoryCraftingMenu>*>(screen.get())) {
                inventoryContainerScreen->setRenderers(&m_renderer->guiRenderer(),
                    m_guiTextureManager.get(),
                    m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
                inventoryContainerScreen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
            } else if (auto* craftingContainerScreen =
                           dynamic_cast<AbstractContainerScreen<mc::CraftingMenu>*>(screen.get())) {
                craftingContainerScreen->setRenderers(&m_renderer->guiRenderer(),
                    m_guiTextureManager.get(),
                    m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
                craftingContainerScreen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
            } else if (auto* chestContainerScreen =
                           dynamic_cast<AbstractContainerScreen<mc::blockentity::ChestContainer>*>(screen.get())) {
                chestContainerScreen->setRenderers(&m_renderer->guiRenderer(),
                    m_guiTextureManager.get(),
                    m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
                chestContainerScreen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
            } else if (auto* furnaceContainerScreen =
                           dynamic_cast<AbstractContainerScreen<mc::blockentity::FurnaceContainer>*>(screen.get())) {
                furnaceContainerScreen->setRenderers(&m_renderer->guiRenderer(),
                    m_guiTextureManager.get(),
                    m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
                furnaceContainerScreen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
            } else if (auto* cartographyContainerScreen =
                           dynamic_cast<AbstractContainerScreen<mc::CartographyContainer>*>(screen.get())) {
                cartographyContainerScreen->setRenderers(&m_renderer->guiRenderer(),
                    m_guiTextureManager.get(),
                    m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
                cartographyContainerScreen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
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
            return;
        }

        if (auto* cartographyScreen = asContainerScreen<mc::CartographyContainer>(currentScreen)) {
            applyContainerContent(cartographyScreen, packet.containerId(), packet.items());
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

    callbacks.onSpawnEntity = [this](u32 entityId,
                                  const std::string& typeId,
                                  f32 x,
                                  f32 y,
                                  f32 z,
                                  f32 yaw,
                                  f32 pitch,
                                  f32 vx,
                                  f32 vy,
                                  f32 vz,
                                  const ItemStack* itemStack) {
        MC_TRACE_INSTANT(
            "client.entity", "onSpawnEntity", "entityId", entityId, "typeId", typeId, "hasItem", itemStack != nullptr);

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
        entity->setVelocity(vx, vy, vz);

        if (typeId == mc::entity::EntityTypes::ITEM) {
            if (itemStack) {
                entity->setItemStack(*itemStack);
            }

            if (entity->hoverStart() == 0.0f) {
                mc::math::Random rng(static_cast<u64>(entityId) * 341873128712ULL + 132897987541ULL);
                entity->setHoverStart(rng.nextFloat() * mc::math::TWO_PI);
            }
        }
    };

    callbacks.onSpawnMob =
        [this](u32 entityId, const std::string& typeId, f32 x, f32 y, f32 z, f32 yaw, f32 pitch, f32 headYaw) {
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

            // 通知音频系统实体生成
            if (m_audioService) {
                m_audioService->onEntitySpawn(entityId, typeId, x, y, z);
            }
        };

    callbacks.onEntityDestroy = [this](const std::vector<u32>& entityIds) {
        MC_TRACE_INSTANT("client.entity", "onEntityDestroy", "count", entityIds.size());

        auto& entityManager = m_world.entityManager();
        for (u32 entityId : entityIds) {
            // 使用 LocalPlayerIdentity 判断是否是本地玩家实体
            if (m_localIdentity.isLocalPlayerEntity(static_cast<EntityId>(entityId))) {
                continue;
            }

            // 清理实体渲染网格（静态+动画），避免 Vulkan 资源泄漏
            if (m_renderer) {
                m_renderer->entityRendererManager().removeEntityMeshes(static_cast<EntityId>(entityId));
            }

            entityManager.removeEntity(static_cast<EntityId>(entityId));

            // 通知音频系统实体移除
            if (m_audioService) {
                m_audioService->onEntityRemove(entityId);
            }
        }
    };

    callbacks.onEntityMove = [this](u32 entityId, f32 deltaX, f32 deltaY, f32 deltaZ) {
        // 使用 LocalPlayerIdentity 判断是否是本地玩家实体
        const EntityId eid = static_cast<EntityId>(entityId);
        if (m_localIdentity.isLocalPlayerEntity(eid)) {
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
        MC_TRACE_INSTANT("client.entity", "onEntityTeleport", "entityId", entityId);

        // 使用 LocalPlayerIdentity 判断是否是本地玩家实体
        const EntityId eid = static_cast<EntityId>(entityId);
        if (m_localIdentity.isLocalPlayerEntity(eid)) {
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
                m_player->setVelocity(
                    static_cast<f32>(vx) * scale, static_cast<f32>(vy) * scale, static_cast<f32>(vz) * scale);
            }
            return;
        }

        ClientEntity* entity = m_world.entityManager().getEntity(eid);
        if (!entity) {
            return;
        }

        entity->setVelocity(static_cast<f32>(vx) * scale, static_cast<f32>(vy) * scale, static_cast<f32>(vz) * scale);
    };

    callbacks.onEntityMetadata = [this](u32 entityId, const std::vector<u8>& metadata) {
        MC_TRACE_INSTANT("client.entity", "onEntityMetadata", "entityId", entityId, "size", metadata.size());

        // 使用 LocalPlayerIdentity 判断是否是本地玩家实体
        const EntityId eid = static_cast<EntityId>(entityId);
        if (m_localIdentity.isLocalPlayerEntity(eid)) {
            return;
        }

        ClientEntity* entity = m_world.entityManager().getEntity(eid);
        if (!entity) {
            return;
        }

        // 检查旧状态（用于声音触发）
        bool wasFallFlying = entity->isFallFlying();
        bool wasAngry = entity->isAngry();

        // 应用新的元数据
        entity->setMetadata(metadata);

        // 检查新状态
        bool isFallFlying = entity->isFallFlying();
        bool isAngry = entity->isAngry();

        // 通知音频系统鞘翅飞行状态变化
        if (m_audioService && wasFallFlying != isFallFlying) {
            m_audioService->onPlayerElytraFlyingChanged(entityId, isFallFlying);
        }

        // 通知音频系统蜜蜂愤怒状态变化
        if (m_audioService && wasAngry != isAngry) {
            m_audioService->onEntityAngerStateChanged(entityId, isAngry);
        }
    };

    callbacks.onEntityAnimation = [this](u32 entityId, u8 animation) {
        auto* entity = m_world.entityManager().getEntity(static_cast<EntityId>(entityId));
        if (!entity) {
            return;
        }
        using Animation = network::EntityAnimationPacket::Animation;
        switch (static_cast<Animation>(animation)) {
            case Animation::SwingMainHand:
                entity->triggerSwingAnimation(0);
                break;
            case Animation::SwingOffHand:
                entity->triggerSwingAnimation(1);
                break;
            case Animation::TakeDamage:
                entity->triggerHurtAnimation();
                break;
            case Animation::LeaveBed:
                entity->triggerLeaveBedAnimation();
                break;
            case Animation::CriticalEffect: {
                // 暴击粒子效果：在实体周围生成暴击粒子
                // 原版使用 TrackingEmitter 持续 3 tick、每 tick 最多 16 个粒子，
                // 这里一次性生成近似数量的粒子
                if (m_world.particleManager() != nullptr && entity != nullptr) {
                    glm::vec3 entityPos(entity->x(), entity->y(), entity->z());
                    for (i32 i = 0; i < 16; ++i) {
                        f32 rx = m_random.nextFloat() * 2.0f - 1.0f;
                        f32 ry = m_random.nextFloat() * 2.0f - 1.0f;
                        f32 rz = m_random.nextFloat() * 2.0f - 1.0f;
                        // 球体内均匀分布：排除球外的随机点
                        if (rx * rx + ry * ry + rz * rz > 1.0f) {
                            continue;
                        }
                        glm::vec3 particlePos = entityPos + glm::vec3(rx * 0.25f, ry * 0.25f + 0.5f, rz * 0.25f);
                        glm::vec3 velocity(rx, ry + 0.2f, rz);
                        m_world.particleManager()->addPendingParticle(
                            client::renderer::trident::particle::ParticleTypeId::Crit, particlePos, velocity, &m_world);
                    }
                }
                break;
            }
            case Animation::MagicCriticalEffect: {
                // 附魔暴击粒子效果：在实体周围生成紫蓝色附魔暴击粒子
                if (m_world.particleManager() != nullptr && entity != nullptr) {
                    glm::vec3 entityPos(entity->x(), entity->y(), entity->z());
                    for (i32 i = 0; i < 16; ++i) {
                        f32 rx = m_random.nextFloat() * 2.0f - 1.0f;
                        f32 ry = m_random.nextFloat() * 2.0f - 1.0f;
                        f32 rz = m_random.nextFloat() * 2.0f - 1.0f;
                        // 球体内均匀分布：排除球外的随机点
                        if (rx * rx + ry * ry + rz * rz > 1.0f) {
                            continue;
                        }
                        glm::vec3 particlePos = entityPos + glm::vec3(rx * 0.25f, ry * 0.25f + 0.5f, rz * 0.25f);
                        glm::vec3 velocity(rx, ry + 0.2f, rz);
                        m_world.particleManager()->addPendingParticle(
                            client::renderer::trident::particle::ParticleTypeId::EnchantedHit,
                            particlePos,
                            velocity,
                            &m_world);
                    }
                }
                break;
            }
        }
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
        // 处理实体状态事件
        using namespace network;

        // 获取实体位置用于粒子效果
        ClientEntity* entity = m_world.entityManager().getEntity(entityId);
        glm::vec3 entityPos(0.0f);
        if (entity != nullptr) {
            entityPos = glm::vec3(entity->x(), entity->y(), entity->z());
        }

        switch (status) {
            case static_cast<u8>(EntityStatusPacket::Status::GuardianAttack): {
                // 状态 21: 守卫者开始攻击
                if (m_audioService) {
                    m_audioService->onGuardianAttack(entityId);
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::TamingSucceeded): {
                // 状态 7: 驯服成功 - 生成 7 个爱心粒子
                // MC 原版: TamableAnimal.spawnTamingParticles — 7个粒子，位置围绕实体包围盒随机分布
                if (m_world.particleManager() != nullptr) {
                    f32 entityWidth = entity != nullptr ? entity->width() : 0.6f;
                    f32 entityHeight = entity != nullptr ? entity->height() : 1.8f;
                    for (i32 i = 0; i < 7; ++i) {
                        f32 rx = (m_random.nextFloat() * 2.0f - 1.0f) * entityWidth;
                        f32 ry = m_random.nextFloat() * entityHeight + 0.5f;
                        f32 rz = (m_random.nextFloat() * 2.0f - 1.0f) * entityWidth;
                        glm::vec3 particlePos = entityPos + glm::vec3(rx, ry, rz);
                        glm::vec3 velocity(m_random.nextGaussian(0.0f, 0.02f),
                            m_random.nextGaussian(0.0f, 0.02f),
                            m_random.nextGaussian(0.0f, 0.02f));
                        m_world.particleManager()->addPendingParticle(
                            client::renderer::trident::particle::ParticleTypeId::Heart,
                            particlePos,
                            velocity,
                            &m_world);
                    }
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::TamingFailed): {
                // 状态 6: 驯服失败 - 生成 7 个烟雾粒子
                // MC 原版: TamableAnimal.spawnTamingParticles — 7个粒子，位置围绕实体包围盒随机分布
                if (m_world.particleManager() != nullptr) {
                    f32 entityWidth = entity != nullptr ? entity->width() : 0.6f;
                    f32 entityHeight = entity != nullptr ? entity->height() : 1.8f;
                    for (i32 i = 0; i < 7; ++i) {
                        f32 rx = (m_random.nextFloat() * 2.0f - 1.0f) * entityWidth;
                        f32 ry = m_random.nextFloat() * entityHeight + 0.5f;
                        f32 rz = (m_random.nextFloat() * 2.0f - 1.0f) * entityWidth;
                        glm::vec3 particlePos = entityPos + glm::vec3(rx, ry, rz);
                        glm::vec3 velocity(m_random.nextGaussian(0.0f, 0.02f),
                            m_random.nextGaussian(0.0f, 0.02f),
                            m_random.nextGaussian(0.0f, 0.02f));
                        m_world.particleManager()->addPendingParticle(
                            client::renderer::trident::particle::ParticleTypeId::Smoke,
                            particlePos,
                            velocity,
                            &m_world);
                    }
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::LoveHeart): {
                // 状态 18: 繁殖爱心效果
                if (m_world.particleManager() != nullptr) {
                    glm::vec3 heartPos = entityPos + glm::vec3(0.0f, 0.5f, 0.0f);
                    m_world.particleManager()->addPendingParticle(
                        client::renderer::trident::particle::ParticleTypeId::Heart,
                        heartPos,
                        glm::vec3(0.0f, 0.0f, 0.0f),
                        &m_world);
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::EatBlock): {
                // 状态 10: 吃草/方块动画（羊低头吃草）或 TNT 矿车引燃
                if (entity != nullptr) {
                    // TNT 矿车收到 status 10 时设置引信值
                    if (entity->typeId() == mc::entity::EntityTypes::TNT_MINECART) {
                        entity->setFuseTimer(80);
                    } else {
                        entity->setEatAnimationTimer(40);
                    }
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::OcelotTrustSucceeded): {
                // 状态 41: 豹猫信任成功 - 生成 7 个爱心粒子
                // MC 原版: Ocelot.spawnTrustingParticles(true) — 与 TamableAnimal.spawnTamingParticles 逻辑相同
                if (m_world.particleManager() != nullptr) {
                    f32 entityWidth = entity != nullptr ? entity->width() : 0.6f;
                    f32 entityHeight = entity != nullptr ? entity->height() : 0.7f;
                    for (i32 i = 0; i < 7; ++i) {
                        f32 rx = (m_random.nextFloat() * 2.0f - 1.0f) * entityWidth;
                        f32 ry = m_random.nextFloat() * entityHeight + 0.5f;
                        f32 rz = (m_random.nextFloat() * 2.0f - 1.0f) * entityWidth;
                        glm::vec3 particlePos = entityPos + glm::vec3(rx, ry, rz);
                        glm::vec3 velocity(m_random.nextGaussian(0.0f, 0.02f),
                            m_random.nextGaussian(0.0f, 0.02f),
                            m_random.nextGaussian(0.0f, 0.02f));
                        m_world.particleManager()->addPendingParticle(
                            client::renderer::trident::particle::ParticleTypeId::Heart,
                            particlePos,
                            velocity,
                            &m_world);
                    }
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::OcelotTrustFailed): {
                // 状态 40: 豹猫信任失败 - 生成 7 个烟雾粒子
                // MC 原版: Ocelot.spawnTrustingParticles(false) — 与 TamableAnimal.spawnTamingParticles 逻辑相同
                if (m_world.particleManager() != nullptr) {
                    f32 entityWidth = entity != nullptr ? entity->width() : 0.6f;
                    f32 entityHeight = entity != nullptr ? entity->height() : 0.7f;
                    for (i32 i = 0; i < 7; ++i) {
                        f32 rx = (m_random.nextFloat() * 2.0f - 1.0f) * entityWidth;
                        f32 ry = m_random.nextFloat() * entityHeight + 0.5f;
                        f32 rz = (m_random.nextFloat() * 2.0f - 1.0f) * entityWidth;
                        glm::vec3 particlePos = entityPos + glm::vec3(rx, ry, rz);
                        glm::vec3 velocity(m_random.nextGaussian(0.0f, 0.02f),
                            m_random.nextGaussian(0.0f, 0.02f),
                            m_random.nextGaussian(0.0f, 0.02f));
                        m_world.particleManager()->addPendingParticle(
                            client::renderer::trident::particle::ParticleTypeId::Smoke,
                            particlePos,
                            velocity,
                            &m_world);
                    }
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::TeleportParticles): {
                // 状态 46: 传送粒子效果（末影人传送）
                if (m_world.particleManager() != nullptr) {
                    // 生成末影人传送粒子
                    for (i32 i = 0; i < 16; ++i) {
                        f32 rx = m_random.nextFloat(-1.0f, 1.0f);
                        f32 ry = m_random.nextFloat(0.0f, 2.0f);
                        f32 rz = m_random.nextFloat(-1.0f, 1.0f);
                        glm::vec3 particlePos = entityPos + glm::vec3(rx * 0.5f, ry, rz * 0.5f);
                        glm::vec3 velocity(m_random.nextGaussian(0.0f, 0.05f),
                            m_random.nextGaussian(0.0f, 0.05f),
                            m_random.nextGaussian(0.0f, 0.05f));
                        m_world.particleManager()->addPendingParticle(
                            client::renderer::trident::particle::ParticleTypeId::Portal,
                            particlePos,
                            velocity,
                            &m_world);
                    }
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::IronGolemAttack): {
                // 状态 4: 铁傀儡攻击动画（举臂）+ 攻击音效
                if (entity != nullptr) {
                    entity->setIronGolemAttackTimer(10);
                    entity->setIronGolemArmsRaised(true);
                }
                // 播放铁傀儡攻击音效
                if (m_audioService) {
                    auto sound = sound::SoundInstance::createLocated(mc::SoundEvents::ENTITY_IRON_GOLEM_ATTACK,
                        mc::sound::SoundCategory::Neutral,
                        entityPos.x,
                        entityPos.y,
                        entityPos.z,
                        1.0f,
                        1.0f);
                    m_audioService->play(std::make_unique<sound::SoundInstance>(std::move(sound)));
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::IronGolemHoldRose): {
                // 状态 11: 铁傀儡开始手持罂粟花
                if (entity != nullptr) {
                    entity->setIronGolemHoldingRose(true);
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::IronGolemStopRose): {
                // 状态 34: 铁傀儡停止手持罂粟花
                if (entity != nullptr) {
                    entity->setIronGolemHoldingRose(false);
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::VillagerHeart): {
                // 状态 12: 村民爱心粒子（繁殖中/幼年村民出生）
                // MC原版 addParticlesAroundSelf: 5个粒子，随机分布在实体周围，速度为高斯分布*0.02
                if (m_world.particleManager() != nullptr) {
                    for (i32 i = 0; i < 5; ++i) {
                        // getRandomX(1.0) = x + (random - 0.5) * width * 2.0, getRandomY() + 1.0 = y + random * height
                        // + 1.0, getRandomZ(1.0) = z + (random - 0.5) * width * 2.0
                        f32 rx = (m_random.nextFloat() - 0.5f) * 2.0f; // [-1, 1]
                        f32 ry = m_random.nextFloat();                 // [0, 1)
                        f32 rz = (m_random.nextFloat() - 0.5f) * 2.0f; // [-1, 1]
                        glm::vec3 particlePos = entityPos + glm::vec3(rx * 0.3f, ry + 1.0f, rz * 0.3f);
                        glm::vec3 velocity(m_random.nextGaussian(0.0f, 0.02f),
                            m_random.nextGaussian(0.0f, 0.02f),
                            m_random.nextGaussian(0.0f, 0.02f));
                        m_world.particleManager()->addPendingParticle(
                            client::renderer::trident::particle::ParticleTypeId::Heart,
                            particlePos,
                            velocity,
                            &m_world);
                    }
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::VillagerAngry): {
                // 状态 13: 村民愤怒粒子（无床位/被攻击）
                if (m_world.particleManager() != nullptr) {
                    for (i32 i = 0; i < 5; ++i) {
                        f32 rx = (m_random.nextFloat() - 0.5f) * 2.0f;
                        f32 ry = m_random.nextFloat();
                        f32 rz = (m_random.nextFloat() - 0.5f) * 2.0f;
                        glm::vec3 particlePos = entityPos + glm::vec3(rx * 0.3f, ry + 1.0f, rz * 0.3f);
                        glm::vec3 velocity(m_random.nextGaussian(0.0f, 0.02f),
                            m_random.nextGaussian(0.0f, 0.02f),
                            m_random.nextGaussian(0.0f, 0.02f));
                        m_world.particleManager()->addPendingParticle(
                            client::renderer::trident::particle::ParticleTypeId::AngryVillager,
                            particlePos,
                            velocity,
                            &m_world);
                    }
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::VillagerHappy): {
                // 状态 14: 村民开心粒子（交易成功/获取职业/找到床位/找到集会点）
                if (m_world.particleManager() != nullptr) {
                    for (i32 i = 0; i < 5; ++i) {
                        f32 rx = (m_random.nextFloat() - 0.5f) * 2.0f;
                        f32 ry = m_random.nextFloat();
                        f32 rz = (m_random.nextFloat() - 0.5f) * 2.0f;
                        glm::vec3 particlePos = entityPos + glm::vec3(rx * 0.3f, ry + 1.0f, rz * 0.3f);
                        glm::vec3 velocity(m_random.nextGaussian(0.0f, 0.02f),
                            m_random.nextGaussian(0.0f, 0.02f),
                            m_random.nextGaussian(0.0f, 0.02f));
                        m_world.particleManager()->addPendingParticle(
                            client::renderer::trident::particle::ParticleTypeId::HappyVillager,
                            particlePos,
                            velocity,
                            &m_world);
                    }
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::VillagerSplash): {
                // 状态 42: 村民水花粒子（突袭中恐慌）
                if (m_world.particleManager() != nullptr) {
                    for (i32 i = 0; i < 5; ++i) {
                        f32 rx = (m_random.nextFloat() - 0.5f) * 2.0f;
                        f32 ry = m_random.nextFloat();
                        f32 rz = (m_random.nextFloat() - 0.5f) * 2.0f;
                        glm::vec3 particlePos = entityPos + glm::vec3(rx * 0.3f, ry + 1.0f, rz * 0.3f);
                        glm::vec3 velocity(m_random.nextGaussian(0.0f, 0.02f),
                            m_random.nextGaussian(0.0f, 0.02f),
                            m_random.nextGaussian(0.0f, 0.02f));
                        m_world.particleManager()->addPendingParticle(
                            client::renderer::trident::particle::ParticleTypeId::Splash,
                            particlePos,
                            velocity,
                            &m_world);
                    }
                }
                break;
            }
            // 装备破损状态 (47-52): 播放物品破碎粒子 + 音效
            // 对应 MC 原版 LivingEntity.handleEntityEvent() 中的 EQUIPMENT_BREAK 处理
            case static_cast<u8>(EntityStatusPacket::Status::EquipmentBreakMainHand):
            case static_cast<u8>(EntityStatusPacket::Status::EquipmentBreakOffHand):
            case static_cast<u8>(EntityStatusPacket::Status::EquipmentBreakHead):
            case static_cast<u8>(EntityStatusPacket::Status::EquipmentBreakChest):
            case static_cast<u8>(EntityStatusPacket::Status::EquipmentBreakLegs):
            case static_cast<u8>(EntityStatusPacket::Status::EquipmentBreakFeet): {
                // 播放物品破碎音效
                if (m_audioService) {
                    auto sound = sound::SoundInstance::createLocated(mc::SoundEvents::ENTITY_ITEM_BREAK,
                        mc::sound::SoundCategory::Players,
                        entityPos.x,
                        entityPos.y,
                        entityPos.z,
                        0.8f,
                        0.8f + m_random.nextFloat() * 0.4f);
                    m_audioService->play(std::make_unique<sound::SoundInstance>(std::move(sound)));
                }
                // 生成物品破碎粒子
                if (m_world.particleManager() != nullptr) {
                    for (i32 i = 0; i < 10; ++i) {
                        f32 rx = m_random.nextFloat() * 2.0f - 1.0f;
                        f32 ry = m_random.nextFloat() * 2.0f;
                        f32 rz = m_random.nextFloat() * 2.0f - 1.0f;
                        glm::vec3 particlePos = entityPos + glm::vec3(rx * 0.3f, ry * 0.6f + 0.5f, rz * 0.3f);
                        glm::vec3 velocity(m_random.nextGaussian(0.0f, 0.05f),
                            m_random.nextGaussian(0.0f, 0.05f) + 0.1f,
                            m_random.nextGaussian(0.0f, 0.05f));
                        m_world.particleManager()->addPendingParticle(
                            client::renderer::trident::particle::ParticleTypeId::Breaking,
                            particlePos,
                            velocity,
                            &m_world);
                    }
                }
                break;
            }
            default: {
                // 检查是否为权限等级变更状态 (status byte 24-28, 其中 level = status - 24)
                i32 permLevel = EntityStatusPacket::toPermissionLevel(status);
                if (permLevel >= 0) {
                    // 权限等级变更，仅对本机玩家生效
                    if (m_localIdentity.isLocalPlayerEntity(static_cast<EntityId>(entityId))) {
                        if (m_player) {
                            spdlog::info("Permission level changed to {}", permLevel);
                            m_player->setPermissionLevel(permLevel);
                        }
                    }
                }
                break;
            }
        }
    };

    callbacks.onGameModeChange = [this](GameMode gameMode) {
        if (!m_player) {
            return;
        }

        spdlog::info("Game mode changed to: {}", static_cast<i32>(gameMode));
        m_player->setGameMode(gameMode);

        closeInventoryScreenIfModeMismatch();
    };

    callbacks.onRainStrengthChange = [this](f32 rainStrength) { m_world.onRainStrengthChange(rainStrength); };

    callbacks.onThunderStrengthChange = [this](
                                            f32 thunderStrength) { m_world.onThunderStrengthChange(thunderStrength); };

    callbacks.onBeginRaining = [this]() { m_world.onBeginRaining(); };

    callbacks.onEndRaining = [this]() { m_world.onEndRaining(); };

    callbacks.onDifficultyChange = [this](Difficulty difficulty, bool locked) {
        spdlog::info("[Client] Difficulty changed to: {}, locked: {}", static_cast<i32>(difficulty), locked);
        m_world.setDifficulty(difficulty);
        m_world.setDifficultyLocked(locked);
    };

    callbacks.onPlayerAbilities =
        [this](bool invulnerable, bool flying, bool canFly, bool creativeMode, f32 flySpeed, f32 walkSpeed) {
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

    callbacks.onLightUpdate = [this](i32 chunkX,
                                  i32 chunkZ,
                                  i32 sectionY,
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
            spdlog::error("Received sound event '{}' but audio service is not initialized", soundEventId.toString());
            return;
        }

        auto sound = sound::SoundInstance::createLocated(soundEventId, category, x, y, z, volume, pitch);

        m_audioService->play(std::make_unique<sound::SoundInstance>(std::move(sound)));
    };

    callbacks.onStopSound = [this](const std::optional<ResourceLocation>& soundEventId,
                                const std::optional<mc::sound::SoundCategory>& category) {
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

    callbacks.onMovingSound = [this](const ResourceLocation& soundEventId,
                                  mc::sound::SoundCategory category,
                                  i32 entityId,
                                  f32 volume,
                                  f32 pitch) {
        if (!m_audioService) {
            spdlog::error(
                "Received moving sound event '{}' but audio service is not initialized", soundEventId.toString());
            return;
        }

        // 使用 AudioService 的 playMovingSound 方法
        // 这会创建一个跟随实体位置的 TickableSound
        m_audioService->playMovingSound(soundEventId, category, static_cast<u32>(entityId), volume, pitch);
    };

    callbacks.onSetExperience = [this](f32 progress, i32 totalXp, i32 level) {
        if (!m_player) {
            return;
        }

        m_player->setExperience(level, progress, totalXp);
    };

    callbacks.onSpawnExperienceOrb = [this](u32 entityId, f64 x, f64 y, f64 z, i16 xpValue) {
        auto& entityManager = m_world.entityManager();
        ClientEntity* entity =
            entityManager.spawnEntity(static_cast<EntityId>(entityId), mc::entity::EntityTypes::EXPERIENCE_ORB);
        if (!entity) {
            entity = entityManager.getEntity(static_cast<EntityId>(entityId));
        }

        if (!entity) {
            return;
        }

        entity->setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
        entity->setXpValue(static_cast<i32>(xpValue));
    };

    // 粒子回调
    callbacks.onParticle = [this](client::renderer::trident::particle::ParticleTypeId type,
                               f64 x,
                               f64 y,
                               f64 z,
                               f32 vx,
                               f32 vy,
                               f32 vz,
                               f32 ox,
                               f32 oy,
                               f32 oz,
                               u32 count) {
        if (!m_world.particleManager()) {
            return;
        }

        // 在指定位置生成粒子
        mc::math::Random rng;
        for (u32 i = 0; i < count; ++i) {
            // 计算随机偏移位置
            f32 px = static_cast<f32>(x) + (count > 1 ? (rng.nextFloat() * 2.0f - 1.0f) * ox : 0.0f);
            f32 py = static_cast<f32>(y) + (count > 1 ? (rng.nextFloat() * 2.0f - 1.0f) * oy : 0.0f);
            f32 pz = static_cast<f32>(z) + (count > 1 ? (rng.nextFloat() * 2.0f - 1.0f) * oz : 0.0f);

            glm::vec3 pos(px, py, pz);
            glm::vec3 vel(vx, vy, vz);

            auto particle = client::renderer::trident::particle::ParticleRegistry::instance().createParticle(
                type, pos, vel, &m_world);

            if (particle) {
                m_world.particleManager()->addParticle(std::move(particle));
            }
        }
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
        }
    };

    callbacks.onPlayerListRemove = [this](const std::vector<std::array<u8, 16>>& uuids) {
        for (const auto& uuid : uuids) {
            m_skinManager->skinManager().removePlayerInfo(uuid);
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

    callbacks.onPlayerListUpdateDisplayName = [this](const std::array<u8, 16>& uuid,
                                                  const std::optional<std::string>& displayName) {
        MC_UNUSED(uuid);
        MC_UNUSED(displayName);
        // 显示名更新 - 暂时不需要特殊处理
    };

    callbacks.onSetPassengers = [this](u32 entityId, const std::vector<u32>& passengerIds) {
        // 处理乘客变化：更新实体的骑乘状态

        const EntityId localPlayerEntityId = m_localIdentity.entityId();
        const EntityId vehicleEntityId = static_cast<EntityId>(entityId);

        // 获取载具实体和旧乘客列表
        ClientEntity* vehicleEntity = m_world.entityManager().getEntity(vehicleEntityId);
        std::vector<u32> oldPassengerIds;
        if (vehicleEntity) {
            oldPassengerIds = vehicleEntity->passengers();
        }

        // 清除不再是乘客的实体的 vehicleId
        for (u32 oldPassengerId : oldPassengerIds) {
            bool stillPassenger = false;
            for (u32 newPassengerId : passengerIds) {
                if (oldPassengerId == newPassengerId) {
                    stillPassenger = true;
                    break;
                }
            }
            if (!stillPassenger) {
                ClientEntity* oldPassenger = m_world.entityManager().getEntity(static_cast<EntityId>(oldPassengerId));
                if (oldPassenger) {
                    oldPassenger->setVehicleId(0);
                }
            }
        }

        // 更新载具的乘客列表
        if (vehicleEntity) {
            vehicleEntity->setPassengers(passengerIds);
        }

        // 检查本地玩家是否是乘客之一，并更新所有乘客的 vehicleId
        bool localPlayerIsRiding = false;
        for (u32 passengerId : passengerIds) {
            if (passengerId == static_cast<u32>(localPlayerEntityId)) {
                localPlayerIsRiding = true;
            }
            ClientEntity* passenger = m_world.entityManager().getEntity(static_cast<EntityId>(passengerId));
            if (passenger) {
                passenger->setVehicleId(vehicleEntityId);
            }
        }

        // 更新本地玩家的音频状态
        ClientEntity* localPlayer = m_world.entityManager().getEntity(localPlayerEntityId);
        if (localPlayer) {
            if (localPlayerIsRiding) {
                // 本地玩家开始骑乘
                if (m_audioService) {
                    m_audioService->updateEntityRidingState(static_cast<u32>(localPlayerEntityId), true, entityId);
                }
            } else if (localPlayer->vehicleId() == vehicleEntityId) {
                // 本地玩家从这个载具下来了（vehicleId 已在上面被清除）
                if (m_audioService) {
                    m_audioService->updateEntityRidingState(static_cast<u32>(localPlayerEntityId), false, 0);
                }
            }
        }
    };

    // ========== 重生/维度切换事件 ==========

    callbacks.onRespawn = [this](i32 dimensionType,
                              DimensionId dimension,
                              u64 hashedSeed,
                              GameMode gameMode,
                              GameMode previousGameMode,
                              bool isDebug,
                              bool isFlat,
                              bool keepData) {
        spdlog::info("Received Respawn: dimensionType={}, dimension={}, gameMode={}, keepData={}",
            dimensionType,
            static_cast<i32>(dimension),
            static_cast<i32>(gameMode),
            keepData);

        // 检测是否是维度切换
        const DimensionId currentDim = m_dimensionManager.currentDimension();
        const bool isDimensionChange = (currentDim != dimension);

        if (isDimensionChange) {
            spdlog::info(
                "[Respawn] Dimension change: {} -> {}", static_cast<i32>(currentDim), static_cast<i32>(dimension));

            // 1. 开始维度切换
            m_dimensionManager.beginDimensionChange(dimension, Vector3d(0, 0, 0));

            // 2. 设置新维度ID（后续收到的旧维度区块数据将被丢弃）
            m_world.setDimensionId(dimension);

            // 3. 清空世界区块
            m_world.clearChunks();

            // 4. 清空实体管理器（保留本地玩家）
            m_world.entityManager().clear();

            // 5. 重置天气状态
            // 下界和末地不应有降雨/雷暴
            m_world.resetWeather();

            // 6. 清理渲染器的区块缓冲
            if (m_renderer && m_renderer->isChunkRendererInitialized()) {
                m_renderer->chunkRenderer().clearChunks();
            }

            // 7. 完成维度切换
            m_dimensionManager.completeDimensionChange();

            // 8. 更新云高度和渲染参数
            updateCloudHeight();

            spdlog::info("[Respawn] Dimension change completed");
        }

        // 9. 更新游戏模式
        if (m_player) {
            m_player->setGameMode(gameMode);

            // 10. 更新玩家维度属性
            m_player->setDimension(dimension);

            // 11. 如果 keepData 为 false（死亡重生），重置玩家状态
            if (!keepData) {
                m_player->respawn();
            }
        }

        // 12. 重置预测器
        if (m_predictor && m_player) {
            m_predictor->reset(Vector3(m_player->position().x, m_player->position().y, m_player->position().z),
                m_player->yaw(),
                m_player->pitch());
        }

        // 13. 发送维度切换确认包（如果是维度切换）
        if (isDimensionChange && m_networkClient) {
            m_networkClient->sendConfirmDimensionChange(dimension);
        }
    };

    callbacks.onDimensionInfo =
        [this](const std::vector<std::tuple<DimensionId, std::string, bool, bool, f32>>& dimensions) {
            spdlog::info("Received DimensionInfo: {} dimensions available", dimensions.size());

            // 转换为 ClientDimensionInfo 格式并更新维度管理器
            std::vector<ClientDimensionInfo> dimensionInfos;
            dimensionInfos.reserve(dimensions.size());

            for (const auto& [id, name, hasSkyLight, hasCeiling, ambientLight] : dimensions) {
                ClientDimensionInfo info;
                info.id = id;
                info.name = name;
                info.hasSkyLight = hasSkyLight;
                info.hasCeiling = hasCeiling;
                info.ambientLight = ambientLight;
                dimensionInfos.push_back(info);
            }

            // 更新维度管理器
            m_dimensionManager.initialize(dimensionInfos);
        };

    callbacks.onSpawnPosition = [this](i32 x, i32 y, i32 z, f32 angle) {
        spdlog::info("Received SpawnPosition: ({}, {}, {}), angle={:.1f}", x, y, z, angle);
        m_world.setSpawnPoint(x, y, z, angle);
    };

    callbacks.onVehicleMove = [this](f64 x, f64 y, f64 z, f32 yaw, f32 pitch) {
        // 获取本地玩家正在骑乘的载具
        const EntityId localPlayerEntityId = m_localIdentity.entityId();
        ClientEntity* localPlayer = m_world.entityManager().getEntity(localPlayerEntityId);
        if (!localPlayer) {
            return;
        }

        EntityId vehicleId = localPlayer->vehicleId();
        if (vehicleId == 0) {
            return;
        }

        ClientEntity* vehicle = m_world.entityManager().getEntity(vehicleId);
        if (!vehicle) {
            return;
        }

        // 设置载具位置（服务端校正）
        vehicle->setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
        vehicle->setRotation(yaw, pitch);

        // 回发确认包
        if (m_networkClient) {
            m_networkClient->sendMoveVehicle(x, y, z, yaw, pitch);
        }
    };

    callbacks.onSleep = [this](u32 entityId, bool isSleeping, i32 bedX, i32 bedY, i32 bedZ) {
        const EntityId eid = static_cast<EntityId>(entityId);

        // 本地玩家睡眠状态
        if (m_localIdentity.isLocalPlayerEntity(eid)) {
            if (m_player) {
                if (isSleeping) {
                    m_player->startSleeping(BlockPos(bedX, bedY, bedZ));
                    spdlog::info("Local player sleeping at ({}, {}, {})", bedX, bedY, bedZ);
                } else {
                    m_player->stopSleeping();
                    spdlog::info("Local player woke up");
                }
            }
            return;
        }

        // 远程实体睡眠状态
        ClientEntity* entity = m_world.entityManager().getEntity(eid);
        if (!entity) {
            return;
        }

        entity->setSleeping(isSleeping);
        if (isSleeping) {
            entity->setSleepingPosition(BlockPos(bedX, bedY, bedZ));
            spdlog::info("Entity {} sleeping at ({}, {}, {})", entityId, bedX, bedY, bedZ);
        } else {
            entity->clearSleepingPosition();
            spdlog::info("Entity {} woke up", entityId);
        }
    };

    callbacks.onHotbarSet = [this](i32 slot) {
        if (!m_player) {
            return;
        }

        // 更新本地玩家的选中槽位
        m_player->inventory().setSelectedSlot(slot);
    };

    // 标题显示回调
    callbacks.onTitle =
        [this](network::TitleAction action, const std::optional<std::string>& text, i32 fadeIn, i32 stay, i32 fadeOut) {
            if (m_kageroEngine) {
                auto* titleWidget =
                    static_cast<ui::minecraft::widgets::TitleWidget*>(m_kageroEngine->getLayer(m_titleLayerId));
                if (titleWidget) {
                    titleWidget->handleTitlePacket(action, text, fadeIn, stay, fadeOut);
                }
            }
        };

    // 世界事件回调（服务端通过 IWorld::playEvent() 发送 WorldEventPacket，客户端接收后根据事件ID播放音效和粒子）
    // 服务端通过 IWorld::playEvent() 发送 WorldEventPacket，客户端接收后根据事件ID播放音效和粒子
    callbacks.onWorldEvent = [this](i32 eventId, i32 x, i32 y, i32 z, i32 data) {
        _handleWorldEvent(eventId, x, y, z, data);
    };

    m_networkClient->setCallbacks(callbacks);
}

std::vector<std::string> ClientApplication::collectPlayerCompletionCandidates() const
{
    std::vector<std::string> candidates;
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

std::vector<std::string> ClientApplication::collectEntityCompletionCandidates() const
{
    return collectPlayerCompletionCandidates();
}

void ClientApplication::handleChatCommand(const std::string& input)
{
    if (input.empty()) {
        return;
    }

    auto* chatWidget = m_kageroEngine
        ? static_cast<ui::minecraft::widgets::ChatWidget*>(m_kageroEngine->getLayer(m_chatLayerId))
        : nullptr;

    if (chatWidget) {
        // 本地回显：用户输入的消息显示在聊天窗口中
        chatWidget->addMessage(input, chat::ChatMessageType::Chat);
    }

    if (input[0] == '/') {
        std::string command = input.substr(1);

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

void ClientApplication::_handleWorldEvent(i32 eventId, i32 x, i32 y, i32 z, i32 data)
{
    using namespace mc::world;
    using namespace mc::sound;
    using namespace mc::client::renderer::trident::particle;

    const f32 px = static_cast<f32>(x) + 0.5f;
    const f32 py = static_cast<f32>(y) + 0.5f;
    const f32 pz = static_cast<f32>(z) + 0.5f;
    math::Random random;

    switch (eventId) {
        // ========================================================================
        // 音效事件 (1000-1043)
        // ========================================================================
        case WorldEvents::DISPENSER_DISPENSE_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_DISPENSER_DISPENSE, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;

        case WorldEvents::DISPENSER_FAIL_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_DISPENSER_FAIL, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;

        case WorldEvents::DISPENSER_LAUNCH_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_DISPENSER_LAUNCH, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;

        case WorldEvents::FIRE_EXTINGUISH_SOUND:
            if (m_audioService) {
                if (data == 0) {
                    m_audioService->play(std::make_unique<sound::SoundInstance>(
                        sound::SoundInstance::createLocated(SoundEvents::BLOCK_FIRE_EXTINGUISH,
                            SoundCategory::Blocks,
                            px,
                            py,
                            pz,
                            0.5f,
                            2.6f + (random.nextFloat() - random.nextFloat()) * 0.8f)));
                } else {
                    m_audioService->play(std::make_unique<sound::SoundInstance>(
                        sound::SoundInstance::createLocated(SoundEvents::ENTITY_GENERIC_EXTINGUISH_FIRE,
                            SoundCategory::Blocks,
                            px,
                            py,
                            pz,
                            0.7f,
                            1.6f + (random.nextFloat() - random.nextFloat()) * 0.4f)));
                }
            }
            break;

        case WorldEvents::GHAST_WARN_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(SoundEvents::ENTITY_GHAST_WARN,
                        SoundCategory::Hostile,
                        px,
                        py,
                        pz,
                        10.0f,
                        random.nextFloat() * 0.2f + 0.85f)));
            }
            break;

        case WorldEvents::BLAZE_SHOOT_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(SoundEvents::ENTITY_BLAZE_SHOOT,
                        SoundCategory::Hostile,
                        px,
                        py,
                        pz,
                        1.0f,
                        random.nextFloat() * 0.2f + 0.85f)));
            }
            break;

        case WorldEvents::ANVIL_DESTROYED_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_ANVIL_DESTROY, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;

        case WorldEvents::ANVIL_LAND_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_ANVIL_LAND, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;

        case WorldEvents::PHANTOM_BITE_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::ENTITY_PHANTOM_BITE, SoundCategory::Hostile, px, py, pz, 1.0f, 1.0f)));
            }
            break;

        case WorldEvents::ZOMBIE_CONVERT_TO_DROWNED_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::ENTITY_ZOMBIE_VILLAGER_CONVERTED, SoundCategory::Hostile, px, py, pz, 1.0f, 1.0f)));
            }
            break;

        case WorldEvents::CRAFTER_CRAFT_SOUND:
            // 合成器合成成功音效
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_CRAFTER_CRAFT, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;

        case WorldEvents::CRAFTER_FAIL_SOUND:
            // 合成器合成失败音效
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_CRAFTER_FAIL, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;

            // ========================================================================
            // 特殊效果事件 (1500-1505)
            // ========================================================================

        case WorldEvents::COMPOSTER_FILLED_UP: {
            // 堆肥桶填充事件
            // data > 0: 堆肥成功升级，播放 COMPOSTER_FILL_SUCCESS 音效
            // data <= 0: 仅填充未升级，播放 COMPOSTER_FILL 音效
            // 无论成功与否，都生成 10 个 HAPPY_VILLAGER 粒子
            if (m_audioService) {
                const auto& soundEvent =
                    (data > 0) ? SoundEvents::BLOCK_COMPOSTER_FILL_SUCCESS : SoundEvents::BLOCK_COMPOSTER_FILL;
                m_audioService->play(std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(soundEvent, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }

            // 计算堆肥桶填充高度处的粒子位置
            // 由于客户端可能还没有最新方块状态，使用方块中心偏上作为近似位置
            const f32 particleBaseY = static_cast<f32>(y) + 0.53125f;
            for (int i = 0; i < 10; ++i) {
                f32 ppx = static_cast<f32>(x) + 0.1875f + 0.625f * random.nextFloat();
                f32 ppy = particleBaseY + random.nextFloat() * 0.46875f;
                f32 ppz = static_cast<f32>(z) + 0.1875f + 0.625f * random.nextFloat();
                f32 vx = static_cast<f32>(random.nextGaussian()) * 0.02f;
                f32 vy = static_cast<f32>(random.nextGaussian()) * 0.02f;
                f32 vz = static_cast<f32>(random.nextGaussian()) * 0.02f;

                m_world.addParticle(ParticleTypeId::HappyVillager, Vector3(ppx, ppy, ppz), Vector3(vx, vy, vz));
            }
            break;
        }

        case WorldEvents::LAVA_EXTINGUISH: {
            // 岩浆熄灭事件：播放音效 + 8个大烟雾粒子
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(SoundEvents::BLOCK_LAVA_EXTINGUISH,
                        SoundCategory::Blocks,
                        px,
                        py,
                        pz,
                        0.5f,
                        2.6f + (random.nextFloat() - random.nextFloat()) * 0.8f)));
            }

            for (int i = 0; i < 8; ++i) {
                f32 lpx = static_cast<f32>(x) + random.nextFloat();
                f32 lpy = static_cast<f32>(y) + 1.2f;
                f32 lpz = static_cast<f32>(z) + random.nextFloat();
                m_world.addParticle(ParticleTypeId::LargeSmoke, Vector3(lpx, lpy, lpz), Vector3(0.0f, 0.0f, 0.0f));
            }
            break;
        }

        case WorldEvents::BONEMEAL_PARTICLES: {
            // 骨粉粒子效果
            // data 为粒子数量，0 则生成 15 个
            i32 count = (data == 0) ? 15 : data;
            m_world.addParticle(ParticleTypeId::HappyVillager,
                Vector3(px, py, pz),
                Vector3(0.0f, 0.0f, 0.0f),
                Vector3(1.0f, 1.0f, 1.0f),
                static_cast<u32>(count));

            // 播放骨粉使用音效
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::ITEM_BONE_MEAL_USE, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;
        }

        case WorldEvents::BREAK_BLOCK_EFFECTS: {
            // 方块破坏效果：根据方块状态ID获取正确的破坏音效和破坏粒子
            // data = 方块状态ID（BlockState::stateId()）
            const BlockState* blockState = BlockRegistry::instance().getBlockState(static_cast<u32>(data));
            if (blockState && !blockState->isAir()) {
                const BlockSoundType& soundType = blockState->getSoundType();
                if (m_audioService) {
                    m_audioService->play(std::make_unique<sound::SoundInstance>(
                        sound::SoundInstance::createLocated(soundType.getBreakSound(),
                            SoundCategory::Blocks,
                            px,
                            py,
                            pz,
                            (soundType.getVolume() + 1.0f) / 2.0f,
                            soundType.getPitch() * 0.8f)));
                }
                // TODO: 生成方块破碎粒子（Breaking 粒子需要方块状态纹理作为附加参数，暂不实现）
            }
            break;
        }

        case WorldEvents::DISPENSER_SMOKE: {
            // 发射器烟雾粒子，data 为方向（Direction.getIndex()）
            for (int i = 0; i < 10; ++i) {
                // 简化实现：在发射器位置周围生成烟雾粒子
                f32 spx = static_cast<f32>(x) + 0.5f + (random.nextFloat() - 0.5f) * 0.5f;
                f32 spy = static_cast<f32>(y) + 0.5f + (random.nextFloat() - 0.5f) * 0.5f;
                f32 spz = static_cast<f32>(z) + 0.5f + (random.nextFloat() - 0.5f) * 0.5f;
                f32 svx = static_cast<f32>(random.nextGaussian()) * 0.02f;
                f32 svy = static_cast<f32>(random.nextGaussian()) * 0.02f + 0.05f;
                f32 svz = static_cast<f32>(random.nextGaussian()) * 0.02f;
                m_world.addParticle(ParticleTypeId::Smoke, Vector3(spx, spy, spz), Vector3(svx, svy, svz));
            }
            break;
        }

        case WorldEvents::SPAWN_EXPLOSION_PARTICLE: {
            // 爆炸粒子
            m_world.addParticle(ParticleTypeId::HugeExplosion, Vector3(px, py, pz), Vector3(0.0f, 0.0f, 0.0f));
            break;
        }

        case WorldEvents::WET_SPONGE_DRY: {
            // 湿海绵在下界变干：8个云粒子（蒸汽） + 火焰熄灭音效
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(SoundEvents::BLOCK_FIRE_EXTINGUISH,
                        SoundCategory::Blocks,
                        px,
                        py,
                        pz,
                        0.5f,
                        2.6f + (random.nextFloat() - random.nextFloat()) * 0.8f)));
            }

            for (int i = 0; i < 8; ++i) {
                f32 lpx = static_cast<f32>(x) + random.nextFloat();
                f32 lpy = static_cast<f32>(y) + 1.2f;
                f32 lpz = static_cast<f32>(z) + random.nextFloat();
                m_world.addParticle(ParticleTypeId::LargeSmoke, Vector3(lpx, lpy, lpz), Vector3(0.0f, 0.0f, 0.0f));
            }
            break;
        }

        case WorldEvents::DRIPSTONE_DRIP: {
            // 滴石滴水粒子效果
            // 事件由服务端在 maybeTransferFluid 中触发（钟乳石成功向炼药锅传输流体时）
            // 客户端需要根据钟乳石上方的流体类型选择正确的粒子类型
            {
                // 获取钟乳石尖端位置的方块状态
                const BlockState* dripstoneState = m_world.getBlockState(x, y, z);
                if (dripstoneState != nullptr) {
                    // 使用 PointedDripstoneBlock 静态方法计算粒子位置和检测流体类型
                    BlockPos tipPos(x, y, z);
                    Vector3 particlePos = blocks::PointedDripstoneBlock::getDripParticlePosition(tipPos);

                    // 检测流体类型：沿钟乳石向上搜索非滴石方块，然后检查其流体状态
                    // 由于 ClientWorld 不继承 IWorld，无法直接调用 getFluidAboveStalactite，
                    // 因此在此处内联流体检测逻辑
                    ParticleTypeId dripType = ParticleTypeId::DrippingDripstoneWater; // 默认水滴

                    if (blocks::PointedDripstoneBlock::isStalactite(*dripstoneState)) {
                        i32 searchX = x, searchY = y + 1, searchZ = z;
                        const BlockState* aboveState = nullptr;
                        for (i32 i = 0; i < 11; ++i) {
                            aboveState = m_world.getBlockState(searchX, searchY, searchZ);
                            if (aboveState == nullptr ||
                                !aboveState->is(block_registry::CaveBlocks::POINTED_DRIPSTONE)) {
                                break;
                            }
                            searchY++;
                        }
                        // aboveState 现在是根方块上方的方块
                        if (aboveState != nullptr) {
                            // 检查是否是泥巴（Mud），泥巴视为水源
                            if (aboveState->is(block_registry::MudBlocks::MUD)) {
                                dripType = ParticleTypeId::DrippingDripstoneWater;
                            } else {
                                // 检查流体状态
                                const fluid::FluidState* fluidState = aboveState->getFluidState();
                                if (fluidState != nullptr && !fluidState->isEmpty()) {
                                    const fluid::Fluid& fluid = fluidState->getFluid();
                                    if (fluid.isIn(fluid::FluidTags::LAVA())) {
                                        dripType = ParticleTypeId::DrippingDripstoneLava;
                                    }
                                }
                            }
                        }
                    }
                    m_world.addParticle(dripType, particlePos, Vector3(0.0f, 0.0f, 0.0f));
                }
            }
            break;
        }

        case WorldEvents::POINTED_DRIPSTONE_LAND_SOUND: {
            // 滴石尖锥落地音效
            if (m_audioService) {
                f32 pitch = random.nextFloat() * 0.1f + 0.9f;
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_POINTED_DRIPSTONE_LAND, SoundCategory::Blocks, px, py, pz, 2.0f, pitch)));
            }
            break;
        }

        case WorldEvents::DRIP_LAVA_INTO_CAULDRON_SOUND: {
            // 熔岩滴入炼药锅音效
            if (m_audioService) {
                f32 pitch = random.nextFloat() * 0.1f + 0.9f;
                m_audioService->play(std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(SoundEvents::BLOCK_POINTED_DRIPSTONE_DRIP_LAVA_INTO_CAULDRON,
                        SoundCategory::Blocks,
                        px,
                        py,
                        pz,
                        2.0f,
                        pitch)));
            }
            break;
        }

        case WorldEvents::DRIP_WATER_INTO_CAULDRON_SOUND: {
            // 水滴入炼药锅音效
            if (m_audioService) {
                f32 pitch = random.nextFloat() * 0.1f + 0.9f;
                m_audioService->play(std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(SoundEvents::BLOCK_POINTED_DRIPSTONE_DRIP_WATER_INTO_CAULDRON,
                        SoundCategory::Blocks,
                        px,
                        py,
                        pz,
                        2.0f,
                        pitch)));
            }
            break;
        }

        case WorldEvents::END_PORTAL_FRAME_FILL: {
            // 末地传送门框填充：播放音效
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_END_PORTAL_FRAME_FILL, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;
        }

        case WorldEvents::REDSTONE_TORCH_BURNOUT: {
            // 红石火把烧断：播放音效 + 烟雾粒子
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_REDSTONE_TORCH_BURNOUT, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }

            for (int i = 0; i < 3; ++i) {
                f32 rspx = static_cast<f32>(x) + 0.5f + (random.nextFloat() - 0.5f) * 0.3f;
                f32 rspy = static_cast<f32>(y) + 0.7f;
                f32 rspz = static_cast<f32>(z) + 0.5f + (random.nextFloat() - 0.5f) * 0.3f;
                m_world.addParticle(ParticleTypeId::Smoke, Vector3(rspx, rspy, rspz), Vector3(0.0f, 0.0f, 0.0f));
            }
            break;
        }

        case WorldEvents::SMASH_ATTACK: {
            // 重锤砸地攻击粒子效果（对应 MC LevelEvent.PARTICLES_SMASH_ATTACK = 2013）
            // data 值（750）用于粒子扩散半径的参数
            // TODO: 当实现专用 SmashAttack 粒子后替换下面的临时实现
            // 当前使用爆炸粒子 + 烟雾粒子近似砸地冲击波效果
            {
                f32 px = static_cast<f32>(x) + 0.5f;
                f32 py = static_cast<f32>(y) + 1.0f;
                f32 pz = static_cast<f32>(z) + 0.5f;
                // 中心爆炸粒子
                m_world.addParticle(ParticleTypeId::HugeExplosion, Vector3(px, py, pz), Vector3(0.0f, 0.0f, 0.0f));
                // 周围烟雾粒子
                for (i32 i = 0; i < 8; ++i) {
                    f32 angle = random.nextFloat() * 6.2831855f;
                    f32 dist = random.nextFloat() * 2.0f;
                    f32 spx = px + std::cos(angle) * dist;
                    f32 spz = pz + std::sin(angle) * dist;
                    m_world.addParticle(ParticleTypeId::Poof, Vector3(spx, py, spz), Vector3(0.0f, 0.1f, 0.0f));
                }
            }
            break;
        }

        default:
            // 未知事件ID，忽略
            break;
    }
}

} // namespace mc::client
