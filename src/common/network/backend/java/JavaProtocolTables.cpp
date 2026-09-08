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
#include "common/network/backend/java/codecs/JavaConfigurationCodecs.hpp"
#include "common/network/backend/java/codecs/JavaPlayCodecs.hpp"
#include "common/network/backend/java/codecs/JavaPlayCodecsExtended.hpp"
#include "common/network/buffer/RegistryByteBuf.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/configuration/ConfigurationPackets.hpp"
#include "common/network/ir/packets/handshake/HandshakePackets.hpp"
#include "common/network/ir/packets/login/LoginPackets.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/ir/packets/status/StatusPackets.hpp"
#include "common/network/pipeline/ProtocolTableSet.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/PacketFlow.hpp"
#include "common/network/protocol/PacketType.hpp"
#include "common/network/protocol/ProtocolInfo.hpp"
#include "common/network/protocol/ProtocolInfoBuilder.hpp"
#include <memory>

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
        12, PacketType{PacketFlow::Clientbound, "update_enabled_features"}, 8, codecs::updateEnabledFeaturesCodec());
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
    //   UseItem(14) UseItemOn(15) ... SignUpdate(69) ServerboundMoveVehicle(83) ClientboundMoveVehicle(84)
    //   PaddleBoat(85) Interact(86) ... PlaceRecipe(88) ... ChunkBatchReceived(107) ChatCommand(108)
    //   SetCreativeModeSlot(109)。
    // Java Sb id（1.21.11 权威表，serverbound 从 0 起）：accept_teleportation=0, chat_command=6, chat=8,
    //   keep_alive=27, set_carried_item=52, move_player_pos=29, move_player_pos_rot=30,
    //   move_player_rot=31, move_player_status_only=32, player_action=40, player_command=41,
    //   player_input=42, use_item=64, use_item_on=63, configuration_acknowledged=15,
    //   container_click=17, container_close=18, interact=25, move_vehicle=33, paddle_boat=34,
    //   place_recipe=38, seen_advancements=49, sign_update=59, set_creative_mode_slot=55。
    b.addPacket<ir::play::AcceptTeleportation>(
        0, PacketType{PacketFlow::Serverbound, "accept_teleportation"}, 0, codecs::acceptTeleportationCodec());
    // chat_command(id=6)：真 Java 1.21.11 客户端命令提交走此包（无签名，单 String 不含 '/')。
    // 与 chat(id=8，带签名链) 分离，altIndex=108 对齐 PlayPacket variant 末尾。
    b.addPacket<ir::play::ChatCommand>(
        6, PacketType{PacketFlow::Serverbound, "chat_command"}, 108, codecs::chatCommandCodec());
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
    b.addPacket<ir::play::UseItem>(64, PacketType{PacketFlow::Serverbound, "use_item"}, 14, codecs::useItemCodec());
    b.addPacket<ir::play::SetCarriedItem>(
        52, PacketType{PacketFlow::Serverbound, "set_carried_item"}, 6, codecs::setCarriedItemCodec());
    b.addPacket<ir::play::ContainerClick>(
        17, PacketType{PacketFlow::Serverbound, "container_click"}, 2, codecs::containerClickCodec());
    b.addPacket<ir::play::ContainerClose>(
        18, PacketType{PacketFlow::Serverbound, "container_close"}, 3, codecs::containerCloseCodec());
    // set_creative_mode_slot(id=55)：真 Java 创造客户端从创造物品栏取物/丢弃走此包。
    // slotNum 为 vanilla InventoryMenu 菜单槽索引（0-45），<0 表丢弃。altIndex=109 对齐 variant 末尾。
    b.addPacket<ir::play::SetCreativeModeSlot>(
        55, PacketType{PacketFlow::Serverbound, "set_creative_mode_slot"}, 109, codecs::setCreativeModeSlotCodec());
    b.addPacket<ir::play::ConfigurationAcknowledged>(15,
        PacketType{PacketFlow::Serverbound, "configuration_acknowledged"},
        1,
        codecs::configurationAcknowledgedCodec());
    // ---- Phase 4a 补全 serverbound ----
    b.addPacket<ir::play::Interact>(25, PacketType{PacketFlow::Serverbound, "interact"}, 86, codecs::interactCodec());
    b.addPacket<ir::play::ServerboundMoveVehicle>(
        33, PacketType{PacketFlow::Serverbound, "move_vehicle"}, 83, codecs::serverboundMoveVehicleCodec());
    b.addPacket<ir::play::PaddleBoat>(
        34, PacketType{PacketFlow::Serverbound, "paddle_boat"}, 85, codecs::paddleBoatCodec());
    b.addPacket<ir::play::PlaceRecipe>(
        38, PacketType{PacketFlow::Serverbound, "place_recipe"}, 88, codecs::placeRecipeCodec());
    b.addPacket<ir::play::SeenAdvancements>(
        49, PacketType{PacketFlow::Serverbound, "seen_advancements"}, 50, codecs::seenAdvancementsCodec());
    b.addPacket<ir::play::SignUpdate>(
        59, PacketType{PacketFlow::Serverbound, "sign_update"}, 69, codecs::signUpdateCodec());
    // ---- 简单状态同步单包（altIndex 95..98）----
    b.addPacket<ir::play::ServerboundPingRequest>(
        37, PacketType{PacketFlow::Serverbound, "ping_request"}, 95, codecs::serverboundPingRequestCodec());
    b.addPacket<ir::play::ServerboundPong>(
        44, PacketType{PacketFlow::Serverbound, "pong"}, 96, codecs::serverboundPongCodec());
    b.addPacket<ir::play::ServerboundChangeDifficulty>(
        3, PacketType{PacketFlow::Serverbound, "change_difficulty"}, 97, codecs::serverboundChangeDifficultyCodec());
    b.addPacket<ir::play::LockDifficulty>(
        28, PacketType{PacketFlow::Serverbound, "lock_difficulty"}, 98, codecs::lockDifficultyCodec());
    // ---- 区块相关数据包（altIndex 107）----
    b.addPacket<ir::play::ChunkBatchReceived>(
        10, PacketType{PacketFlow::Serverbound, "chunk_batch_received"}, 107, codecs::chunkBatchReceivedCodec());
    return b.build();
}

[[nodiscard]] std::unique_ptr<protocol::ProtocolInfo<B, ir::PlayPacket>> buildPlayCb()
{
    ProtocolInfoBuilder<B, ir::PlayPacket> b(ConnectionProtocol::Play, PacketFlow::Clientbound);
    // clientbound 权威表：bundle_delimiter 占 id 0，故 add_entity=1，其余整体后移一位。
    // PlayPacket variant altIndex 注释见 IrPacket.hpp。各包 Java Cb id（1.21.11 权威）：
    //   add_entity=1, animate=2, block_destruction=5, block_entity_data=6, block_event=7, block_update=8,
    //   boss_event=9, change_difficulty=10, clear_titles=14, commands=16, container_set_content=18,
    //   container_set_data=19, container_set_slot=20, disconnect=32, explode=36, game_event=38,
    //   hurt_animation=41, initialize_border=42, keep_alive=43, level_chunk_with_light=44, level_event=45,
    //   level_particles=46, light_update=47, login=48, map_item_data=49, move_entity_pos=51,
    //   move_entity_pos_rot=52, move_entity_rot=54, move_vehicle=55, open_screen=57, open_sign_editor=58,
    //   place_ghost_recipe=61, player_abilities=62, player_info_remove=67, player_info_update=68,
    //   player_position=70, recipe_book_add=72, recipe_book_remove=73, remove_entities=75, reset_score=77,
    //   respawn=80, rotate_head=81, select_advancement_tab=83, set_action_bar_text=85, set_border_center=86,
    //   set_border_lerp_size=87, set_border_size=88, set_border_warning_delay=89, set_border_warning_distance=90,
    //   set_camera=91, set_chunk_cache_center=92, set_chunk_cache_radius=93, set_cursor_item=94,
    //   set_default_spawn_position=95, set_display_objective=96, set_entity_data=97,
    //   set_entity_link=98, set_entity_motion=99, set_experience=101, set_held_slot=103, set_objective=104,
    //   set_passengers=105, set_player_inventory=106, set_player_team=107, set_score=108, set_subtitle_text=110,
    //   set_time=111, set_title_text=112, set_titles_animation=113, sound_entity=114, sound=115, stop_sound=117,
    //   take_item_entity=122, teleport_entity=123, update_advancements=128, update_recipes=131。
    b.addPacket<ir::play::AddEntity>(
        1, PacketType{PacketFlow::Clientbound, "add_entity"}, 28, codecs::addEntityCodec());
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
        32, PacketType{PacketFlow::Clientbound, "disconnect"}, 16, codecs::playDisconnectCodec());
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
        43, PacketType{PacketFlow::Clientbound, "keep_alive"}, 5, codecs::keepAliveCodec());
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
    b.addPacket<ir::play::SetDefaultSpawnPosition>(95,
        PacketType{PacketFlow::Clientbound, "set_default_spawn_position"},
        22,
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
    b.addPacket<ir::play::SetTime>(111, PacketType{PacketFlow::Clientbound, "set_time"}, 19, codecs::setTimeCodec());
    b.addPacket<ir::play::PlayerAbilities>(
        62, PacketType{PacketFlow::Clientbound, "player_abilities"}, 20, codecs::playerAbilitiesCodec());
    // ---- Phase 4a 补全 clientbound ----
    b.addPacket<ir::play::Animate>(2, PacketType{PacketFlow::Clientbound, "animate"}, 74, codecs::animateCodec());
    b.addPacket<ir::play::BlockDestruction>(
        5, PacketType{PacketFlow::Clientbound, "block_destruction"}, 77, codecs::blockDestructionCodec());
    b.addPacket<ir::play::BlockEntityData>(
        6, PacketType{PacketFlow::Clientbound, "block_entity_data"}, 79, codecs::blockEntityDataCodec());
    b.addPacket<ir::play::BlockEvent>(
        7, PacketType{PacketFlow::Clientbound, "block_event"}, 78, codecs::blockEventCodec());
    b.addPacket<ir::play::BossEvent>(
        9, PacketType{PacketFlow::Clientbound, "boss_event"}, 48, codecs::bossEventCodec());
    b.addPacket<ir::play::ClearTitles>(
        14, PacketType{PacketFlow::Clientbound, "clear_titles"}, 60, codecs::clearTitlesCodec());
    b.addPacket<ir::play::Commands>(16, PacketType{PacketFlow::Clientbound, "commands"}, 87, codecs::commandsCodec());
    b.addPacket<ir::play::Explosion>(36, PacketType{PacketFlow::Clientbound, "explode"}, 82, codecs::explosionCodec());
    b.addPacket<ir::play::HurtAnimation>(
        41, PacketType{PacketFlow::Clientbound, "hurt_animation"}, 75, codecs::hurtAnimationCodec());
    b.addPacket<ir::play::InitializeBorder>(
        42, PacketType{PacketFlow::Clientbound, "initialize_border"}, 61, codecs::initializeBorderCodec());
    b.addPacket<ir::play::LevelEvent>(
        45, PacketType{PacketFlow::Clientbound, "level_event"}, 46, codecs::levelEventCodec());
    b.addPacket<ir::play::LevelParticles>(
        46, PacketType{PacketFlow::Clientbound, "level_particles"}, 47, codecs::levelParticlesCodec());
    b.addPacket<ir::play::MapItemData>(
        49, PacketType{PacketFlow::Clientbound, "map_item_data"}, 67, codecs::mapItemDataCodec());
    b.addPacket<ir::play::ClientboundMoveVehicle>(
        55, PacketType{PacketFlow::Clientbound, "move_vehicle"}, 84, codecs::clientboundMoveVehicleCodec());
    b.addPacket<ir::play::OpenSignEditor>(
        58, PacketType{PacketFlow::Clientbound, "open_sign_editor"}, 68, codecs::openSignEditorCodec());
    b.addPacket<ir::play::ResetScore>(
        77, PacketType{PacketFlow::Clientbound, "reset_score"}, 53, codecs::resetScoreCodec());
    b.addPacket<ir::play::Respawn>(80, PacketType{PacketFlow::Clientbound, "respawn"}, 80, codecs::respawnCodec());
    b.addPacket<ir::play::SelectAdvancementTab>(
        83, PacketType{PacketFlow::Clientbound, "select_advancement_tab"}, 49, codecs::selectAdvancementTabCodec());
    b.addPacket<ir::play::SetActionBarText>(
        85, PacketType{PacketFlow::Clientbound, "set_action_bar_text"}, 58, codecs::setActionBarTextCodec());
    b.addPacket<ir::play::SetBorderCenter>(
        86, PacketType{PacketFlow::Clientbound, "set_border_center"}, 62, codecs::setBorderCenterCodec());
    b.addPacket<ir::play::SetBorderLerpSize>(
        87, PacketType{PacketFlow::Clientbound, "set_border_lerp_size"}, 63, codecs::setBorderLerpSizeCodec());
    b.addPacket<ir::play::SetBorderSize>(
        88, PacketType{PacketFlow::Clientbound, "set_border_size"}, 64, codecs::setBorderSizeCodec());
    b.addPacket<ir::play::SetBorderWarningDelay>(
        89, PacketType{PacketFlow::Clientbound, "set_border_warning_delay"}, 65, codecs::setBorderWarningDelayCodec());
    b.addPacket<ir::play::SetBorderWarningDistance>(90,
        PacketType{PacketFlow::Clientbound, "set_border_warning_distance"},
        66,
        codecs::setBorderWarningDistanceCodec());
    b.addPacket<ir::play::SetCamera>(
        91, PacketType{PacketFlow::Clientbound, "set_camera"}, 70, codecs::setCameraCodec());
    b.addPacket<ir::play::SetDisplayObjective>(
        96, PacketType{PacketFlow::Clientbound, "set_display_objective"}, 54, codecs::setDisplayObjectiveCodec());
    b.addPacket<ir::play::SetEntityLink>(
        98, PacketType{PacketFlow::Clientbound, "set_entity_link"}, 71, codecs::setEntityLinkCodec());
    b.addPacket<ir::play::SetExperience>(
        101, PacketType{PacketFlow::Clientbound, "set_experience"}, 81, codecs::setExperienceCodec());
    b.addPacket<ir::play::SetObjective>(
        104, PacketType{PacketFlow::Clientbound, "set_objective"}, 51, codecs::setObjectiveCodec());
    b.addPacket<ir::play::SetPassengers>(
        105, PacketType{PacketFlow::Clientbound, "set_passengers"}, 72, codecs::setPassengersCodec());
    // EntityEvent（S→C，id=34）：codec 与 visitor 消费逻辑早已就绪，此前漏登记致服务端
    // 发不出 entity_event、客户端实体事件处理成为死代码。altIndex=73 与 PlayPacket variant 下标对齐。
    b.addPacket<ir::play::EntityEvent>(
        34, PacketType{PacketFlow::Clientbound, "entity_event"}, 73, codecs::entityEventCodec());
    b.addPacket<ir::play::SetPlayerTeam>(
        107, PacketType{PacketFlow::Clientbound, "set_player_team"}, 55, codecs::setPlayerTeamCodec());
    b.addPacket<ir::play::SetScore>(108, PacketType{PacketFlow::Clientbound, "set_score"}, 52, codecs::setScoreCodec());
    b.addPacket<ir::play::SetSubtitleText>(
        110, PacketType{PacketFlow::Clientbound, "set_subtitle_text"}, 57, codecs::setSubtitleTextCodec());
    b.addPacket<ir::play::SetTitleText>(
        112, PacketType{PacketFlow::Clientbound, "set_title_text"}, 56, codecs::setTitleTextCodec());
    b.addPacket<ir::play::SetTitlesAnimation>(
        113, PacketType{PacketFlow::Clientbound, "set_titles_animation"}, 59, codecs::setTitlesAnimationCodec());
    b.addPacket<ir::play::SoundEntity>(
        114, PacketType{PacketFlow::Clientbound, "sound_entity"}, 45, codecs::soundEntityCodec());
    b.addPacket<ir::play::PlaySound>(115, PacketType{PacketFlow::Clientbound, "sound"}, 43, codecs::playSoundCodec());
    b.addPacket<ir::play::StopSound>(
        117, PacketType{PacketFlow::Clientbound, "stop_sound"}, 44, codecs::stopSoundCodec());
    b.addPacket<ir::play::TakeItemEntity>(
        122, PacketType{PacketFlow::Clientbound, "take_item_entity"}, 76, codecs::takeItemEntityCodec());
    b.addPacket<ir::play::SetChunkCacheCenter>(
        92, PacketType{PacketFlow::Clientbound, "set_chunk_cache_center"}, 89, codecs::setChunkCacheCenterCodec());
    // ---- 简单状态同步单包（altIndex 90..94）----
    b.addPacket<ir::play::SetChunkCacheRadius>(
        93, PacketType{PacketFlow::Clientbound, "set_chunk_cache_radius"}, 90, codecs::setChunkCacheRadiusCodec());
    b.addPacket<ir::play::SetSimulationDistance>(
        109, PacketType{PacketFlow::Clientbound, "set_simulation_distance"}, 91, codecs::setSimulationDistanceCodec());
    b.addPacket<ir::play::SetHealth>(
        102, PacketType{PacketFlow::Clientbound, "set_health"}, 92, codecs::setHealthCodec());
    b.addPacket<ir::play::ClientboundPing>(
        59, PacketType{PacketFlow::Clientbound, "ping"}, 93, codecs::clientboundPingCodec());
    b.addPacket<ir::play::PongResponse>(
        60, PacketType{PacketFlow::Clientbound, "pong_response"}, 94, codecs::pongResponseCodec());
    // SystemChat（S→C，wire id=119/0x77，在 store_cookie(118) 与 playerlist_header(120) 之间）。
    // vanilla ClientboundSystemChatPacket：content(Component NBT) + isActionBar(bool)。
    b.addPacket<ir::play::SystemChat>(
        119, PacketType{PacketFlow::Clientbound, "system_chat"}, 99, codecs::systemChatCodec());
    // ---- 区块相关数据包（altIndex 100..106）----
    // 消费端仅 ForgetLevelChunk 落业务，其余为 TODO 桩；codec 双向自洽故先登记。
    b.addPacket<ir::play::BundleDelimiter>(
        0, PacketType{PacketFlow::Clientbound, "bundle_delimiter"}, 100, codecs::bundleDelimiterCodec());
    b.addPacket<ir::play::BlockChangedAck>(
        4, PacketType{PacketFlow::Clientbound, "block_changed_ack"}, 101, codecs::blockChangedAckCodec());
    b.addPacket<ir::play::ChunkBatchFinished>(
        11, PacketType{PacketFlow::Clientbound, "chunk_batch_finished"}, 102, codecs::chunkBatchFinishedCodec());
    b.addPacket<ir::play::ChunkBatchStart>(
        12, PacketType{PacketFlow::Clientbound, "chunk_batch_start"}, 103, codecs::chunkBatchStartCodec());
    b.addPacket<ir::play::ChunkBiomes>(
        13, PacketType{PacketFlow::Clientbound, "chunk_biomes"}, 104, codecs::chunkBiomesCodec());
    b.addPacket<ir::play::ForgetLevelChunk>(
        37, PacketType{PacketFlow::Clientbound, "forget_level_chunk"}, 105, codecs::forgetLevelChunkCodec());
    b.addPacket<ir::play::SectionBlocksUpdate>(
        82, PacketType{PacketFlow::Clientbound, "section_blocks_update"}, 106, codecs::sectionBlocksUpdateCodec());
    // SetPlayerInventory（S→C，id=106）：单槽同步玩家物品栏（PlayerInventory 内部索引 0-40）。
    // 仅容器关闭塞回 carried 残留物品时下发。altIndex=110 对齐 variant 末尾。
    b.addPacket<ir::play::SetPlayerInventory>(
        106, PacketType{PacketFlow::Clientbound, "set_player_inventory"}, 110, codecs::setPlayerInventoryCodec());
    // ---- 玩家战斗数据包（altIndex 111..113）----
    // 对应 Java 1.21.11 ClientboundPlayerCombatEnterPacket/EndPacket/KillPacket。
    // 三包均 S→C 仅发给当事玩家，驱动客户端战斗状态机与死亡画面（DeathScreen）。
    // wire id 对应 packet_ids.json：play:cb:64/65/66。
    b.addPacket<ir::play::PlayerCombatEnter>(
        65, PacketType{PacketFlow::Clientbound, "player_combat_enter"}, 111, codecs::playerCombatEnterCodec());
    b.addPacket<ir::play::PlayerCombatEnd>(
        64, PacketType{PacketFlow::Clientbound, "player_combat_end"}, 112, codecs::playerCombatEndCodec());
    b.addPacket<ir::play::PlayerCombatKill>(
        66, PacketType{PacketFlow::Clientbound, "player_combat_kill"}, 113, codecs::playerCombatKillCodec());
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
