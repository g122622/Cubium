/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, without limitation the rights
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

#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"

#include <functional>

namespace mc {
class ItemStack;
using ContainerId = i32;
enum class ClickAction : u8;
} // namespace mc

namespace mc::client {
class ClientApplication;
namespace ui::minecraft::widgets {
class ScreenStackWidget;
}
} // namespace mc::client

namespace mc::client::net {

/**
 * @brief 入站 Play/Configuration 包 visitor：std::visit over ir::PlayPacket / ConfigurationPacket
 *
 * 每个分支体处理一个 IR 包变体，出站经 m_app.m_network->send(ir::play::*)。
 *
 * 入站 57 旧用例 → ~73 IR visitor 分支（5 处拆分：SpawnMob→AddEntity+SetEntityData、
 * EntityAnimation→Animate+HurtAnimation、PlayerListItem→PlayerInfoUpdate+PlayerInfoRemove、
 * EntityMove→MoveEntityPos/PosRot/Rot、Title→5 title struct；2 处内部分流：GameEvent 1→5
 * 回调、LevelParticles 1→8 回调）。出站 17 send 方法 → ir::play serverbound struct。
 *
 * 复杂 opaque 包（Commands/Explosion/MapItemData/LevelParticles 等）IR 字段
 * 不足支撑原逻辑时，分支保留并 return（真 Java 互通的完整 codec 属独立子项），不破坏编译。
 *
 * 非拥有 m_app：由 ClientApplication 持有本 visitor 与 ClientNetwork。friend 访问
 * ClientApplication 私有成员。
 */
class ClientPlayVisitor {
public:
    explicit ClientPlayVisitor(ClientApplication& app)
        : m_app(app)
    {}

    /// 分发一个入站 Play 包（ClientNetwork 在 Play 阶段调用）
    [[nodiscard]] Result<void> handle(const mc::network::ir::IrPacket& packet);

    /// 分发一个入站 Configuration 包（ClientNetwork 在 Configuration 阶段调用）
    [[nodiscard]] Result<void> handleConfiguration(const mc::network::ir::IrPacket& packet);

private:
    ClientApplication& m_app;

    /// 取屏幕栈（friend 访问 ClientApplication 私有 m_kageroEngine/m_screenStackLayerId）
    [[nodiscard]] ui::minecraft::widgets::ScreenStackWidget* getScreenStack();

    /// 构造点击发送回调（出站走 m_app.m_network->send(ContainerClick)）
    [[nodiscard]] std::function<void(mc::ContainerId, i32, i32, i16, mc::ClickAction, const mc::ItemStack&)>
    makeContainerClickSender();

    /// 构造关闭容器发送回调
    [[nodiscard]] std::function<void(mc::ContainerId)> makeContainerCloseSender();
};

} // namespace mc::client::net
