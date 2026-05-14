#include "PlayerAbilitiesPacket.hpp"
#include "../../entity/entities/player/GameModeUtils.hpp"
#include "../../entity/entities/player/Player.hpp"
#include "../../physics/PhysicsConstants.hpp"
#include "PacketSerializer.hpp"

namespace mc::network {

PlayerAbilitiesPacket::PlayerAbilitiesPacket()
    : Packet(PacketType::PlayerAbilities)
    , m_flags(0)
    , m_flySpeed(physics::FLY_SPEED)
    , m_walkSpeed(physics::WALK_SPEED)
{}

PlayerAbilitiesPacket::PlayerAbilitiesPacket(const PlayerAbilities& abilities)
    : Packet(PacketType::PlayerAbilities)
    , m_flags(0)
    , m_flySpeed(abilities.flySpeed)
    , m_walkSpeed(abilities.walkSpeed)
{
    setInvulnerable(abilities.invulnerable);
    setFlying(abilities.flying);
    setCanFly(abilities.canFly);
    setCreativeMode(abilities.creativeMode);
}

PlayerAbilitiesPacket PlayerAbilitiesPacket::fromPlayer(const Player& player)
{
    return PlayerAbilitiesPacket(player.abilities());
}

PlayerAbilitiesPacket PlayerAbilitiesPacket::fromGameMode(GameMode mode)
{
    // 使用 GameModeUtils 计算能力
    PlayerAbilities abilities = entity::GameModeUtils::getAbilitiesForGameMode(mode);
    return PlayerAbilitiesPacket(abilities);
}

// ============================================================================
// 序列化
// ============================================================================

Result<std::vector<u8>> PlayerAbilitiesPacket::serialize() const
{
    PacketSerializer serializer(expectedSize());

    serializer.writeU8(m_flags);
    serializer.writeF32(m_flySpeed);
    serializer.writeF32(m_walkSpeed);

    std::vector<u8> result;
    result.insert(result.end(), serializer.data(), serializer.data() + serializer.size());
    return result;
}

Result<void> PlayerAbilitiesPacket::deserialize(const u8* data, size_t size)
{
    if (size < expectedSize()) {
        return Error(ErrorCode::InvalidData, "PlayerAbilitiesPacket: insufficient data");
    }

    PacketDeserializer deserializer(data, size);

    auto flagsResult = deserializer.readU8();
    if (!flagsResult.success()) {
        return flagsResult.error();
    }
    m_flags = flagsResult.value();

    auto flySpeedResult = deserializer.readF32();
    if (!flySpeedResult.success()) {
        return flySpeedResult.error();
    }
    m_flySpeed = flySpeedResult.value();

    auto walkSpeedResult = deserializer.readF32();
    if (!walkSpeedResult.success()) {
        return walkSpeedResult.error();
    }
    m_walkSpeed = walkSpeedResult.value();

    return {};
}

size_t PlayerAbilitiesPacket::expectedSize() const
{
    // 1 byte flags + 4 bytes flySpeed + 4 bytes walkSpeed
    return 9;
}

} // namespace mc::network
