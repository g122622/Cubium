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
#include "client/renderer/trident/particle/data/DustParticleData.hpp"
#include "client/renderer/trident/particle/data/EntityEffectParticleData.hpp"
#include "client/renderer/trident/particle/data/ItemParticleData.hpp"
#include "client/renderer/trident/particle/data/TrailParticleData.hpp"
#include "client/renderer/trident/particle/data/VibrationParticleData.hpp"
#include "client/skin/ClientSkinManager.hpp"
#include "client/sound/AudioService.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "client/ui/minecraft/screens/CartographyScreen.hpp"
#include "client/ui/minecraft/screens/ChestScreen.hpp"
#include "client/ui/minecraft/screens/CraftingScreen.hpp"
#include "client/ui/minecraft/screens/CreativeScreen.hpp"
#include "client/ui/minecraft/screens/FurnaceScreen.hpp"
#include "client/ui/minecraft/screens/InventoryScreen.hpp"
#include "client/ui/minecraft/screens/SignEditScreen.hpp"
#include "client/ui/minecraft/widgets/ChatWidget.hpp"
#include "client/ui/minecraft/widgets/ScreenStackWidget.hpp"
#include "client/ui/minecraft/widgets/TitleWidget.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "client/world/player/ClientPlayerPredictor.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/container/ItemPickerMenu.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/ExperiencePackets.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/network/SkinPackets.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockSoundType.hpp"
#include "common/world/block/IGrowable.hpp"
#include "common/world/block/blocks/cave/PointedDripstoneBlock.hpp"
#include "common/world/block/registry/CaveBlocks.hpp"
#include "common/world/block/registry/MudBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include "common/world/blockentity/processing/FurnaceInventory.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "server/menu/CraftingMenu.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

using namespace mc::trace;

namespace mc::client {

namespace {

template <typename Menu>
void applyContainerContent(Menu* menu, ContainerId containerId, const std::vector<ItemStack>& items)
{
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

/**
 * @brief 应用容器内容包 + 光标物品到客户端菜单
 *
 * 槽位走 applyContainerContent；末尾 carried 同步到 menu->setCarriedItem
 * （修复此前光标物品从不回传客户端的缺陷）。
 */
template <typename Menu>
void applyContainerContentWithCarried(
    Menu* menu, ContainerId containerId, const std::vector<ItemStack>& items, const ItemStack& carried)
{
    if (!menu || menu->getId() != containerId) {
        return;
    }
    applyContainerContent(menu, containerId, items);
    menu->setCarriedItem(carried);
}

template <typename Menu>
bool applyContainerSlot(Menu* menu, ContainerId containerId, i32 slotIndex, const ItemStack& item)
{
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
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "SetupNetworkCallbacks");

    if (!m_networkClient) return;

    NetworkClientCallbacks callbacks;

    callbacks.onLoginSuccess =
        [this](PlayerId playerId, EntityInstanceId entityId, const Uuid& uuid, const std::string& username) {
            spdlog::info("Login successful: playerId={}, entityId={}, username={}", playerId, entityId, username);

            // 设置本地玩家身份
            m_localIdentity.setIdentity(playerId, entityId);
            // 注册本地玩家身份（UUID 来自扩展后的 LoginResponsePacket）
            m_identityRegistry.registerLocalPlayer(entityId, playerId, uuid, username);

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
        m_identityRegistry.clear();
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
        m_identityRegistry.clear();

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

    callbacks.onChunkData = [this](ChunkCoord x,
                                ChunkCoord z,
                                DimensionId dimension,
                                const u8* data,
                                size_t size,
                                std::shared_ptr<std::vector<u8>> buffer) {
        m_world.onChunkData(x, z, dimension, data, size, std::move(buffer));
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

        // 远程玩家使用 playerId 作为 entityId（服务端在 EntityTracker 中这样处理）。
        // 此处通过 PlayerIdentityRegistry 登记身份，避免散落的 static_cast 反模式，
        // 并与 PlayerListEntry 的 UUID 关联（供渲染层按 entityId 查皮肤区域）。
        const EntityInstanceId entityId = static_cast<EntityInstanceId>(playerId);
        m_identityRegistry.registerNetworkPlayer(entityId, playerId, username);

        ClientEntity* entity = entityManager.spawnEntity(entityId, mc::entity::EntityTypeKeys::PLAYER);
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

        // 远程玩家：移除实体与身份登记
        const EntityInstanceId entityId = static_cast<EntityInstanceId>(playerId);
        m_identityRegistry.removeByEntityId(entityId);
        m_world.entityManager().removeEntity(entityId);
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

        ClientEntity* entity = m_world.entityManager().getEntity(static_cast<EntityInstanceId>(playerId));
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

        // 玩家背包屏走 kagero 体系（InventoryScreen / CreativeScreen 共享 containerId=0）
        if (auto* kageroScreen =
                dynamic_cast<ui::minecraft::InventoryScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            applyContainerContent(kageroScreen->getMenu(), mc::inventory::PLAYER_CONTAINER_ID, items);
            kageroScreen->syncSlots();
        } else if (auto* creativeScreen = dynamic_cast<ui::minecraft::CreativeScreen*>(
                       ScreenManager::instance().getCurrentKageroScreen())) {
            applyContainerContent(creativeScreen->getMenu(), mc::inventory::PLAYER_CONTAINER_ID, items);
            creativeScreen->syncSlots();
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

        // 工作台屏走 kagero 体系（CraftingScreen 继承 ContainerScreenBase<CraftingMenu>）
        if (type == ContainerType::Crafting) {
            auto craftingMenu =
                std::make_unique<mc::CraftingMenu>(packet.containerId(), &m_player->inventory(), nullptr);
            auto screen = std::make_unique<ui::minecraft::CraftingScreen>(std::move(craftingMenu),
                makeContainerClickSender(m_networkClient.get()),
                makeContainerCloseSender(m_networkClient.get()));
            if (m_renderer && m_renderer->isGuiRendererInitialized()) {
                screen->setRenderers(&m_renderer->guiRenderer(),
                    m_guiTextureManager.get(),
                    m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
                screen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
            }
            ScreenManager::instance().openScreen(std::move(screen));
            return;
        }

        // 箱子屏走 kagero 体系（ChestScreen 继承 ContainerScreenBase<ChestContainer>）
        if (type == ContainerType::Generic9x1 || type == ContainerType::Generic9x2 ||
            type == ContainerType::Generic9x3 || type == ContainerType::Generic9x4 ||
            type == ContainerType::Generic9x5 || type == ContainerType::Generic9x6 ||
            type == ContainerType::ShulkerBox) {
            i32 rows = 3;
            switch (type) {
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
            auto chestContainer = std::make_unique<mc::blockentity::ChestContainer>(packet.containerId(),
                &m_player->inventory(),
                std::shared_ptr<mc::IInventory>(std::make_shared<mc::blockentity::SimpleInventory>(
                    rows * mc::blockentity::ChestContainer::SLOTS_PER_ROW)),
                rows);
            auto screen = std::make_unique<ui::minecraft::ChestScreen>(std::move(chestContainer),
                makeContainerClickSender(m_networkClient.get()),
                makeContainerCloseSender(m_networkClient.get()));
            if (m_renderer && m_renderer->isGuiRendererInitialized()) {
                screen->setRenderers(&m_renderer->guiRenderer(),
                    m_guiTextureManager.get(),
                    m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
                screen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
            }
            ScreenManager::instance().openScreen(std::move(screen));
            return;
        }

        // 熔炉屏走 kagero 体系（FurnaceScreen 继承 ContainerScreenBase<FurnaceContainer>）
        // 客户端无熔炉方块实体，构造本地 FurnaceInventory 持有槽位，燃烧/熔炼进度经
        // WindowPropertyPacket 同步到 FurnaceContainer 的 tracked int，再驱动火焰/箭头动画。
        if (type == ContainerType::Furnace || type == ContainerType::BlastFurnace || type == ContainerType::Smoker) {
            auto furnaceContainer = std::make_unique<mc::blockentity::FurnaceContainer>(packet.containerId(),
                &m_player->inventory(),
                std::shared_ptr<mc::IInventory>(std::make_shared<mc::blockentity::FurnaceInventory>()),
                nullptr);
            auto screen = std::make_unique<ui::minecraft::FurnaceScreen>(std::move(furnaceContainer),
                makeContainerClickSender(m_networkClient.get()),
                makeContainerCloseSender(m_networkClient.get()));
            if (m_renderer && m_renderer->isGuiRendererInitialized()) {
                screen->setRenderers(&m_renderer->guiRenderer(),
                    m_guiTextureManager.get(),
                    m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
                screen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
            }
            ScreenManager::instance().openScreen(std::move(screen));
            return;
        }

        // 制图台屏走 kagero 体系（CartographyScreen 继承 ContainerScreenBase<CartographyContainer>）
        // 客户端无制图台方块实体，world 传 nullptr：updateResult 空操作，结果槽由服务端经
        // ContainerSlotPacket 下推。客户端不计算地图缩放/锁定/复制结果。
        if (type == ContainerType::Cartography) {
            auto cartographyContainer = std::make_unique<mc::CartographyContainer>(
                packet.containerId(), &m_player->inventory(), BlockPos(0, 0, 0), nullptr);
            auto screen = std::make_unique<ui::minecraft::CartographyScreen>(std::move(cartographyContainer),
                makeContainerClickSender(m_networkClient.get()),
                makeContainerCloseSender(m_networkClient.get()));
            if (m_renderer && m_renderer->isGuiRendererInitialized()) {
                screen->setRenderers(&m_renderer->guiRenderer(),
                    m_guiTextureManager.get(),
                    m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
                screen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
            }
            ScreenManager::instance().openScreen(std::move(screen));
            return;
        }

        spdlog::error("Ignored unsupported container type {}", static_cast<i32>(type));
    };

    callbacks.onSignEditorOpen = [this](const network::OpenSignEditorPacket& packet) {
        // 获取屏幕栈
        auto* screenStack = getScreenStackWidget(this);
        if (!screenStack) {
            spdlog::warn("[Network] Cannot open sign editor: ScreenStackWidget not available");
            return;
        }

        // 从客户端 BlockEntity 存储读取告示牌当前文本
        // 服务端在打开编辑器前会通过 BlockEntityData 包同步告示牌数据到客户端
        std::array<std::string, ui::minecraft::SignEditScreen::LINE_COUNT> initialLines{};
        const BlockEntity* entity = m_world.getBlockEntity(packet.pos());
        if (entity != nullptr && entity->getType() == BlockEntityType::Sign) {
            const auto* signEntity = static_cast<const blockentity::SignEntity*>(entity);
            for (i32 i = 0; i < ui::minecraft::SignEditScreen::LINE_COUNT; ++i) {
                initialLines[static_cast<std::size_t>(i)] = signEntity->getLineText(i);
            }
        }

        // 创建告示牌编辑屏幕
        auto signScreen = std::make_unique<ui::minecraft::SignEditScreen>(
            packet.pos(),
            initialLines,
            packet.isFrontSide(),
            // 提交回调：发送 UpdateSignPacket 给服务端
            [this](const BlockPos& pos,
                const std::array<std::string, ui::minecraft::SignEditScreen::LINE_COUNT>& lines,
                bool isFrontSide) {
                if (m_networkClient) {
                    m_networkClient->sendUpdateSign(pos, lines, isFrontSide);
                }
            },
            // 关闭回调：弹出屏幕栈
            [screenStack]() { screenStack->pop(); });

        // 设置屏幕大小
        signScreen->setBounds(ui::kagero::Rect(0, 0, m_guiScaleState.width, m_guiScaleState.height));

        // 添加到屏幕栈（push 会自动调用 onOpen）
        screenStack->push(std::move(signScreen));
    };

    callbacks.onBlockEntityData = [this](const network::BlockEntityDataPacket& packet) {
        // 收到方块实体数据更新包，转发给 ClientWorld 更新本地 BlockEntity 存储
        m_world.onBlockEntityData(packet.pos(), packet.type(), packet.nbtData());
    };

    callbacks.onContainerContent = [this](const ContainerContentPacket& packet) {
        if (!m_player) {
            return;
        }

        // 优先查 kagero 体系背包屏（InventoryScreen / CreativeScreen 共享 containerId=0）
        if (auto* kageroScreen =
                dynamic_cast<ui::minecraft::InventoryScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            applyContainerContentWithCarried(
                kageroScreen->getMenu(), packet.containerId(), packet.items(), packet.carriedItem());
            kageroScreen->syncSlots();
            return;
        }
        if (auto* creativeScreen =
                dynamic_cast<ui::minecraft::CreativeScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            applyContainerContentWithCarried(
                creativeScreen->getMenu(), packet.containerId(), packet.items(), packet.carriedItem());
            creativeScreen->syncSlots();
            return;
        }

        if (auto* craftingScreen =
                dynamic_cast<ui::minecraft::CraftingScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            applyContainerContentWithCarried(
                craftingScreen->getMenu(), packet.containerId(), packet.items(), packet.carriedItem());
            craftingScreen->syncSlots();
            return;
        }
        if (auto* chestScreen =
                dynamic_cast<ui::minecraft::ChestScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            applyContainerContentWithCarried(
                chestScreen->getMenu(), packet.containerId(), packet.items(), packet.carriedItem());
            chestScreen->syncSlots();
            return;
        }

        if (auto* furnaceScreen =
                dynamic_cast<ui::minecraft::FurnaceScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            applyContainerContentWithCarried(
                furnaceScreen->getMenu(), packet.containerId(), packet.items(), packet.carriedItem());
            furnaceScreen->syncSlots();
            return;
        }

        if (auto* cartographyScreen =
                dynamic_cast<ui::minecraft::CartographyScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            applyContainerContentWithCarried(
                cartographyScreen->getMenu(), packet.containerId(), packet.items(), packet.carriedItem());
            cartographyScreen->syncSlots();
            return;
        }
    };

    callbacks.onContainerSlot = [this](const ContainerSlotPacket& packet) {
        if (!m_player) {
            return;
        }

        // 优先查 kagero 体系背包屏（InventoryScreen / CreativeScreen 共享 containerId=0）
        if (auto* kageroScreen =
                dynamic_cast<ui::minecraft::InventoryScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            if (applyContainerSlot(kageroScreen->getMenu(), packet.containerId(), packet.slotIndex(), packet.item())) {
                kageroScreen->syncSlots();
                return;
            }
        }
        if (auto* creativeScreen =
                dynamic_cast<ui::minecraft::CreativeScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            if (applyContainerSlot(
                    creativeScreen->getMenu(), packet.containerId(), packet.slotIndex(), packet.item())) {
                creativeScreen->syncSlots();
                return;
            }
        }
        if (auto* craftingScreen =
                dynamic_cast<ui::minecraft::CraftingScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            if (applyContainerSlot(
                    craftingScreen->getMenu(), packet.containerId(), packet.slotIndex(), packet.item())) {
                craftingScreen->syncSlots();
                return;
            }
        }
        if (auto* chestScreen =
                dynamic_cast<ui::minecraft::ChestScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            if (applyContainerSlot(chestScreen->getMenu(), packet.containerId(), packet.slotIndex(), packet.item())) {
                chestScreen->syncSlots();
                return;
            }
        }
        if (auto* furnaceScreen =
                dynamic_cast<ui::minecraft::FurnaceScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            if (applyContainerSlot(furnaceScreen->getMenu(), packet.containerId(), packet.slotIndex(), packet.item())) {
                furnaceScreen->syncSlots();
                return;
            }
        }
        if (auto* cartographyScreen =
                dynamic_cast<ui::minecraft::CartographyScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            if (applyContainerSlot(
                    cartographyScreen->getMenu(), packet.containerId(), packet.slotIndex(), packet.item())) {
                cartographyScreen->syncSlots();
                return;
            }
        }
    };

    callbacks.onCloseContainer = [this](ContainerId containerId) {
        // kagero 体系背包屏（InventoryScreen / CreativeScreen 共享 containerId=0）
        if (auto* kageroScreen =
                dynamic_cast<ui::minecraft::InventoryScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            if (kageroScreen->getMenu() && kageroScreen->getMenu()->getId() == containerId) {
                ScreenManager::instance().closeScreen();
            }
            return;
        }
        if (auto* creativeScreen =
                dynamic_cast<ui::minecraft::CreativeScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            if (creativeScreen->getMenu() && creativeScreen->getMenu()->getId() == containerId) {
                ScreenManager::instance().closeScreen();
            }
            return;
        }
        if (auto* craftingScreen =
                dynamic_cast<ui::minecraft::CraftingScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            if (craftingScreen->getMenu() && craftingScreen->getMenu()->getId() == containerId) {
                ScreenManager::instance().closeScreen();
            }
            return;
        }
        if (auto* chestScreen =
                dynamic_cast<ui::minecraft::ChestScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            if (chestScreen->getMenu() && chestScreen->getMenu()->getId() == containerId) {
                ScreenManager::instance().closeScreen();
            }
            return;
        }
        if (auto* furnaceScreen =
                dynamic_cast<ui::minecraft::FurnaceScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            if (furnaceScreen->getMenu() && furnaceScreen->getMenu()->getId() == containerId) {
                ScreenManager::instance().closeScreen();
            }
            return;
        }
        if (auto* cartographyScreen =
                dynamic_cast<ui::minecraft::CartographyScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
            if (cartographyScreen->getMenu() && cartographyScreen->getMenu()->getId() == containerId) {
                ScreenManager::instance().closeScreen();
            }
            return;
        }
    };

    // 熔炉燃烧/熔炼进度同步：服务端经 WindowPropertyPacket 下推 tracked int 到客户端。
    // 客户端按属性索引写入 FurnaceContainer 的 tracked int，FurnaceScreen 渲染时读取驱动火焰/箭头。
    callbacks.onWindowProperty = [this](const WindowPropertyPacket& packet) {
        auto* furnaceScreen =
            dynamic_cast<ui::minecraft::FurnaceScreen*>(ScreenManager::instance().getCurrentKageroScreen());
        if (furnaceScreen == nullptr || furnaceScreen->getMenu() == nullptr) {
            return;
        }
        if (furnaceScreen->getMenu()->getId() != packet.containerId()) {
            return;
        }
        furnaceScreen->getMenu()->setTrackedInt(packet.property(), packet.value());
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
        MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Entity,
            "onSpawnEntity",
            "entityId",
            entityId,
            "typeId",
            typeId,
            "hasItem",
            itemStack != nullptr);

        auto& entityManager = m_world.entityManager();
        ClientEntity* entity = entityManager.spawnEntity(static_cast<EntityInstanceId>(entityId), typeId);
        if (!entity) {
            entity = entityManager.getEntity(static_cast<EntityInstanceId>(entityId));
        }

        if (!entity) {
            return;
        }

        entity->setPosition(x, y, z);
        entity->setRotation(yaw, pitch);
        entity->setVelocity(vx, vy, vz);

        if (typeId == mc::entity::EntityTypeKeys::ITEM) {
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
            ClientEntity* entity = entityManager.spawnEntity(static_cast<EntityInstanceId>(entityId), typeId);
            if (!entity) {
                entity = entityManager.getEntity(static_cast<EntityInstanceId>(entityId));
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
        MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Entity, "onEntityDestroy", "count", entityIds.size());

        auto& entityManager = m_world.entityManager();
        for (u32 entityId : entityIds) {
            // 使用 LocalPlayerIdentity 判断是否是本地玩家实体
            if (m_localIdentity.isLocalPlayerEntity(static_cast<EntityInstanceId>(entityId))) {
                continue;
            }

            // 清理实体渲染网格（静态+动画），避免 Vulkan 资源泄漏
            if (m_renderer) {
                m_renderer->entityRendererManager().removeEntityMeshes(static_cast<EntityInstanceId>(entityId));
            }

            entityManager.removeEntity(static_cast<EntityInstanceId>(entityId));

            // 通知音频系统实体移除
            if (m_audioService) {
                m_audioService->onEntityRemove(entityId);
            }
        }
    };

    callbacks.onEntityMove = [this](u32 entityId, f32 deltaX, f32 deltaY, f32 deltaZ) {
        // 使用 LocalPlayerIdentity 判断是否是本地玩家实体
        const EntityInstanceId eid = static_cast<EntityInstanceId>(entityId);
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
        MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Entity, "onEntityTeleport", "entityId", entityId);

        // 使用 LocalPlayerIdentity 判断是否是本地玩家实体
        const EntityInstanceId eid = static_cast<EntityInstanceId>(entityId);
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
        const EntityInstanceId eid = static_cast<EntityInstanceId>(entityId);
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

    callbacks.onExplosion = [this](const network::ExplosionPacket& packet) {
        // 对应 MC Java ClientPacketListener.handleExplosion:
        //   1. 应用受影响方块的客户端破坏视觉（方块设为空气）
        //   2. 若包携带玩家击退向量（motion != 0），调用 player.addDeltaMovement(vec) 累加到本地速度
        //
        // 关键：使用 addVelocity（累加）而非 setVelocity（覆盖），因为 ExplosionPacket 携带的是
        // 击退增量，必须叠加到玩家当前速度上。这与 MC Java 的 addDeltaMovement 语义一致。
        //
        // 服务端在 Explosion/WindChargeEntity 的玩家分支已跳过 addVelocity 并清除 hurtMarked，
        // 保证不会通过 EntityVelocityPacket 覆盖客户端速度。因此此处累加是击退的唯一来源。
        //
        // 注意：爆炸音效与粒子效果由服务端通过 Explosion::_playSound / _spawnParticles
        // （调用 IWorld::playSound / IWorld::addParticle）或 WindChargeEntity 的对应方法
        // 直接发送给追踪玩家，ExplosionPacket 不携带音效/粒子信息（与 MC Java
        // ClientboundExplodePacket 携带 explosionSound/explosionParticle 不同）。
        // 因此此处不需要重复触发音效/粒子。

        // 1. 应用受影响方块的客户端视觉：将每个方块设为空气
        // ExplosionPacket 中的 affectedBlocks 使用相对坐标（相对于 floor(explosionPos)），
        // 需转换为绝对坐标后通知客户端世界。
        const Vector3& explosionPos = packet.position();
        const i32 floorX = static_cast<i32>(std::floor(explosionPos.x));
        const i32 floorY = static_cast<i32>(std::floor(explosionPos.y));
        const i32 floorZ = static_cast<i32>(std::floor(explosionPos.z));
        const BlockState* airState = BlockRegistry::instance().airState();
        for (const BlockPos& relPos : packet.affectedBlocks()) {
            const BlockPos absPos(floorX + relPos.x, floorY + relPos.y, floorZ + relPos.z);
            m_world.setBlockState(absPos.x, absPos.y, absPos.z, airState);
        }

        // 2. 应用玩家击退（累加到本地玩家速度）
        const Vector3& motion = packet.motion();
        if (motion.x != 0.0f || motion.y != 0.0f || motion.z != 0.0f) {
            if (m_player) {
                m_player->addVelocity(motion.x, motion.y, motion.z);
            }
        }
    };

    callbacks.onEntityMetadata = [this](u32 entityId, const std::vector<u8>& metadata) {
        MC_TRACE_INSTANT_EVENT(
            TraceEvents.Client.Entity, "onEntityMetadata", "entityId", entityId, "size", metadata.size());

        // 使用 LocalPlayerIdentity 判断是否是本地玩家实体
        const EntityInstanceId eid = static_cast<EntityInstanceId>(entityId);
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
        auto* entity = m_world.entityManager().getEntity(static_cast<EntityInstanceId>(entityId));
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
                            ::mc::particle::ParticleTypeId::Crit, particlePos, velocity, &m_world);
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
                            ::mc::particle::ParticleTypeId::EnchantedHit, particlePos, velocity, &m_world);
                    }
                }
                break;
            }
        }
    };

    callbacks.onHurtAnimation = [this](u32 entityId, f32 hurtDir) {
        // 同步 damageTilt 的 hurtDir：写到 ClientEntity 代理；若为本地玩家，额外写到
        // 实际 Player 对象（第一人称 FirstPersonRenderer 从 context.player 读取 hurtTime/hurtDir）。
        auto* entity = m_world.entityManager().getEntity(static_cast<EntityInstanceId>(entityId));
        if (entity != nullptr) {
            entity->setHurtDir(hurtDir);
        }
        if (m_player != nullptr && m_localIdentity.isLocalPlayerEntity(static_cast<EntityInstanceId>(entityId))) {
            m_player->animateHurt(hurtDir);
        }
    };

    callbacks.onEntityHeadLook = [this](u32 entityId, f32 headYaw) {
        // 使用 LocalPlayerIdentity 判断是否是本地玩家实体
        const EntityInstanceId eid = static_cast<EntityInstanceId>(entityId);
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
                            ::mc::particle::ParticleTypeId::Heart, particlePos, velocity, &m_world);
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
                            ::mc::particle::ParticleTypeId::Smoke, particlePos, velocity, &m_world);
                    }
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::LoveHeart): {
                // 状态 18: 繁殖爱心效果
                if (m_world.particleManager() != nullptr) {
                    glm::vec3 heartPos = entityPos + glm::vec3(0.0f, 0.5f, 0.0f);
                    m_world.particleManager()->addPendingParticle(
                        ::mc::particle::ParticleTypeId::Heart, heartPos, glm::vec3(0.0f, 0.0f, 0.0f), &m_world);
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::EatBlock): {
                // 状态 10: 吃草/方块动画（羊低头吃草）或 TNT 矿车引燃
                if (entity != nullptr) {
                    // TNT 矿车收到 status 10 时设置引信值
                    if (entity->entityType() == mc::entity::VanillaEntityTypeKeys::TNT_MINECART) {
                        entity->setFuseTimer(80);
                    } else {
                        entity->setEatAnimationTimer(40);
                    }
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::ShakeOffWater): {
                // 状态 8: 狼开始甩水（ShakeOffWater）
                // 对应 MC Wolf.aiStep() 第 300-307 行：服务端检测到 isWet && !isShaking && onGround
                // 时广播 byte 8，客户端收到后开始甩水动画。
                // 客户端甩水进度由 ClientEntity::tick() 推进（每 tick +0.05）。
                if (entity != nullptr) {
                    const std::string& typeId = entity->getTypeId();
                    if (typeId == "minecraft:wolf" || typeId == "wolf") {
                        entity->setWolfShaking(true);
                        entity->setWolfIsWet(true);
                        entity->setWolfShakeAnim(0.0f);
                        entity->setWolfShakeAnimO(0.0f);
                    }
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::WolfStopShaking): {
                // 状态 56: 狼取消甩水（WolfStopShaking）
                // 对应 MC Wolf.tick() 第 327-330 行：甩水中再次接触水时广播 byte 56，
                // 客户端收到后立即取消甩水动画。
                if (entity != nullptr) {
                    const std::string& typeId = entity->getTypeId();
                    if (typeId == "minecraft:wolf" || typeId == "wolf") {
                        entity->setWolfShaking(false);
                        entity->setWolfShakeAnim(0.0f);
                        entity->setWolfShakeAnimO(0.0f);
                    }
                }
                break;
            }
            case static_cast<u8>(EntityStatusPacket::Status::RabbitJump): {
                // 状态 1: 兔子开始跳跃动画
                // 对应 MC 1.21.11 Rabbit.jumpFromGround() 中 broadcastEntityState(this, (byte)1)
                // 客户端收到后启动 jumpDuration=10 计时器，用于 RabbitModel 计算 jumpRotation
                // （参考 MC Rabbit.handleEntityEvent(byte 1)：jumpDuration=10; jumpTicks=0;）
                if (entity != nullptr) {
                    const std::string& typeId = entity->getTypeId();
                    if (typeId == "minecraft:rabbit" || typeId == "rabbit") {
                        entity->setRabbitJumpStart();
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
                            ::mc::particle::ParticleTypeId::Heart, particlePos, velocity, &m_world);
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
                            ::mc::particle::ParticleTypeId::Smoke, particlePos, velocity, &m_world);
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
                            ::mc::particle::ParticleTypeId::Portal, particlePos, velocity, &m_world);
                    }
                }
                break;
            }
            // 状态 4: 铁傀儡攻击动画（举臂）或疣猪兽/僵尸疣兽攻击动画（甩头）
            // MC 原版中铁傀儡和疣猪兽/僵尸疣兽共用状态码 4，客户端按实体类型区分
            // HoglinAttack 和 IronGolemAttack 的值都是 4，只需写一个 case
            case static_cast<u8>(EntityStatusPacket::Status::IronGolemAttack): {
                if (entity != nullptr) {
                    const std::string& typeId = entity->getTypeId();
                    if (typeId == "minecraft:hoglin" || typeId == "hoglin" || typeId == "minecraft:zoglin" ||
                        typeId == "zoglin") {
                        // 疣猪兽/僵尸疣兽攻击动画
                        entity->setFlingAnimationTicks(10);
                    } else {
                        // 铁傀儡攻击动画
                        entity->setIronGolemAttackTimer(10);
                        entity->setIronGolemArmsRaised(true);
                    }
                }
                // 播放攻击音效：根据实体类型选择不同音效
                if (m_audioService && entity != nullptr) {
                    const std::string& typeId = entity->getTypeId();
                    if (typeId == "minecraft:hoglin" || typeId == "hoglin") {
                        auto sound = sound::SoundInstance::createLocated(mc::SoundEvents::ENTITY_HOGLIN_ATTACK,
                            mc::sound::SoundCategory::Hostile,
                            entityPos.x,
                            entityPos.y,
                            entityPos.z,
                            1.0f,
                            1.0f);
                        m_audioService->play(std::make_unique<sound::SoundInstance>(std::move(sound)));
                    } else if (typeId == "minecraft:zoglin" || typeId == "zoglin") {
                        auto sound = sound::SoundInstance::createLocated(mc::SoundEvents::ENTITY_ZOGLIN_ATTACK,
                            mc::sound::SoundCategory::Hostile,
                            entityPos.x,
                            entityPos.y,
                            entityPos.z,
                            1.0f,
                            1.0f);
                        m_audioService->play(std::make_unique<sound::SoundInstance>(std::move(sound)));
                    } else {
                        // 铁傀儡攻击音效
                        auto sound = sound::SoundInstance::createLocated(mc::SoundEvents::ENTITY_IRON_GOLEM_ATTACK,
                            mc::sound::SoundCategory::Neutral,
                            entityPos.x,
                            entityPos.y,
                            entityPos.z,
                            1.0f,
                            1.0f);
                        m_audioService->play(std::make_unique<sound::SoundInstance>(std::move(sound)));
                    }
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
                            ::mc::particle::ParticleTypeId::Heart, particlePos, velocity, &m_world);
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
                            ::mc::particle::ParticleTypeId::AngryVillager, particlePos, velocity, &m_world);
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
                            ::mc::particle::ParticleTypeId::HappyVillager, particlePos, velocity, &m_world);
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
                            ::mc::particle::ParticleTypeId::Splash, particlePos, velocity, &m_world);
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
                            ::mc::particle::ParticleTypeId::Breaking, particlePos, velocity, &m_world);
                    }
                }
                break;
            }
            default: {
                // 检查是否为权限等级变更状态 (status byte 24-28, 其中 level = status - 24)
                i32 permLevel = EntityStatusPacket::toPermissionLevel(status);
                if (permLevel >= 0) {
                    // 权限等级变更，仅对本机玩家生效
                    if (m_localIdentity.isLocalPlayerEntity(static_cast<EntityInstanceId>(entityId))) {
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

    callbacks.onBlockBreakAnim = [this](EntityInstanceId breakerEntityId, i32 x, i32 y, i32 z, i8 stage) {
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

    callbacks.onBlockEvent = [this](i32 x, i32 y, i32 z, u8 paramA, u8 paramB, u32 blockStateId) {
        // 客户端收到方块事件后，查找方块并调用 Block::triggerEvent()
        // 参考 MC Java: ClientPacketListener.handleBlockEvent() -> ClientLevel.blockEvent()
        //
        // 当前 ClientWorld 不继承 IWorld（无法作为 IWorld& 传入 triggerEvent），
        // 且客户端尚无 BlockEntity 系统（getBlockEntity() 返回 nullptr），
        // 因此暂时仅记录事件。待以下两项完成后实现：
        //
        // 1. ClientWorld 继承 IWorld 或提供等效的 getBlockEntity() 接口
        // 2. 客户端 BlockEntity 渲染系统就绪（ChestEntity/EnderChestEntity/
        //    ShulkerBoxEntity 盖子动画、DecoratedPotBlockEntity 摇晃动画、
        //    EndGatewayEntity 冷却视觉效果、MobSpawnerBlockEntity 生成粒子、
        //    活塞伸缩动画等）
        //
        // 届时实现逻辑：
        //   BlockPos pos(x, y, z);
        //   const BlockState* state = m_world.getBlockState(x, y, z);
        //   if (state != nullptr) {
        //       state->getBlock().triggerEvent(*state, m_world, pos, paramA, paramB);
        //   }
        (void)x;
        (void)y;
        (void)z;
        (void)paramA;
        (void)paramB;
        (void)blockStateId;
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
        ClientEntity* entity = entityManager.spawnEntity(
            static_cast<EntityInstanceId>(entityId), mc::entity::EntityTypeKeys::EXPERIENCE_ORB);
        if (!entity) {
            entity = entityManager.getEntity(static_cast<EntityInstanceId>(entityId));
        }

        if (!entity) {
            return;
        }

        entity->setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
        entity->setXpValue(static_cast<i32>(xpValue));
    };

    // 粒子回调
    callbacks.onParticle = [this](::mc::particle::ParticleTypeId type,
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

    // 振动粒子回调（携带目标位置来源和到达时间）
    callbacks.onVibrationParticle = [this](f64 x,
                                        f64 y,
                                        f64 z,
                                        u8 targetKind,
                                        f64 targetX,
                                        f64 targetY,
                                        f64 targetZ,
                                        EntityInstanceId targetEntityId,
                                        f32 yOffset,
                                        i32 arrivalInTicks) {
        if (!m_world.particleManager()) {
            return;
        }

        glm::vec3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));

        // 通过粒子数据管线创建粒子
        // - 方块来源（targetKind=0）：目标位置已解析为方块中心坐标，直接传入固定 Vector3d
        // - 实体来源（targetKind=1）：持有实体 ID + yOffset，粒子在每 tick 中通过
        //   ClientWorld.entityManager().getEntity(id) 重新解析实体当前位置，
        //   因此当目标实体移动时粒子会持续跟随。实体消失时粒子立即过期。
        //   对应 MC Java VibrationSignalParticle.tick() 中
        //   Optional<Vec3> optional = this.target.getPosition(this.level);
        //   if (optional.isEmpty()) { this.remove(); }
        using namespace client::renderer::trident::particle;
        std::unique_ptr<data::VibrationParticleData> vibrationData;
        if (targetKind == 1) {
            // 实体来源：将实体 ID 和 Y 偏移直接传入数据，由粒子 tick 动态解析
            vibrationData = std::make_unique<data::VibrationParticleData>(targetEntityId, yOffset, arrivalInTicks);
        } else {
            // 方块来源：使用已解析的方块中心坐标
            vibrationData =
                std::make_unique<data::VibrationParticleData>(Vector3d(targetX, targetY, targetZ), arrivalInTicks);
        }
        m_world.particleManager()->addPendingParticle(
            particle::ParticleTypeId::Vibration, pos, glm::vec3(0.0f), &m_world, std::move(vibrationData));
    };

    // 轨迹粒子回调（携带目标位置、颜色和持续时间）
    callbacks.onTrailParticle =
        [this](f64 x, f64 y, f64 z, f64 targetX, f64 targetY, f64 targetZ, u32 color, i32 durationInTicks) {
            if (!m_world.particleManager()) {
                return;
            }

            // 创建轨迹粒子，从当前位置飞向目标位置
            glm::vec3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
            Vector3d targetPosition(targetX, targetY, targetZ);

            // 通过粒子数据管线创建粒子
            using namespace client::renderer::trident::particle;
            auto trailData = std::make_unique<data::TrailParticleData>(targetPosition, color, durationInTicks);
            m_world.particleManager()->addPendingParticle(
                particle::ParticleTypeId::Trail, pos, glm::vec3(0.0f), &m_world, std::move(trailData));
        };

    // 灰尘粒子回调（携带 ARGB 颜色和缩放）
    callbacks.onDustParticle = [this](::mc::particle::ParticleTypeId type,
                                   f64 x,
                                   f64 y,
                                   f64 z,
                                   f32 vx,
                                   f32 vy,
                                   f32 vz,
                                   f32 ox,
                                   f32 oy,
                                   f32 oz,
                                   u32 count,
                                   u32 color,
                                   f32 scale) {
        if (!m_world.particleManager()) {
            return;
        }

        // 通过粒子数据管线创建灰尘粒子
        using namespace client::renderer::trident::particle;
        glm::vec3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
        glm::vec3 velocity(vx, vy, vz);
        glm::vec3 offset(ox, oy, oz);

        auto dustData = std::make_unique<data::DustParticleData>(color, scale);
        for (u32 i = 0; i < count; ++i) {
            m_world.particleManager()->addPendingParticle(type, pos, velocity, &m_world, dustData->clone());
        }
    };

    // 颜色过渡灰尘粒子回调（携带起始颜色、目标颜色和缩放）
    callbacks.onDustColorTransitionParticle = [this](f64 x,
                                                  f64 y,
                                                  f64 z,
                                                  f32 vx,
                                                  f32 vy,
                                                  f32 vz,
                                                  f32 ox,
                                                  f32 oy,
                                                  f32 oz,
                                                  u32 count,
                                                  u32 fromColor,
                                                  u32 toColor,
                                                  f32 scale) {
        if (!m_world.particleManager()) {
            return;
        }

        // 通过粒子数据管线创建颜色过渡灰尘粒子
        using namespace client::renderer::trident::particle;
        glm::vec3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
        glm::vec3 velocity(vx, vy, vz);
        glm::vec3 offset(ox, oy, oz);

        auto transitionData = std::make_unique<data::DustColorTransitionParticleData>(fromColor, toColor, scale);
        for (u32 i = 0; i < count; ++i) {
            m_world.particleManager()->addPendingParticle(
                particle::ParticleTypeId::DustColorTransition, pos, velocity, &m_world, transitionData->clone());
        }
    };

    // 实体效果粒子回调（携带 ARGB 颜色）
    callbacks.onEntityEffectParticle =
        [this](f64 x, f64 y, f64 z, f32 vx, f32 vy, f32 vz, f32 ox, f32 oy, f32 oz, u32 count, u32 color) {
            if (!m_world.particleManager()) {
                return;
            }

            // 通过粒子数据管线创建实体效果粒子
            using namespace client::renderer::trident::particle;
            glm::vec3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
            glm::vec3 velocity(vx, vy, vz);
            glm::vec3 offset(ox, oy, oz);
            MC_UNUSED(offset);

            auto effectData = std::make_unique<data::EntityEffectParticleData>(color);
            for (u32 i = 0; i < count; ++i) {
                m_world.particleManager()->addPendingParticle(
                    particle::ParticleTypeId::EntityEffect, pos, velocity, &m_world, effectData->clone());
            }
        };

    // 方块粒子回调（携带方块状态 ID）
    // 用于旋风人地面粒子、长跳轨迹粒子等需要方块纹理的场景。
    // 通过 BlockRegistry 解析 stateId 回 BlockState，再调用 ClientWorld::addBlockParticle 生成。
    callbacks.onBlockParticle = [this](::mc::particle::ParticleTypeId type,
                                    f64 x,
                                    f64 y,
                                    f64 z,
                                    f32 vx,
                                    f32 vy,
                                    f32 vz,
                                    f32 ox,
                                    f32 oy,
                                    f32 oz,
                                    u32 count,
                                    u32 blockStateId) {
        MC_UNUSED(ox);
        MC_UNUSED(oy);
        MC_UNUSED(oz);
        MC_UNUSED(count);

        // 通过 BlockRegistry 解析方块状态 ID
        const auto* blockState = ::mc::BlockRegistry::instance().getBlockState(blockStateId);
        if (blockState == nullptr) {
            return;
        }

        // 调用 ClientWorld::addBlockParticle 生成方块粒子
        const ::mc::Vector3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
        const ::mc::Vector3 velocity(vx, vy, vz);
        m_world.addBlockParticle(type, pos, velocity, *blockState);
    };

    // 物品粒子回调（携带物品堆）
    // 用于 Item/ItemSlime/ItemCobweb/ItemSnowball 等粒子。
    // 通过粒子数据管线创建物品粒子：将 ItemStack 包装为 ItemParticleData，
    // 由 ParticleRegistry 的数据工厂调用 ItemParticle::createWithItemStack 创建粒子。
    callbacks.onItemParticle = [this](::mc::particle::ParticleTypeId type,
                                   f64 x,
                                   f64 y,
                                   f64 z,
                                   f32 vx,
                                   f32 vy,
                                   f32 vz,
                                   f32 ox,
                                   f32 oy,
                                   f32 oz,
                                   u32 count,
                                   const ::mc::ItemStack& itemStack) {
        if (!m_world.particleManager()) {
            return;
        }

        // 通过粒子数据管线创建物品粒子
        using namespace client::renderer::trident::particle;
        glm::vec3 pos(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
        glm::vec3 velocity(vx, vy, vz);
        glm::vec3 offset(ox, oy, oz);
        MC_UNUSED(offset);

        auto itemData = std::make_unique<data::ItemParticleData>(type, itemStack);
        for (u32 i = 0; i < count; ++i) {
            m_world.particleManager()->addPendingParticle(type, pos, velocity, &m_world, itemData->clone());
        }
    };

    // 玩家列表回调 - 皮肤系统集成
    callbacks.onPlayerListAdd = [this](const std::vector<::mc::skin::PlayerListEntry>& entries) {
        if (!m_skinManager) {
            return;
        }

        for (const auto& entry : entries) {
            // 登记 UUID↔username（与已注册的网络玩家实体关联，供渲染层按 entityId 查 UUID）
            m_identityRegistry.registerPlayerListUuid(entry.uuid, entry.name);

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
            // 联动清理：皮肤信息（含图集动态区域，见 ClientSkinManager::removePlayerInfo）+ 身份登记
            m_skinManager->removePlayerInfo(uuid);
            m_identityRegistry.removeByUuid(uuid);
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

        const EntityInstanceId localPlayerEntityId = m_localIdentity.entityId();
        const EntityInstanceId vehicleEntityId = static_cast<EntityInstanceId>(entityId);

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
                ClientEntity* oldPassenger =
                    m_world.entityManager().getEntity(static_cast<EntityInstanceId>(oldPassengerId));
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
            ClientEntity* passenger = m_world.entityManager().getEntity(static_cast<EntityInstanceId>(passengerId));
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

    // ========== 旁观者摄像机事件 ==========

    callbacks.onSetCamera = [this](u32 cameraEntityId) {
        // 设置客户端的摄像机跟踪目标实体
        // cameraEntityId 为本地玩家自身 ID 时表示恢复正常视角
        const EntityInstanceId localPlayerEntityId = m_localIdentity.entityId();

        if (cameraEntityId == static_cast<u32>(localPlayerEntityId)) {
            // 恢复自身视角
            if (m_player) {
                m_player->setCameraEntityId(std::nullopt);
            }
            spdlog::info("SetCamera: reset to self (entityId={})", cameraEntityId);
        } else {
            // 跟踪目标实体
            if (m_player) {
                m_player->setCameraEntityId(static_cast<EntityInstanceId>(cameraEntityId));
            }
            spdlog::info("SetCamera: spectating entity {}", cameraEntityId);
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
                              bool keepData,
                              std::optional<GlobalPos> lastDeathLocation) {
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

            // 12. 更新玩家的上次死亡位置（从服务端同步）
            m_player->setLastDeathLocation(std::move(lastDeathLocation));
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
        const EntityInstanceId localPlayerEntityId = m_localIdentity.entityId();
        ClientEntity* localPlayer = m_world.entityManager().getEntity(localPlayerEntityId);
        if (!localPlayer) {
            return;
        }

        EntityInstanceId vehicleId = localPlayer->vehicleId();
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
        const EntityInstanceId eid = static_cast<EntityInstanceId>(entityId);

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
            // 对应 MC ClientLevel.addDestroyBlockEffect / LevelEventHandler case 2001
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

                // 生成方块破碎粒子
                // 算法对齐 MC ClientLevel.addDestroyBlockEffect：
                // 获取方块的形状，对每个AABB按0.25格间距均匀分布粒子
                // 标准完整方块(1x1x1)生成 4x4x4 = 64 个粒子
                const auto& shape = blockState->getShape();
                for (const auto& box : shape.boxes()) {
                    const f32 d1 = std::min(1.0f, box.maxX - box.minX); // AABB宽度
                    const f32 d2 = std::min(1.0f, box.maxY - box.minY); // AABB高度
                    const f32 d3 = std::min(1.0f, box.maxZ - box.minZ); // AABB深度

                    const i32 countX = std::max(2, static_cast<i32>(std::ceil(d1 / 0.25)));
                    const i32 countY = std::max(2, static_cast<i32>(std::ceil(d2 / 0.25)));
                    const i32 countZ = std::max(2, static_cast<i32>(std::ceil(d3 / 0.25)));

                    for (i32 ix = 0; ix < countX; ++ix) {
                        for (i32 iy = 0; iy < countY; ++iy) {
                            for (i32 iz = 0; iz < countZ; ++iz) {
                                // 归一化位置（0~1），位于网格单元中心
                                const f32 nx = (static_cast<f32>(ix) + 0.5f) / static_cast<f32>(countX);
                                const f32 ny = (static_cast<f32>(iy) + 0.5f) / static_cast<f32>(countY);
                                const f32 nz = (static_cast<f32>(iz) + 0.5f) / static_cast<f32>(countZ);

                                // 粒子世界位置：AABB内偏移 + 方块位置
                                const f32 particleX = px + nx * d1 + box.minX;
                                const f32 particleY = py + ny * d2 + box.minY;
                                const f32 particleZ = pz + nz * d3 + box.minZ;

                                // 粒子速度：从中心向外扩散
                                const f32 vx = nx - 0.5f;
                                const f32 vy = ny - 0.5f;
                                const f32 vz = nz - 0.5f;

                                m_world.addBlockParticle(ParticleTypeId::Breaking,
                                    Vector3(particleX, particleY, particleZ),
                                    Vector3(vx, vy, vz),
                                    *blockState);
                            }
                        }
                    }
                }
            }
            break;
        }

        case WorldEvents::DISPENSER_SMOKE: {
            // 发射器烟雾粒子，data 为方向（Direction.getIndex()）
            {
                Direction dir = static_cast<Direction>(data);
                i32 stepX = Directions::xOffset(dir);
                i32 stepY = Directions::yOffset(dir);
                i32 stepZ = Directions::zOffset(dir);
                for (int i = 0; i < 10; ++i) {
                    f32 speed = static_cast<f32>(random.nextDouble() * 0.2 + 0.01);
                    f32 spx = static_cast<f32>(x) + static_cast<f32>(stepX) * 0.6f + 0.5f +
                        static_cast<f32>(stepX) * 0.01f +
                        static_cast<f32>(random.nextFloat() - 0.5f) * static_cast<f32>(stepZ) * 0.5f;
                    f32 spy = static_cast<f32>(y) + static_cast<f32>(stepY) * 0.6f + 0.5f +
                        static_cast<f32>(stepY) * 0.01f +
                        static_cast<f32>(random.nextFloat() - 0.5f) * static_cast<f32>(stepY) * 0.5f;
                    f32 spz = static_cast<f32>(z) + static_cast<f32>(stepZ) * 0.6f + 0.5f +
                        static_cast<f32>(stepZ) * 0.01f +
                        static_cast<f32>(random.nextFloat() - 0.5f) * static_cast<f32>(stepX) * 0.5f;
                    f32 svx = static_cast<f32>(stepX) * speed + static_cast<f32>(random.nextGaussian()) * 0.01f;
                    f32 svy = static_cast<f32>(stepY) * speed + static_cast<f32>(random.nextGaussian()) * 0.01f;
                    f32 svz = static_cast<f32>(stepZ) * speed + static_cast<f32>(random.nextGaussian()) * 0.01f;
                    m_world.addParticle(ParticleTypeId::Smoke, Vector3(spx, spy, spz), Vector3(svx, svy, svz));
                }
            }
            break;
        }

        case WorldEvents::MOB_SPAWNER_PARTICLES: {
            // 刷怪笼成功生成实体时爆发烟雾和火焰粒子
            // 客户端在方块中心2格范围内随机生成20个烟雾粒子和20个火焰粒子
            {
                f32 cx = static_cast<f32>(x) + 0.5f;
                f32 cy = static_cast<f32>(y) + 0.5f;
                f32 cz = static_cast<f32>(z) + 0.5f;
                for (i32 i = 0; i < 20; ++i) {
                    f32 spx = cx + (random.nextFloat() - 0.5f) * 2.0f;
                    f32 spy = cy + (random.nextFloat() - 0.5f) * 2.0f;
                    f32 spz = cz + (random.nextFloat() - 0.5f) * 2.0f;
                    m_world.addParticle(ParticleTypeId::Smoke, Vector3(spx, spy, spz), Vector3(0.0f, 0.0f, 0.0f));
                    m_world.addParticle(ParticleTypeId::Flame, Vector3(spx, spy, spz), Vector3(0.0f, 0.0f, 0.0f));
                }
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

        case WorldEvents::PLANT_GROWTH_EFFECT: {
            // 植物生长粒子与音效事件（由骨粉使用触发）
            // data 为粒子数量，0 则生成 15 个
            // 与 BONEMEAL_PARTICLES(2005) 的区别：1505 根据 IGrowable::getBoneMealType()
            // 决定粒子分布方式，同时播放骨粉使用音效。
            {
                i32 count = (data == 0) ? 15 : data;

                // 查询位置处的方块是否为 IGrowable，根据骨粉类型决定粒子分布
                const BlockState* blockState = m_world.getBlockState(x, y, z);
                if (blockState != nullptr) {
                    const Block& block = blockState->owner();
                    const IGrowable* growable = dynamic_cast<const IGrowable*>(&block);

                    if (growable != nullptr) {
                        // 获取粒子生成位置（NEIGHBOR_SPREADER 类型在方块上方，GROWER 类型在方块自身）
                        BlockPos particlePos = growable->getParticlePos(BlockPos(x, y, z));

                        switch (growable->getBoneMealType()) {
                            case IGrowable::BoneMealType::NEIGHBOR_SPREADER:
                                // 邻居传播型（草方块、菌岩等）：粒子水平扩散 3 倍数量
                                m_world.addParticle(ParticleTypeId::HappyVillager,
                                    Vector3(static_cast<f32>(particlePos.x) + 0.5f,
                                        static_cast<f32>(particlePos.y),
                                        static_cast<f32>(particlePos.z) + 0.5f),
                                    Vector3(0.0f, 0.0f, 0.0f),
                                    Vector3(3.0f, 1.0f, 3.0f),
                                    static_cast<u32>(count * 3));
                                break;

                            case IGrowable::BoneMealType::GROWER:
                            default:
                                // 自身成长型（作物、树苗等）：粒子在方块形状高度内生成
                                // 使用方块碰撞箱的 Y 轴最大值作为垂直范围
                                {
                                    // 获取方块的碰撞形状高度
                                    f32 shapeHeight = 1.0f;
                                    const CollisionShape& shape = blockState->getShape();
                                    if (!shape.isEmpty() && !shape.isFullBlock()) {
                                        // 获取碰撞箱的最大 Y 值
                                        for (const auto& box : shape.boxes()) {
                                            if (box.maxY > shapeHeight) {
                                                shapeHeight = box.maxY;
                                            }
                                        }
                                    }
                                    m_world.addParticle(ParticleTypeId::HappyVillager,
                                        Vector3(static_cast<f32>(particlePos.x) + 0.5f,
                                            static_cast<f32>(particlePos.y),
                                            static_cast<f32>(particlePos.z) + 0.5f),
                                        Vector3(0.0f, 0.0f, 0.0f),
                                        Vector3(0.5f, shapeHeight, 0.5f),
                                        static_cast<u32>(count));
                                }
                                break;
                        }
                    } else {
                        // 非 IGrowable 方块（如水面）：使用与 NEIGHBOR_SPREADER 相同的分布
                        m_world.addParticle(ParticleTypeId::HappyVillager,
                            Vector3(px, py, pz),
                            Vector3(0.0f, 0.0f, 0.0f),
                            Vector3(3.0f, 1.0f, 3.0f),
                            static_cast<u32>(count * 3));
                    }
                } else {
                    // 方块状态不可用时，使用默认的 GROWER 分布
                    m_world.addParticle(ParticleTypeId::HappyVillager,
                        Vector3(px, py, pz),
                        Vector3(0.0f, 0.0f, 0.0f),
                        Vector3(0.5f, 1.0f, 0.5f),
                        static_cast<u32>(count));
                }

                // 播放骨粉使用音效
                if (m_audioService) {
                    m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                        SoundEvents::ITEM_BONE_MEAL_USE, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
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
            // 使用 DustPillar 粒子携带方块状态纹理，分两层分布：
            //   - 内层簇（count/3 个）：高斯分布聚集在中心
            //   - 外层环（count/1.5 个）：半径 3.5 的圆形均匀分布
            {
                // 获取冲击位置方块的状态，用于 DustPillar 粒子纹理
                const BlockState* blockState = m_world.getBlockState(x, y, z);
                if (blockState == nullptr || blockState->isAir()) {
                    // 方块状态不可用或为空气，跳过粒子生成
                    break;
                }

                // 中心点位于方块中心偏上 0.5 格
                f32 cx = static_cast<f32>(x) + 0.5f;
                f32 cy = static_cast<f32>(y) + 1.0f;
                f32 cz = static_cast<f32>(z) + 0.5f;

                i32 count = (data == 0) ? 750 : data;

                // 内层簇：count/3 个粒子，高斯分布在中心附近
                // 速度会被 DustPillarProvider 覆盖：X/Z → gaussian/30，Y → 传入Y + gaussian/2
                i32 innerCount = static_cast<i32>(count / 3.0f);
                for (i32 i = 0; i < innerCount; ++i) {
                    f32 px = cx + static_cast<f32>(random.nextGaussian()) / 2.0f;
                    f32 py = cy;
                    f32 pz = cz + static_cast<f32>(random.nextGaussian()) / 2.0f;
                    f32 vx = static_cast<f32>(random.nextGaussian()) * 0.2f;
                    f32 vy = static_cast<f32>(random.nextGaussian()) * 0.2f;
                    f32 vz = static_cast<f32>(random.nextGaussian()) * 0.2f;
                    m_world.addBlockParticle(
                        ParticleTypeId::DustPillar, Vector3(px, py, pz), Vector3(vx, vy, vz), *blockState);
                }

                // 外层环：count/1.5 个粒子，半径 3.5 的均匀圆形分布
                i32 outerCount = static_cast<i32>(count / 1.5f);
                for (i32 j = 0; j < outerCount; ++j) {
                    f32 angle = static_cast<f32>(j) * math::TWO_PI / static_cast<f32>(outerCount);
                    f32 px = cx + 3.5f * std::cos(angle) + static_cast<f32>(random.nextGaussian()) / 2.0f;
                    f32 py = cy;
                    f32 pz = cz + 3.5f * std::sin(angle) + static_cast<f32>(random.nextGaussian()) / 2.0f;
                    f32 vx = static_cast<f32>(random.nextGaussian()) * 0.05f;
                    f32 vy = static_cast<f32>(random.nextGaussian()) * 0.05f;
                    f32 vz = static_cast<f32>(random.nextGaussian()) * 0.05f;
                    m_world.addBlockParticle(
                        ParticleTypeId::DustPillar, Vector3(px, py, pz), Vector3(vx, vy, vz), *blockState);
                }
            }
            break;
        }

        case WorldEvents::SHOOT_WHITE_SMOKE: {
            // 白烟粒子效果（方向性），与 DISPENSER_SMOKE(2000) 类似但为白色烟雾
            // data 为烟雾方向（Direction.getIndex()）
            {
                Direction dir = static_cast<Direction>(data);
                i32 stepX = Directions::xOffset(dir);
                i32 stepY = Directions::yOffset(dir);
                i32 stepZ = Directions::zOffset(dir);
                for (int i = 0; i < 10; ++i) {
                    f32 speed = static_cast<f32>(random.nextDouble() * 0.2 + 0.01);
                    f32 spx = static_cast<f32>(x) + static_cast<f32>(stepX) * 0.6f + 0.5f +
                        static_cast<f32>(stepX) * 0.01f +
                        static_cast<f32>(random.nextFloat() - 0.5f) * static_cast<f32>(stepZ) * 0.5f;
                    f32 spy = static_cast<f32>(y) + static_cast<f32>(stepY) * 0.6f + 0.5f +
                        static_cast<f32>(stepY) * 0.01f +
                        static_cast<f32>(random.nextFloat() - 0.5f) * static_cast<f32>(stepY) * 0.5f;
                    f32 spz = static_cast<f32>(z) + static_cast<f32>(stepZ) * 0.6f + 0.5f +
                        static_cast<f32>(stepZ) * 0.01f +
                        static_cast<f32>(random.nextFloat() - 0.5f) * static_cast<f32>(stepX) * 0.5f;
                    f32 svx = static_cast<f32>(stepX) * speed + static_cast<f32>(random.nextGaussian()) * 0.01f;
                    f32 svy = static_cast<f32>(stepY) * speed + static_cast<f32>(random.nextGaussian()) * 0.01f;
                    f32 svz = static_cast<f32>(stepZ) * speed + static_cast<f32>(random.nextGaussian()) * 0.01f;
                    m_world.addParticle(ParticleTypeId::WhiteSmoke, Vector3(spx, spy, spz), Vector3(svx, svy, svz));
                }
            }
            break;
        }

        case WorldEvents::PLANT_GROWTH_PARTICLES: {
            // 植物生长粒子效果（蜜蜂授粉促进作物生长时触发）
            // data 为粒子数量（通常为 15），0 则生成 15 个
            // 与 BONEMEAL_PARTICLES(2005) 的区别：不播放骨粉使用音效
            {
                i32 count = (data == 0) ? 15 : data;
                m_world.addParticle(ParticleTypeId::HappyVillager,
                    Vector3(px, py, pz),
                    Vector3(0.0f, 0.0f, 0.0f),
                    Vector3(0.5f, 1.0f, 0.5f),
                    static_cast<u32>(count));
            }
            break;
        }

        case WorldEvents::TURTLE_EGG_PLACEMENT: {
            // 海龟蛋放置粒子效果
            // 与 PLANT_GROWTH_PARTICLES(2011) 逻辑相同，均为 HappyVillager 粒子
            {
                i32 count = (data == 0) ? 15 : data;
                m_world.addParticle(ParticleTypeId::HappyVillager,
                    Vector3(px, py, pz),
                    Vector3(0.0f, 0.0f, 0.0f),
                    Vector3(0.5f, 1.0f, 0.5f),
                    static_cast<u32>(count));
            }
            break;
        }

        default:
            // 未知事件ID，忽略
            break;
    }
}

} // namespace mc::client
