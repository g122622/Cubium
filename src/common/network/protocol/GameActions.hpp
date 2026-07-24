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

#include "common/core/Types.hpp"

namespace mc::network {

/// @brief 玩家方块交互动作（对应 ir::play::PlayerAction.action 字段值）
///
/// 1.21.11 PlayerAction 的 action 值：0=StartDestroyBlock 1=AbortDestroyBlock 2=StopDestroyBlock。
/// 客户端挖掘流程与服务端 BlockInteractionManager/MiningManager 共用此枚举。
enum class BlockInteractionAction : u8 {
    StartDestroyBlock = 0,
    AbortDestroyBlock = 1,
    StopDestroyBlock = 2,
};

/// @brief Boss 栏动作（对应 ir::play::BossEvent.operation 字段值）
///
/// 1.21.11 BossEvent 单包 + operation 分发：0=ADD 1=REMOVE 2=UPDATE_PROGRESS
/// 3=UPDATE_NAME 4=UPDATE_STYLE 5=UPDATE_PROPERTIES。
enum class BossInfoAction : u8 {
    Add = 0,
    Remove = 1,
    UpdatePercent = 2,
    UpdateName = 3,
    UpdateStyle = 4,
    UpdateProperties = 5,
};

/// @brief 玩家能力标志位（对应 ir::play::PlayerAbilities.flags 字段 bit）
///
/// 1.21.11 PlayerAbilities 的 flags 字节按位组合：bit0=Invulnerable bit1=Flying
/// bit2=CanFly bit3=CreativeMode。
enum class PlayerAbilityFlags : u8 {
    None = 0,
    Invulnerable = 0x01,
    Flying = 0x02,
    CanFly = 0x04,
    CreativeMode = 0x08,
};

} // namespace mc::network
