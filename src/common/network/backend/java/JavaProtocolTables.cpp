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

#include "common/network/backend/java/JavaProtocolTables.hpp"
#include "common/network/backend/java/codecs/JavaCodecs.hpp"
#include "common/network/protocol/PacketFlow.hpp"
#include "common/network/protocol/PacketType.hpp"
#include "common/network/protocol/ProtocolInfoBuilder.hpp"

namespace mc::network::backend::java {

namespace {

using protocol::ConnectionProtocol;
using protocol::PacketFlow;
using protocol::PacketType;
using protocol::ProtocolInfoBuilder;

using B = buffer::RegistryByteBuf;

// ============================================================================
// 各阶段包表构建（addPacket 显式 id 严格对齐 GameProtocols.java 注册顺序）
// 在用包子集：只登记当前 IR 已有的包，id 与 Java 一致；未登记 id 解码报错由调用方跳过。
// ============================================================================

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::HandshakePacket>> buildHandshakeSb()
{
    ProtocolInfoBuilder<B, ir::HandshakePacket> b(ConnectionProtocol::Handshaking, PacketFlow::Serverbound);
    // id=0 ClientIntention（握手阶段唯一包）。altIndex 由 IrPacket.hpp variant 顺序定。
    b.addPacket<ir::handshake::ClientIntention>(
        0, PacketType{PacketFlow::Serverbound, "client_intention"}, 0, codecs::clientIntentionCodec());
    return b.build();
}

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::StatusPacket>> buildStatusSb()
{
    ProtocolInfoBuilder<B, ir::StatusPacket> b(ConnectionProtocol::Status, PacketFlow::Serverbound);
    b.addPacket<ir::status::StatusRequest>(
        0, PacketType{PacketFlow::Serverbound, "status_request"}, 0, codecs::statusRequestCodec());
    b.addPacket<ir::status::PingRequest>(
        1, PacketType{PacketFlow::Serverbound, "ping_request"}, 2, codecs::pingRequestCodec());
    return b.build();
}

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::StatusPacket>> buildStatusCb()
{
    ProtocolInfoBuilder<B, ir::StatusPacket> b(ConnectionProtocol::Status, PacketFlow::Clientbound);
    b.addPacket<ir::status::StatusResponse>(
        0, PacketType{PacketFlow::Clientbound, "status_response"}, 1, codecs::statusResponseCodec());
    b.addPacket<ir::status::PingResponse>(
        1, PacketType{PacketFlow::Clientbound, "pong_response"}, 3, codecs::pingResponseCodec());
    return b.build();
}

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::LoginPacket>> buildLoginSb()
{
    ProtocolInfoBuilder<B, ir::LoginPacket> b(ConnectionProtocol::Login, PacketFlow::Serverbound);
    // LoginPacket variant: Hello(0) HelloBound(1) Key(2) LoginFinished(3) LoginCompression(4)
    //                     LoginAcknowledged(5) Disconnect(6)
    b.addPacket<ir::login::Hello>(0, PacketType{PacketFlow::Serverbound, "hello"}, 0, codecs::helloCodec());
    b.addPacket<ir::login::Key>(1, PacketType{PacketFlow::Serverbound, "key"}, 2, codecs::keyCodec());
    // id=2 custom_query_answer 未登记（IR 暂无），跳过保持 id 对齐。
    b.addPacket<ir::login::LoginAcknowledged>(
        3, PacketType{PacketFlow::Serverbound, "login_acknowledged"}, 5, codecs::loginAcknowledgedCodec());
    return b.build();
}

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::LoginPacket>> buildLoginCb()
{
    ProtocolInfoBuilder<B, ir::LoginPacket> b(ConnectionProtocol::Login, PacketFlow::Clientbound);
    b.addPacket<ir::login::Disconnect>(
        0, PacketType{PacketFlow::Clientbound, "login_disconnect"}, 6, codecs::loginDisconnectCodec());
    b.addPacket<ir::login::HelloBound>(1, PacketType{PacketFlow::Clientbound, "hello"}, 1, codecs::helloBoundCodec());
    b.addPacket<ir::login::LoginFinished>(
        2, PacketType{PacketFlow::Clientbound, "login_finished"}, 3, codecs::loginFinishedCodec());
    b.addPacket<ir::login::LoginCompression>(
        3, PacketType{PacketFlow::Clientbound, "login_compression"}, 4, codecs::loginCompressionCodec());
    return b.build();
}

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::ConfigurationPacket>> buildConfigurationSb()
{
    ProtocolInfoBuilder<B, ir::ConfigurationPacket> b(ConnectionProtocol::Configuration, PacketFlow::Serverbound);
    // ConfigurationPacket variant: ClientInformation(0) CustomPayload(1) Disconnect(2) FinishConfiguration(3)
    //                             KeepAlive(4) Ping(5) RegistryData(6) SelectKnownPacks(7)
    //                             UpdateEnabledFeatures(8) UpdateTags(9)
    // Java Sb id: client_information=0, custom_payload=2, finish_configuration=3, keep_alive=4,
    //             pong=5, select_known_packs=7。
    b.addPacket<ir::configuration::ClientInformation>(
        0, PacketType{PacketFlow::Serverbound, "client_information"}, 0, codecs::clientInformationCodec());
    b.addPacket<ir::configuration::CustomPayload>(
        2, PacketType{PacketFlow::Serverbound, "custom_payload"}, 1, codecs::configurationCustomPayloadCodec());
    b.addPacket<ir::configuration::FinishConfiguration>(
        3, PacketType{PacketFlow::Serverbound, "finish_configuration"}, 3, codecs::finishConfigurationCodec());
    b.addPacket<ir::configuration::KeepAlive>(
        4, PacketType{PacketFlow::Serverbound, "keep_alive"}, 4, codecs::configurationKeepAliveCodec());
    b.addPacket<ir::configuration::Ping>(
        5, PacketType{PacketFlow::Serverbound, "pong"}, 5, codecs::configurationPingCodec());
    b.addPacket<ir::configuration::SelectKnownPacks>(
        7, PacketType{PacketFlow::Serverbound, "select_known_packs"}, 7, codecs::selectKnownPacksCodec());
    return b.build();
}

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::ConfigurationPacket>> buildConfigurationCb()
{
    ProtocolInfoBuilder<B, ir::ConfigurationPacket> b(ConnectionProtocol::Configuration, PacketFlow::Clientbound);
    // Java Cb id: custom_payload=1, disconnect=2, finish_configuration=3, keep_alive=4, ping=5,
    //             registry_data=7, update_enabled_features=12, update_tags=13, select_known_packs=14。
    b.addPacket<ir::configuration::CustomPayload>(
        1, PacketType{PacketFlow::Clientbound, "custom_payload"}, 1, codecs::configurationCustomPayloadCodec());
    b.addPacket<ir::configuration::Disconnect>(
        2, PacketType{PacketFlow::Clientbound, "disconnect"}, 2, codecs::configurationDisconnectCodec());
    b.addPacket<ir::configuration::FinishConfiguration>(
        3, PacketType{PacketFlow::Clientbound, "finish_configuration"}, 3, codecs::finishConfigurationCodec());
    b.addPacket<ir::configuration::KeepAlive>(
        4, PacketType{PacketFlow::Clientbound, "keep_alive"}, 4, codecs::configurationKeepAliveCodec());
    b.addPacket<ir::configuration::Ping>(
        5, PacketType{PacketFlow::Clientbound, "ping"}, 5, codecs::configurationPingCodec());
    b.addPacket<ir::configuration::RegistryData>(
        7, PacketType{PacketFlow::Clientbound, "registry_data"}, 6, codecs::registryDataCodec());
    b.addPacket<ir::configuration::UpdateEnabledFeatures>(
        12, PacketType{PacketFlow::Clientbound, "update_enabled_features"}, 8,
        codecs::updateEnabledFeaturesCodec());
    b.addPacket<ir::configuration::UpdateTags>(
        13, PacketType{PacketFlow::Clientbound, "update_tags"}, 9, codecs::updateTagsCodec());
    b.addPacket<ir::configuration::SelectKnownPacks>(
        14, PacketType{PacketFlow::Clientbound, "select_known_packs"}, 7, codecs::selectKnownPacksCodec());
    return b.build();
}

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::PlayPacket>> buildPlaySb()
{
    ProtocolInfoBuilder<B, ir::PlayPacket> b(ConnectionProtocol::Play, PacketFlow::Serverbound);
    // PlayPacket variant altIndex：AcceptTeleportation(0) ConfigurationAcknowledged(1) ContainerClick(2)
    //   ContainerClose(3) Chat(4) KeepAlive(5) SetCarriedItem(6) MovePlayerPos(7) MovePlayerPosRot(8)
    //   MovePlayerRot(9) MovePlayerStatusOnly(10) PlayerAction(11) PlayerCommand(12) PlayerInput(13)
    //   UseItem(14) UseItemOn(15)
    // Java Sb id（1.21.11）：accept_teleportation=0, configuration_acknowledged=15, container_click=17,
    //   container_close=18, chat=8, keep_alive=27, set_carried_item=52, move_player_pos=29,
    //   move_player_pos_rot=30, move_player_rot=31, move_player_status_only=32, player_action=40,
    //   player_command=41, player_input=42, use_item=64, use_item_on=63。
    b.addPacket<ir::play::AcceptTeleportation>(
        0, PacketType{PacketFlow::Serverbound, "accept_teleportation"}, 0, codecs::acceptTeleportationCodec());
    b.addPacket<ir::play::Chat>(8, PacketType{PacketFlow::Serverbound, "chat"}, 4, codecs::chatCodec());
    b.addPacket<ir::play::KeepAlive>(
        27, PacketType{PacketFlow::Serverbound, "keep_alive"}, 5, codecs::keepAliveCodec());
    b.addPacket<ir::play::MovePlayerPos>(
        29, PacketType{PacketFlow::Serverbound, "move_player_pos"}, 7, codecs::movePlayerPosCodec());
    b.addPacket<ir::play::MovePlayerPosRot>(
        30, PacketType{PacketFlow::Serverbound, "move_player_pos_rot"}, 8, codecs::movePlayerPosRotCodec());
    b.addPacket<ir::play::MovePlayerRot>(
        31, PacketType{PacketFlow::Serverbound, "move_player_rot"}, 9, codecs::movePlayerRotCodec());
    b.addPacket<ir::play::MovePlayerStatusOnly>(
        32, PacketType{PacketFlow::Serverbound, "move_player_status_only"}, 10, codecs::movePlayerStatusOnlyCodec());
    b.addPacket<ir::play::PlayerAction>(
        40, PacketType{PacketFlow::Serverbound, "player_action"}, 11, codecs::playerActionCodec());
    b.addPacket<ir::play::PlayerCommand>(
        41, PacketType{PacketFlow::Serverbound, "player_command"}, 12, codecs::playerCommandCodec());
    b.addPacket<ir::play::PlayerInput>(
        42, PacketType{PacketFlow::Serverbound, "player_input"}, 13, codecs::playerInputCodec());
    b.addPacket<ir::play::UseItemOn>(
        63, PacketType{PacketFlow::Serverbound, "use_item_on"}, 15, codecs::useItemOnCodec());
    b.addPacket<ir::play::UseItem>(
        64, PacketType{PacketFlow::Serverbound, "use_item"}, 14, codecs::useItemCodec());
    b.addPacket<ir::play::SetCarriedItem>(
        52, PacketType{PacketFlow::Serverbound, "set_carried_item"}, 6, codecs::setCarriedItemCodec());
    b.addPacket<ir::play::ContainerClick>(
        17, PacketType{PacketFlow::Serverbound, "container_click"}, 2, codecs::containerClickCodec());
    b.addPacket<ir::play::ContainerClose>(
        18, PacketType{PacketFlow::Serverbound, "container_close"}, 3, codecs::containerCloseCodec());
    b.addPacket<ir::play::ConfigurationAcknowledged>(
        15, PacketType{PacketFlow::Serverbound, "configuration_acknowledged"}, 1,
        codecs::configurationAcknowledgedCodec());
    return b.build();
}

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::PlayPacket>> buildPlayCb()
{
    ProtocolInfoBuilder<B, ir::PlayPacket> b(ConnectionProtocol::Play, PacketFlow::Clientbound);
    // PlayPacket variant altIndex：Disconnect(16) Login(17) PlayerPosition(18) SetTime(19)
    //   PlayerAbilities(20) SetHeldSlot(21) SetDefaultSpawnPosition(22) ChangeDifficulty(23)
    //   GameEvent(24) PlayerInfoUpdate(25) PlayerInfoRemove(26) SetEntityData(27) AddEntity(28)
    //   RemoveEntities(29) TeleportEntity(30) MoveEntityPos(31) MoveEntityPosRot(32) MoveEntityRot(33)
    //   SetEntityMotion(34) RotateHead(35) LevelChunkWithLight(36) LightUpdate(37) BlockUpdate(38)
    //   ContainerSetContent(39) ContainerSetSlot(40) OpenScreen(41) ContainerSetData(42)
    // Java Cb id（1.21.11）：add_entity=1, block_update=8, change_difficulty=10, game_event=38,
    //   open_screen=57, keep_alive=42, remove_entities=75, level_chunk_with_light=44, light_update=47,
    //   login=48, move_entity_pos=51, move_entity_pos_rot=52, move_entity_rot=54, rotate_head=81,
    //   set_entity_data=97, set_entity_motion=99, player_info_remove=67, player_info_update=68,
    //   player_position=70, teleport_entity=123, set_held_slot=103, set_default_spawn_position=95,
    //   set_time=111, player_abilities=62, container_set_content=18, container_set_data=19,
    //   container_set_slot=20, disconnect=31。
    b.addPacket<ir::play::AddEntity>(1, PacketType{PacketFlow::Clientbound, "add_entity"}, 28, codecs::addEntityCodec());
    b.addPacket<ir::play::BlockUpdate>(
        8, PacketType{PacketFlow::Clientbound, "block_update"}, 38, codecs::blockUpdateCodec());
    b.addPacket<ir::play::ChangeDifficulty>(
        10, PacketType{PacketFlow::Clientbound, "change_difficulty"}, 23, codecs::changeDifficultyCodec());
    b.addPacket<ir::play::ContainerSetContent>(
        18, PacketType{PacketFlow::Clientbound, "container_set_content"}, 39, codecs::containerSetContentCodec());
    b.addPacket<ir::play::ContainerSetData>(
        19, PacketType{PacketFlow::Clientbound, "container_set_data"}, 42, codecs::containerSetDataCodec());
    b.addPacket<ir::play::ContainerSetSlot>(
        20, PacketType{PacketFlow::Clientbound, "container_set_slot"}, 40, codecs::containerSetSlotCodec());
    b.addPacket<ir::play::Disconnect>(
        31, PacketType{PacketFlow::Clientbound, "disconnect"}, 16, codecs::playDisconnectCodec());
    b.addPacket<ir::play::LevelChunkWithLight>(
        44, PacketType{PacketFlow::Clientbound, "level_chunk_with_light"}, 36, codecs::levelChunkWithLightCodec());
    b.addPacket<ir::play::LightUpdate>(
        47, PacketType{PacketFlow::Clientbound, "light_update"}, 37, codecs::lightUpdateCodec());
    b.addPacket<ir::play::Login>(48, PacketType{PacketFlow::Clientbound, "login"}, 17, codecs::loginCodec());
    b.addPacket<ir::play::GameEvent>(
        38, PacketType{PacketFlow::Clientbound, "game_event"}, 24, codecs::gameEventCodec());
    b.addPacket<ir::play::OpenScreen>(
        57, PacketType{PacketFlow::Clientbound, "open_screen"}, 41, codecs::openScreenCodec());
    b.addPacket<ir::play::KeepAlive>(
        42, PacketType{PacketFlow::Clientbound, "keep_alive"}, 5, codecs::keepAliveCodec());
    b.addPacket<ir::play::MoveEntityPos>(
        51, PacketType{PacketFlow::Clientbound, "move_entity_pos"}, 31, codecs::moveEntityPosCodec());
    b.addPacket<ir::play::MoveEntityPosRot>(
        52, PacketType{PacketFlow::Clientbound, "move_entity_pos_rot"}, 32, codecs::moveEntityPosRotCodec());
    b.addPacket<ir::play::MoveEntityRot>(
        54, PacketType{PacketFlow::Clientbound, "move_entity_rot"}, 33, codecs::moveEntityRotCodec());
    b.addPacket<ir::play::PlayerInfoRemove>(
        67, PacketType{PacketFlow::Clientbound, "player_info_remove"}, 26, codecs::playerInfoRemoveCodec());
    b.addPacket<ir::play::PlayerInfoUpdate>(
        68, PacketType{PacketFlow::Clientbound, "player_info_update"}, 25, codecs::playerInfoUpdateCodec());
    b.addPacket<ir::play::PlayerPosition>(
        70, PacketType{PacketFlow::Clientbound, "player_position"}, 18, codecs::playerPositionCodec());
    b.addPacket<ir::play::RemoveEntities>(
        75, PacketType{PacketFlow::Clientbound, "remove_entities"}, 29, codecs::removeEntitiesCodec());
    b.addPacket<ir::play::SetDefaultSpawnPosition>(
        95, PacketType{PacketFlow::Clientbound, "set_default_spawn_position"}, 22,
        codecs::setDefaultSpawnPositionCodec());
    b.addPacket<ir::play::SetEntityData>(
        97, PacketType{PacketFlow::Clientbound, "set_entity_data"}, 27, codecs::setEntityDataCodec());
    b.addPacket<ir::play::SetEntityMotion>(
        99, PacketType{PacketFlow::Clientbound, "set_entity_motion"}, 34, codecs::setEntityMotionCodec());
    b.addPacket<ir::play::RotateHead>(
        81, PacketType{PacketFlow::Clientbound, "rotate_head"}, 35, codecs::rotateHeadCodec());
    b.addPacket<ir::play::TeleportEntity>(
        123, PacketType{PacketFlow::Clientbound, "teleport_entity"}, 30, codecs::teleportEntityCodec());
    b.addPacket<ir::play::SetHeldSlot>(
        103, PacketType{PacketFlow::Clientbound, "set_held_slot"}, 21, codecs::setHeldSlotCodec());
    b.addPacket<ir::play::SetTime>(
        111, PacketType{PacketFlow::Clientbound, "set_time"}, 19, codecs::setTimeCodec());
    b.addPacket<ir::play::PlayerAbilities>(
        62, PacketType{PacketFlow::Clientbound, "player_abilities"}, 20, codecs::playerAbilitiesCodec());
    return b.build();
}

} // namespace

std::shared_ptr<pipeline::ProtocolTableSet<B>> JavaProtocolTables::build()
{
    auto tables = std::make_shared<pipeline::ProtocolTableSet<B>>();
    tables->handshakeSb = buildHandshakeSb();
    tables->statusSb = buildStatusSb();
    tables->statusCb = buildStatusCb();
    tables->loginSb = buildLoginSb();
    tables->loginCb = buildLoginCb();
    tables->configurationSb = buildConfigurationSb();
    tables->configurationCb = buildConfigurationCb();
    tables->playSb = buildPlaySb();
    tables->playCb = buildPlayCb();
    // handshakeCb 在 Java 协议中不存在（握手只有 C→S），留空。
    return tables;
}

} // namespace mc::network::backend::java
