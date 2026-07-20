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

// 单元测试：PacketHandler 内部 detail 命名空间中的载具校正辅助函数
//
// 覆盖：
// - detail::sendVehicleMoveCorrection: 构造并发送 VehicleMovePacket 校正包
// - detail::isEntityCollidingWithAnythingNew: 检测载具移动到目标位置后是否与新实体碰撞
//
// 这些函数原本位于匿名命名空间，为了可测试性已移至 mc::server::core::detail 命名空间
// （声明在 PacketHandlerInternal.hpp），但仍属于内部实现细节。

#include "server/core/PacketHandlerInternal.hpp"

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include "common/network/connection/LocalServerConnection.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include <unordered_map>
#include <gtest/gtest.h>

using namespace mc;
using mc::network::LocalConnectionPair;
using mc::network::LocalServerConnection;
using mc::network::PacketType;
using mc::network::VehicleMovePacket;
using mc::server::core::ConnectionManager;
using mc::server::core::PlayerManager;
using mc::server::core::detail::isEntityCollidingWithAnythingNew;
using mc::server::core::detail::kMaxVehicleSpeedSq;
using mc::server::core::detail::kMovedWronglyThresholdSq;
using mc::server::core::detail::sendVehicleMoveCorrection;

namespace {

/// 测试用实体：默认 canBeCollidedWith() 返回 true（与 Entity 默认行为一致）
class TestVehicleEntity : public Entity {
public:
    explicit TestVehicleEntity(EntityInstanceId id)
        : Entity(id)
    {}

    /// 带 World 的构造：用于需要骑乘链遍历的测试（isRidingSameEntity 需要 m_world）
    explicit TestVehicleEntity(EntityInstanceId id, IWorld* world)
        : Entity(id, world)
    {}

    void tick() override {}
};

/// 测试用实体：重写 canBeCollidedWith() 返回 false（模拟不可碰撞实体，如掉落物）
class NonCollidableTestEntity : public Entity {
public:
    explicit NonCollidableTestEntity(EntityInstanceId id)
        : Entity(id)
    {}

    void tick() override {}

    [[nodiscard]] bool canBeCollidedWith() const override { return false; }
};

/// 测试用世界：可注入自定义实体集合，用于 isEntityCollidingWithAnythingNew 测试
class CollisionTestWorld : public mc::test::BaseTestWorld {
public:
    /// 由测试用例填充：在 getEntitiesInAABB 调用时返回的实体集合
    mutable std::vector<Entity*> m_injectedEntities;

    /// 由测试用例填充：在 getEntity 调用时返回的实体映射
    /// （用于支持 isRidingSameEntity 通过 world->getEntity 遍历骑乘链）
    std::unordered_map<EntityInstanceId, Entity*> m_entityRegistry;

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return m_injectedEntities;
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        const auto it = m_entityRegistry.find(id);
        return it != m_entityRegistry.end() ? it->second : nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        const auto it = m_entityRegistry.find(id);
        return it != m_entityRegistry.end() ? it->second : nullptr;
    }
};

/// 辅助：构造一个 PlayerManager + ConnectionManager + 已连接玩家，
/// 并返回玩家 ID 与 LocalConnectionPair，用于 sendVehicleMoveCorrection 测试
struct ConnectionFixture {
    std::unique_ptr<LocalConnectionPair> connectionPair;
    std::shared_ptr<LocalServerConnection> serverConnection;
    std::unique_ptr<PlayerManager> playerManager;
    std::unique_ptr<ConnectionManager> connectionManager;
    PlayerId playerId = 1;

    void setUp()
    {
        connectionPair = std::make_unique<LocalConnectionPair>();
        connectionPair->connect();
        serverConnection = std::make_shared<LocalServerConnection>(&connectionPair->serverEndpoint());
        playerManager = std::make_unique<PlayerManager>(20);
        connectionManager = std::make_unique<ConnectionManager>(*playerManager);

        auto* player = playerManager->addPlayer(playerId,
            mc::util::uuidToString(mc::util::generateOfflineUuid("TestPlayer")),
            "TestPlayer",
            serverConnection);
        ASSERT_NE(player, nullptr);
    }

    /// 读取客户端端点接收到的原始字节
    std::vector<u8> receiveRaw()
    {
        std::vector<u8> buffer;
        connectionPair->clientEndpoint().receive(buffer);
        return buffer;
    }
};

} // namespace

// ============================================================================
// detail::sendVehicleMoveCorrection 单元测试
// ============================================================================

TEST(PacketHandlerMoveVehicleHelpersTest, SendVehicleMoveCorrection_SendsPacketWithCorrectPositionAndRotation)
{
    ConnectionFixture f;
    f.setUp();

    // 构造一个载具实体并设置朝向
    TestVehicleEntity vehicle(EntityInstanceId{1});
    vehicle.setPosition(10.0f, 64.0f, 20.0f);
    vehicle.setRotation(90.0f, 45.0f);

    // 校正位置（模拟服务端已知位置）
    const Vector3 correctionPos(100.0f, 64.0f, 200.0f);

    sendVehicleMoveCorrection(*f.connectionManager, f.playerId, vehicle, correctionPos);

    // 从客户端端点读取数据
    auto received = f.receiveRaw();
    ASSERT_GT(received.size(), mc::network::PACKET_HEADER_SIZE);

    // 解析头部（大端序）
    const auto* headerBytes = received.data();
    const mc::u32 totalSize = static_cast<mc::u32>(headerBytes[0]) << 24 | static_cast<mc::u32>(headerBytes[1]) << 16 |
        static_cast<mc::u32>(headerBytes[2]) << 8 | static_cast<mc::u32>(headerBytes[3]);
    const mc::u16 packetType = static_cast<mc::u16>(headerBytes[4]) << 8 | static_cast<mc::u16>(headerBytes[5]);
    EXPECT_EQ(static_cast<PacketType>(packetType), PacketType::VehicleMove);
    EXPECT_GE(totalSize, mc::network::PACKET_HEADER_SIZE + 32); // 头部 + 32 字节负载

    // 反序列化负载并验证校正包内容
    VehicleMovePacket receivedPacket;
    auto parseResult = receivedPacket.deserialize(
        received.data() + mc::network::PACKET_HEADER_SIZE, received.size() - mc::network::PACKET_HEADER_SIZE);
    ASSERT_TRUE(parseResult.success());

    // 校正包中的位置应是传入的 correctionPos（f32 → f64 隐式扩展）
    EXPECT_DOUBLE_EQ(receivedPacket.x(), 100.0);
    EXPECT_DOUBLE_EQ(receivedPacket.y(), 64.0);
    EXPECT_DOUBLE_EQ(receivedPacket.z(), 200.0);
    // 朝向取自 vehicle.yaw() / vehicle.pitch()
    EXPECT_FLOAT_EQ(receivedPacket.yaw(), 90.0f);
    EXPECT_FLOAT_EQ(receivedPacket.pitch(), 45.0f);
}

TEST(PacketHandlerMoveVehicleHelpersTest, SendVehicleMoveCorrection_ReturnsFalseForNonExistentPlayer)
{
    PlayerManager playerManager(20);
    ConnectionManager connectionManager(playerManager);

    TestVehicleEntity vehicle(EntityInstanceId{1});
    vehicle.setPosition(0.0f, 64.0f, 0.0f);
    vehicle.setRotation(0.0f, 0.0f);

    const Vector3 correctionPos(1.0f, 64.0f, 1.0f);

    // 发送给不存在的玩家：sendPacketToPlayer 返回 false，但 sendVehicleMoveCorrection 本身无返回值，
    // 这里仅验证不崩溃且不抛异常
    EXPECT_NO_THROW(sendVehicleMoveCorrection(connectionManager, /*playerId=*/9999, vehicle, correctionPos));
}

TEST(PacketHandlerMoveVehicleHelpersTest, SendVehicleMoveCorrection_UsesVehicleRotationNotPosition)
{
    // 验证校正包的 rotation 字段取自 vehicle.yaw()/pitch()，
    // 而 position 字段取自传入的 correctionPos（而非 vehicle.position()）
    // 这是 MC Java ClientboundMoveVehiclePacket.fromEntity(entity) 行为的关键差异：
    // 服务端用 vehicle 的旧位置作为校正位置，但朝向用 vehicle 当前朝向
    ConnectionFixture f;
    f.setUp();

    TestVehicleEntity vehicle(EntityInstanceId{1});
    vehicle.setPosition(0.0f, 0.0f, 0.0f); // 载具当前在原点
    vehicle.setRotation(180.0f, -30.0f);   // 朝向

    const Vector3 correctionPos(500.0f, 100.0f, -500.0f); // 校正到 (500, 100, -500)

    sendVehicleMoveCorrection(*f.connectionManager, f.playerId, vehicle, correctionPos);

    auto received = f.receiveRaw();
    ASSERT_GT(received.size(), mc::network::PACKET_HEADER_SIZE);

    VehicleMovePacket receivedPacket;
    ASSERT_TRUE(receivedPacket
            .deserialize(
                received.data() + mc::network::PACKET_HEADER_SIZE, received.size() - mc::network::PACKET_HEADER_SIZE)
            .success());

    // 位置应是 correctionPos（500, 100, -500），而非 vehicle.position()（0, 0, 0）
    EXPECT_DOUBLE_EQ(receivedPacket.x(), 500.0);
    EXPECT_DOUBLE_EQ(receivedPacket.y(), 100.0);
    EXPECT_DOUBLE_EQ(receivedPacket.z(), -500.0);

    // 朝向应是 vehicle 的朝向（180, -30）
    EXPECT_FLOAT_EQ(receivedPacket.yaw(), 180.0f);
    EXPECT_FLOAT_EQ(receivedPacket.pitch(), -30.0f);
}

// ============================================================================
// detail::isEntityCollidingWithAnythingNew 单元测试
// ============================================================================

TEST(PacketHandlerMoveVehicleHelpersTest, IsEntityCollidingWithAnythingNew_ReturnsFalseWhenNoEntities)
{
    CollisionTestWorld world;
    world.m_injectedEntities = {}; // 空世界

    TestVehicleEntity vehicle(EntityInstanceId{1});
    vehicle.setPosition(0.0f, 64.0f, 0.0f);
    const AxisAlignedBB oldAABB = vehicle.boundingBox();
    const Vector3 targetPos(1.0f, 64.0f, 1.0f);

    EXPECT_FALSE(isEntityCollidingWithAnythingNew(world, vehicle, oldAABB, targetPos));
}

TEST(PacketHandlerMoveVehicleHelpersTest, IsEntityCollidingWithAnythingNew_ReturnsTrueWhenCollidableEntityAtTarget)
{
    CollisionTestWorld world;

    TestVehicleEntity vehicle(EntityInstanceId{1});
    vehicle.setPosition(0.0f, 64.0f, 0.0f);

    // 在目标位置放一个可碰撞实体
    TestVehicleEntity obstacle(EntityInstanceId{2});
    obstacle.setPosition(1.0f, 64.0f, 1.0f);
    world.m_injectedEntities = {&obstacle};

    const AxisAlignedBB oldAABB = vehicle.boundingBox();
    const Vector3 targetPos(1.0f, 64.0f, 1.0f);

    // obstacle 在目标位置，且 canBeCollidedWith() 默认为 true → 应检测到碰撞
    EXPECT_TRUE(isEntityCollidingWithAnythingNew(world, vehicle, oldAABB, targetPos));
}

TEST(PacketHandlerMoveVehicleHelpersTest, IsEntityCollidingWithAnythingNew_ReturnsFalseWhenEntityNotCollidable)
{
    CollisionTestWorld world;

    TestVehicleEntity vehicle(EntityInstanceId{1});
    vehicle.setPosition(0.0f, 64.0f, 0.0f);

    // 在目标位置放一个不可碰撞实体（canBeCollidedWith() 返回 false）
    NonCollidableTestEntity obstacle(EntityInstanceId{2});
    obstacle.setPosition(1.0f, 64.0f, 1.0f);
    world.m_injectedEntities = {&obstacle};

    const AxisAlignedBB oldAABB = vehicle.boundingBox();
    const Vector3 targetPos(1.0f, 64.0f, 1.0f);

    // obstacle 不可碰撞 → 应过滤掉，返回 false
    EXPECT_FALSE(isEntityCollidingWithAnythingNew(world, vehicle, oldAABB, targetPos));
}

TEST(PacketHandlerMoveVehicleHelpersTest, IsEntityCollidingWithAnythingNew_ExcludesSelfFromCollisionCheck)
{
    CollisionTestWorld world;

    TestVehicleEntity vehicle(EntityInstanceId{1});
    vehicle.setPosition(0.0f, 64.0f, 0.0f);

    // 将 vehicle 自身注入世界实体集合（模拟 getEntitiesInAABB 返回 vehicle 的情况）
    // 辅助函数应通过 except=&vehicle 参数排除自身
    world.m_injectedEntities = {&vehicle};

    const AxisAlignedBB oldAABB = vehicle.boundingBox();
    const Vector3 targetPos(1.0f, 64.0f, 1.0f);

    // 即使 vehicle 在返回列表中，也应被排除（因为 except=&vehicle）
    EXPECT_FALSE(isEntityCollidingWithAnythingNew(world, vehicle, oldAABB, targetPos));
}

TEST(PacketHandlerMoveVehicleHelpersTest, IsEntityCollidingWithAnythingNew_SkipsNullptrEntries)
{
    CollisionTestWorld world;

    TestVehicleEntity vehicle(EntityInstanceId{1});
    vehicle.setPosition(0.0f, 64.0f, 0.0f);

    // 注入一个 nullptr（模拟 EntityManager 返回空槽位的情况）
    world.m_injectedEntities = {nullptr};

    const AxisAlignedBB oldAABB = vehicle.boundingBox();
    const Vector3 targetPos(1.0f, 64.0f, 1.0f);

    // nullptr 应被安全跳过，不崩溃，返回 false
    EXPECT_NO_THROW(isEntityCollidingWithAnythingNew(world, vehicle, oldAABB, targetPos));
}

TEST(PacketHandlerMoveVehicleHelpersTest, IsEntityCollidingWithAnythingNew_ReturnsTrueWithMixedCollidables)
{
    CollisionTestWorld world;

    TestVehicleEntity vehicle(EntityInstanceId{1});
    vehicle.setPosition(0.0f, 64.0f, 0.0f);

    NonCollidableTestEntity nonCollidable(EntityInstanceId{2});
    nonCollidable.setPosition(1.0f, 64.0f, 1.0f);

    TestVehicleEntity collidable(EntityInstanceId{3});
    collidable.setPosition(1.0f, 64.0f, 1.0f);

    // 混合：[nullptr, 不可碰撞, 可碰撞, vehicle自身]
    world.m_injectedEntities = {nullptr, &nonCollidable, &collidable, &vehicle};

    const AxisAlignedBB oldAABB = vehicle.boundingBox();
    const Vector3 targetPos(1.0f, 64.0f, 1.0f);

    // 应跳过 nullptr、nonCollidable、vehicle 自身，遇到 collidable 返回 true
    EXPECT_TRUE(isEntityCollidingWithAnythingNew(world, vehicle, oldAABB, targetPos));
}

TEST(PacketHandlerMoveVehicleHelpersTest, IsEntityCollidingWithAnythingNew_ZeroMovementReturnsCurrentEntities)
{
    // 边界场景：目标位置与当前位置相同（零位移）
    // 此时 targetAABB 应等于 oldAABB（grow border 后），仍查询该范围内的实体
    CollisionTestWorld world;

    TestVehicleEntity vehicle(EntityInstanceId{1});
    vehicle.setPosition(5.0f, 64.0f, 5.0f);

    TestVehicleEntity obstacle(EntityInstanceId{2});
    obstacle.setPosition(5.0f, 64.0f, 5.0f); // 与 vehicle 重叠
    world.m_injectedEntities = {&obstacle};

    const AxisAlignedBB oldAABB = vehicle.boundingBox();
    const Vector3 targetPos(5.0f, 64.0f, 5.0f); // 零位移

    EXPECT_TRUE(isEntityCollidingWithAnythingNew(world, vehicle, oldAABB, targetPos));
}

TEST(PacketHandlerMoveVehicleHelpersTest, IsEntityCollidingWithAnythingNew_FiltersPassengerOfSameVehicle)
{
    // 对齐 MC Java Entity.canCollideWith：
    //   return other.canBeCollidedWith() && !isPassengerOfSameVehicle(other);
    // 即：若候选实体与载具同处一条骑乘链（同一 root vehicle），
    // 不视为"新"碰撞，避免载具把自己的乘客误判为碰撞物而触发回退。
    //
    // 构造：vehicle(boat) + passenger(player) 都在目标位置，
    //       两者 AABB 相交，但 passenger 是 vehicle 的乘客，
    //       isEntityCollidingWithAnythingNew 应返回 false。
    CollisionTestWorld world;

    // 用带 World 的构造创建实体，使 isRidingSameEntity 能通过 world->getEntity 遍历骑乘链
    TestVehicleEntity vehicle(EntityInstanceId{1}, &world);
    vehicle.setPosition(0.0f, 64.0f, 0.0f);

    TestVehicleEntity passenger(EntityInstanceId{2}, &world);
    passenger.setPosition(0.0f, 64.0f, 0.0f); // 与 vehicle 重叠

    // 注册到世界的实体映射，使 world->getEntity 能解析
    world.m_entityRegistry[EntityInstanceId{1}] = &vehicle;
    world.m_entityRegistry[EntityInstanceId{2}] = &passenger;

    // 建立骑乘关系：passenger 骑上 vehicle（同一条骑乘链）
    ASSERT_TRUE(passenger.startRiding(vehicle));
    ASSERT_TRUE(passenger.isRiding());
    ASSERT_EQ(passenger.getVehicle(), vehicle.id());

    // 验证 canCollideWith 语义：vehicle 不应与自己的乘客碰撞
    EXPECT_FALSE(vehicle.canCollideWith(passenger))
        << "Vehicle should not collide with its own passenger (isPassengerOfSameVehicle)";

    // 注入乘客到查询结果（模拟 getEntitiesInAABB 返回 passenger）
    world.m_injectedEntities = {&passenger};

    const AxisAlignedBB oldAABB = vehicle.boundingBox();
    const Vector3 targetPos(1.0f, 64.0f, 1.0f);

    // 由于 passenger 与 vehicle 同处一条骑乘链，应被过滤掉，返回 false
    EXPECT_FALSE(isEntityCollidingWithAnythingNew(world, vehicle, oldAABB, targetPos));
}

TEST(PacketHandlerMoveVehicleHelpersTest, IsEntityCollidingWithAnythingNew_FiltersIndirectPassengerOfSameVehicle)
{
    // 间接骑乘链：A 骑 B，C 骑 A，则 B 和 C 同处一条骑乘链（root = B），
    // 互相不应碰撞。
    CollisionTestWorld world;

    TestVehicleEntity vehicleB(EntityInstanceId{1}, &world);
    vehicleB.setPosition(0.0f, 64.0f, 0.0f);

    TestVehicleEntity middleA(EntityInstanceId{2}, &world);
    middleA.setPosition(0.0f, 64.0f, 0.0f);

    TestVehicleEntity passengerC(EntityInstanceId{3}, &world);
    passengerC.setPosition(1.0f, 64.0f, 1.0f); // 与 vehicle 的目标位置重叠

    world.m_entityRegistry[EntityInstanceId{1}] = &vehicleB;
    world.m_entityRegistry[EntityInstanceId{2}] = &middleA;
    world.m_entityRegistry[EntityInstanceId{3}] = &passengerC;

    // 建立链：middleA 骑 vehicleB，passengerC 骑 middleA
    ASSERT_TRUE(middleA.startRiding(vehicleB));
    ASSERT_TRUE(passengerC.startRiding(middleA));

    // 验证 canCollideWith：vehicleB 不应与 passengerC 碰撞（同一条骑乘链）
    EXPECT_FALSE(vehicleB.canCollideWith(passengerC))
        << "Vehicle should not collide with indirect passenger (same root vehicle)";

    // 注入 passengerC 到查询结果
    world.m_injectedEntities = {&passengerC};

    const AxisAlignedBB oldAABB = vehicleB.boundingBox();
    const Vector3 targetPos(1.0f, 64.0f, 1.0f);

    EXPECT_FALSE(isEntityCollidingWithAnythingNew(world, vehicleB, oldAABB, targetPos));
}

// ============================================================================
// detail 常量正确性验证
// ============================================================================

TEST(PacketHandlerMoveVehicleHelpersTest, MovedWronglyThresholdMatchesMcJava)
{
    // MC Java: d10 > 0.0625 → 0.25² = 0.0625
    EXPECT_DOUBLE_EQ(kMovedWronglyThresholdSq, 0.0625);
}

TEST(PacketHandlerMoveVehicleHelpersTest, MaxVehicleSpeedSqMatchesMcJava)
{
    // MC Java: d9 - d10 > 100.0
    EXPECT_DOUBLE_EQ(kMaxVehicleSpeedSq, 100.0);
}
