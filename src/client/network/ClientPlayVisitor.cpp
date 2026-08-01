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

#include "client/network/ClientPlayVisitor.hpp"

#include "client/application/ClientApplication.hpp"
#include "client/command/ClientCommandManager.hpp"
#include "client/network/ClientNetwork.hpp"
#include "client/renderer/trident/block/BreakProgressManager.hpp"
#include "client/renderer/trident/chunk/ChunkRenderer.hpp"
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/particle/ParticleManager.hpp"
#include "client/renderer/trident/particle/ParticleRegistry.hpp"
#include "client/renderer/trident/particle/data/BlockParticleData.hpp"
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
#include "common/item/component/DataComponentMap.hpp"
#include "common/network/backend/java/mappings/JavaItemIdMap.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/network/ir/packets/configuration/ConfigurationPackets.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/EntityEvents.hpp" // EntityAnimation / EntityStatus 枚举（客户端入站动画/状态字节分流用）
#include "common/network/protocol/TitleActions.hpp" // TitleAction 枚举（TitleWidget::handleTitlePacket 用）
#include "common/resource/ResourceLocation.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/network/SkinPackets.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include "common/world/blockentity/processing/FurnaceInventory.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/menu/CraftingMenu.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <optional>
#include <variant>

namespace mc::client::net {

namespace {

using namespace mc::trace;
namespace irplay = mc::network::ir::play;

/// packed degrees(byte) → float 角度，对齐 1.21.11 ClientboundMoveEntity 旋转编码
constexpr f32 unpackDegrees(i8 packed) noexcept
{
    return static_cast<f32>(packed) * (360.0f / 256.0f);
}

/// 由整数实体类型 ID 查回 typeId 字符串（AddEntity 解码用）
const std::string& entityTypeIdString(i32 numericId)
{
    static const std::string kEmpty;
    const auto* type = mc::entity::EntityRegistry::instance().getTypeById(static_cast<u32>(numericId));
    if (type == nullptr) {
        return kEmpty;
    }
    return type->name();
}

/// 维度 ResourceKey（如 "minecraft:overworld"）→ DimensionId。
/// 1.21.11 Login/Respawn 的 CommonPlayerSpawnInfo.dimension 是字符串 ResourceKey，
/// 客户端 ClientDimensionManager 不提供 name→id 反查，故在此内联映射。
/// 未知 key 默认主世界（与 DimensionManager::getDimensionIdByName 语义一致）。
DimensionId dimensionKeyToId(const std::string& key) noexcept
{
    if (key == "minecraft:overworld") {
        return DimensionManager::OVERWORLD;
    }
    if (key == "minecraft:the_nether") {
        return DimensionManager::NETHER;
    }
    if (key == "minecraft:the_end") {
        return DimensionManager::THE_END;
    }
    return DimensionManager::OVERWORLD;
}

/// ItemStack → 1.21.11 HashedStack（降级：仅 itemId+count，组件哈希 patch 双端写空）。
/// 空栈 present=false。出站 ContainerClick 的 carriedItem 用。
/// 完整 1.21.11 HashedStack 需 HashedPatchMap（added/removed 组件哈希），依赖组件哈希体系
/// （DataComponentPatch→哈希），与 PlayPackets.hpp:293 的 IR 降级约定一致，待组件哈希落地后统一升级。
irplay::HashedStack toHashedStack(const ItemStack& stack)
{
    irplay::HashedStack hs{};
    if (stack.isEmpty() || stack.getItem() == nullptr) {
        hs.present = false;
        hs.itemId = 0;
        hs.count = 0;
        return hs;
    }
    hs.present = true;
    // HashedStack.itemId 与 ItemStackView.itemId 同为 wire 上的 vanilla registry id（见
    // ItemStackBridge），边界处由 JavaItemIdMap 翻译，业务侧零感知（贯彻 IR 思想）。
    hs.itemId = ::mc::network::backend::java::JavaItemIdMap::instance().toJavaRegistryId(*stack.getItem());
    hs.count = stack.getCount();
    return hs;
}

/// 容器内容应用：把 items 写入 menu 对应槽位
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

/// 把 ItemStackView 列表还原为 ItemStack 列表（IR→业务侧转换，失败项落为空栈）
std::vector<ItemStack> viewsToStacks(const std::vector<mc::network::ir::play::ItemStackView>& views)
{
    std::vector<ItemStack> out;
    out.reserve(views.size());
    for (const auto& view : views) {
        auto r = mc::network::ir::fromItemStackView(view);
        out.push_back(r.failed() ? ItemStack{} : std::move(r.value()));
    }
    return out;
}

/// PlaySound/SoundEntity 的 soundHolder 是结构化 Holder<SoundEvent>（对齐 vanilla 1.21.11 wire）。
/// 内联模式（direct=true）：identifier 即声音 id，解析为 ResourceLocation。
/// 引用模式（direct=false）：本项目无 sound registry 整数 id 表，无法还原，返回 nullopt 静默丢弃。
[[nodiscard]] std::optional<ResourceLocation> parseSoundHolder(const irplay::SoundEventHolder& holder)
{
    if (!holder.direct) {
        return std::nullopt; // 引用模式无资源可查
    }
    auto rl = ResourceLocation::parse(holder.identifier);
    if (!rl.isValid()) {
        return std::nullopt;
    }
    return rl;
}

/// IR source(i32) → SoundCategory（与服务端 static_cast<i32>(category) 对称），越界返回 nullopt。
[[nodiscard]] std::optional<mc::sound::SoundCategory> sourceToCategory(i32 source)
{
    if (source < 0 || source >= static_cast<i32>(mc::sound::SoundCategory::Count)) {
        return std::nullopt;
    }
    return static_cast<mc::sound::SoundCategory>(source);
}

} // namespace

// ============================================================================
// ClientPlayVisitor 私有方法（friend 访问 ClientApplication 私有成员）
// ============================================================================

ui::minecraft::widgets::ScreenStackWidget* ClientPlayVisitor::getScreenStack()
{
    if (!m_app.m_kageroEngine) {
        return nullptr;
    }
    return static_cast<ui::minecraft::widgets::ScreenStackWidget*>(
        m_app.m_kageroEngine->getLayer(m_app.m_screenStackLayerId));
}

std::function<void(ContainerId, i32, i32, i16, ClickAction, const ItemStack&)>
ClientPlayVisitor::makeContainerClickSender()
{
    return [this](ContainerId containerId,
               i32 slotIndex,
               i32 button,
               i16 transactionId,
               ClickAction action,
               const ItemStack& cursorItem) {
        irplay::ContainerClick pkt;
        pkt.containerId = containerId;
        pkt.stateId = 0;
        pkt.slotNum = slotIndex;
        pkt.buttonNum = button;
        pkt.clickType = static_cast<i32>(action);
        // carriedItem 降级为 itemId+count（与 toHashedStack 的 HashedPatchMap 缺省一致）。
        pkt.carriedItem = toHashedStack(cursorItem);
        (void)transactionId;
        if (m_app.m_network) {
            (void)m_app.m_network->send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{irplay::ContainerClick{pkt}}});
        }
    };
}

std::function<void(ContainerId)> ClientPlayVisitor::makeContainerCloseSender()
{
    return [this](ContainerId containerId) {
        irplay::ContainerClose pkt;
        pkt.containerId = containerId;
        if (m_app.m_network) {
            (void)m_app.m_network->send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{irplay::ContainerClose{pkt}}});
        }
    };
}

// ============================================================================
// Configuration 阶段
// ============================================================================

Result<void> ClientPlayVisitor::handleConfiguration(const mc::network::ir::IrPacket& packet)
{
    MC_ASSERT_RELEASE(packet.phase == mc::network::protocol::ConnectionProtocol::Configuration);
    const auto* cfg = std::get_if<mc::network::ir::ConfigurationPacket>(&packet.packet);
    if (cfg == nullptr) {
        return Error(
            ErrorCode::InvalidData, "Configuration phase variant missing", "ClientPlayVisitor::handleConfiguration");
    }

    // Configuration 阶段包由 ClientNetwork 内部状态机处理（SelectKnownPacks/FinishConfiguration
    // 回包、RegistryData/UpdateTags/UpdateEnabledFeatures 忽略），余下入站包在此 visitor 处理：
    // KeepAlive/Ping 回对称包、CustomPayload(brand) 存储、Disconnect 记日志。
    // 注：ClientInformation 是 C→S 出站包（客户端设置），属 ClientNetwork 出站职责，不在此。
    return std::visit(
        [this](const auto& cfg) -> Result<void> {
            using C = std::decay_t<decltype(cfg)>;
            if constexpr (std::is_same_v<C, mc::network::ir::configuration::KeepAlive>) {
                // S→C KeepAlive：回 C→S KeepAlive 同 id（心跳对称）。
                if (m_app.m_network) {
                    (void)m_app.m_network->send(
                        mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Configuration,
                            mc::network::ir::ConfigurationPacket{mc::network::ir::configuration::KeepAlive{cfg.id}}});
                }
                return Result<void>::ok();
            } else if constexpr (std::is_same_v<C, mc::network::ir::configuration::Ping>) {
                // S→C Ping：回 C→S Pong（复用 Ping struct 同 parameter）。
                if (m_app.m_network) {
                    (void)m_app.m_network->send(
                        mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Configuration,
                            mc::network::ir::ConfigurationPacket{mc::network::ir::configuration::Ping{cfg.parameter}}});
                }
                return Result<void>::ok();
            } else if constexpr (std::is_same_v<C, mc::network::ir::configuration::CustomPayload>) {
                // S→C CustomPayload：minecraft:brand 存 m_serverBrand；其余 payload 暂不消费。
                if (cfg.identifier == "minecraft:brand") {
                    m_app.m_serverBrand = std::string(cfg.payload.begin(), cfg.payload.end());
                }
                return Result<void>::ok();
            } else if constexpr (std::is_same_v<C, mc::network::ir::configuration::Disconnect>) {
                spdlog::warn("[Configuration] Disconnected by server: {}", cfg.reason);
                return Result<void>::ok();
            } else {
                // RegistryData/UpdateTags/UpdateEnabledFeatures/ClientInformation(不应收到)：忽略。
                return Result<void>::ok();
            }
        },
        *cfg);
}

// ============================================================================
// Play 阶段
// ============================================================================

Result<void> ClientPlayVisitor::handle(const mc::network::ir::IrPacket& packet)
{
    MC_ASSERT_RELEASE(packet.phase == mc::network::protocol::ConnectionProtocol::Play);
    const auto* play = std::get_if<mc::network::ir::PlayPacket>(&packet.packet);
    if (play == nullptr) {
        return Error(ErrorCode::InvalidData, "Play phase variant missing", "ClientPlayVisitor::handle");
    }

    return std::visit(
        [this](const auto& pkt) -> Result<void> {
            using T = std::decay_t<decltype(pkt)>;
            (void)pkt;

            // ---- 登录 / 连接 ----
            if constexpr (std::is_same_v<T, irplay::Login>) {
                // play::Login 触发本地玩家生成（旧 onLoginSuccess 等价）。
                // playerId 来自包；entityId/uuid/username 由 ClientNetwork::onLoginReady 在此之前
                // 已注入 ClientApplication（m_localIdentity/m_network->uuid()/username()）。
                const auto& p = pkt;
                const i32 playerId = p.playerId;
                const auto& uuid = m_app.m_network ? m_app.m_network->uuid() : std::array<u8, 16>{};
                const std::string username = m_app.m_network ? m_app.m_network->username() : std::string{};
                // entityId：1.21.11 Login 不携带，沿用 playerId 作为本地玩家 entityId
                // （与旧 EntityTracker 约定一致：远程玩家 playerId 即 entityId）。
                const EntityInstanceId entityId = static_cast<EntityInstanceId>(playerId);

                spdlog::info("Login successful: playerId={}, entityId={}, username={}", playerId, entityId, username);

                m_app.m_localIdentity.setIdentity(playerId, entityId);
                m_app.m_identityRegistry.registerLocalPlayer(entityId, playerId, uuid, username);

                auto& entityManager = m_app.m_world.entityManager();
                ClientEntity* playerEntity = entityManager.spawnLocalPlayer(entityId, playerId, username);
                if (playerEntity) {
                    spdlog::info("Local player entity created: entityId={}", entityId);
                }

                m_app.m_predictor = std::make_unique<ClientPlayerPredictor>();

                if (m_app.m_player) {
                    m_app.m_player->setPlayerId(playerId);
                }
                m_app.m_knownPlayerNames[playerId] = username;
                return Result<void>::ok();
            }
            // ---- 断开 ----
            else if constexpr (std::is_same_v<T, irplay::Disconnect>) {
                const auto& p = pkt;
                const std::string reason(p.reason.begin(), p.reason.end());
                spdlog::info("Disconnected from server: {}", reason);

                if (m_app.m_stateMachine.isLeavingWorld()) {
                    spdlog::info("[Network] Disconnection during world leave - continuing to main menu");
                    return Result<void>::ok();
                }
                if (m_app.m_stateMachine.isShuttingDown()) {
                    spdlog::info("[Network] Disconnection during shutdown - ignoring");
                    return Result<void>::ok();
                }

                spdlog::error("[Network] Unexpected disconnection - returning to main menu");
                m_app.m_localIdentity.clear();
                m_app.m_identityRegistry.clear();
                m_app.m_world.entityManager().clearLocalPlayer();
                m_app.m_predictor.reset();
                m_app.m_knownPlayerNames.clear();
                if (m_app.m_commandManager) {
                    m_app.m_commandManager->clear();
                }
                m_app.m_hasServerTimeSync = false;
                m_app.destroyGameSession();
                m_app.m_stateMachine.forceState(ClientAppState::MainMenu);
                m_app.showMainMenu();
                return Result<void>::ok();
            }
            // ---- 命令树 ----
            else if constexpr (std::is_same_v<T, irplay::Commands>) {
                // 1.21.11 Commands 是 opaque Node 树，需完整解析为 treeJson。我方命令系统本地驱动、
                // 不依赖服务端下发，故此处仅 warn 跳过。真 Java 互通的 Node 树 codec 属独立子项。
                spdlog::warn("[ClientPlayVisitor] Commands packet received; Node-tree parsing not implemented (local "
                             "command system)");
                return Result<void>::ok();
            }
            // ---- 玩家传送 ----
            else if constexpr (std::is_same_v<T, irplay::PlayerPosition>) {
                const auto& p = pkt;
                if (!m_app.m_player) {
                    return Result<void>::ok();
                }
                // 1.21.11：服务端发起传送（PlayerPosition/PlayerPosRot），客户端须回
                // AcceptTeleportation{teleportId} 确认，否则服务端会重发或判定未确认。
                if (m_app.m_network) {
                    (void)m_app.m_network->send(
                        mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
                            mc::network::ir::PlayPacket{irplay::AcceptTeleportation{p.teleportId}}});
                }
                if (m_app.m_predictor) {
                    m_app.m_predictor->reset(
                        Vector3(static_cast<f32>(p.x), static_cast<f32>(p.y), static_cast<f32>(p.z)), p.yRot, p.xRot);
                }
                m_app.m_player->setPosition(static_cast<f32>(p.x), static_cast<f32>(p.y), static_cast<f32>(p.z));
                m_app.m_player->setRotation(p.yRot, p.xRot);
                m_app.m_lastSentX = static_cast<f32>(p.x);
                m_app.m_lastSentY = static_cast<f32>(p.y);
                m_app.m_lastSentZ = static_cast<f32>(p.z);
                m_app.m_lastSentYaw = p.yRot;
                m_app.m_lastSentPitch = p.xRot;
                return Result<void>::ok();
            }
            // ---- 区块数据 ----
            else if constexpr (std::is_same_v<T, irplay::LevelChunkWithLight>) {
                const auto& p = pkt;
                // IR 结构体经 LocalTransport 零拷贝直传（本地客户端）或经 codec 解码（远程）。
                // ClientWorld 内部调 readLevelChunkWithLightIR 还原 ChunkData，异步下沉 worker。
                m_app.m_world.onLevelChunkWithLight(p, m_app.m_dimensionManager.currentDimension());
                return Result<void>::ok();
            }
            // 1.21.11 已删 UnloadChunk 包：区块卸载由客户端按距离自行管理
            // （ClientWorld::onChunkUnload 已就绪，待接距离判据），无入站信令。
            // ---- 方块更新 ----
            else if constexpr (std::is_same_v<T, irplay::BlockUpdate>) {
                const auto& p = pkt;
                const BlockPos pos = BlockPos::fromLong(p.blockPosPacked);
                m_app.m_world.setBlockState(
                    pos.x, pos.y, pos.z, BlockRegistry::instance().getBlockState(p.blockStateId));
                return Result<void>::ok();
            }
            // ---- 时间更新 ----
            else if constexpr (std::is_same_v<T, irplay::SetTime>) {
                const auto& p = pkt;
                m_app.m_world.onTimeUpdate(p.gameTime, p.dayTime, p.tickDayTime);
                return Result<void>::ok();
            }
            // ---- 玩家能力 ----
            else if constexpr (std::is_same_v<T, irplay::PlayerAbilities>) {
                const auto& p = pkt;
                if (m_app.m_player) {
                    PlayerAbilities& abilities = m_app.m_player->abilities();
                    abilities.invulnerable = (p.flags & 0x01) != 0;
                    abilities.flying = (p.flags & 0x02) != 0;
                    abilities.canFly = (p.flags & 0x04) != 0;
                    abilities.creativeMode = (p.flags & 0x08) != 0;
                    abilities.flySpeed = p.flyingSpeed;
                    abilities.walkSpeed = p.walkingSpeed;
                }
                m_app.closeInventoryScreenIfModeMismatch();
                return Result<void>::ok();
            }
            // ---- 主手槽同步 ----
            else if constexpr (std::is_same_v<T, irplay::SetHeldSlot>) {
                const auto& p = pkt;
                if (m_app.m_player) {
                    m_app.m_player->inventory().setSelectedSlot(p.slot);
                }
                return Result<void>::ok();
            }
            // ---- 出生点 ----
            else if constexpr (std::is_same_v<T, irplay::SetDefaultSpawnPosition>) {
                const auto& p = pkt;
                const BlockPos pos = BlockPos::fromLong(p.blockPosPacked);
                spdlog::info("Received SpawnPosition: ({}, {}, {}), angle={:.1f}", pos.x, pos.y, pos.z, p.yaw);
                m_app.m_world.setSpawnPoint(pos.x, pos.y, pos.z, p.yaw);
                return Result<void>::ok();
            }
            // ---- 难度 ----
            else if constexpr (std::is_same_v<T, irplay::ChangeDifficulty>) {
                const auto& p = pkt;
                const Difficulty difficulty = static_cast<Difficulty>(p.difficulty);
                spdlog::info("[Client] Difficulty changed to: {}, locked: {}", p.difficulty, p.locked);
                m_app.m_world.setDifficulty(difficulty);
                m_app.m_world.setDifficultyLocked(p.locked);
                return Result<void>::ok();
            }
            // ---- 游戏状态变更（1 包 fan-out 多回调）----
            else if constexpr (std::is_same_v<T, irplay::GameEvent>) {
                const auto& p = pkt;
                // 1.21.11 GameEvent event 枚举（对应旧 GameStateChange）：
                // 0=NoRespawnBlock,1=BeginRaining,2=EndRaining,3=ChangeGameMode,4=WinGame,
                // 5=DemoEvent,6=ArrowHitPlayer,7=RainLevelChange,8=ThunderLevelChange,
                // 9=PufferFishSting,10=GuardianElderEffect,11=ImmediateRespawn,12=LimitedCrafting
                switch (p.event) {
                    case 1:
                        m_app.m_world.onBeginRaining();
                        break;
                    case 2:
                        m_app.m_world.onEndRaining();
                        break;
                    case 3: {
                        const GameMode gameMode = static_cast<GameMode>(static_cast<i32>(p.value));
                        if (m_app.m_player) {
                            spdlog::info("Game mode changed to: {}", static_cast<i32>(gameMode));
                            m_app.m_player->setGameMode(gameMode);
                            m_app.closeInventoryScreenIfModeMismatch();
                        }
                        break;
                    }
                    case 7:
                        m_app.m_world.onRainStrengthChange(p.value);
                        break;
                    case 8:
                        m_app.m_world.onThunderStrengthChange(p.value);
                        break;
                    default:
                        // 其余事件暂不处理（WinGame/DemoEvent 等）
                        break;
                }
                return Result<void>::ok();
            }
            // ---- 实体生成（合并旧 onSpawnMob / onSpawnEntity）----
            else if constexpr (std::is_same_v<T, irplay::AddEntity>) {
                const auto& p = pkt;
                const u32 entityId = static_cast<u32>(p.entityId);
                const std::string typeId = entityTypeIdString(p.entityTypeId);
                const f32 x = static_cast<f32>(p.x);
                const f32 y = static_cast<f32>(p.y);
                const f32 z = static_cast<f32>(p.z);
                const f32 yaw = unpackDegrees(p.yRot);
                const f32 pitch = unpackDegrees(p.xRot);
                const f32 headYaw = unpackDegrees(p.yHeadRot);
                const f32 vx = static_cast<f32>(p.movementX);
                const f32 vy = static_cast<f32>(p.movementY);
                const f32 vz = static_cast<f32>(p.movementZ);

                // 玩家实体走 onPlayerSpawn 路径（AddEntity type=PLAYER）
                // 注：服务端 PlayerSpawn 经 PlayerInfoUpdate+AddEntity 合成，这里按 type 区分。
                if (typeId == mc::entity::EntityTypeKeys::PLAYER) {
                    const PlayerId playerId = static_cast<PlayerId>(entityId);
                    // username 由 PlayerInfoUpdate 预先登记于 m_knownPlayerNames；此处取回
                    auto it = m_app.m_knownPlayerNames.find(playerId);
                    const std::string username = (it != m_app.m_knownPlayerNames.end()) ? it->second : std::string();
                    if (!m_app.m_localIdentity.isLocalPlayer(playerId)) {
                        auto& entityManager = m_app.m_world.entityManager();
                        const EntityInstanceId eid = static_cast<EntityInstanceId>(entityId);
                        m_app.m_identityRegistry.registerNetworkPlayer(eid, playerId, username);
                        ClientEntity* entity = entityManager.spawnEntity(eid, mc::entity::EntityTypeKeys::PLAYER);
                        if (!entity) {
                            entity = entityManager.getEntity(eid);
                        }
                        if (entity) {
                            entity->setPosition(x, y, z);
                        }
                    } else if (m_app.m_player) {
                        m_app.m_player->setPosition(x, y, z);
                    }
                    return Result<void>::ok();
                }

                // 经验球走 onSpawnExperienceOrb 路径
                if (typeId == mc::entity::EntityTypeKeys::EXPERIENCE_ORB) {
                    auto& entityManager = m_app.m_world.entityManager();
                    ClientEntity* entity = entityManager.spawnEntity(
                        static_cast<EntityInstanceId>(entityId), mc::entity::EntityTypeKeys::EXPERIENCE_ORB);
                    if (!entity) {
                        entity = entityManager.getEntity(static_cast<EntityInstanceId>(entityId));
                    }
                    if (entity) {
                        entity->setPosition(x, y, z);
                        entity->setXpValue(p.data);
                    }
                    return Result<void>::ok();
                }

                // 通用实体（含 Item 等带 data 的实体）
                MC_TRACE_INSTANT_EVENT(
                    TraceEvents.Client.Entity, "onAddEntity", "entityId", entityId, "typeId", typeId);

                auto& entityManager = m_app.m_world.entityManager();
                ClientEntity* entity = entityManager.spawnEntity(static_cast<EntityInstanceId>(entityId), typeId);
                if (!entity) {
                    entity = entityManager.getEntity(static_cast<EntityInstanceId>(entityId));
                }
                if (!entity) {
                    return Result<void>::ok();
                }
                entity->setPosition(x, y, z);
                entity->setRotation(yaw, pitch);
                entity->setVelocity(vx, vy, vz);
                entity->setHeadRotation(headYaw);

                // Item 实体：data 携带物品信息需后续 SetEntityData，此处先设 hoverStart
                if (typeId == mc::entity::EntityTypeKeys::ITEM) {
                    if (entity->hoverStart() == 0.0f) {
                        mc::math::Random rng(static_cast<u64>(entityId) * 341873128712ULL + 132897987541ULL);
                        entity->setHoverStart(rng.nextFloat() * mc::math::TWO_PI);
                    }
                }

                // 下落方块：对齐 MC 1.21.11，BlockState 经 AddEntity.data 字段下发 stateId
                // （vanilla FallingBlockEntity.getEntityData() = Block.BLOCK_STATE_REGISTRY.getId(blockState)）。
                // 客户端据此解析 BlockState 用于渲染；SynchedEntityData 仅有 DATA_START_POS(BlockPos)。
                // data<=0 视为空气（1.21.5+ 允许 falling_block 为空气，实体将立即移除）。
                if (typeId == mc::entity::EntityTypeKeys::FALLING_BLOCK) {
                    const i32 stateId = p.data;
                    if (stateId > 0) {
                        entity->setFallingBlockState(
                            mc::BlockRegistry::instance().getBlockState(static_cast<u32>(stateId)));
                    } else {
                        entity->setFallingBlockState(nullptr);
                    }
                }

                if (m_app.m_audioService) {
                    m_app.m_audioService->onEntitySpawn(entityId, typeId, x, y, z);
                }
                return Result<void>::ok();
            }
            // ---- 实体移除 ----
            else if constexpr (std::is_same_v<T, irplay::RemoveEntities>) {
                const auto& p = pkt;
                MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Entity, "onEntityDestroy", "count", p.entityIds.size());
                auto& entityManager = m_app.m_world.entityManager();
                for (i32 eidRaw : p.entityIds) {
                    const u32 entityId = static_cast<u32>(eidRaw);
                    const EntityInstanceId eid = static_cast<EntityInstanceId>(eidRaw);
                    if (m_app.m_localIdentity.isLocalPlayerEntity(eid)) {
                        continue;
                    }
                    if (m_app.m_renderer) {
                        m_app.m_renderer->entityRendererManager().removeEntityMeshes(eid);
                    }
                    entityManager.removeEntity(eid);
                    if (m_app.m_audioService) {
                        m_app.m_audioService->onEntityRemove(entityId);
                    }
                }
                return Result<void>::ok();
            }
            // ---- 实体传送 ----
            else if constexpr (std::is_same_v<T, irplay::TeleportEntity>) {
                const auto& p = pkt;
                const u32 entityId = static_cast<u32>(p.entityId);
                MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Entity, "onEntityTeleport", "entityId", entityId);
                const EntityInstanceId eid = static_cast<EntityInstanceId>(p.entityId);
                if (m_app.m_localIdentity.isLocalPlayerEntity(eid)) {
                    return Result<void>::ok();
                }
                ClientEntity* entity = m_app.m_world.entityManager().getEntity(eid);
                if (!entity) {
                    return Result<void>::ok();
                }
                entity->setPosition(static_cast<f32>(p.x), static_cast<f32>(p.y), static_cast<f32>(p.z));
                entity->setRotation(p.yRot, p.xRot);
                return Result<void>::ok();
            }
            // ---- 实体相对位移 ----
            else if constexpr (std::is_same_v<T, irplay::MoveEntityPos>) {
                const auto& p = pkt;
                const EntityInstanceId eid = static_cast<EntityInstanceId>(p.entityId);
                if (m_app.m_localIdentity.isLocalPlayerEntity(eid)) {
                    return Result<void>::ok();
                }
                ClientEntity* entity = m_app.m_world.entityManager().getEntity(eid);
                if (!entity) {
                    return Result<void>::ok();
                }
                const Vector3 position = entity->targetPosition();
                const f32 scale = 1.0f / 4096.0f; // 1.21.11 MoveEntityPos delta 是 1/4096 定点
                entity->setTargetPosition(position.x + static_cast<f32>(p.deltaX) * scale,
                    position.y + static_cast<f32>(p.deltaY) * scale,
                    position.z + static_cast<f32>(p.deltaZ) * scale);
                return Result<void>::ok();
            }
            // ---- 实体相对位移+旋转 ----
            else if constexpr (std::is_same_v<T, irplay::MoveEntityPosRot>) {
                const auto& p = pkt;
                const EntityInstanceId eid = static_cast<EntityInstanceId>(p.entityId);
                if (m_app.m_localIdentity.isLocalPlayerEntity(eid)) {
                    return Result<void>::ok();
                }
                ClientEntity* entity = m_app.m_world.entityManager().getEntity(eid);
                if (!entity) {
                    return Result<void>::ok();
                }
                const Vector3 position = entity->targetPosition();
                const f32 scale = 1.0f / 4096.0f;
                entity->setTargetPosition(position.x + static_cast<f32>(p.deltaX) * scale,
                    position.y + static_cast<f32>(p.deltaY) * scale,
                    position.z + static_cast<f32>(p.deltaZ) * scale);
                entity->setTargetRotation(unpackDegrees(p.yRot), unpackDegrees(p.xRot));
                return Result<void>::ok();
            }
            // ---- 实体旋转 ----
            else if constexpr (std::is_same_v<T, irplay::MoveEntityRot>) {
                const auto& p = pkt;
                const EntityInstanceId eid = static_cast<EntityInstanceId>(p.entityId);
                if (m_app.m_localIdentity.isLocalPlayerEntity(eid)) {
                    return Result<void>::ok();
                }
                ClientEntity* entity = m_app.m_world.entityManager().getEntity(eid);
                if (!entity) {
                    return Result<void>::ok();
                }
                entity->setTargetRotation(unpackDegrees(p.yRot), unpackDegrees(p.xRot));
                return Result<void>::ok();
            }
            // ---- 实体速度 ----
            else if constexpr (std::is_same_v<T, irplay::SetEntityMotion>) {
                const auto& p = pkt;
                const u32 entityId = static_cast<u32>(p.entityId);
                const EntityInstanceId eid = static_cast<EntityInstanceId>(p.entityId);
                if (m_app.m_localIdentity.isLocalPlayerEntity(eid)) {
                    if (m_app.m_player) {
                        m_app.m_player->setVelocity(
                            static_cast<f32>(p.x), static_cast<f32>(p.y), static_cast<f32>(p.z));
                    }
                    return Result<void>::ok();
                }
                ClientEntity* entity = m_app.m_world.entityManager().getEntity(eid);
                if (!entity) {
                    return Result<void>::ok();
                }
                entity->setVelocity(static_cast<f32>(p.x), static_cast<f32>(p.y), static_cast<f32>(p.z));
                (void)entityId;
                return Result<void>::ok();
            }
            // ---- 实体头部旋转 ----
            else if constexpr (std::is_same_v<T, irplay::RotateHead>) {
                const auto& p = pkt;
                const EntityInstanceId eid = static_cast<EntityInstanceId>(p.entityId);
                if (m_app.m_localIdentity.isLocalPlayerEntity(eid)) {
                    return Result<void>::ok();
                }
                ClientEntity* entity = m_app.m_world.entityManager().getEntity(eid);
                if (!entity) {
                    return Result<void>::ok();
                }
                entity->setTargetHeadRotation(unpackDegrees(p.yHeadRot));
                return Result<void>::ok();
            }
            // ---- 实体元数据 ----
            else if constexpr (std::is_same_v<T, irplay::SetEntityData>) {
                const auto& p = pkt;
                const u32 entityId = static_cast<u32>(p.entityId);
                MC_TRACE_INSTANT_EVENT(
                    TraceEvents.Client.Entity, "onEntityMetadata", "entityId", entityId, "size", p.packedItems.size());
                const EntityInstanceId eid = static_cast<EntityInstanceId>(p.entityId);
                if (m_app.m_localIdentity.isLocalPlayerEntity(eid)) {
                    return Result<void>::ok();
                }
                ClientEntity* entity = m_app.m_world.entityManager().getEntity(eid);
                if (!entity) {
                    return Result<void>::ok();
                }
                bool wasFallFlying = entity->isFallFlying();
                bool wasAngry = entity->isAngry();
                entity->setMetadata(p.packedItems);
                bool isFallFlying = entity->isFallFlying();
                bool isAngry = entity->isAngry();
                if (m_app.m_audioService && wasFallFlying != isFallFlying) {
                    m_app.m_audioService->onPlayerElytraFlyingChanged(entityId, isFallFlying);
                }
                if (m_app.m_audioService && wasAngry != isAngry) {
                    m_app.m_audioService->onEntityAngerStateChanged(entityId, isAngry);
                }
                return Result<void>::ok();
            }
            // ---- 容器内容（合并 onPlayerInventory / onContainerContent）----
            else if constexpr (std::is_same_v<T, irplay::ContainerSetContent>) {
                const auto& p = pkt;
                if (!m_app.m_player) {
                    return Result<void>::ok();
                }
                const ContainerId containerId = static_cast<ContainerId>(p.containerId);
                const auto items = viewsToStacks(p.items);
                auto carriedResult = mc::network::ir::fromItemStackView(p.carriedItem);
                const ItemStack carried = carriedResult.failed() ? ItemStack{} : std::move(carriedResult.value());

                // containerId == 0：玩家背包（onPlayerInventory）
                if (containerId == mc::inventory::PLAYER_CONTAINER_ID) {
                    auto& inventory = m_app.m_player->inventory();
                    // selectedSlot 不在此包中（由 SetHeldSlot 单独同步），保留当前选中槽
                    for (i32 slotIndex = 0;
                        slotIndex < static_cast<i32>(items.size()) && slotIndex < inventory.getContainerSize();
                        ++slotIndex) {
                        inventory.setItem(slotIndex, items[static_cast<size_t>(slotIndex)]);
                    }
                    if (auto* kageroScreen = dynamic_cast<ui::minecraft::InventoryScreen*>(
                            ScreenManager::instance().getCurrentKageroScreen())) {
                        applyContainerContentWithCarried(kageroScreen->getMenu(), containerId, items, carried);
                        kageroScreen->syncSlots();
                    } else if (auto* creativeScreen = dynamic_cast<ui::minecraft::CreativeScreen*>(
                                   ScreenManager::instance().getCurrentKageroScreen())) {
                        applyContainerContentWithCarried(creativeScreen->getMenu(), containerId, items, carried);
                        creativeScreen->syncSlots();
                    }
                    return Result<void>::ok();
                }

                // 其它容器（onContainerContent）：依次匹配各类 kagero 屏
                if (auto* kageroScreen = dynamic_cast<ui::minecraft::InventoryScreen*>(
                        ScreenManager::instance().getCurrentKageroScreen())) {
                    applyContainerContentWithCarried(kageroScreen->getMenu(), containerId, items, carried);
                    kageroScreen->syncSlots();
                    return Result<void>::ok();
                }
                if (auto* creativeScreen = dynamic_cast<ui::minecraft::CreativeScreen*>(
                        ScreenManager::instance().getCurrentKageroScreen())) {
                    applyContainerContentWithCarried(creativeScreen->getMenu(), containerId, items, carried);
                    creativeScreen->syncSlots();
                    return Result<void>::ok();
                }
                if (auto* craftingScreen = dynamic_cast<ui::minecraft::CraftingScreen*>(
                        ScreenManager::instance().getCurrentKageroScreen())) {
                    applyContainerContentWithCarried(craftingScreen->getMenu(), containerId, items, carried);
                    craftingScreen->syncSlots();
                    return Result<void>::ok();
                }
                if (auto* chestScreen =
                        dynamic_cast<ui::minecraft::ChestScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
                    applyContainerContentWithCarried(chestScreen->getMenu(), containerId, items, carried);
                    chestScreen->syncSlots();
                    return Result<void>::ok();
                }
                if (auto* furnaceScreen = dynamic_cast<ui::minecraft::FurnaceScreen*>(
                        ScreenManager::instance().getCurrentKageroScreen())) {
                    applyContainerContentWithCarried(furnaceScreen->getMenu(), containerId, items, carried);
                    furnaceScreen->syncSlots();
                    return Result<void>::ok();
                }
                if (auto* cartographyScreen = dynamic_cast<ui::minecraft::CartographyScreen*>(
                        ScreenManager::instance().getCurrentKageroScreen())) {
                    applyContainerContentWithCarried(cartographyScreen->getMenu(), containerId, items, carried);
                    cartographyScreen->syncSlots();
                    return Result<void>::ok();
                }
                return Result<void>::ok();
            }
            // ---- 容器单槽 ----
            else if constexpr (std::is_same_v<T, irplay::ContainerSetSlot>) {
                const auto& p = pkt;
                if (!m_app.m_player) {
                    return Result<void>::ok();
                }
                const ContainerId containerId = static_cast<ContainerId>(p.containerId);
                auto itemResult = mc::network::ir::fromItemStackView(p.item);
                const ItemStack item = itemResult.failed() ? ItemStack{} : std::move(itemResult.value());

                if (auto* kageroScreen = dynamic_cast<ui::minecraft::InventoryScreen*>(
                        ScreenManager::instance().getCurrentKageroScreen())) {
                    if (applyContainerSlot(kageroScreen->getMenu(), containerId, p.slot, item)) {
                        kageroScreen->syncSlots();
                        return Result<void>::ok();
                    }
                }
                if (auto* creativeScreen = dynamic_cast<ui::minecraft::CreativeScreen*>(
                        ScreenManager::instance().getCurrentKageroScreen())) {
                    if (applyContainerSlot(creativeScreen->getMenu(), containerId, p.slot, item)) {
                        creativeScreen->syncSlots();
                        return Result<void>::ok();
                    }
                }
                if (auto* craftingScreen = dynamic_cast<ui::minecraft::CraftingScreen*>(
                        ScreenManager::instance().getCurrentKageroScreen())) {
                    if (applyContainerSlot(craftingScreen->getMenu(), containerId, p.slot, item)) {
                        craftingScreen->syncSlots();
                        return Result<void>::ok();
                    }
                }
                if (auto* chestScreen =
                        dynamic_cast<ui::minecraft::ChestScreen*>(ScreenManager::instance().getCurrentKageroScreen())) {
                    if (applyContainerSlot(chestScreen->getMenu(), containerId, p.slot, item)) {
                        chestScreen->syncSlots();
                        return Result<void>::ok();
                    }
                }
                if (auto* furnaceScreen = dynamic_cast<ui::minecraft::FurnaceScreen*>(
                        ScreenManager::instance().getCurrentKageroScreen())) {
                    if (applyContainerSlot(furnaceScreen->getMenu(), containerId, p.slot, item)) {
                        furnaceScreen->syncSlots();
                        return Result<void>::ok();
                    }
                }
                if (auto* cartographyScreen = dynamic_cast<ui::minecraft::CartographyScreen*>(
                        ScreenManager::instance().getCurrentKageroScreen())) {
                    if (applyContainerSlot(cartographyScreen->getMenu(), containerId, p.slot, item)) {
                        cartographyScreen->syncSlots();
                        return Result<void>::ok();
                    }
                }
                return Result<void>::ok();
            }
            // ---- 打开容器屏 ----
            else if constexpr (std::is_same_v<T, irplay::OpenScreen>) {
                const auto& p = pkt;
                if (!m_app.m_player) {
                    return Result<void>::ok();
                }
                if (ScreenManager::instance().hasScreen()) {
                    ScreenManager::instance().closeAll();
                }
                const ContainerId containerId = static_cast<ContainerId>(p.containerId);
                const ContainerType type = static_cast<ContainerType>(p.menuType);

                if (type == ContainerType::Crafting) {
                    auto craftingMenu =
                        std::make_unique<mc::CraftingMenu>(containerId, &m_app.m_player->inventory(), nullptr);
                    auto screen = std::make_unique<ui::minecraft::CraftingScreen>(
                        std::move(craftingMenu), makeContainerClickSender(), makeContainerCloseSender());
                    if (m_app.m_renderer && m_app.m_renderer->isGuiRendererInitialized()) {
                        screen->setRenderers(&m_app.m_renderer->guiRenderer(),
                            m_app.m_guiTextureManager.get(),
                            m_app.m_renderer->isItemRendererInitialized() ? &m_app.m_renderer->itemRenderer()
                                                                          : nullptr);
                        screen->setScreenSize(m_app.m_guiScaleState.width, m_app.m_guiScaleState.height);
                    }
                    ScreenManager::instance().openScreen(std::move(screen));
                    return Result<void>::ok();
                }
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
                    auto chestContainer = std::make_unique<mc::blockentity::ChestContainer>(containerId,
                        &m_app.m_player->inventory(),
                        std::shared_ptr<mc::IInventory>(std::make_shared<mc::blockentity::SimpleInventory>(
                            rows * mc::blockentity::ChestContainer::SLOTS_PER_ROW)),
                        rows);
                    auto screen = std::make_unique<ui::minecraft::ChestScreen>(
                        std::move(chestContainer), makeContainerClickSender(), makeContainerCloseSender());
                    if (m_app.m_renderer && m_app.m_renderer->isGuiRendererInitialized()) {
                        screen->setRenderers(&m_app.m_renderer->guiRenderer(),
                            m_app.m_guiTextureManager.get(),
                            m_app.m_renderer->isItemRendererInitialized() ? &m_app.m_renderer->itemRenderer()
                                                                          : nullptr);
                        screen->setScreenSize(m_app.m_guiScaleState.width, m_app.m_guiScaleState.height);
                    }
                    ScreenManager::instance().openScreen(std::move(screen));
                    return Result<void>::ok();
                }
                if (type == ContainerType::Furnace || type == ContainerType::BlastFurnace ||
                    type == ContainerType::Smoker) {
                    auto furnaceContainer = std::make_unique<mc::blockentity::FurnaceContainer>(containerId,
                        &m_app.m_player->inventory(),
                        std::shared_ptr<mc::IInventory>(std::make_shared<mc::blockentity::FurnaceInventory>()),
                        nullptr);
                    auto screen = std::make_unique<ui::minecraft::FurnaceScreen>(
                        std::move(furnaceContainer), makeContainerClickSender(), makeContainerCloseSender());
                    if (m_app.m_renderer && m_app.m_renderer->isGuiRendererInitialized()) {
                        screen->setRenderers(&m_app.m_renderer->guiRenderer(),
                            m_app.m_guiTextureManager.get(),
                            m_app.m_renderer->isItemRendererInitialized() ? &m_app.m_renderer->itemRenderer()
                                                                          : nullptr);
                        screen->setScreenSize(m_app.m_guiScaleState.width, m_app.m_guiScaleState.height);
                    }
                    ScreenManager::instance().openScreen(std::move(screen));
                    return Result<void>::ok();
                }
                if (type == ContainerType::Cartography) {
                    auto cartographyContainer = std::make_unique<mc::CartographyContainer>(
                        containerId, &m_app.m_player->inventory(), BlockPos(0, 0, 0), nullptr);
                    auto screen = std::make_unique<ui::minecraft::CartographyScreen>(
                        std::move(cartographyContainer), makeContainerClickSender(), makeContainerCloseSender());
                    if (m_app.m_renderer && m_app.m_renderer->isGuiRendererInitialized()) {
                        screen->setRenderers(&m_app.m_renderer->guiRenderer(),
                            m_app.m_guiTextureManager.get(),
                            m_app.m_renderer->isItemRendererInitialized() ? &m_app.m_renderer->itemRenderer()
                                                                          : nullptr);
                        screen->setScreenSize(m_app.m_guiScaleState.width, m_app.m_guiScaleState.height);
                    }
                    // 注入地图渲染器（制图台预览结果槽的地图内容）
                    if (m_app.m_mapRenderer) {
                        screen->setMapRenderer(m_app.m_mapRenderer.get());
                    }
                    ScreenManager::instance().openScreen(std::move(screen));
                    return Result<void>::ok();
                }
                spdlog::error("Ignored unsupported container type {}", static_cast<i32>(type));
                return Result<void>::ok();
            }
            // ---- 容器属性（熔炉进度等）----
            else if constexpr (std::is_same_v<T, irplay::ContainerSetData>) {
                const auto& p = pkt;
                auto* furnaceScreen =
                    dynamic_cast<ui::minecraft::FurnaceScreen*>(ScreenManager::instance().getCurrentKageroScreen());
                if (furnaceScreen == nullptr || furnaceScreen->getMenu() == nullptr) {
                    return Result<void>::ok();
                }
                if (furnaceScreen->getMenu()->getId() != static_cast<ContainerId>(p.containerId)) {
                    return Result<void>::ok();
                }
                furnaceScreen->getMenu()->setTrackedInt(p.property, p.value);
                return Result<void>::ok();
            }
            // ---- 玩家列表更新（9 位 actions fan-out）----
            else if constexpr (std::is_same_v<T, irplay::PlayerInfoUpdate>) {
                const auto& p = pkt;
                if (!m_app.m_skinManager) {
                    return Result<void>::ok();
                }
                // actions 位：0=ADD_PLAYER 1=INITIALIZE_CHAT 2=UPDATE_GAME_MODE 3=UPDATE_LISTED
                // 4=UPDATE_LATENCY 5=UPDATE_DISPLAY_NAME 6=UPDATE_LIST_ORDER 7=UPDATE_HAT 8=INITIALIZE_CHAT2
                const bool addPlayer = (p.actions & 0x0001) != 0;
                if (addPlayer) {
                    std::vector<::mc::skin::PlayerListEntry> entries;
                    for (const auto& e : p.entries) {
                        ::mc::skin::PlayerListEntry entry;
                        entry.uuid = e.uuid;
                        if (e.name) {
                            entry.name = *e.name;
                        }
                        for (const auto& [k, v] : e.properties) {
                            entry.properties.emplace_back(k, v);
                        }
                        entries.push_back(std::move(entry));
                    }
                    for (const auto& entry : entries) {
                        m_app.m_identityRegistry.registerPlayerListUuid(entry.uuid, entry.name);
                        ::mc::skin::GameProfile profile(entry.uuid, entry.name);
                        for (const auto& prop : entry.properties) {
                            profile.addProperty(prop);
                        }
                        m_app.m_skinManager->registerPlayerSkin(profile);
                    }
                }
                // 其它 actions（UPDATE_LATENCY/DISPLAY_NAME 等）当前无处理逻辑，保留扩展点
                return Result<void>::ok();
            }
            // ---- 玩家列表移除 ----
            else if constexpr (std::is_same_v<T, irplay::PlayerInfoRemove>) {
                const auto& p = pkt;
                for (const auto& uuid : p.uuids) {
                    m_app.m_skinManager->removePlayerInfo(uuid);
                    m_app.m_identityRegistry.removeByUuid(uuid);
                }
                return Result<void>::ok();
            }
            // ---- 实体动画 ----
            else if constexpr (std::is_same_v<T, irplay::Animate>) {
                const auto& p = pkt;
                const u32 entityId = static_cast<u32>(p.id);
                auto* entity = m_app.m_world.entityManager().getEntity(static_cast<EntityInstanceId>(p.id));
                if (!entity) {
                    return Result<void>::ok();
                }
                using Animation = mc::network::EntityAnimation;
                switch (static_cast<Animation>(p.action)) {
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
                        if (m_app.m_world.particleManager() != nullptr && entity != nullptr) {
                            glm::vec3 entityPos(entity->x(), entity->y(), entity->z());
                            for (i32 i = 0; i < 16; ++i) {
                                f32 rx = m_app.m_random.nextFloat() * 2.0f - 1.0f;
                                f32 ry = m_app.m_random.nextFloat() * 2.0f - 1.0f;
                                f32 rz = m_app.m_random.nextFloat() * 2.0f - 1.0f;
                                if (rx * rx + ry * ry + rz * rz > 1.0f) {
                                    continue;
                                }
                                glm::vec3 particlePos =
                                    entityPos + glm::vec3(rx * 0.25f, ry * 0.25f + 0.5f, rz * 0.25f);
                                glm::vec3 velocity(rx, ry + 0.2f, rz);
                                m_app.m_world.particleManager()->addPendingParticle(
                                    ::mc::particle::ParticleTypeId::Crit, particlePos, velocity, &m_app.m_world);
                            }
                        }
                        break;
                    }
                    case Animation::MagicCriticalEffect: {
                        if (m_app.m_world.particleManager() != nullptr && entity != nullptr) {
                            glm::vec3 entityPos(entity->x(), entity->y(), entity->z());
                            for (i32 i = 0; i < 16; ++i) {
                                f32 rx = m_app.m_random.nextFloat() * 2.0f - 1.0f;
                                f32 ry = m_app.m_random.nextFloat() * 2.0f - 1.0f;
                                f32 rz = m_app.m_random.nextFloat() * 2.0f - 1.0f;
                                if (rx * rx + ry * ry + rz * rz > 1.0f) {
                                    continue;
                                }
                                glm::vec3 particlePos =
                                    entityPos + glm::vec3(rx * 0.25f, ry * 0.25f + 0.5f, rz * 0.25f);
                                glm::vec3 velocity(rx, ry + 0.2f, rz);
                                m_app.m_world.particleManager()->addPendingParticle(
                                    ::mc::particle::ParticleTypeId::EnchantedHit,
                                    particlePos,
                                    velocity,
                                    &m_app.m_world);
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
                (void)entityId;
                return Result<void>::ok();
            }
            // ---- 受伤动画 ----
            else if constexpr (std::is_same_v<T, irplay::HurtAnimation>) {
                const auto& p = pkt;
                // 1.21.11 HurtAnimation 携带 yaw（damage tilt 方向），对应旧 onHurtAnimation 的 hurtDir
                const f32 hurtDir = p.yaw;
                auto* entity = m_app.m_world.entityManager().getEntity(static_cast<EntityInstanceId>(p.id));
                if (entity != nullptr) {
                    entity->setHurtDir(hurtDir);
                }
                if (m_app.m_player != nullptr &&
                    m_app.m_localIdentity.isLocalPlayerEntity(static_cast<EntityInstanceId>(p.id))) {
                    m_app.m_player->animateHurt(hurtDir);
                }
                return Result<void>::ok();
            }
            // ---- 玩家拾取物品 ----
            else if constexpr (std::is_same_v<T, irplay::TakeItemEntity>) {
                // 旧 onCollectItem 无客户端处理体（仅日志），保留空实现
                return Result<void>::ok();
            }
            // ---- 方块破坏动画 ----
            else if constexpr (std::is_same_v<T, irplay::BlockDestruction>) {
                const auto& p = pkt;
                using namespace mc::client::renderer::trident::block;
                auto& manager = BreakProgressManager::instance();
                const BlockPos pos = BlockPos::fromLong(p.blockPosPacked);
                const u64 currentTick = static_cast<u64>(m_app.m_world.gameTime());
                const EntityInstanceId breakerId = static_cast<EntityInstanceId>(p.id);
                const i8 stage = static_cast<i8>(p.progress);
                if (stage < 0) {
                    manager.removeRemoteProgress(breakerId);
                } else {
                    manager.updateRemoteProgress(breakerId, pos, stage, currentTick);
                }
                return Result<void>::ok();
            }
            // ---- 方块事件（箱子开合、音符盒、活塞等，委托 BlockEntity::triggerEvent）----
            else if constexpr (std::is_same_v<T, irplay::BlockEvent>) {
                const auto& p = pkt;
                const BlockPos pos = BlockPos::fromLong(p.blockPosPacked);
                // 1.21.11 BlockEvent: blockPos + b0(action) + b1(param) + blockId。
                // 客户端按位置取 BlockEntity 调 triggerEvent(action, param) 触发客户端侧动画
                // （BellBlockEntity/DecoratedPotBlockEntity/EndGatewayEntity 等已实现该虚函数）。
                // 无 BlockEntity 的方块（活塞/音符盒）当前无客户端表现，静默忽略。
                if (auto* be = m_app.m_world.getBlockEntity(pos)) {
                    (void)be->triggerEvent(static_cast<i32>(p.b0), static_cast<i32>(p.b1));
                }
                return Result<void>::ok();
            }
            // ---- 方块实体数据 ----
            else if constexpr (std::is_same_v<T, irplay::BlockEntityData>) {
                const auto& p = pkt;
                const BlockPos pos = BlockPos::fromLong(p.blockPosPacked);
                // 1.21.11 BlockEntityData 直携 CompoundTag；nullptr 视为空 NBT。
                static const nbt::CompoundTag kEmpty{};
                m_app.m_world.onBlockEntityData(
                    pos, static_cast<BlockEntityType>(p.blockEntityType), p.tag ? *p.tag : kEmpty);
                return Result<void>::ok();
            }
            // ---- 打开告示牌编辑器 ----
            else if constexpr (std::is_same_v<T, irplay::OpenSignEditor>) {
                const auto& p = pkt;
                auto* screenStack = getScreenStack();
                if (!screenStack) {
                    spdlog::warn("[Network] Cannot open sign editor: ScreenStackWidget not available");
                    return Result<void>::ok();
                }
                const BlockPos pos = BlockPos::fromLong(p.blockPosPacked);
                std::array<std::string, ui::minecraft::SignEditScreen::LINE_COUNT> initialLines{};
                const BlockEntity* entity = m_app.m_world.getBlockEntity(pos);
                if (entity != nullptr && entity->getType() == BlockEntityType::Sign) {
                    const auto* signEntity = static_cast<const blockentity::SignEntity*>(entity);
                    for (i32 i = 0; i < ui::minecraft::SignEditScreen::LINE_COUNT; ++i) {
                        initialLines[static_cast<std::size_t>(i)] = signEntity->getLineText(i);
                    }
                }
                auto signScreen = std::make_unique<ui::minecraft::SignEditScreen>(
                    pos,
                    initialLines,
                    p.isFrontText,
                    [this](const BlockPos& signPos,
                        const std::array<std::string, ui::minecraft::SignEditScreen::LINE_COUNT>& lines,
                        bool isFrontSide) {
                        if (m_app.m_network) {
                            irplay::SignUpdate pkt2;
                            pkt2.blockPosPacked = signPos.asLong();
                            pkt2.isFrontText = isFrontSide;
                            pkt2.lines = lines;
                            (void)m_app.m_network->send(
                                mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
                                    mc::network::ir::PlayPacket{irplay::SignUpdate{pkt2}}});
                        }
                    },
                    [screenStack]() { screenStack->pop(); });
                signScreen->setBounds(
                    ui::kagero::Rect(0, 0, m_app.m_guiScaleState.width, m_app.m_guiScaleState.height));
                screenStack->push(std::move(signScreen));
                return Result<void>::ok();
            }
            // ---- 经验 ----
            else if constexpr (std::is_same_v<T, irplay::SetExperience>) {
                const auto& p = pkt;
                if (!m_app.m_player) {
                    return Result<void>::ok();
                }
                // SetExperience 字段：experienceProgress(f32)/experienceLevel(i32)/totalExperience(i32)
                m_app.m_player->setExperience(p.experienceLevel, p.experienceProgress, p.totalExperience);
                return Result<void>::ok();
            }
            // ---- 乘客 ----
            else if constexpr (std::is_same_v<T, irplay::SetPassengers>) {
                const auto& p = pkt;
                const u32 entityId = static_cast<u32>(p.vehicle);
                const std::vector<u32> passengerIds(p.passengers.begin(), p.passengers.end());
                const EntityInstanceId localPlayerEntityId = m_app.m_localIdentity.entityId();
                const EntityInstanceId vehicleEntityId = static_cast<EntityInstanceId>(p.vehicle);
                ClientEntity* vehicleEntity = m_app.m_world.entityManager().getEntity(vehicleEntityId);
                std::vector<u32> oldPassengerIds;
                if (vehicleEntity) {
                    oldPassengerIds = vehicleEntity->passengers();
                }
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
                            m_app.m_world.entityManager().getEntity(static_cast<EntityInstanceId>(oldPassengerId));
                        if (oldPassenger) {
                            oldPassenger->setVehicleId(0);
                        }
                    }
                }
                if (vehicleEntity) {
                    vehicleEntity->setPassengers(passengerIds);
                }
                bool localPlayerIsRiding = false;
                for (u32 passengerId : passengerIds) {
                    if (passengerId == static_cast<u32>(localPlayerEntityId)) {
                        localPlayerIsRiding = true;
                    }
                    ClientEntity* passenger =
                        m_app.m_world.entityManager().getEntity(static_cast<EntityInstanceId>(passengerId));
                    if (passenger) {
                        passenger->setVehicleId(vehicleEntityId);
                    }
                }
                ClientEntity* localPlayer = m_app.m_world.entityManager().getEntity(localPlayerEntityId);
                if (localPlayer) {
                    if (localPlayerIsRiding) {
                        if (m_app.m_audioService) {
                            m_app.m_audioService->updateEntityRidingState(
                                static_cast<u32>(localPlayerEntityId), true, entityId);
                        }
                    } else if (localPlayer->vehicleId() == vehicleEntityId) {
                        if (m_app.m_audioService) {
                            m_app.m_audioService->updateEntityRidingState(
                                static_cast<u32>(localPlayerEntityId), false, 0);
                        }
                    }
                }
                return Result<void>::ok();
            }
            // ---- 旁观者摄像机 ----
            else if constexpr (std::is_same_v<T, irplay::SetCamera>) {
                const auto& p = pkt;
                const u32 cameraEntityId = static_cast<u32>(p.cameraId);
                const EntityInstanceId localPlayerEntityId = m_app.m_localIdentity.entityId();
                if (cameraEntityId == static_cast<u32>(localPlayerEntityId)) {
                    if (m_app.m_player) {
                        m_app.m_player->setCameraEntityId(std::nullopt);
                    }
                    spdlog::info("SetCamera: reset to self (entityId={})", cameraEntityId);
                } else {
                    if (m_app.m_player) {
                        m_app.m_player->setCameraEntityId(static_cast<EntityInstanceId>(cameraEntityId));
                    }
                    spdlog::info("SetCamera: spectating entity {}", cameraEntityId);
                }
                return Result<void>::ok();
            }
            // ---- 重生 / 维度切换 ----
            else if constexpr (std::is_same_v<T, irplay::Respawn>) {
                const auto& p = pkt;
                // spawnInfo.dimension 是 ResourceKey 字符串，需映射为 DimensionId
                const DimensionId dimension = dimensionKeyToId(p.spawnInfo.dimension);
                const GameMode gameMode = p.spawnInfo.gameType;
                const bool keepData = (p.dataToKeep != 0);
                spdlog::info("Received Respawn: dimensionType={}, dimension={}, gameMode={}, keepData={}",
                    p.spawnInfo.dimensionType,
                    static_cast<i32>(dimension),
                    static_cast<i32>(gameMode),
                    keepData);

                const DimensionId currentDim = m_app.m_dimensionManager.currentDimension();
                const bool isDimensionChange = (currentDim != dimension);
                if (isDimensionChange) {
                    spdlog::info("[Respawn] Dimension change: {} -> {}",
                        static_cast<i32>(currentDim),
                        static_cast<i32>(dimension));
                    m_app.m_dimensionManager.beginDimensionChange(dimension, Vector3d(0, 0, 0));
                    m_app.m_world.setDimensionId(dimension);
                    m_app.m_world.clearChunks();
                    m_app.m_world.entityManager().clear();
                    m_app.m_world.resetWeather();
                    if (m_app.m_renderer && m_app.m_renderer->isChunkRendererInitialized()) {
                        m_app.m_renderer->chunkRenderer().clearChunks();
                    }
                    m_app.m_dimensionManager.completeDimensionChange();
                    m_app.updateCloudHeight();
                    spdlog::info("[Respawn] Dimension change completed");
                }
                if (m_app.m_player) {
                    m_app.m_player->setGameMode(gameMode);
                    m_app.m_player->setDimension(dimension);
                    if (!keepData) {
                        m_app.m_player->respawn();
                    }
                    std::optional<GlobalPos> lastDeath;
                    if (p.spawnInfo.lastDeathLocation) {
                        // optional<pair<string,i64>> → GlobalPos(dimension, BlockPos)
                        // pair.first=维度 ResourceKey，pair.second=BlockPos asLong。
                        const auto& [dimKey, packedPos] = *p.spawnInfo.lastDeathLocation;
                        lastDeath = GlobalPos(dimensionKeyToId(dimKey), BlockPos::fromLong(packedPos));
                    }
                    m_app.m_player->setLastDeathLocation(std::move(lastDeath));
                }
                if (m_app.m_predictor && m_app.m_player) {
                    m_app.m_predictor->reset(
                        Vector3(
                            m_app.m_player->position().x, m_app.m_player->position().y, m_app.m_player->position().z),
                        m_app.m_player->yaw(),
                        m_app.m_player->pitch());
                }
                // 1.21.11 维度切换：Respawn 不重新进入 Configuration 阶段，客户端无需发
                // ConfigurationAcknowledged（该 C→S terminal 仅在 Configuration→Play 切换时发，
                // 我方以 FinishConfiguration 为 terminal，已由 ClientNetwork 处理）。
                // AcceptTeleportation 由 PlayerPosition 分支按 teleportId 回确认。
                return Result<void>::ok();
            }
            // ---- 载具移动（回发确认）----
            else if constexpr (std::is_same_v<T, irplay::ClientboundMoveVehicle>) {
                const auto& p = pkt;
                const EntityInstanceId localPlayerEntityId = m_app.m_localIdentity.entityId();
                ClientEntity* localPlayer = m_app.m_world.entityManager().getEntity(localPlayerEntityId);
                if (!localPlayer) {
                    return Result<void>::ok();
                }
                EntityInstanceId vehicleId = localPlayer->vehicleId();
                if (vehicleId == 0) {
                    return Result<void>::ok();
                }
                ClientEntity* vehicle = m_app.m_world.entityManager().getEntity(vehicleId);
                if (!vehicle) {
                    return Result<void>::ok();
                }
                vehicle->setPosition(static_cast<f32>(p.x), static_cast<f32>(p.y), static_cast<f32>(p.z));
                vehicle->setRotation(p.yRot, p.xRot);
                if (m_app.m_network) {
                    irplay::ServerboundMoveVehicle pkt2;
                    pkt2.x = p.x;
                    pkt2.y = p.y;
                    pkt2.z = p.z;
                    pkt2.yRot = p.yRot;
                    pkt2.xRot = p.xRot;
                    pkt2.onGround = true;
                    (void)m_app.m_network->send(
                        mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
                            mc::network::ir::PlayPacket{irplay::ServerboundMoveVehicle{pkt2}}});
                }
                return Result<void>::ok();
            }
            // ---- 光照更新 ----
            else if constexpr (std::is_same_v<T, irplay::LightUpdate>) {
                const auto& p = pkt;
                // 1.21.11 ClientboundLightUpdatePacket：codec 已把线格式解析为结构化字段
                //   lightMasks[0..3] = skyYMask / blockYMask / emptySkyYMask / emptyBlockYMask（最小长整型数组）
                //   lightUpdates[0..1] = skyUpdates / blockUpdates（每条一个光照段 2048 字节 nibble）
                // BitSet 位 i 对应光照段 Y = minLightSection + i（minLightSection = MIN_SECTION_Y - 1，主世界=-5）。
                // yMask 的每个置位位对应 lightUpdates 列表里按序一条 nibble，逐位下发到对应 ChunkSection。
                constexpr i32 kMinLightSection = mc::world::MIN_SECTION_Y - 1; // 主世界=-5

                auto dispatchLayer =
                    [&](bool isSky, const std::vector<i64>& mask, const std::vector<std::vector<u8>>& updates) {
                        usize updateIdx = 0;
                        for (usize li = 0; li < mask.size(); ++li) {
                            const u64 word = static_cast<u64>(mask[li]);
                            if (word == 0) {
                                continue;
                            }
                            for (u32 b = 0; b < 64; ++b) {
                                if (((word >> b) & 1ULL) == 0) {
                                    continue;
                                }
                                if (updateIdx >= updates.size()) {
                                    break; // yMask 置位数与列表长度不符，防御：剩余位丢弃
                                }
                                const i32 bitIndex = static_cast<i32>(li * 64 + b);
                                const i32 sectionY = kMinLightSection + bitIndex;
                                const std::vector<u8>& nibble = updates[updateIdx++];
                                m_app.m_world.onLightSection(p.x, p.z, sectionY, isSky, nibble);
                            }
                        }
                    };

                dispatchLayer(true, p.lightMasks[0], p.lightUpdates[0]);  // sky
                dispatchLayer(false, p.lightMasks[1], p.lightUpdates[1]); // block
                return Result<void>::ok();
            }
            // ---- 声音 ----
            else if constexpr (std::is_same_v<T, irplay::PlaySound>) {
                // 1.21.11 PlaySound：soundHolder(结构化 Holder<SoundEvent>) + source + 坐标(×8) +
                // volume + pitch + seed。坐标需 /8.0f 还原。
                const auto& p = pkt;
                if (m_app.m_audioService == nullptr) {
                    return Result<void>::ok();
                }
                auto soundId = parseSoundHolder(p.soundHolder);
                auto category = sourceToCategory(p.source);
                if (!soundId || !category) {
                    return Result<void>::ok(); // 降级编码失败/越界，静默丢弃
                }
                const f32 x = static_cast<f32>(p.x) / 8.0f;
                const f32 y = static_cast<f32>(p.y) / 8.0f;
                const f32 z = static_cast<f32>(p.z) / 8.0f;
                auto sound =
                    mc::client::sound::SoundInstance::createLocated(*soundId, *category, x, y, z, p.volume, p.pitch);
                m_app.m_audioService->play(std::make_unique<mc::client::sound::SoundInstance>(std::move(sound)));
                return Result<void>::ok();
            } else if constexpr (std::is_same_v<T, irplay::StopSound>) {
                // 1.21.11 StopSound：flags(0x01=按 source,0x02=按 name) + source + name。
                const auto& p = pkt;
                if (m_app.m_audioService == nullptr) {
                    return Result<void>::ok();
                }
                if ((p.flags & 0x01) != 0) {
                    if (auto category = sourceToCategory(p.source)) {
                        m_app.m_audioService->stop(*category);
                    }
                }
                if ((p.flags & 0x02) != 0) {
                    auto rl = ResourceLocation::parse(p.name);
                    if (rl.isValid()) {
                        m_app.m_audioService->stop(rl);
                    }
                }
                if (p.flags == 0) {
                    m_app.m_audioService->stopAll();
                }
                return Result<void>::ok();
            } else if constexpr (std::is_same_v<T, irplay::SoundEntity>) {
                // 1.21.11 SoundEntity：soundHolder + source + entityId + volume + pitch + seed。
                // 用实体当前位置播放（playMovingSound 跟随实体，但需实体存在且非本地玩家）。
                const auto& p = pkt;
                if (m_app.m_audioService == nullptr) {
                    return Result<void>::ok();
                }
                auto soundId = parseSoundHolder(p.soundHolder);
                auto category = sourceToCategory(p.source);
                if (!soundId || !category) {
                    return Result<void>::ok();
                }
                ClientEntity* entity =
                    m_app.m_world.entityManager().getEntity(static_cast<EntityInstanceId>(p.entityId));
                if (entity == nullptr) {
                    return Result<void>::ok(); // 实体未加载，静默丢弃
                }
                const auto pos = entity->position();
                auto sound = mc::client::sound::SoundInstance::createLocated(
                    *soundId, *category, pos.x, pos.y, pos.z, p.volume, p.pitch);
                m_app.m_audioService->play(std::make_unique<mc::client::sound::SoundInstance>(std::move(sound)));
                return Result<void>::ok();
            }
            // ---- 标题（5 拆，映射回 TitleAction 喂 TitleWidget）----
            else if constexpr (std::is_same_v<T, irplay::SetTitleText>) {
                const auto& p = pkt;
                if (m_app.m_kageroEngine) {
                    auto* titleWidget = static_cast<ui::minecraft::widgets::TitleWidget*>(
                        m_app.m_kageroEngine->getLayer(m_app.m_titleLayerId));
                    if (titleWidget) {
                        const std::string text(p.text.begin(), p.text.end());
                        titleWidget->handleTitlePacket(mc::network::TitleAction::Title, text, 0, 0, 0);
                    }
                }
                return Result<void>::ok();
            } else if constexpr (std::is_same_v<T, irplay::SetSubtitleText>) {
                const auto& p = pkt;
                if (m_app.m_kageroEngine) {
                    auto* titleWidget = static_cast<ui::minecraft::widgets::TitleWidget*>(
                        m_app.m_kageroEngine->getLayer(m_app.m_titleLayerId));
                    if (titleWidget) {
                        const std::string text(p.text.begin(), p.text.end());
                        titleWidget->handleTitlePacket(mc::network::TitleAction::Subtitle, text, 0, 0, 0);
                    }
                }
                return Result<void>::ok();
            } else if constexpr (std::is_same_v<T, irplay::SetActionBarText>) {
                const auto& p = pkt;
                if (m_app.m_kageroEngine) {
                    auto* titleWidget = static_cast<ui::minecraft::widgets::TitleWidget*>(
                        m_app.m_kageroEngine->getLayer(m_app.m_titleLayerId));
                    if (titleWidget) {
                        const std::string text(p.text.begin(), p.text.end());
                        titleWidget->handleTitlePacket(mc::network::TitleAction::Actionbar, text, 0, 0, 0);
                    }
                }
                return Result<void>::ok();
            } else if constexpr (std::is_same_v<T, irplay::SetTitlesAnimation>) {
                const auto& p = pkt;
                if (m_app.m_kageroEngine) {
                    auto* titleWidget = static_cast<ui::minecraft::widgets::TitleWidget*>(
                        m_app.m_kageroEngine->getLayer(m_app.m_titleLayerId));
                    if (titleWidget) {
                        titleWidget->handleTitlePacket(
                            mc::network::TitleAction::Times, std::nullopt, p.fadeIn, p.stay, p.fadeOut);
                    }
                }
                return Result<void>::ok();
            } else if constexpr (std::is_same_v<T, irplay::ClearTitles>) {
                const auto& p = pkt;
                if (m_app.m_kageroEngine) {
                    auto* titleWidget = static_cast<ui::minecraft::widgets::TitleWidget*>(
                        m_app.m_kageroEngine->getLayer(m_app.m_titleLayerId));
                    if (titleWidget) {
                        const auto action =
                            p.resetTimes ? mc::network::TitleAction::Reset : mc::network::TitleAction::Clear;
                        titleWidget->handleTitlePacket(action, std::nullopt, 0, 0, 0);
                    }
                }
                return Result<void>::ok();
            }
            // ---- 世界事件（音效/粒子）----
            else if constexpr (std::is_same_v<T, irplay::LevelEvent>) {
                const auto& p = pkt;
                const BlockPos pos = BlockPos::fromLong(p.blockPosPacked);
                m_app._handleWorldEvent(p.type, pos.x, pos.y, pos.z, p.data);
                return Result<void>::ok();
            }
            // ---- 实体状态事件（旧 onEntityStatus，switch 主体迁入）----
            else if constexpr (std::is_same_v<T, irplay::EntityEvent>) {
                const auto& p = pkt;
                using namespace mc::network;
                const u32 entityId = static_cast<u32>(p.entityId);
                const u8 status = p.eventId;
                ClientEntity* entity =
                    m_app.m_world.entityManager().getEntity(static_cast<EntityInstanceId>(p.entityId));
                glm::vec3 entityPos(0.0f);
                if (entity != nullptr) {
                    entityPos = glm::vec3(entity->x(), entity->y(), entity->z());
                }
                switch (status) {
                    case static_cast<u8>(EntityStatus::GuardianAttack): {
                        if (m_app.m_audioService) {
                            m_app.m_audioService->onGuardianAttack(entityId);
                        }
                        break;
                    }
                    case static_cast<u8>(EntityStatus::TamingSucceeded): {
                        if (m_app.m_world.particleManager() != nullptr) {
                            f32 entityWidth = entity != nullptr ? entity->width() : 0.6f;
                            f32 entityHeight = entity != nullptr ? entity->height() : 1.8f;
                            for (i32 i = 0; i < 7; ++i) {
                                f32 rx = (m_app.m_random.nextFloat() * 2.0f - 1.0f) * entityWidth;
                                f32 ry = m_app.m_random.nextFloat() * entityHeight + 0.5f;
                                f32 rz = (m_app.m_random.nextFloat() * 2.0f - 1.0f) * entityWidth;
                                glm::vec3 particlePos = entityPos + glm::vec3(rx, ry, rz);
                                glm::vec3 velocity(m_app.m_random.nextGaussian(0.0f, 0.02f),
                                    m_app.m_random.nextGaussian(0.0f, 0.02f),
                                    m_app.m_random.nextGaussian(0.0f, 0.02f));
                                m_app.m_world.particleManager()->addPendingParticle(
                                    ::mc::particle::ParticleTypeId::Heart, particlePos, velocity, &m_app.m_world);
                            }
                        }
                        break;
                    }
                    case static_cast<u8>(EntityStatus::TamingFailed): {
                        if (m_app.m_world.particleManager() != nullptr) {
                            f32 entityWidth = entity != nullptr ? entity->width() : 0.6f;
                            f32 entityHeight = entity != nullptr ? entity->height() : 1.8f;
                            for (i32 i = 0; i < 7; ++i) {
                                f32 rx = (m_app.m_random.nextFloat() * 2.0f - 1.0f) * entityWidth;
                                f32 ry = m_app.m_random.nextFloat() * entityHeight + 0.5f;
                                f32 rz = (m_app.m_random.nextFloat() * 2.0f - 1.0f) * entityWidth;
                                glm::vec3 particlePos = entityPos + glm::vec3(rx, ry, rz);
                                glm::vec3 velocity(m_app.m_random.nextGaussian(0.0f, 0.02f),
                                    m_app.m_random.nextGaussian(0.0f, 0.02f),
                                    m_app.m_random.nextGaussian(0.0f, 0.02f));
                                m_app.m_world.particleManager()->addPendingParticle(
                                    ::mc::particle::ParticleTypeId::Smoke, particlePos, velocity, &m_app.m_world);
                            }
                        }
                        break;
                    }
                    case static_cast<u8>(EntityStatus::LoveHeart): {
                        if (m_app.m_world.particleManager() != nullptr) {
                            glm::vec3 heartPos = entityPos + glm::vec3(0.0f, 0.5f, 0.0f);
                            m_app.m_world.particleManager()->addPendingParticle(::mc::particle::ParticleTypeId::Heart,
                                heartPos,
                                glm::vec3(0.0f, 0.0f, 0.0f),
                                &m_app.m_world);
                        }
                        break;
                    }
                    case static_cast<u8>(EntityStatus::EatBlock): {
                        if (entity != nullptr) {
                            if (entity->entityType() == mc::entity::VanillaEntityTypeKeys::TNT_MINECART) {
                                entity->setFuseTimer(80);
                            } else {
                                entity->setEatAnimationTimer(40);
                            }
                        }
                        break;
                    }
                    case static_cast<u8>(EntityStatus::ShakeOffWater): {
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
                    case static_cast<u8>(EntityStatus::WolfStopShaking): {
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
                    case static_cast<u8>(EntityStatus::RabbitJump): {
                        if (entity != nullptr) {
                            const std::string& typeId = entity->getTypeId();
                            if (typeId == "minecraft:rabbit" || typeId == "rabbit") {
                                entity->setRabbitJumpStart();
                            }
                        }
                        break;
                    }
                    case static_cast<u8>(EntityStatus::OcelotTrustSucceeded):
                    case static_cast<u8>(EntityStatus::OcelotTrustFailed): {
                        if (m_app.m_world.particleManager() != nullptr) {
                            f32 entityWidth = entity != nullptr ? entity->width() : 0.6f;
                            f32 entityHeight = entity != nullptr ? entity->height() : 0.7f;
                            const auto particleType = (status == static_cast<u8>(EntityStatus::OcelotTrustSucceeded))
                                ? ::mc::particle::ParticleTypeId::Heart
                                : ::mc::particle::ParticleTypeId::Smoke;
                            for (i32 i = 0; i < 7; ++i) {
                                f32 rx = (m_app.m_random.nextFloat() * 2.0f - 1.0f) * entityWidth;
                                f32 ry = m_app.m_random.nextFloat() * entityHeight + 0.5f;
                                f32 rz = (m_app.m_random.nextFloat() * 2.0f - 1.0f) * entityWidth;
                                glm::vec3 particlePos = entityPos + glm::vec3(rx, ry, rz);
                                glm::vec3 velocity(m_app.m_random.nextGaussian(0.0f, 0.02f),
                                    m_app.m_random.nextGaussian(0.0f, 0.02f),
                                    m_app.m_random.nextGaussian(0.0f, 0.02f));
                                m_app.m_world.particleManager()->addPendingParticle(
                                    particleType, particlePos, velocity, &m_app.m_world);
                            }
                        }
                        break;
                    }
                    case static_cast<u8>(EntityStatus::TeleportParticles): {
                        if (m_app.m_world.particleManager() != nullptr) {
                            for (i32 i = 0; i < 16; ++i) {
                                f32 rx = m_app.m_random.nextFloat(-1.0f, 1.0f);
                                f32 ry = m_app.m_random.nextFloat(0.0f, 2.0f);
                                f32 rz = m_app.m_random.nextFloat(-1.0f, 1.0f);
                                glm::vec3 particlePos = entityPos + glm::vec3(rx * 0.5f, ry, rz * 0.5f);
                                glm::vec3 velocity(m_app.m_random.nextGaussian(0.0f, 0.05f),
                                    m_app.m_random.nextGaussian(0.0f, 0.05f),
                                    m_app.m_random.nextGaussian(0.0f, 0.05f));
                                m_app.m_world.particleManager()->addPendingParticle(
                                    ::mc::particle::ParticleTypeId::Portal, particlePos, velocity, &m_app.m_world);
                            }
                        }
                        break;
                    }
                    case static_cast<u8>(EntityStatus::IronGolemAttack): {
                        if (entity != nullptr) {
                            const std::string& typeId = entity->getTypeId();
                            if (typeId == "minecraft:hoglin" || typeId == "hoglin" || typeId == "minecraft:zoglin" ||
                                typeId == "zoglin") {
                                entity->setFlingAnimationTicks(10);
                            } else {
                                entity->setIronGolemAttackTimer(10);
                                entity->setIronGolemArmsRaised(true);
                            }
                        }
                        if (m_app.m_audioService && entity != nullptr) {
                            const std::string& typeId = entity->getTypeId();
                            if (typeId == "minecraft:hoglin" || typeId == "hoglin") {
                                auto sound = mc::client::sound::SoundInstance::createLocated(
                                    mc::SoundEvents::ENTITY_HOGLIN_ATTACK,
                                    mc::sound::SoundCategory::Hostile,
                                    entityPos.x,
                                    entityPos.y,
                                    entityPos.z,
                                    1.0f,
                                    1.0f);
                                m_app.m_audioService->play(
                                    std::make_unique<mc::client::sound::SoundInstance>(std::move(sound)));
                            } else if (typeId == "minecraft:zoglin" || typeId == "zoglin") {
                                auto sound = mc::client::sound::SoundInstance::createLocated(
                                    mc::SoundEvents::ENTITY_ZOGLIN_ATTACK,
                                    mc::sound::SoundCategory::Hostile,
                                    entityPos.x,
                                    entityPos.y,
                                    entityPos.z,
                                    1.0f,
                                    1.0f);
                                m_app.m_audioService->play(
                                    std::make_unique<mc::client::sound::SoundInstance>(std::move(sound)));
                            } else {
                                auto sound = mc::client::sound::SoundInstance::createLocated(
                                    mc::SoundEvents::ENTITY_IRON_GOLEM_ATTACK,
                                    mc::sound::SoundCategory::Neutral,
                                    entityPos.x,
                                    entityPos.y,
                                    entityPos.z,
                                    1.0f,
                                    1.0f);
                                m_app.m_audioService->play(
                                    std::make_unique<mc::client::sound::SoundInstance>(std::move(sound)));
                            }
                        }
                        break;
                    }
                    case static_cast<u8>(EntityStatus::IronGolemHoldRose): {
                        if (entity != nullptr) {
                            entity->setIronGolemHoldingRose(true);
                        }
                        break;
                    }
                    case static_cast<u8>(EntityStatus::IronGolemStopRose): {
                        if (entity != nullptr) {
                            entity->setIronGolemHoldingRose(false);
                        }
                        break;
                    }
                    case static_cast<u8>(EntityStatus::VillagerHeart):
                    case static_cast<u8>(EntityStatus::VillagerAngry):
                    case static_cast<u8>(EntityStatus::VillagerHappy):
                    case static_cast<u8>(EntityStatus::VillagerSplash): {
                        if (m_app.m_world.particleManager() != nullptr) {
                            ::mc::particle::ParticleTypeId ptype = ::mc::particle::ParticleTypeId::Heart;
                            if (status == static_cast<u8>(EntityStatus::VillagerAngry)) {
                                ptype = ::mc::particle::ParticleTypeId::AngryVillager;
                            } else if (status == static_cast<u8>(EntityStatus::VillagerHappy)) {
                                ptype = ::mc::particle::ParticleTypeId::HappyVillager;
                            } else if (status == static_cast<u8>(EntityStatus::VillagerSplash)) {
                                ptype = ::mc::particle::ParticleTypeId::Splash;
                            }
                            for (i32 i = 0; i < 5; ++i) {
                                f32 rx = (m_app.m_random.nextFloat() - 0.5f) * 2.0f;
                                f32 ry = m_app.m_random.nextFloat();
                                f32 rz = (m_app.m_random.nextFloat() - 0.5f) * 2.0f;
                                glm::vec3 particlePos = entityPos + glm::vec3(rx * 0.3f, ry + 1.0f, rz * 0.3f);
                                glm::vec3 velocity(m_app.m_random.nextGaussian(0.0f, 0.02f),
                                    m_app.m_random.nextGaussian(0.0f, 0.02f),
                                    m_app.m_random.nextGaussian(0.0f, 0.02f));
                                m_app.m_world.particleManager()->addPendingParticle(
                                    ptype, particlePos, velocity, &m_app.m_world);
                            }
                        }
                        break;
                    }
                    case static_cast<u8>(EntityStatus::EquipmentBreakMainHand):
                    case static_cast<u8>(EntityStatus::EquipmentBreakOffHand):
                    case static_cast<u8>(EntityStatus::EquipmentBreakHead):
                    case static_cast<u8>(EntityStatus::EquipmentBreakChest):
                    case static_cast<u8>(EntityStatus::EquipmentBreakLegs):
                    case static_cast<u8>(EntityStatus::EquipmentBreakFeet): {
                        if (m_app.m_audioService) {
                            auto sound =
                                mc::client::sound::SoundInstance::createLocated(mc::SoundEvents::ENTITY_ITEM_BREAK,
                                    mc::sound::SoundCategory::Players,
                                    entityPos.x,
                                    entityPos.y,
                                    entityPos.z,
                                    0.8f,
                                    0.8f + m_app.m_random.nextFloat() * 0.4f);
                            m_app.m_audioService->play(
                                std::make_unique<mc::client::sound::SoundInstance>(std::move(sound)));
                        }
                        if (m_app.m_world.particleManager() != nullptr) {
                            for (i32 i = 0; i < 10; ++i) {
                                f32 rx = m_app.m_random.nextFloat() * 2.0f - 1.0f;
                                f32 ry = m_app.m_random.nextFloat() * 2.0f;
                                f32 rz = m_app.m_random.nextFloat() * 2.0f - 1.0f;
                                glm::vec3 particlePos = entityPos + glm::vec3(rx * 0.3f, ry * 0.6f + 0.5f, rz * 0.3f);
                                glm::vec3 velocity(m_app.m_random.nextGaussian(0.0f, 0.05f),
                                    m_app.m_random.nextGaussian(0.0f, 0.05f) + 0.1f,
                                    m_app.m_random.nextGaussian(0.0f, 0.05f));
                                m_app.m_world.particleManager()->addPendingParticle(
                                    ::mc::particle::ParticleTypeId::Breaking, particlePos, velocity, &m_app.m_world);
                            }
                        }
                        break;
                    }
                    default: {
                        i32 permLevel = toPermissionLevel(status);
                        if (permLevel >= 0) {
                            if (m_app.m_localIdentity.isLocalPlayerEntity(static_cast<EntityInstanceId>(entityId))) {
                                if (m_app.m_player) {
                                    spdlog::info("Permission level changed to {}", permLevel);
                                    m_app.m_player->setPermissionLevel(permLevel);
                                }
                            }
                        }
                        break;
                    }
                }
                return Result<void>::ok();
            }
            // ---- 粒子（1.21.11 LevelParticles，ParticleOptions 已结构化）----
            else if constexpr (std::is_same_v<T, irplay::LevelParticles>) {
                using namespace mc::particle;
                using namespace mc::client::renderer::trident::particle::data;
                const auto& p = pkt;
                const ParticleTypeId type = p.particle.type;
                if (type == ParticleTypeId::Invalid) {
                    return Result<void>::ok();
                }

                // 位置/偏移/count 直接取自外层字段；速度由粒子管理器按偏移+maxSpeed 自决，
                // 故 velocity 传零（与 Java 客户端 level.addParticle(...,xd,yd,zd,speed) 语义一致）。
                const Vector3 pos(static_cast<f32>(p.x), static_cast<f32>(p.y), static_cast<f32>(p.z));
                const Vector3 offset(p.xDist, p.yDist, p.zDist);
                const Vector3 zeroVel(0.0f, 0.0f, 0.0f);

                auto& world = m_app.m_world;
                if (requiresBlockState(type)) {
                    // BlockParticleOption：blockStateId → BlockState → BlockParticleData
                    const BlockState* state = BlockRegistry::instance().getBlockState(p.particle.blockStateId);
                    if (state != nullptr) {
                        auto data = std::make_unique<BlockParticleData>(type, *state);
                        world.addParticleWithData(type, pos, zeroVel, std::move(data));
                    }
                } else if (requiresItemData(type)) {
                    // ItemParticleOption：ItemStack wire → ItemStack → ItemParticleData
                    auto stackResult = mc::network::ir::fromItemStackView(p.particle.item);
                    if (stackResult.success()) {
                        auto data = std::make_unique<ItemParticleData>(type, stackResult.value());
                        world.addParticleWithData(type, pos, zeroVel, std::move(data));
                    }
                } else if (type == ParticleTypeId::Dust || type == ParticleTypeId::Redstone) {
                    auto data = std::make_unique<DustParticleData>(p.particle.color, p.particle.scale);
                    world.addParticleWithData(type, pos, zeroVel, std::move(data));
                } else if (type == ParticleTypeId::DustColorTransition) {
                    auto data = std::make_unique<DustColorTransitionParticleData>(
                        p.particle.fromColor, p.particle.toColor, p.particle.scale);
                    world.addParticleWithData(type, pos, zeroVel, std::move(data));
                } else if (type == ParticleTypeId::EntityEffect || type == ParticleTypeId::Flash ||
                    type == ParticleTypeId::TintedLeaves) {
                    auto data = std::make_unique<EntityEffectParticleData>(p.particle.color);
                    world.addParticleWithData(type, pos, zeroVel, std::move(data));
                } else if (type == ParticleTypeId::Vibration) {
                    if (p.particle.vibrationSourceKind == 0) {
                        // 方块目标：packed 坐标 → 方块中心 Vector3d
                        const BlockPos bp = BlockPos::fromLong(p.particle.vibrationBlockPosPacked);
                        const Vector3d target(bp.x + 0.5, bp.y + 0.5, bp.z + 0.5);
                        auto data = std::make_unique<VibrationParticleData>(target, p.particle.arrivalInTicks);
                        world.addParticleWithData(type, pos, zeroVel, std::move(data));
                    } else {
                        auto data = std::make_unique<VibrationParticleData>(
                            static_cast<EntityInstanceId>(p.particle.vibrationEntityId),
                            p.particle.vibrationYOffset,
                            p.particle.arrivalInTicks);
                        world.addParticleWithData(type, pos, zeroVel, std::move(data));
                    }
                } else if (type == ParticleTypeId::Trail) {
                    const Vector3d target(p.particle.trailTargetX, p.particle.trailTargetY, p.particle.trailTargetZ);
                    auto data = std::make_unique<TrailParticleData>(target, p.particle.color, p.particle.trailDuration);
                    world.addParticleWithData(type, pos, zeroVel, std::move(data));
                } else {
                    // SimpleParticleType（无 options）：走普通粒子批量生成（按 count/offset 扇出）
                    world.addParticle(type, pos, zeroVel, offset, static_cast<u32>(p.count));
                }
                return Result<void>::ok();
            }
            // ---- 爆炸（1.21.11 结构化：center/radius/blockCount/knockback/particle/sound）----
            else if constexpr (std::is_same_v<T, irplay::Explosion>) {
                const auto& p = pkt;
                const Vector3 center(
                    static_cast<f32>(p.centerX), static_cast<f32>(p.centerY), static_cast<f32>(p.centerZ));
                const Vector3 zeroVel(0.0f, 0.0f, 0.0f);
                const Vector3 zeroOffset(0.0f, 0.0f, 0.0f);

                // 1. 生成爆炸主粒子（对应 Java level.addParticle(EXPLOSION, x,y,z,...)）
                auto& world = m_app.m_world;
                const ::mc::particle::ParticleTypeId particleType = p.explosionParticle.type;
                if (particleType != ::mc::particle::ParticleTypeId::Invalid) {
                    world.addParticle(particleType, center, zeroVel, zeroOffset, 1u);
                }

                // 2. 播放爆炸音效（对应 Java level.playLocalSound(x,y,z, GENERIC_EXPLODE, ...)）
                // 服务端用内联 SoundEvent 发送 "minecraft:entity.generic.explode"，此处复现。
                if (m_app.m_audioService && p.explosionSound.direct) {
                    auto sound =
                        mc::client::sound::SoundInstance::createLocated(mc::SoundEvents::ENTITY_GENERIC_EXPLODE,
                            mc::sound::SoundCategory::Blocks,
                            center.x,
                            center.y,
                            center.z,
                            4.0f, // 爆炸音效默认音量（MC Java GENERIC_EXPLODE 衰减因子 4.0）
                            1.0f);
                    m_app.m_audioService->play(std::make_unique<mc::client::sound::SoundInstance>(std::move(sound)));
                }

                // 3. 应用本地玩家击退（对应 Java player.addDeltaMovement(knockback)）。
                //    击退仅对本地玩家生效（服务端已按 targetPlayerId 过滤），客户端累加到现有速度。
                if (p.hasPlayerKnockback && m_app.m_player) {
                    m_app.m_player->addVelocity(
                        static_cast<f32>(p.knockbackX), static_cast<f32>(p.knockbackY), static_cast<f32>(p.knockbackZ));
                }

                return Result<void>::ok();
            }
            // ---- 地图数据（结构化，对齐 1.21.11 ClientboundMapItemDataPacket）----
            else if constexpr (std::is_same_v<T, irplay::MapItemData>) {
                const auto& p = pkt;
                // 还原 MapData 到客户端缓存，再刷新 MapRenderer 纹理。
                if (m_app.m_mapDataCache != nullptr && m_app.m_mapRenderer != nullptr) {
                    if (m_app.m_mapDataCache->apply(p)) {
                        m_app.m_mapRenderer->updateMapTexture(p.mapId, *m_app.m_mapDataCache->getMapData(p.mapId));
                    }
                }
                return Result<void>::ok();
            }
            // ---- 睡眠（走 SetEntityData metadata，无独立包）----
            // ---- 生命/饥饿/饱食度同步 ----
            else if constexpr (std::is_same_v<T, irplay::SetHealth>) {
                const auto& p = pkt;
                if (m_app.m_player) {
                    m_app.m_player->setHealth(p.health);
                    m_app.m_player->foodStats().setFoodLevel(p.food);
                    m_app.m_player->foodStats().setSaturationLevel(p.saturation);
                }
                return Result<void>::ok();
            }
            // ---- 视野半径 ----
            else if constexpr (std::is_same_v<T, irplay::SetChunkCacheRadius>) {
                const auto& p = pkt;
                m_app.m_world.setRenderDistance(p.radius);
                return Result<void>::ok();
            }
            // ---- 模拟距离（对齐 Java ClientLevel.setServerSimulationDistance，仅存字段）----
            else if constexpr (std::is_same_v<T, irplay::SetSimulationDistance>) {
                const auto& p = pkt;
                m_app.m_world.setSimulationDistance(p.simulationDistance);
                return Result<void>::ok();
            }
            // ---- 服务端 ping(common 通道)，回 sb:44 pong 同 id ----
            else if constexpr (std::is_same_v<T, irplay::ClientboundPing>) {
                const auto& p = pkt;
                if (m_app.m_network) {
                    (void)m_app.m_network->send(
                        mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
                            mc::network::ir::PlayPacket{irplay::ServerboundPong{p.id}}});
                }
                return Result<void>::ok();
            }
            // ---- PongResponse(ping 协议通道，sb:37 往返收尾) ----
            else if constexpr (std::is_same_v<T, irplay::PongResponse>) {
                const auto& p = pkt;
                // RTT = 收包时刻 - 出站 ping 时记录的 time，对齐 Java PingDebugMonitor.onPongReceived。
                // time 由客户端发 ServerboundPingRequest 时写入（TimeUtils::getCurrentTimeMs 单调时钟毫秒）。
                if (m_app.m_network) {
                    const i64 rtt = static_cast<i64>(mc::util::TimeUtils::getCurrentTimeMs()) - p.time;
                    m_app.m_network->setPingMs(rtt);
                }
                return Result<void>::ok();
            }
            // ---- 默认：未处理包静默忽略 ----
            else {
                return Result<void>::ok();
            }
        },
        *play);
}

} // namespace mc::client::net
