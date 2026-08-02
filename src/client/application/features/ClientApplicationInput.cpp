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
#include "client/application/features/ClientApplicationHelpers.hpp"

#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/GameActions.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/world/block/Block.hpp"

#include "client/renderer/trident/block/BreakProgressManager.hpp"
#include "client/ui/minecraft/widgets/ChatWidget.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "common/item/items/map/FilledMapItem.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

using namespace mc::trace;

namespace {

enum class MiningInputState : mc::i32 {
    Active = 0,
    NoMouseOrPlayer,
    AttackNotPressed,
    NoTargetBlock,
    InvalidTargetState,
};

} // namespace

namespace mc::client {

void ClientApplication::setupInputBindings()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "SetupInputBindings");

    m_input.bindKeyAction(GLFW_KEY_ESCAPE, "exit");

    m_input.bindActionCallback("exit", [this]() {
        auto* chatWidget = m_kageroEngine
            ? static_cast<ui::minecraft::widgets::ChatWidget*>(m_kageroEngine->getLayer(m_chatLayerId))
            : nullptr;

        // 如果聊天框打开，关闭聊天框
        if (chatWidget && chatWidget->isOpen()) {
            return;
        }

        // 如果有屏幕打开，关闭屏幕
        if (ScreenManager::instance().hasScreen()) {
            ScreenManager::instance().closeScreen();
            mc::client::application::features::captureMouseAfterScreens(m_input, m_mouseCaptured);
            return;
        }

        // 根据当前状态决定行为
        switch (m_stateMachine.state()) {
            case ClientAppState::InGame:
                // 游戏中：打开暂停菜单
                showPauseMenu();
                break;

            case ClientAppState::Paused:
                // 暂停菜单中：返回游戏
                if (m_stateMachine.resume()) {
                    // 恢复游戏后重新捕获鼠标
                    if (!m_mouseCaptured) {
                        toggleMouseCapture();
                    }
                }
                break;

            case ClientAppState::MainMenu:
                // 主菜单：退出应用
                spdlog::info("Exit key pressed in main menu");
                stop();
                break;

            case ClientAppState::LoadingWorld:
            case ClientAppState::LeavingWorld:
                // 加载/离开中：忽略 ESC
                break;

            default:
                // 其他状态：停止应用
                spdlog::info(
                    "Exit key pressed in state: {}", ClientAppStateMachine::stateToString(m_stateMachine.state()));
                stop();
                break;
        }
    });
}

void ClientApplication::setupCamera()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "SetupCamera");

    CameraConfig cameraConfig;
    cameraConfig.fov = m_settings.fov.get();
    cameraConfig.aspectRatio = static_cast<f32>(m_window.width()) / static_cast<f32>(m_window.height());
    cameraConfig.nearPlane = 0.1f;
    cameraConfig.farPlane = 1000.0f;
    cameraConfig.moveSpeed = 10.0f;
    cameraConfig.mouseSensitivity = m_settings.mouseSensitivity.get() * 0.2f;
    m_camera = Camera(cameraConfig);

    m_camera.setPosition(8.0f, 50.0f, 8.0f);
    m_camera.setYaw(45.0f);
    m_camera.update(0.0f);

    m_cameraController.setCamera(&m_camera);
}

void ClientApplication::toggleMouseCapture()
{
    m_mouseCaptured = !m_mouseCaptured;
    m_input.setMouseLocked(m_mouseCaptured);
}

void ClientApplication::cancelBreakingBlock()
{
    if (!m_breakingBlockActive) {
        return;
    }

    sendBlockInteraction(network::BlockInteractionAction::AbortDestroyBlock, m_breakingBlockPos, m_breakingBlockFace);

    using namespace mc::client::renderer::trident::block;
    BreakProgressManager::instance().stopBreaking();

    m_breakingBlockActive = false;
    m_breakingBlockProgress = 0.0f;
    m_breakingBlockFace = Direction::None;

    MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Mining,
        "abortBreakingBlock",
        "state",
        static_cast<i32>(MiningInputState::Active),
        "reason",
        "abort local breaking state");
}

void ClientApplication::beginBreakingBlock(
    const BlockPos& currentTargetPos, Direction currentTargetFace, bool attackJustPressed)
{
    m_breakingBlockPos = currentTargetPos;
    m_breakingBlockFace = currentTargetFace;
    m_breakingBlockActive = true;
    m_breakingBlockProgress = 0.0f;

    using namespace mc::client::renderer::trident::block;
    BreakProgressManager::instance().startBreaking(m_breakingBlockPos);

    MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Mining,
        "startBreaking",
        "pos",
        fmt::format("({}, {}, {})", m_breakingBlockPos.x, m_breakingBlockPos.y, m_breakingBlockPos.z),
        "face",
        static_cast<i32>(m_breakingBlockFace),
        "justPressed",
        attackJustPressed,
        [flow = ::perfetto::Flow::ProcessScoped(m_breakingBlockPos.toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    sendBlockInteraction(network::BlockInteractionAction::StartDestroyBlock, m_breakingBlockPos, m_breakingBlockFace);
}

void ClientApplication::completeBreakingBlock(bool instantBreak)
{
    if (!m_breakingBlockActive) {
        return;
    }

    using namespace mc::client::renderer::trident::block;
    BreakProgressManager::instance().stopBreaking();

    if (instantBreak) {
        MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Mining,
            "instantBreak",
            "pos",
            fmt::format("({}, {}, {})", m_breakingBlockPos.x, m_breakingBlockPos.y, m_breakingBlockPos.z),
            "face",
            static_cast<i32>(m_breakingBlockFace));
    } else {
        MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Mining,
            "breakComplete",
            "pos",
            fmt::format("({}, {}, {})", m_breakingBlockPos.x, m_breakingBlockPos.y, m_breakingBlockPos.z),
            "face",
            static_cast<i32>(m_breakingBlockFace),
            "progress",
            m_breakingBlockProgress,
            [flow = ::perfetto::Flow::ProcessScoped(m_breakingBlockPos.toId())](
                ::perfetto::EventContext ctx) { flow(ctx); });
    }

    sendBlockInteraction(network::BlockInteractionAction::StopDestroyBlock, m_breakingBlockPos, m_breakingBlockFace);

    m_breakingBlockActive = false;
    m_breakingBlockProgress = 0.0f;
    m_breakingBlockFace = Direction::None;
}

void ClientApplication::handleBlockMiningInput(f32 deltaTime)
{
    static MiningInputState s_lastMiningState = MiningInputState::Active;
    auto setMiningState = [](MiningInputState state, const char* reason) {
        MC_TRACE_INSTANT_EVENT(
            TraceEvents.Client.Mining, "setMiningState", "state", static_cast<i32>(state), "reason", reason);
        if (s_lastMiningState != state) {
            s_lastMiningState = state;
        }
    };

    if (!m_mouseCaptured || !m_player) {
        setMiningState(MiningInputState::NoMouseOrPlayer, "mouse not captured or player missing");
        cancelBreakingBlock();
        return;
    }

    const bool attackPressed = m_input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
    const bool attackJustPressed = m_input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);
    const bool hasTargetBlock = m_raycastResult.isHit();

    if (!attackPressed) {
        setMiningState(MiningInputState::AttackNotPressed, "attack button released");
        cancelBreakingBlock();
        return;
    }

    if (!hasTargetBlock) {
        setMiningState(MiningInputState::NoTargetBlock, "raycast miss while attack pressed");
        cancelBreakingBlock();
        return;
    }

    setMiningState(MiningInputState::Active, "attack pressed and target hit");

    const BlockPos currentTargetPos = m_raycastResult.blockPos();
    const Direction currentTargetFace = m_raycastResult.face();
    const bool targetChanged =
        !m_breakingBlockActive || currentTargetPos != m_breakingBlockPos || currentTargetFace != m_breakingBlockFace;

    if (targetChanged) {
        cancelBreakingBlock();
        beginBreakingBlock(currentTargetPos, currentTargetFace, attackJustPressed);
    }

    const BlockState* targetState =
        m_world.getBlockState(m_breakingBlockPos.x, m_breakingBlockPos.y, m_breakingBlockPos.z);
    if (targetState == nullptr || targetState->isAir() || targetState->hardness() < 0.0f) {
        setMiningState(MiningInputState::InvalidTargetState, "target state is null/air/unbreakable");
        cancelBreakingBlock();
        return;
    }

    if (m_player->gameMode() == GameMode::Creative || targetState->hardness() == 0.0f) {
        if (!attackJustPressed) {
            return;
        }

        completeBreakingBlock(true);
        return;
    }

    m_breakingBlockProgress += deltaTime * constants::TICK_RATE *
        mc::client::application::features::calculateBlockBreakingDelta(*m_player, *targetState);

    {
        using namespace mc::client::renderer::trident::block;
        BreakProgressManager::instance().updateLocalProgress(m_breakingBlockPos, m_breakingBlockProgress);
    }

    if (m_breakingBlockProgress >= 1.0f) {
        completeBreakingBlock(false);
    }
}

void ClientApplication::handleBlockPlacementInput(f32 deltaTime)
{
    enum class PlaceInputState : i32 {
        Active = 0,
        NoMouseOrPlayer,
        NoUsePress,
        Cooldown,
        RaycastMiss,
    };

    static PlaceInputState s_lastPlaceState = PlaceInputState::Active;
    auto setPlaceState = [](PlaceInputState state, const char* reason) {
        if (s_lastPlaceState != state) {
            s_lastPlaceState = state;
            spdlog::info("[PlaceInput] state={} reason={}", static_cast<i32>(state), reason);
        }
    };

    m_placeCooldown = std::max(0.0f, m_placeCooldown - deltaTime);

    if (!m_mouseCaptured || !m_player) {
        setPlaceState(PlaceInputState::NoMouseOrPlayer, "mouse not captured or player missing");
        return;
    }

    const bool usePressed = m_input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT);
    if (!usePressed) {
        setPlaceState(PlaceInputState::NoUsePress, "right button not just pressed");
        return;
    }

    if (m_placeCooldown > 0.0f) {
        setPlaceState(PlaceInputState::Cooldown, "placement cooldown active");
        return;
    }

    if (m_raycastResult.isMiss()) {
        // 右键空挥：若主手持已填充地图，纯客户端本地打开地图查看屏
        // （1.21.11 原版 MapScreen 是客户端本地开，不走服务端 OpenScreen 下推）。
        const ItemStack mainHand = m_player->getHeldItem(Hand::MainHand);
        if (item::items::FilledMapItem::isFilledMap(mainHand)) {
            const i32 mapId = item::items::FilledMapItem::getMapId(mainHand);
            if (mapId >= 0) {
                // 释放鼠标光标，使 ScreenStackWidget 能接收点击/键盘
                if (m_mouseCaptured) {
                    m_input.setMouseLocked(false);
                    m_mouseCaptured = false;
                }
                openMapScreen(mapId);
                m_placeCooldown = PLACE_COOLDOWN_TIME;
                return;
            }
        }
        // 非地图的右键空挥：发 UseItem 包让服务端执行物品在空气中的使用
        // （对齐 Java MultiPlayerGameMode.useItemOnMainThread 未命中方块时发 ServerboundUseItem）。
        sendUseItem();
        m_placeCooldown = PLACE_COOLDOWN_TIME;
        setPlaceState(PlaceInputState::RaycastMiss, "raycast miss on use");
        return;
    }

    setPlaceState(PlaceInputState::Active, "right button pressed and target hit");

    BlockPos pos = m_raycastResult.blockPos();
    Direction face = m_raycastResult.face();
    Vector3 hitPos = m_raycastResult.hitPosition();
    Vector3 blockPosFloat(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z));
    Vector3 relativeHit = hitPos - blockPosFloat;

    sendBlockPlacement(pos, face, relativeHit);
    m_placeCooldown = PLACE_COOLDOWN_TIME;
}

void ClientApplication::sendBlockPlacement(const BlockPos& pos, Direction face, const Vector3& hitPos)
{
    if (!m_network || !m_network->isPlaying()) {
        spdlog::info("[Place] Skip sending block placement because client is not logged in");
        return;
    }

    spdlog::info("[Place] Send placement pos=({}, {}, {}) face={} hit=({:.2f}, {:.2f}, {:.2f})",
        pos.x,
        pos.y,
        pos.z,
        static_cast<i32>(face),
        hitPos.x,
        hitPos.y,
        hitPos.z);

    // 1.21.11 UseItemOn：hand(0=MAIN_HAND) + BlockHitResult + sequence。
    // inside=true（命中点在方块内），worldBorderHit=false。
    namespace irplay = mc::network::ir::play;
    irplay::UseItemOn useItemOn;
    useItemOn.hand = 0; // MAIN_HAND
    useItemOn.blockHit.blockPosPacked = pos.asLong();
    useItemOn.blockHit.direction = static_cast<i32>(face);
    useItemOn.blockHit.hitX = hitPos.x;
    useItemOn.blockHit.hitY = hitPos.y;
    useItemOn.blockHit.hitZ = hitPos.z;
    useItemOn.blockHit.inside = true;
    useItemOn.blockHit.worldBorderHit = false;
    useItemOn.sequence = 0;
    (void)m_network->send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{irplay::UseItemOn{std::move(useItemOn)}}});
}

void ClientApplication::sendUseItem()
{
    // 右键空挥出站 UseItem（对齐 Java ServerboundUseItemPacket）。
    // 1.21.11 字段：hand(0=MAIN_HAND) + sequence + yRot + xRot。
    if (!m_network || !m_network->isPlaying()) {
        spdlog::info("[UseItem] Skip sending UseItem because client is not logged in");
        return;
    }
    if (!m_player) {
        return;
    }

    namespace irplay = mc::network::ir::play;
    irplay::UseItem useItem;
    useItem.hand = static_cast<i32>(Hand::MainHand);
    useItem.sequence = 0;
    useItem.yRot = m_player->yaw();
    useItem.xRot = m_player->pitch();
    (void)m_network->send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{irplay::UseItem{std::move(useItem)}}});
}

void ClientApplication::sendBlockInteraction(
    network::BlockInteractionAction action, const BlockPos& pos, Direction face)
{
    if (!m_network || !m_network->isPlaying()) {
        return;
    }

    MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Mining,
        "sendBlockInteraction",
        "action",
        static_cast<i32>(action),
        "pos",
        fmt::format("({}, {}, {})", pos.x, pos.y, pos.z),
        "face",
        static_cast<i32>(face),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    // 1.21.11 PlayerAction：action 值与旧 BlockInteractionAction 完全一致
    // （0=START_DESTROY 1=ABORT_DESTROY 2=STOP_DESTROY）。
    namespace irplay = mc::network::ir::play;
    irplay::PlayerAction playerAction;
    playerAction.action = static_cast<i32>(action);
    playerAction.blockPosPacked = pos.asLong();
    playerAction.direction = static_cast<i32>(face);
    playerAction.sequence = 0;
    (void)m_network->send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{irplay::PlayerAction{std::move(playerAction)}}});
}

void ClientApplication::sendPingRequest()
{
    if (!m_network || !m_network->isPlaying()) {
        return;
    }

    // 主动 ping（ping 协议通道 sb:37），对齐 Java PingDebugMonitor.tick。
    // time 取单调时钟毫秒，服务端原样回 PongResponse(cb:60) 同 time，
    // 客户端据此算 RTT 写入 ClientNetwork::m_pingMs（见 ClientPlayVisitor PongResponse 分支）。
    namespace irplay = mc::network::ir::play;
    irplay::ServerboundPingRequest ping;
    ping.time = static_cast<i64>(mc::util::TimeUtils::getCurrentTimeMs());
    (void)m_network->send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{irplay::ServerboundPingRequest{ping}}});
}

void ClientApplication::sendChangeDifficulty(Difficulty difficulty)
{
    if (!m_network || !m_network->isPlaying()) {
        return;
    }

    // 难度切换（sb:3）。仅集成服主机有权限，服务端校验通过后广播 cb:10 回环。
    namespace irplay = mc::network::ir::play;
    irplay::ServerboundChangeDifficulty pkt;
    pkt.difficulty = static_cast<i32>(difficulty);
    (void)m_network->send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{irplay::ServerboundChangeDifficulty{pkt}}});
}

void ClientApplication::sendLockDifficulty()
{
    if (!m_network || !m_network->isPlaying()) {
        return;
    }

    // 难度锁定（sb:28）。锁定不可逆，恒发 locked=true（对齐 Java 仅发 true）。
    namespace irplay = mc::network::ir::play;
    irplay::LockDifficulty pkt;
    pkt.locked = true;
    (void)m_network->send(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{irplay::LockDifficulty{pkt}}});
}

void ClientApplication::sendPlayerPosition()
{
    if (!m_network || !m_network->isPlaying() || !m_player) {
        return;
    }

    const auto& pos = m_player->position();

    bool positionChanged = std::abs(pos.x - m_lastSentX) > 0.001f || std::abs(pos.y - m_lastSentY) > 0.001f ||
        std::abs(pos.z - m_lastSentZ) > 0.001f;

    bool rotationChanged =
        std::abs(m_player->yaw() - m_lastSentYaw) > 0.01f || std::abs(m_player->pitch() - m_lastSentPitch) > 0.01f;

    // 1.21.11 四个 MovePlayer 变体：PosRot/Pos/Rot/StatusOnly，共用 MovePlayerFlags。
    // 直接按 positionChanged/rotationChanged 选择变体，无需额外判别枚举。
    namespace irplay = mc::network::ir::play;
    const irplay::MovePlayerFlags flags{m_player->onGround(), false};
    mc::network::ir::PlayPacket pkt;
    if (positionChanged && rotationChanged) {
        pkt = mc::network::ir::PlayPacket{irplay::MovePlayerPosRot{static_cast<f64>(pos.x),
            static_cast<f64>(pos.y),
            static_cast<f64>(pos.z),
            m_player->yaw(),
            m_player->pitch(),
            flags}};
    } else if (positionChanged) {
        pkt = mc::network::ir::PlayPacket{
            irplay::MovePlayerPos{static_cast<f64>(pos.x), static_cast<f64>(pos.y), static_cast<f64>(pos.z), flags}};
    } else if (rotationChanged) {
        pkt = mc::network::ir::PlayPacket{irplay::MovePlayerRot{m_player->yaw(), m_player->pitch(), flags}};
    } else {
        pkt = mc::network::ir::PlayPacket{irplay::MovePlayerStatusOnly{flags}};
    }
    (void)m_network->send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play, std::move(pkt)});

    m_lastSentX = pos.x;
    m_lastSentY = pos.y;
    m_lastSentZ = pos.z;
    m_lastSentYaw = m_player->yaw();
    m_lastSentPitch = m_player->pitch();
}

} // namespace mc::client