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
#include "client/ui/screen/CraftingScreen.hpp"
#include "client/ui/screen/CreativeScreen.hpp"

#include "client/ui/minecraft/widgets/ChatWidget.hpp"
#include "client/ui/minecraft/widgets/ScreenStackWidget.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/perfetto/TraceEvents.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>

using namespace mc::trace;

namespace mc::client {

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
        if (m_networkClient) {
            m_networkClient->sendContainerClick(
                ContainerClickPacket(containerId, slotIndex, button, transactionId, action, cursorItem));
        }
    };

    auto closeSender = [this](ContainerId containerId) {
        if (m_networkClient) {
            m_networkClient->sendCloseContainer(containerId);
        }
    };

    auto inventoryScreen = std::make_unique<InventoryCraftingScreen>(
        std::make_unique<mc::InventoryCraftingMenu>(inventory::PLAYER_CONTAINER_ID, &m_player->inventory()),
        clickSender,
        closeSender);

    if (m_renderer && m_renderer->isGuiRendererInitialized()) {
        inventoryScreen->setRenderers(&m_renderer->guiRenderer(),
            m_guiTextureManager.get(),
            m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
        inventoryScreen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
    }

    ScreenManager::instance().openScreen(std::move(inventoryScreen));
}

void ClientApplication::openCreativeScreen()
{
    if (!m_player) {
        return;
    }

    auto actionSender = [this](i32 slotIndex, const ItemStack& item) {
        if (m_networkClient) {
            m_networkClient->sendCreativeInventoryAction(CreativeInventoryActionPacket(slotIndex, item));
        }
    };

    auto creativeScreen = std::make_unique<CreativeScreen>(m_player->inventory(), actionSender);
    if (m_renderer && m_renderer->isGuiRendererInitialized()) {
        creativeScreen->setRenderers(&m_renderer->guiRenderer(),
            m_guiTextureManager.get(),
            m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
        creativeScreen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
    }

    ScreenManager::instance().openScreen(std::move(creativeScreen));
}

void ClientApplication::closeInventoryScreenIfModeMismatch()
{
    if (!ScreenManager::instance().hasScreen() || !m_player) {
        return;
    }

    IScreen* currentScreen = ScreenManager::instance().getCurrentScreen();
    const bool creativeMode = isCreativeModeActive();
    const bool isInventoryScreen = dynamic_cast<InventoryCraftingScreen*>(currentScreen) != nullptr;
    const bool isCreativeScreen = dynamic_cast<CreativeScreen*>(currentScreen) != nullptr;

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
    if (m_player->isRiding() && m_networkClient && m_networkClient->isLoggedIn()) {
        // 骑乘状态：发送 PlayerInputPacket 到服务器
        m_networkClient->sendPlayerInput(strafe, forward, jumping, sneaking);

        // 同时发送 MoveVehiclePacket 同步载具位置
        const auto& pos = m_player->position();
        m_networkClient->sendMoveVehicle(pos.x, pos.y, pos.z, m_player->yaw(), m_player->pitch());

        // 如果骑乘的是船，发送划桨状态
        // 划桨状态：左桨 = 左移或前进，右桨 = 右移或前进
        EntityId vehicleId = m_player->getVehicle();
        if (vehicleId != INVALID_ENTITY_ID) {
            ClientEntity* vehicle = m_world.entityManager().getEntity(vehicleId);
            if (vehicle != nullptr && vehicle->typeId() == mc::entity::EntityTypes::BOAT) {
                bool leftPaddle = (strafe < 0.0f) || (forward > 0.0f);  // A or W
                bool rightPaddle = (strafe > 0.0f) || (forward > 0.0f); // D or W
                m_networkClient->sendSteerBoat(leftPaddle, rightPaddle);
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
        if (m_networkClient && m_networkClient->isLoggedIn()) {
            m_networkClient->sendHotbarSelect(selectedSlot);
        }
    }
}

} // namespace mc::client
