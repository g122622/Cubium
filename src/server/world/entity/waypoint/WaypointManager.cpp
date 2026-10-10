#include "WaypointManager.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/scoreboard/core/Team.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/text/TextStyle.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/network/sync/chunk/PlayerChunkTracker.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <cmath>
#include <unordered_set>

namespace mc::server {
namespace {
namespace _ir = mc::network::ir;
using _Waypoint = _ir::play::Waypoint;
using _Operation = _ir::play::WaypointOperation;

bool _send(IServer& server, PlayerId receiver, const _Waypoint& waypoint)
{
    const auto* player = server.playerManager().getPlayer(receiver);
    return player != nullptr && player->loggedIn &&
        player->send(_ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play, _ir::PlayPacket{waypoint}});
}

_Waypoint _removal(const _Waypoint& snapshot)
{
    return {_Operation::Untrack, snapshot.identifier, "minecraft:default", std::nullopt, std::monostate{}};
}

bool _hidden(const LivingEntity& source)
{
    if (source.hasEffect(entity::effect::EffectType::Invisibility)) return true;
    if (dynamic_cast<const Player*>(&source) != nullptr && source.pose() == EntityPose::Crouching) return true;
    const auto head = source.getEquipment(EquipmentSlot::Head).getItem();
    return head != nullptr &&
        (head == Items::CARVED_PUMPKIN || head == Items::PLAYER_HEAD || head == Items::ZOMBIE_HEAD ||
            head == Items::CREEPER_HEAD || head == Items::DRAGON_HEAD || head == Items::PIGLIN_HEAD ||
            head == Items::SKELETON_SKULL || head == Items::WITHER_SKELETON_SKULL);
}

bool _hasReceiverAsPassenger(const LivingEntity& source, const Player& receiver, ServerWorld& world)
{
    auto vehicle = receiver.getVehicle();
    while (vehicle != INVALID_ENTITY_ID) {
        if (vehicle == source.id()) return true;
        const auto* entity = world.entityManager().getEntity(vehicle);
        if (entity == nullptr) break;
        vehicle = entity->getVehicle();
    }
    return false;
}

bool _needsTrack(const _Waypoint& previous, const _Waypoint& next)
{
    if (previous.identifier != next.identifier || previous.style != next.style || previous.color != next.color ||
        previous.position.index() != next.position.index())
        return true;
    if (const auto* pos = std::get_if<_ir::play::WaypointPosition>(&previous.position)) {
        const auto& current = std::get<_ir::play::WaypointPosition>(next.position);
        return std::abs(pos->x - current.x) + std::abs(pos->y - current.y) + std::abs(pos->z - current.z) > 1;
    }
    if (const auto* pos = std::get_if<_ir::play::WaypointChunk>(&previous.position)) {
        const auto& current = std::get<_ir::play::WaypointChunk>(next.position);
        return std::max(std::abs(pos->x - current.x), std::abs(pos->z - current.z)) > 1;
    }
    return false;
}
} // namespace

void WaypointManager::tick(IServer& server, ServerWorld& world)
{
    std::vector<LivingEntity*> sources;
    world.entityManager().forEachEntity([&](Entity* entity) {
        if (auto* living = dynamic_cast<LivingEntity*>(entity); living != nullptr && living->isAlive() &&
            !living->isRemoved() &&
            living->getAttributeValue(entity::attribute::Attributes::WAYPOINT_TRANSMIT_RANGE, 0.0) > 0.0 &&
            !_hidden(*living))
            sources.push_back(living);
        return true;
    });
    std::unordered_set<PlayerId> active;
    for (const auto playerId : server.playerManager().getPlayerIds()) {
        auto* data = server.playerManager().getPlayer(playerId);
        auto* receiver = server.playerEntityManager().getPlayerEntity(playerId, world);
        if (receiver == nullptr || receiver->world() != &world || !data->loggedIn || !data->hasClientLoaded() ||
            !data->hasConnection())
            continue;
        active.insert(playerId);
        auto& sent = m_sent[playerId];
        SnapshotMap desired;
        const f64 receiveRange =
            receiver->getAttributeValue(entity::attribute::Attributes::WAYPOINT_RECEIVE_RANGE, 0.0);
        if (receiveRange > 0.0 && world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::LOCATOR_BAR))
            for (const auto* source : sources) {
                if (source == receiver) continue;
                const f64 distance = source->position().distance(receiver->position());
                if (!receiver->isSpectator() &&
                    (source->isSpectator() || _hasReceiverAsPassenger(*source, *receiver, world) ||
                        distance >= std::min(receiveRange,
                                        source->getAttributeValue(
                                            entity::attribute::Attributes::WAYPOINT_TRANSMIT_RANGE, 0.0))))
                    continue;
                _Waypoint next{_Operation::Track,
                    util::uuidFromString(source->uuid()),
                    source->waypointStyle(),
                    source->waypointColor(),
                    std::monostate{}};
                if (!next.color)
                    if (const auto* team = source->getTeam(); team != nullptr && text::isColor(team->getColor())) {
                        const u32 rgb = text::getFormattingColor(team->getColor()) & 0xffffffu;
                        next.color = rgb == 0 ? 0x303030u : rgb;
                    }
                const auto pos = BlockPos(
                    math::floorTo<i32>(source->x()), math::floorTo<i32>(source->y()), math::floorTo<i32>(source->z()));
                const auto chunkX = world::toChunkCoord(pos.x), chunkZ = world::toChunkCoord(pos.z);
                if (distance > 332.0)
                    next.position = static_cast<f32>(std::atan2(
                        static_cast<f64>(receiver->x() - source->x()), static_cast<f64>(source->z() - receiver->z())));
                else if (data->chunkTracker != nullptr && data->chunkTracker->view().isChunkInView(chunkX, chunkZ))
                    next.position = _ir::play::WaypointPosition{pos.x, pos.y, pos.z};
                else
                    next.position = _ir::play::WaypointChunk{chunkX, chunkZ};
                desired.emplace(source->id(), std::move(next));
            }
        for (auto it = sent.begin(); it != sent.end();) {
            if (!desired.contains(it->first) && _send(server, playerId, _removal(it->second)))
                it = sent.erase(it);
            else
                ++it;
        }
        for (const auto& [entityId, next] : desired) {
            auto previous = sent.find(entityId);
            if (previous != sent.end()) {
                if (previous->second == next) continue;
                if (const auto* angle = std::get_if<f32>(&next.position); angle != nullptr &&
                    previous->second.position.index() == 3 && !_needsTrack(previous->second, next) &&
                    std::abs(std::remainder(*angle - std::get<f32>(previous->second.position),
                        static_cast<f32>(2.0 * math::PI))) <= 0.008726646f)
                    continue;
            }
            auto packet = next;
            if (previous != sent.end() && !_needsTrack(previous->second, next)) packet.operation = _Operation::Update;
            if (_send(server, playerId, packet)) sent[entityId] = next;
        }
    }
    for (auto it = m_sent.begin(); it != m_sent.end();) {
        if (!active.contains(it->first)) {
            for (const auto& [entityId, snapshot] : it->second) {
                (void)entityId;
                static_cast<void>(_send(server, it->first, _removal(snapshot)));
            }
            it = m_sent.erase(it);
        } else
            ++it;
    }
}

void WaypointManager::clearReceiver(IServer& server, PlayerId playerId)
{
    if (auto receiver = m_sent.find(playerId); receiver != m_sent.end()) {
        for (const auto& [entityId, snapshot] : receiver->second) {
            (void)entityId;
            static_cast<void>(_send(server, playerId, _removal(snapshot)));
        }
        m_sent.erase(receiver);
    }
}

void WaypointManager::removePlayer(IServer& server, PlayerId playerId)
{
    clearReceiver(server, playerId);
    const auto id = server.playerEntityManager().getPlayerEntityId(playerId);
    for (auto& [receiver, sent] : m_sent)
        if (auto source = sent.find(id); source != sent.end()) {
            static_cast<void>(_send(server, receiver, _removal(source->second)));
            sent.erase(source);
        }
}
} // namespace mc::server
