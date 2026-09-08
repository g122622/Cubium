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

#pragma once

#include "common/network/ir/IrPacketBase.hpp"
#include "common/network/ir/packets/configuration/ConfigurationPackets.hpp"
#include "common/network/ir/packets/handshake/HandshakePackets.hpp"
#include "common/network/ir/packets/login/LoginPackets.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/ir/packets/status/StatusPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"

#include <variant>

namespace mc::network::ir {

/**
 * @brief 握手阶段包变体
 *
 * ClientIntention（握手阶段唯一包），已是完整集。
 */
using HandshakePacket = std::variant<handshake::ClientIntention>;

/**
 * @brief 状态阶段包变体
 */
using StatusPacket =
    std::variant<status::StatusRequest, status::StatusResponse, status::PingRequest, status::PingResponse>;

/**
 * @brief 登录阶段包变体
 */
using LoginPacket = std::variant<login::Hello,
    login::HelloBound,
    login::Key,
    login::LoginFinished,
    login::LoginCompression,
    login::LoginAcknowledged,
    login::Disconnect>;

/**
 * @brief 配置阶段包变体
 *
 * 备选项顺序即 altIndex（JavaProtocolTables 登记时按此下标）。
 */
using ConfigurationPacket = std::variant<configuration::ClientInformation, // 0
    configuration::CustomPayload,                                          // 1
    configuration::Disconnect,                                             // 2
    configuration::FinishConfiguration,                                    // 3
    configuration::KeepAlive,                                              // 4
    configuration::Ping,                                                   // 5
    configuration::RegistryData,                                           // 6
    configuration::SelectKnownPacks,                                       // 7
    configuration::UpdateEnabledFeatures,                                  // 8
    configuration::UpdateTags>;                                            // 9

/**
 * @brief 游戏阶段包变体
 *
 * 备选项顺序即 altIndex（JavaProtocolTables 登记时按此下标）。
 */
using PlayPacket = std::variant<play::AcceptTeleportation, // 0
    play::ConfigurationAcknowledged,                       // 1
    play::ContainerClick,                                  // 2
    play::ContainerClose,                                  // 3
    play::Chat,                                            // 4
    play::KeepAlive,                                       // 5
    play::SetCarriedItem,                                  // 6
    play::MovePlayerPos,                                   // 7
    play::MovePlayerPosRot,                                // 8
    play::MovePlayerRot,                                   // 9
    play::MovePlayerStatusOnly,                            // 10
    play::PlayerAction,                                    // 11
    play::PlayerCommand,                                   // 12
    play::PlayerInput,                                     // 13
    play::UseItem,                                         // 14
    play::UseItemOn,                                       // 15
    play::Disconnect,                                      // 16
    play::Login,                                           // 17
    play::PlayerPosition,                                  // 18
    play::SetTime,                                         // 19
    play::PlayerAbilities,                                 // 20
    play::SetHeldSlot,                                     // 21
    play::SetDefaultSpawnPosition,                         // 22
    play::ChangeDifficulty,                                // 23
    play::GameEvent,                                       // 24
    play::PlayerInfoUpdate,                                // 25
    play::PlayerInfoRemove,                                // 26
    play::SetEntityData,                                   // 27
    play::AddEntity,                                       // 28
    play::RemoveEntities,                                  // 29
    play::TeleportEntity,                                  // 30
    play::MoveEntityPos,                                   // 31
    play::MoveEntityPosRot,                                // 32
    play::MoveEntityRot,                                   // 33
    play::SetEntityMotion,                                 // 34
    play::RotateHead,                                      // 35
    play::LevelChunkWithLight,                             // 36
    play::LightUpdate,                                     // 37
    play::BlockUpdate,                                     // 38
    play::ContainerSetContent,                             // 39
    play::ContainerSetSlot,                                // 40
    play::OpenScreen,                                      // 41
    play::ContainerSetData,                                // 42
    // ---- 以下为 Phase 4a 补全（altIndex 43..88，PlayPacketsExtended.hpp）----
    play::PlaySound,                // 43
    play::StopSound,                // 44
    play::SoundEntity,              // 45
    play::LevelEvent,               // 46
    play::LevelParticles,           // 47
    play::BossEvent,                // 48
    play::SelectAdvancementTab,     // 49
    play::SeenAdvancements,         // 50
    play::SetObjective,             // 51
    play::SetScore,                 // 52
    play::ResetScore,               // 53
    play::SetDisplayObjective,      // 54
    play::SetPlayerTeam,            // 55
    play::SetTitleText,             // 56
    play::SetSubtitleText,          // 57
    play::SetActionBarText,         // 58
    play::SetTitlesAnimation,       // 59
    play::ClearTitles,              // 60
    play::InitializeBorder,         // 61
    play::SetBorderCenter,          // 62
    play::SetBorderLerpSize,        // 63
    play::SetBorderSize,            // 64
    play::SetBorderWarningDelay,    // 65
    play::SetBorderWarningDistance, // 66
    play::MapItemData,              // 67
    play::OpenSignEditor,           // 68
    play::SignUpdate,               // 69
    play::SetCamera,                // 70
    play::SetEntityLink,            // 71
    play::SetPassengers,            // 72
    play::EntityEvent,              // 73
    play::Animate,                  // 74
    play::HurtAnimation,            // 75
    play::TakeItemEntity,           // 76
    play::BlockDestruction,         // 77
    play::BlockEvent,               // 78
    play::BlockEntityData,          // 79
    play::Respawn,                  // 80
    play::SetExperience,            // 81
    play::Explosion,                // 82
    play::ServerboundMoveVehicle,   // 83
    play::ClientboundMoveVehicle,   // 84
    play::PaddleBoat,               // 85
    play::Interact,                 // 86
    play::Commands,                 // 87
    play::PlaceRecipe,              // 88
    play::SetChunkCacheCenter,      // 89
    // ---- 以下为简单状态同步单包（altIndex 90..98，PlayPacketsExtended.hpp）----
    play::SetChunkCacheRadius,         // 90
    play::SetSimulationDistance,       // 91
    play::SetHealth,                   // 92
    play::ClientboundPing,             // 93
    play::PongResponse,                // 94
    play::ServerboundPingRequest,      // 95
    play::ServerboundPong,             // 96
    play::ServerboundChangeDifficulty, // 97
    play::LockDifficulty,              // 98
    play::SystemChat,                  // 99
    // ---- 以下为区块相关数据包（altIndex 100..107，PlayPacketsExtended.hpp）----
    play::BundleDelimiter,     // 100
    play::BlockChangedAck,     // 101
    play::ChunkBatchFinished,  // 102
    play::ChunkBatchStart,     // 103
    play::ChunkBiomes,         // 104
    play::ForgetLevelChunk,    // 105
    play::SectionBlocksUpdate, // 106
    play::ChunkBatchReceived,  // 107
    play::ChatCommand,         // 108
    play::SetCreativeModeSlot, // 109
    play::SetPlayerInventory,  // 110
    // ---- 以下为玩家战斗数据包（altIndex 111..113，PlayPacketsExtended.hpp）----
    play::PlayerCombatEnter, // 111
    play::PlayerCombatEnd,   // 112
    play::PlayerCombatKill>; // 113

/**
 * @brief 顶层包标签：携带阶段信息 + 阶段变体
 *
 * pipeline 层解码出原始字节后，按当前阶段构造对应阶段变体，再包成 IrPacket。
 * 游戏逻辑侧用 std::visit(visitor, packet) 消费，零虚函数开销。
 *
 * 设计为带阶段标签的 tagged union，而非单 mega-variant——避免一个 variant 含上百备选项
 * 导致编译/调试困难，且阶段切换天然隔离。
 */
struct IrPacket {
    protocol::ConnectionProtocol phase;
    std::variant<HandshakePacket, StatusPacket, LoginPacket, ConfigurationPacket, PlayPacket> packet;
};

} // namespace mc::network::ir
