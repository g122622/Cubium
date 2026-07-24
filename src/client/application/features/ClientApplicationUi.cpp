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
#include "client/ui/minecraft/screens/CreativeScreen.hpp"
#include "client/ui/minecraft/screens/InventoryScreen.hpp"
#include "client/ui/minecraft/widgets/ChatWidget.hpp"
#include "client/ui/minecraft/widgets/ScreenStackWidget.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/inventory/container/ItemPickerMenu.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "server/menu/CraftingMenu.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>

using namespace mc::trace;

namespace mc::client {

namespace {

namespace irplay = mc::network::ir::play;

/// ItemStack → 1.21.11 HashedStack（仅 itemId+count，组件哈希留 TODO(Phase6)）。
/// 空栈 present=false。出站 ContainerClick 的 carriedItem 用。
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
    hs.itemId = stack.getItem()->itemId();
    hs.count = stack.getCount();
    return hs;
}

/// 构造 ContainerClick IR 包。stateId 由调用方传入（取自最近 ContainerSetContent/SetSlot）；
/// changedSlots 客户端发送点击时为空（服务端回算）。cursorItem→carriedItem。
mc::network::ir::IrPacket makeContainerClickPacket(
    ContainerId containerId, i32 stateId, i32 slotIndex, i32 button, ClickAction action, const ItemStack& cursorItem)
{
    irplay::ContainerClick click;
    click.containerId = static_cast<i32>(containerId);
    click.stateId = stateId;
    click.slotNum = static_cast<i16>(slotIndex);
    click.buttonNum = static_cast<i8>(button);
    click.clickType = static_cast<i32>(action); // ClickAction 值与 1.21.11 clickType 一致
    click.changedSlots = {};
    click.carriedItem = toHashedStack(cursorItem);
    return mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{irplay::ContainerClick{std::move(click)}}};
}

/// 构造 ContainerClose IR 包。
mc::network::ir::IrPacket makeContainerClosePacket(ContainerId containerId)
{
    irplay::ContainerClose close;
    close.containerId = static_cast<i32>(containerId);
    return mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{irplay::ContainerClose{std::move(close)}}};
}

/// 构造 PlayerCommand（OPEN_INVENTORY，action=5）IR 包。entityId 取本地玩家。
/// 旧 sendOpenPlayerInventory 等价：通知服务端在 containerId=0 建菜单。
mc::network::ir::IrPacket makeOpenPlayerInventoryPacket(i32 playerId)
{
    irplay::PlayerCommand cmd;
    cmd.entityId = playerId;
    cmd.action = 5; // OPEN_INVENTORY
    cmd.data = 0;
    return mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{irplay::PlayerCommand{std::move(cmd)}}};
}

/// 1.21.11 PlayerInput 位掩码：bit0=forward bit1=backward bit2=left bit3=right
/// bit4=jump bit5=shift bit6=sprint。旧 sendPlayerInput(strafe,forward,jumping,sneaking) 映射。
u8 packPlayerInput(f32 strafe, f32 forward, bool jumping, bool sneaking)
{
    u8 v = 0;
    if (forward > 0.0f) v |= 0x01; // forward
    if (forward < 0.0f) v |= 0x02; // backward
    if (strafe < 0.0f) v |= 0x04;  // left（A）
    if (strafe > 0.0f) v |= 0x08;  // right（D）
    if (jumping) v |= 0x10;        // jump
    if (sneaking) v |= 0x20;       // shift
    return v;
}

} // namespace

[[nodiscard]] bool ClientApplication::isCreativeModeActive() const
{
    if (!m_player) {
        return false;
    }

    return m_player->gameMode() == GameMode::Creative || m_player->abilities().creativeMode;
}

void ClientApplication::openInventoryScreen()
{
    if (!m_player) {
        return;
    }

    auto clickSender = [this](ContainerId containerId,
                           i32 slotIndex,
                           i32 button,
                           i16 transactionId,
                           ClickAction action,
                           const ItemStack& cursorItem) {
        MC_UNUSED(transactionId);
        if (m_network) {
            // TODO(Phase6): stateId 需取自最近 ContainerSetContent/SetSlot，当前填 0。
            (void)m_network->send(makeContainerClickPacket(containerId, 0, slotIndex, button, action, cursorItem));
        }
    };

    auto closeSender = [this](ContainerId containerId) {
        if (m_network) {
            (void)m_network->send(makeContainerClosePacket(containerId));
        }
    };

    auto menu = std::make_unique<mc::InventoryCraftingMenu>(inventory::PLAYER_CONTAINER_ID, &m_player->inventory());

    auto inventoryScreen = std::make_unique<ui::minecraft::InventoryScreen>(std::move(menu), clickSender, closeSender);

    if (m_renderer && m_renderer->isGuiRendererInitialized()) {
        inventoryScreen->setRenderers(&m_renderer->guiRenderer(),
            m_guiTextureManager.get(),
            m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
        inventoryScreen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
    }

    // 通知服务端在 containerId=0 上建立 InventoryCraftingMenu，使后续
    // ContainerClickPacket 能被正确受理（修复点击静默丢弃）。
    if (m_network && m_player) {
        (void)m_network->send(makeOpenPlayerInventoryPacket(m_player->playerId()));
    }

    ScreenManager::instance().openScreen(std::move(inventoryScreen));
}

void ClientApplication::openCreativeScreen()
{
    if (!m_player) {
        return;
    }

    auto clickSender = [this](ContainerId containerId,
                           i32 slotIndex,
                           i32 button,
                           i16 transactionId,
                           ClickAction action,
                           const ItemStack& cursorItem) {
        MC_UNUSED(transactionId);
        if (m_network) {
            // TODO(Phase6): stateId 需取自最近 ContainerSetContent/SetSlot，当前填 0。
            (void)m_network->send(makeContainerClickPacket(containerId, 0, slotIndex, button, action, cursorItem));
        }
    };

    auto closeSender = [this](ContainerId containerId) {
        if (m_network) {
            (void)m_network->send(makeContainerClosePacket(containerId));
        }
    };

    // 创造屏复用 containerId=0（PLAYER_CONTAINER_ID），客户端构造 ItemPickerMenu
    // （不加载物品池：调色板渲染由 CreativeScreen 本地 buildCreativePaletteEntries 提供，
    // clone 取物由服务端 ItemPickerMenu 处理，carried 经 ContainerContentPacket 回传）。
    auto menu = std::make_unique<mc::ItemPickerMenu>(inventory::PLAYER_CONTAINER_ID, &m_player->inventory(), false);

    auto creativeScreen = std::make_unique<ui::minecraft::CreativeScreen>(std::move(menu), clickSender, closeSender);
    if (m_renderer && m_renderer->isGuiRendererInitialized()) {
        creativeScreen->setRenderers(&m_renderer->guiRenderer(),
            m_guiTextureManager.get(),
            m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
        creativeScreen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
    }

    // 通知服务端在 containerId=0 上建立 ItemPickerMenu（创造模式分流），使后续
    // ContainerClickPacket（含调色板 Clone）能被正确受理。
    if (m_network && m_player) {
        (void)m_network->send(makeOpenPlayerInventoryPacket(m_player->playerId()));
    }

    ScreenManager::instance().openScreen(std::move(creativeScreen));
}

void ClientApplication::closeInventoryScreenIfModeMismatch()
{
    if (!ScreenManager::instance().hasScreen() || !m_player) {
        return;
    }

    ui::minecraft::Screen* currentKageroScreen = ScreenManager::instance().getCurrentKageroScreen();
    const bool creativeMode = isCreativeModeActive();
    const bool isInventoryScreen = dynamic_cast<ui::minecraft::InventoryScreen*>(currentKageroScreen) != nullptr;
    const bool isCreativeScreen = dynamic_cast<ui::minecraft::CreativeScreen*>(currentKageroScreen) != nullptr;

    if ((isCreativeScreen && !creativeMode) || (isInventoryScreen && creativeMode)) {
        ScreenManager::instance().closeScreen();
        if (!ScreenManager::instance().hasScreen()) {
            mc::client::application::features::captureMouseAfterScreens(m_input, m_mouseCaptured);
        }
    }
}

void ClientApplication::handleEvents()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "ClientApplication::handleEvents");

    m_window.pollEvents();
    m_input.update();

    if (handleUiOverlayInput()) {
        return;
    }

    handleGameplayInput();
}

[[nodiscard]] bool ClientApplication::handleUiOverlayInput()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "ClientApplication::handleUiOverlayInput");

    // 处理聊天框键盘输入（优先于游戏输入）
    // 检查聊天框是否打开
    auto* chatWidget = m_kageroEngine
        ? static_cast<ui::minecraft::widgets::ChatWidget*>(m_kageroEngine->getLayer(m_chatLayerId))
        : nullptr;
    if (chatWidget && chatWidget->isOpen()) {
        // 聊天框打开时，ESC 关闭聊天框
        if (m_input.isKeyJustPressed(GLFW_KEY_ESCAPE)) {
            chatWidget->close();
            m_input.setMouseLocked(true);
            m_mouseCaptured = true;
        }
        return true;
    }

    // 处理 Screen 栈事件
    auto* screenStack = m_kageroEngine
        ? static_cast<ui::minecraft::widgets::ScreenStackWidget*>(m_kageroEngine->getLayer(m_screenStackLayerId))
        : nullptr;
    if (screenStack && screenStack->hasScreen()) {
        const i32 guiMouseX =
            static_cast<i32>(m_input.mouseX() / static_cast<f64>(std::max(m_guiScaleState.scaleFactor, 1)));
        const i32 guiMouseY =
            static_cast<i32>(m_input.mouseY() / static_cast<f64>(std::max(m_guiScaleState.scaleFactor, 1)));

        if (m_input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            m_kageroEngine->handleClick(guiMouseX, guiMouseY, GLFW_MOUSE_BUTTON_LEFT, m_input.currentMods());
        }
        if (m_input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
            m_kageroEngine->handleClick(guiMouseX, guiMouseY, GLFW_MOUSE_BUTTON_RIGHT, m_input.currentMods());
        }
        if (m_input.isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_LEFT)) {
            m_kageroEngine->handleRelease(guiMouseX, guiMouseY, GLFW_MOUSE_BUTTON_LEFT, m_input.currentMods());
        }
        if (m_input.isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_RIGHT)) {
            m_kageroEngine->handleRelease(guiMouseX, guiMouseY, GLFW_MOUSE_BUTTON_RIGHT, m_input.currentMods());
        }

        const f64 screenScrollDelta = m_input.scrollDeltaY();
        if (screenScrollDelta != 0.0 && m_kageroEngine) {
            m_kageroEngine->handleScroll(guiMouseX, guiMouseY, screenScrollDelta);
        }

        // 转发鼠标移动事件以更新控件悬停状态
        if (m_kageroEngine) {
            m_kageroEngine->handleMouseMove(guiMouseX, guiMouseY);
        }

        if (!screenStack->hasScreen()) {
            mc::client::application::features::captureMouseAfterScreens(m_input, m_mouseCaptured);
        }
        return true;
    }

    return false;
}

void ClientApplication::handleGameplayInput()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "ClientApplication::handleGameplayInput");

    auto* chatWidget = m_kageroEngine
        ? static_cast<ui::minecraft::widgets::ChatWidget*>(m_kageroEngine->getLayer(m_chatLayerId))
        : nullptr;

    if (handleGameplayShortcutInput(chatWidget)) {
        return;
    }

    handleMouseAndMovementInput();
}

bool ClientApplication::handleGameplayShortcutInput(ui::minecraft::widgets::ChatWidget* chatWidget)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "ClientApplication::handleGameplayShortcutInput");

    // 检查ALT键切换鼠标捕获
    if (m_input.isKeyJustPressed(GLFW_KEY_LEFT_ALT) || m_input.isKeyJustPressed(GLFW_KEY_RIGHT_ALT)) {
        toggleMouseCapture();
    }

    // T 键打开聊天框
    if (m_input.isKeyJustPressed(GLFW_KEY_T)) {
        if (chatWidget) {
            chatWidget->open(false);
            if (m_mouseCaptured) {
                m_input.setMouseLocked(false);
                m_mouseCaptured = false;
            }
        }
        return true;
    }

    // / 键打开命令框
    if (m_input.isKeyJustPressed(GLFW_KEY_SLASH)) {
        if (chatWidget) {
            chatWidget->open(true);
            if (m_mouseCaptured) {
                m_input.setMouseLocked(false);
                m_mouseCaptured = false;
            }
        }
        return true;
    }

    if (m_input.isKeyJustPressed(GLFW_KEY_E) && m_player) {
        mc::client::application::features::releaseMouseForScreen(m_input, m_mouseCaptured);
        if (isCreativeModeActive()) {
            openCreativeScreen();
        } else {
            openInventoryScreen();
        }
        return true;
    }

    // 飞行模式切换（F键）
    if (m_input.isKeyJustPressed(GLFW_KEY_F)) {
        if (m_player && m_player->abilities().canFly) {
            m_player->toggleFlying();
        }
    }

    return false;
}

void ClientApplication::handleMouseAndMovementInput()
{
    if (!(m_mouseCaptured && m_player)) {
        return;
    }

    // 传递键盘输入到玩家和鼠标控制
    // 鼠标视角控制 - 更新玩家朝向（使用设置中的灵敏度）
    // InputManager 返回 f64 (GLFW)，转换为 f32 用于内部计算
    f32 sensitivity = m_settings.mouseSensitivity.get() * 0.2f;
    f32 deltaYaw = static_cast<f32>(m_input.mouseDeltaX()) * sensitivity;
    f32 deltaPitch = static_cast<f32>(m_input.mouseDeltaY()) * sensitivity;
    m_player->rotate(deltaYaw, -deltaPitch); // pitch方向相反

    // 收集移动输入
    f32 forward = 0.0f;
    f32 strafe = 0.0f;
    bool jumping = false;
    bool sneaking = false;

    if (m_input.isKeyPressed(GLFW_KEY_W)) forward += 1.0f;
    if (m_input.isKeyPressed(GLFW_KEY_S)) forward -= 1.0f;
    if (m_input.isKeyPressed(GLFW_KEY_A)) strafe -= 1.0f;
    if (m_input.isKeyPressed(GLFW_KEY_D)) strafe += 1.0f;
    if (m_input.isKeyPressed(GLFW_KEY_SPACE)) jumping = true;
    if (m_input.isKeyPressed(GLFW_KEY_LEFT_SHIFT)) sneaking = true;

    // 检查玩家是否正在骑乘
    if (m_player->isRiding() && m_network && m_network->isPlaying()) {
        // 骑乘状态：发送 PlayerInput（u8 位掩码）到服务器
        irplay::PlayerInput playerInput;
        playerInput.input = packPlayerInput(strafe, forward, jumping, sneaking);
        (void)m_network->send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{irplay::PlayerInput{std::move(playerInput)}}});

        // 同时发送 ServerboundMoveVehicle 同步载具位置
        const auto& pos = m_player->position();
        irplay::ServerboundMoveVehicle moveVehicle;
        moveVehicle.x = static_cast<f64>(pos.x);
        moveVehicle.y = static_cast<f64>(pos.y);
        moveVehicle.z = static_cast<f64>(pos.z);
        moveVehicle.yRot = m_player->yaw();
        moveVehicle.xRot = m_player->pitch();
        moveVehicle.onGround = m_player->onGround();
        (void)m_network->send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{irplay::ServerboundMoveVehicle{std::move(moveVehicle)}}});

        // 如果骑乘的是船，发送划桨状态
        // 划桨状态：左桨 = 左移或前进，右桨 = 右移或前进
        EntityInstanceId vehicleId = m_player->getVehicle();
        if (vehicleId != INVALID_ENTITY_ID) {
            ClientEntity* vehicle = m_world.entityManager().getEntity(vehicleId);
            if (vehicle != nullptr && vehicle->entityType() == mc::entity::VanillaEntityTypeKeys::BOAT) {
                bool leftPaddle = (strafe < 0.0f) || (forward > 0.0f);  // A or W
                bool rightPaddle = (strafe > 0.0f) || (forward > 0.0f); // D or W
                irplay::PaddleBoat paddle;
                paddle.left = leftPaddle;
                paddle.right = rightPaddle;
                (void)m_network->send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
                    mc::network::ir::PlayPacket{irplay::PaddleBoat{std::move(paddle)}}});
            }
        }
    } else {
        // 正常状态：传递输入给玩家进行本地移动
        m_player->handleMovementInput(forward, strafe, jumping, sneaking);
    }

    // 滚轮选择物品栏槽位（scrollDeltaY 返回 f64）
    const f64 scrollDelta = m_input.scrollDeltaY();
    if (scrollDelta != 0.0) {
        i32 selectedSlot = m_player->inventory().getSelectedSlot();
        i32 delta = scrollDelta > 0.0 ? -1 : 1;
        selectedSlot = (selectedSlot + delta + PlayerInventory::HOTBAR_SIZE) % PlayerInventory::HOTBAR_SIZE;
        m_player->inventory().setSelectedSlot(selectedSlot);
        if (m_network && m_network->isPlaying()) {
            // 1.21.11 SetCarriedItem：切热栏槽（slot 0..8）
            irplay::SetCarriedItem carried;
            carried.slot = static_cast<i16>(selectedSlot);
            (void)m_network->send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{irplay::SetCarriedItem{std::move(carried)}}});
        }
    }
}

} // namespace mc::client
