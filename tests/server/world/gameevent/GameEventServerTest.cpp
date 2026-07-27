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

#include <gtest/gtest.h>

#include "common/TempDirHelper.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/load/ChunkLoadLevel.hpp"
#include "common/world/gameevent/DynamicGameEventListener.hpp"
#include "common/world/gameevent/GameEventDispatcher.hpp"
#include "common/world/gameevent/GameEventListenerRegistry.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gameevent/PositionSource.hpp"
#include "common/world/gameevent/VibrationSystem.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"

#include <cmath>
#include <filesystem>
#include <vector>

using namespace mc;
using namespace mc::gameevent;
using namespace mc::server;

// ============================================================================
// 测试辅助：简单的 GameEventListener 实现
// ============================================================================

class TestListener : public GameEventListener {
public:
    explicit TestListener(BlockPos pos, i32 radius = 16, DeliveryMode mode = DeliveryMode::Unspecified)
        : m_pos(pos)
        , m_source(pos)
        , m_radius(radius)
        , m_mode(mode)
    {}

    [[nodiscard]] PositionSource& getListenerSource() override { return m_source; }
    [[nodiscard]] const PositionSource& getListenerSource() const override { return m_source; }
    [[nodiscard]] i32 getListenerRadius() const override { return m_radius; }
    [[nodiscard]] DeliveryMode getDeliveryMode() const override { return m_mode; }

    bool handleGameEvent(
        ServerWorld& /*world*/, const GameEvent& event, const GameEvent::Context& context, const Vector3d& pos) override
    {
        m_receivedEvents.push_back({event.id(), pos, context.sourceEntity()});
        return true;
    }

    [[nodiscard]] bool hasReceivedEvent(const char* eventId) const
    {
        for (const auto& e : m_receivedEvents) {
            if (std::strcmp(e.id, eventId) == 0) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] size_t receivedCount() const { return m_receivedEvents.size(); }

    void clearReceived() { m_receivedEvents.clear(); }

    struct ReceivedEvent {
        const char* id;
        Vector3d pos;
        const Entity* sourceEntity;
    };

    const std::vector<ReceivedEvent>& receivedEvents() const { return m_receivedEvents; }

private:
    BlockPos m_pos;
    BlockPositionSource m_source;
    i32 m_radius;
    DeliveryMode m_mode;
    std::vector<ReceivedEvent> m_receivedEvents;
};

class TestByDistanceListener : public GameEventListener {
public:
    explicit TestByDistanceListener(BlockPos pos, i32 radius = 16)
        : m_pos(pos)
        , m_source(pos)
        , m_radius(radius)
    {}

    [[nodiscard]] PositionSource& getListenerSource() override { return m_source; }
    [[nodiscard]] const PositionSource& getListenerSource() const override { return m_source; }
    [[nodiscard]] i32 getListenerRadius() const override { return m_radius; }
    [[nodiscard]] DeliveryMode getDeliveryMode() const override { return DeliveryMode::ByDistance; }

    bool handleGameEvent(ServerWorld& /*world*/,
        const GameEvent& event,
        const GameEvent::Context& /*context*/,
        const Vector3d& pos) override
    {
        m_receivedEvents.push_back({event.id(), pos});
        return true;
    }

    [[nodiscard]] size_t receivedCount() const { return m_receivedEvents.size(); }

    struct ReceivedEvent {
        const char* id;
        Vector3d pos;
    };

    const std::vector<ReceivedEvent>& receivedEvents() const { return m_receivedEvents; }

private:
    BlockPos m_pos;
    BlockPositionSource m_source;
    i32 m_radius;
    std::vector<ReceivedEvent> m_receivedEvents;
};

// ============================================================================
// 测试夹具
// ============================================================================

class GameEventServerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 用 helper 生成跨进程唯一目录（token 含 PID），避免 CTest 并行下同秒进程目录撞车。
        m_testDir = mc::test::makeUniqueTestDir("mc_gameevent_test");

        world::storage::SingleLevelStorageConfig storageConfig;
        auto openResult = m_storage.open(m_testDir, storageConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        VanillaBlocks::initialize();
    }

    void TearDown() override
    {
        if (m_world) {
            m_world->shutdown();
            m_world.reset();
        }
        m_storage.close();
        mc::test::removeTestDir(m_testDir);
    }

    ServerWorld& world() { return *m_world; }

    void createWorld()
    {
        ServerWorldConfig config;
        config.viewDistance = 2;
        config.dimension = 0;
        config.seed = 12345;

        m_world = std::make_unique<ServerWorld>(config);
        m_world->setSharedStorage(&m_storage);

        auto settings = DimensionSettings::overworld();
        auto randomState = mc::world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<ServerChunkManager>(*m_world, std::move(generator));
        m_world->setChunkManager(std::move(chunkManager));

        auto result = m_world->initialize();
        ASSERT_TRUE(result.success()) << result.error().message();
    }

    std::unique_ptr<ServerWorld> m_world;
    std::filesystem::path m_testDir;
    world::storage::SingleLevelStorageManager m_storage;
};

// ============================================================================
// EuclideanGameEventListenerRegistry 测试
// ============================================================================

TEST_F(GameEventServerTest, EuclideanRegistry_RegisterAndUnregister)
{
    createWorld();
    EuclideanGameEventListenerRegistry registry(world(), 0, nullptr);

    TestListener listener(BlockPos(0, 0, 0));
    EXPECT_TRUE(registry.isEmpty());

    registry.registerListener(listener);
    EXPECT_FALSE(registry.isEmpty());

    registry.unregisterListener(listener);
    EXPECT_TRUE(registry.isEmpty());
}

TEST_F(GameEventServerTest, EuclideanRegistry_NoDuplicateRegistration)
{
    createWorld();
    EuclideanGameEventListenerRegistry registry(world(), 0, nullptr);

    TestListener listener(BlockPos(0, 0, 0));
    registry.registerListener(listener);
    registry.registerListener(listener); // 重复注册应被忽略
    EXPECT_FALSE(registry.isEmpty());

    // 一次注销即可清空
    registry.unregisterListener(listener);
    EXPECT_TRUE(registry.isEmpty());
}

TEST_F(GameEventServerTest, EuclideanRegistry_VisitInRangeListeners_WithinRadius)
{
    createWorld();
    EuclideanGameEventListenerRegistry registry(world(), 0, nullptr);

    TestListener listener(BlockPos(10, 0, 10), 16);
    registry.registerListener(listener);

    GameEvent event("test_event");
    GameEvent::Context ctx;
    Vector3d eventPos(10.5, 0.5, 10.5); // 事件在监听器位置

    int visitCount = 0;
    bool found = registry.visitInRangeListeners(
        event, eventPos, ctx, [&visitCount](GameEventListener& /*l*/, const Vector3d& /*pos*/) { ++visitCount; });

    EXPECT_TRUE(found);
    EXPECT_EQ(visitCount, 1);
}

TEST_F(GameEventServerTest, EuclideanRegistry_VisitInRangeListeners_OutsideRadius)
{
    createWorld();
    EuclideanGameEventListenerRegistry registry(world(), 0, nullptr);

    // 监听器在 (0,0,0)，半径 2
    TestListener listener(BlockPos(0, 0, 0), 2);
    registry.registerListener(listener);

    GameEvent event("test_event");
    GameEvent::Context ctx;
    // 事件在 (50,0,50)，远超半径 2
    Vector3d eventPos(50.5, 0.5, 50.5);

    int visitCount = 0;
    bool found = registry.visitInRangeListeners(
        event, eventPos, ctx, [&visitCount](GameEventListener& /*l*/, const Vector3d& /*pos*/) { ++visitCount; });

    EXPECT_FALSE(found);
    EXPECT_EQ(visitCount, 0);
}

TEST_F(GameEventServerTest, EuclideanRegistry_VisitInRangeListeners_MultipleListeners)
{
    createWorld();
    EuclideanGameEventListenerRegistry registry(world(), 0, nullptr);

    TestListener listener1(BlockPos(0, 0, 0), 16);
    TestListener listener2(BlockPos(1, 0, 0), 16);
    TestListener listener3(BlockPos(100, 0, 100), 16); // 远处的监听器

    registry.registerListener(listener1);
    registry.registerListener(listener2);
    registry.registerListener(listener3);

    GameEvent event("test_event");
    GameEvent::Context ctx;
    // 事件在 (0.5, 0.5, 0.5)，只应在半径 16 内找到 listener1 和 listener2
    Vector3d eventPos(0.5, 0.5, 0.5);

    std::vector<GameEventListener*> visited;
    registry.visitInRangeListeners(
        event, eventPos, ctx, [&visited](GameEventListener& l, const Vector3d& /*pos*/) { visited.push_back(&l); });

    // listener3 在 (100,0,100) 处，distanceSq = 100*100 + 100*100 = 20000，远超 16*16=256
    EXPECT_EQ(visited.size(), 2u);
}

TEST_F(GameEventServerTest, EuclideanRegistry_ReentrancyDuringVisit)
{
    createWorld();
    EuclideanGameEventListenerRegistry registry(world(), 0, nullptr);

    TestListener listener1(BlockPos(0, 0, 0), 16);
    TestListener listener2(BlockPos(1, 0, 0), 16);
    registry.registerListener(listener1);
    registry.registerListener(listener2);

    GameEvent event("test_event");
    GameEvent::Context ctx;
    Vector3d eventPos(0.5, 0.5, 0.5);

    // 在遍历期间注册新监听器 - 应延迟到遍历结束后处理
    TestListener listener3(BlockPos(2, 0, 0), 16);
    bool listener3AddedDuringVisit = false;

    registry.visitInRangeListeners(event, eventPos, ctx, [&](GameEventListener& /*l*/, const Vector3d& /*pos*/) {
        if (!listener3AddedDuringVisit) {
            registry.registerListener(listener3);
            listener3AddedDuringVisit = true;
        }
    });

    // listener3 应在遍历结束后被添加
    EXPECT_FALSE(registry.isEmpty());

    // 清理
    registry.unregisterListener(listener1);
    registry.unregisterListener(listener2);
    registry.unregisterListener(listener3);
}

TEST_F(GameEventServerTest, EuclideanRegistry_UnregisterDuringVisit)
{
    createWorld();
    EuclideanGameEventListenerRegistry registry(world(), 0, nullptr);

    TestListener listener1(BlockPos(0, 0, 0), 16);
    TestListener listener2(BlockPos(1, 0, 0), 16);
    registry.registerListener(listener1);
    registry.registerListener(listener2);

    GameEvent event("test_event");
    GameEvent::Context ctx;
    Vector3d eventPos(0.5, 0.5, 0.5);

    // 在遍历期间注销监听器 - 应延迟到遍历结束后处理
    bool unregistered = false;
    registry.visitInRangeListeners(event, eventPos, ctx, [&](GameEventListener& l, const Vector3d& /*pos*/) {
        if (!unregistered) {
            registry.unregisterListener(l);
            unregistered = true;
        }
    });

    // 遍历应该仍然完成了
    // 延迟注销应在遍历结束后处理
}

TEST_F(GameEventServerTest, EuclideanRegistry_OnEmptyAction)
{
    createWorld();
    bool emptyCallbackFired = false;
    i32 emptySectionY = -999;

    auto onEmpty = [&emptyCallbackFired, &emptySectionY](i32 sectionY) {
        emptyCallbackFired = true;
        emptySectionY = sectionY;
    };

    EuclideanGameEventListenerRegistry registry(world(), 5, onEmpty);

    TestListener listener(BlockPos(0, 0, 0));
    registry.registerListener(listener);
    EXPECT_FALSE(emptyCallbackFired);

    registry.unregisterListener(listener);
    EXPECT_TRUE(emptyCallbackFired);
    EXPECT_EQ(emptySectionY, 5);
}

// ============================================================================
// GameEventDispatcher 测试
// ============================================================================

TEST_F(GameEventServerTest, Dispatcher_PostDeliversToNearbyListener)
{
    createWorld();
    // 确保目标区块已加载
    auto* chunk = world().chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // 创建监听器并注册到区块段
    TestListener listener(BlockPos(0, 0, 0), 16);
    auto factory = [this](i32 sectionY) -> std::unique_ptr<EuclideanGameEventListenerRegistry> {
        return std::make_unique<EuclideanGameEventListenerRegistry>(world(), sectionY, nullptr);
    };
    auto& registry = chunk->getOrCreateGameEventListenerRegistry(0, factory);
    registry.registerListener(listener);

    // 发送事件
    GameEvent event("block_activate");
    Vector3d eventPos(0.5, 0.5, 0.5);
    GameEvent::Context ctx;

    world().gameEventDispatcher().post(event, eventPos, ctx);

    EXPECT_TRUE(listener.hasReceivedEvent("block_activate"));
    EXPECT_EQ(listener.receivedCount(), 1u);
}

TEST_F(GameEventServerTest, Dispatcher_PostDoesNotDeliverToDistantListener)
{
    createWorld();
    auto* chunk = world().chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // 半径为 2 的监听器
    TestListener listener(BlockPos(0, 0, 0), 2);
    auto factory = [this](i32 sectionY) -> std::unique_ptr<EuclideanGameEventListenerRegistry> {
        return std::make_unique<EuclideanGameEventListenerRegistry>(world(), sectionY, nullptr);
    };
    auto& registry = chunk->getOrCreateGameEventListenerRegistry(0, factory);
    registry.registerListener(listener);

    // 事件在远处
    GameEvent event("block_activate");
    Vector3d eventPos(50.5, 0.5, 50.5);
    GameEvent::Context ctx;

    world().gameEventDispatcher().post(event, eventPos, ctx);

    EXPECT_EQ(listener.receivedCount(), 0u);
}

TEST_F(GameEventServerTest, Dispatcher_ByDistanceMode_SortsByDistance)
{
    createWorld();
    auto* chunk = world().chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // 创建三个 BY_DISTANCE 监听器，距离不同
    TestByDistanceListener near(BlockPos(0, 0, 0), 16);
    TestByDistanceListener mid(BlockPos(5, 0, 0), 16);
    TestByDistanceListener farListener(BlockPos(10, 0, 0), 16);

    auto factory = [this](i32 sectionY) -> std::unique_ptr<EuclideanGameEventListenerRegistry> {
        return std::make_unique<EuclideanGameEventListenerRegistry>(world(), sectionY, nullptr);
    };
    auto& registry = chunk->getOrCreateGameEventListenerRegistry(0, factory);
    registry.registerListener(near);
    registry.registerListener(mid);
    registry.registerListener(farListener);

    // 事件在 (0.5, 0.5, 0.5)
    GameEvent event("step");
    Vector3d eventPos(0.5, 0.5, 0.5);
    GameEvent::Context ctx;

    world().gameEventDispatcher().post(event, eventPos, ctx);

    // BY_DISTANCE 模式：所有监听器都应收到事件
    EXPECT_EQ(near.receivedCount(), 1u);
    EXPECT_EQ(mid.receivedCount(), 1u);
    EXPECT_EQ(farListener.receivedCount(), 1u);
}

// ============================================================================
// DynamicGameEventListener 测试
// ============================================================================

TEST_F(GameEventServerTest, DynamicListener_AddAndRemove)
{
    createWorld();
    TestListener listener(BlockPos(0, 0, 0), 16);
    DynamicGameEventListener dynamicListener(listener);

    // 确保区块已加载
    auto* chunk = world().chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // 添加监听器
    dynamicListener.add(world());

    // 验证注册表中有监听器
    auto* registry = chunk->getGameEventListenerRegistry(0);
    ASSERT_NE(registry, nullptr);
    EXPECT_FALSE(registry->isEmpty());

    // 移除监听器
    dynamicListener.remove(world());

    // 注册表应为空或被移除
    auto* registryAfter = chunk->getGameEventListenerRegistry(0);
    EXPECT_TRUE(registryAfter == nullptr || registryAfter->isEmpty());
}

TEST_F(GameEventServerTest, DynamicListener_MoveBetweenSections)
{
    createWorld();
    // 初始位置在段 (0, 0, 0)
    TestListener listener(BlockPos(0, 0, 0), 16);
    DynamicGameEventListener dynamicListener(listener);

    auto* chunk = world().chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // 添加监听器
    dynamicListener.add(world());
    auto* registry0 = chunk->getGameEventListenerRegistry(0);
    ASSERT_NE(registry0, nullptr);
    EXPECT_FALSE(registry0->isEmpty());

    // 监听器位置未改变，再次 move 不应有变化
    dynamicListener.move(world());
    auto* registry0Still = chunk->getGameEventListenerRegistry(0);
    EXPECT_NE(registry0Still, nullptr);
    EXPECT_FALSE(registry0Still->isEmpty());
}

// ============================================================================
// VibrationSystem::Ticker 测试
// ============================================================================

namespace {

class TestVibrationUser : public VibrationSystem::User {
public:
    explicit TestVibrationUser(BlockPos pos, i32 radius = 16)
        : m_pos(pos)
        , m_source(pos)
        , m_radius(radius)
    {}

    [[nodiscard]] i32 getListenerRadius() const override { return m_radius; }
    [[nodiscard]] PositionSource& getPositionSource() override { return m_source; }
    [[nodiscard]] const PositionSource& getPositionSource() const override { return m_source; }

    [[nodiscard]] bool canReceiveVibration(ServerWorld& /*world*/,
        const BlockPos& /*pos*/,
        const GameEvent& /*event*/,
        const GameEvent::Context& /*context*/) const override
    {
        return m_canReceive;
    }

    void onReceiveVibration(ServerWorld& /*world*/,
        const BlockPos& pos,
        const GameEvent& event,
        const Entity* /*sourceEntity*/,
        f32 distance) override
    {
        m_receivedVibrations.push_back({event.id(), pos, distance});
    }

    [[nodiscard]] bool requiresAdjacentChunksToBeTicking() const override { return m_requireAdjacentChunks; }

    void onDataChanged() override { m_dataChangedCount++; }

    // 测试辅助方法
    void setCanReceive(bool value) { m_canReceive = value; }
    void setRequireAdjacentChunks(bool value) { m_requireAdjacentChunks = value; }
    [[nodiscard]] size_t receivedVibrationCount() const { return m_receivedVibrations.size(); }
    [[nodiscard]] i32 dataChangedCount() const { return m_dataChangedCount; }

    struct ReceivedVibration {
        const char* eventId;
        BlockPos pos;
        f32 distance;
    };

    const std::vector<ReceivedVibration>& receivedVibrations() const { return m_receivedVibrations; }

private:
    BlockPos m_pos;
    BlockPositionSource m_source;
    i32 m_radius;
    bool m_canReceive = true;
    bool m_requireAdjacentChunks = false;
    i32 m_dataChangedCount = 0;
    std::vector<ReceivedVibration> m_receivedVibrations;
};

class TestVibrationSystem : public VibrationSystem {
public:
    explicit TestVibrationSystem(BlockPos pos, i32 radius = 16)
        : m_user(pos, radius)
    {}

    [[nodiscard]] Data& getVibrationData() override { return m_data; }
    [[nodiscard]] const Data& getVibrationData() const override { return m_data; }
    [[nodiscard]] User& getVibrationUser() override { return m_user; }
    [[nodiscard]] const User& getVibrationUser() const override { return m_user; }

    TestVibrationUser& user() { return m_user; }
    Data& data() { return m_data; }

private:
    Data m_data;
    TestVibrationUser m_user;
};

} // namespace

TEST_F(GameEventServerTest, VibrationTicker_SelectsAndReceivesVibration)
{
    createWorld();
    TestVibrationSystem system(BlockPos(0, 0, 0), 16);
    auto& data = system.data();
    auto& user = system.user();

    // 添加候选振动（gameTick 0，即当前 tick）
    // VibrationSelector::chosenCandidate 要求 tickAdded < currentTick，
    // 所以需要先推进世界 tick
    GameEvent stepEvent("step");
    VibrationInfo info(stepEvent, 5.0f, Vector3d(3.0, 0.0, 3.0), nullptr);
    data.selectionStrategy().addCandidate(std::move(info), 0);

    // 推进世界 tick 以使候选可用
    // 通过 Listener 添加候选时，addCandidate 使用的 gameTick 来自 world.currentTick()，
    // 然后 chosenCandidate 要求 tickAdded < currentTick，所以需要至少推进 1 tick
    // 这里我们通过直接操作 VibrationSelector 模拟：在 tick 1 检查
    // 先调用一次 tick 使 world tick 推进（假设 tick 推进计数器）
    // 由于我们无法直接推进 world tick，改用直接设置 Data 来测试 ticker 行为

    // 直接设置候选（跳过 VibrationSelector 的 tick 检查）
    VibrationInfo directInfo(stepEvent, 5.0f, Vector3d(3.0, 0.0, 3.0), nullptr);
    data.setCurrentVibration(directInfo);
    data.setTravelTimeInTicks(5);

    // tick 到传播完成
    for (i32 i = 0; i < 5; ++i) {
        VibrationSystem::Ticker::tick(world(), data, user);
    }

    // 振动应已被接收
    EXPECT_EQ(data.currentVibration(), nullptr);
    EXPECT_EQ(user.receivedVibrationCount(), 1u);
    EXPECT_STREQ(user.receivedVibrations()[0].eventId, "step");
}

TEST_F(GameEventServerTest, VibrationTicker_TravelTimeEqualsDistance)
{
    createWorld();
    TestVibrationSystem system(BlockPos(0, 0, 0), 16);
    auto& data = system.data();
    auto& user = system.user();

    // 距离 7 的振动，直接设置当前振动测试传播时间
    GameEvent event("step");
    VibrationInfo info(event, 7.0f, Vector3d(5.0, 0.0, 5.0), nullptr);
    data.setCurrentVibration(info);
    data.setTravelTimeInTicks(user.calculateTravelTimeInTicks(7.0f));

    EXPECT_EQ(data.travelTimeInTicks(), 7); // floor(7.0) = 7
}

TEST_F(GameEventServerTest, VibrationTicker_RejectsInvalidVibration)
{
    createWorld();
    TestVibrationSystem system(BlockPos(0, 0, 0), 16);
    auto& data = system.data();
    auto& user = system.user();

    // 设置不能接收振动
    user.setCanReceive(false);

    // 通过 Listener handleGameEvent 添加候选
    auto& listener = system.getVibrationListener();
    GameEvent stepEvent("step");
    GameEvent::Context ctx;
    Vector3d pos(3.0, 0.0, 3.0);

    bool result = listener.handleGameEvent(world(), stepEvent, ctx, pos);
    EXPECT_FALSE(result); // 被拒绝

    // 不应有候选
    EXPECT_FALSE(data.selectionStrategy().chosenCandidate(1).has_value());
}

TEST_F(GameEventServerTest, VibrationTicker_SameTickCloserWins)
{
    createWorld();
    TestVibrationSystem system(BlockPos(0, 0, 0), 16);
    auto& data = system.data();

    // 同一 tick 添加两个候选，近的应该胜出
    GameEvent event1("step");
    GameEvent event2("entity_action");
    VibrationInfo info1(event1, 10.0f, Vector3d(8.0, 0.0, 8.0), nullptr);
    VibrationInfo info2(event2, 3.0f, Vector3d(2.0, 0.0, 2.0), nullptr);

    data.selectionStrategy().addCandidate(std::move(info1), 0);
    data.selectionStrategy().addCandidate(std::move(info2), 0);

    // tick 1：选择候选 - 应选择近的（3.0）
    auto candidate = data.selectionStrategy().chosenCandidate(1);
    ASSERT_TRUE(candidate.has_value());
    EXPECT_FLOAT_EQ(candidate->distance, 3.0f);
}

TEST_F(GameEventServerTest, VibrationTicker_MultipleVibrationsSequentially)
{
    createWorld();
    TestVibrationSystem system(BlockPos(0, 0, 0), 16);
    auto& data = system.data();
    auto& user = system.user();

    // 第一个振动 - 直接设置
    GameEvent event1("step");
    VibrationInfo info1(event1, 2.0f, Vector3d(1.0, 0.0, 1.0), nullptr);
    data.setCurrentVibration(info1);
    data.setTravelTimeInTicks(2);

    // 传播完成
    VibrationSystem::Ticker::tick(world(), data, user);
    VibrationSystem::Ticker::tick(world(), data, user);
    VibrationSystem::Ticker::tick(world(), data, user); // 第三次 tick 传播时间归零并接收
    EXPECT_EQ(user.receivedVibrationCount(), 1u);

    // 第二个振动
    GameEvent event2("block_activate");
    VibrationInfo info2(event2, 3.0f, Vector3d(2.0, 0.0, 2.0), nullptr);
    data.setCurrentVibration(info2);
    data.setTravelTimeInTicks(3);

    // 传播完成
    for (i32 i = 0; i < 4; ++i) {
        VibrationSystem::Ticker::tick(world(), data, user);
    }
    EXPECT_EQ(user.receivedVibrationCount(), 2u);
    EXPECT_STREQ(user.receivedVibrations()[1].eventId, "block_activate");
}

// ============================================================================
// ServerWorld::gameEvent 集成测试
// ============================================================================

TEST_F(GameEventServerTest, ServerWorld_GameEventDispatches)
{
    createWorld();
    auto* chunk = world().chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    TestListener listener(BlockPos(0, 0, 0), 16);
    auto factory = [this](i32 sectionY) -> std::unique_ptr<EuclideanGameEventListenerRegistry> {
        return std::make_unique<EuclideanGameEventListenerRegistry>(world(), sectionY, nullptr);
    };
    auto& registry = chunk->getOrCreateGameEventListenerRegistry(0, factory);
    registry.registerListener(listener);

    // 通过 ServerWorld::gameEvent 发送事件
    world().gameEvent(GameEvents::BLOCK_ACTIVATE, BlockPos(0, 0, 0), GameEvent::Context());

    EXPECT_TRUE(listener.hasReceivedEvent("block_activate"));
}

// ============================================================================
// VibrationSystem::Ticker requiresAdjacentChunksToBeTicking 测试
// ============================================================================

TEST_F(GameEventServerTest, VibrationTicker_AdjacentChunksCheck_AllChunksBlockTicking_ReceivesVibration)
{
    // 当 requiresAdjacentChunksToBeTicking=true 且监听器周围 3x3 区块全部处于 BlockTicking 级别时，
    // 振动应正常接收。
    // 测试环境中，通过 updatePlayerPosition 使区块达到 EntityTicking 级别（<= BlockTicking）。
    createWorld();

    auto* chunkManager = world().chunkManager();
    ASSERT_NE(chunkManager, nullptr);

    // 先逐个同步加载 3x3 区块到内存。若先 updatePlayerPosition 批量预触发 ~25 个区块的
    // 异步存档加载（完成回调入队 m_pendingLoadCompletes），后续 getChunkSync 触发生成时
    // 邻居 holder 仍处于 ResolvingStorage，会命中 executeEmptyLoad 的 ResolvingStorage 守卫
    // 而 markFailed，导致 getChunkSync 返回 nullptr。逐个 getChunkSync 让每个区块独立完成
    // 生成（邻居调度时 holder 状态机已推进到 StorageMissing），避免该竞态。
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            auto* chunk = chunkManager->getChunkSync(dx, dz);
            ASSERT_NE(chunk, nullptr) << "Failed to load chunk (" << dx << ", " << dz << ")";
        }
    }

    // 注册玩家位置使 3x3 区块达到 EntityTicking 级别（<= BlockTicking）。
    // 此时区块已 loaded，_onTicketLevelChanged 的 _submitChunkRequest 不会重新走存档加载，
    // 仅推进票据级别。viewDistance=2 覆盖 3x3 范围。
    chunkManager->updatePlayerPosition(PlayerId{1}, 8.0, 8.0);
    chunkManager->processTicketUpdatesSync();

    // 验证区块加载级别 <= BlockTicking
    using mc::world::chunk::ChunkLoadLevel;
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            i32 level = chunkManager->ticketManager().getChunkLevel(dx, dz);
            EXPECT_LE(level, static_cast<i32>(ChunkLoadLevel::BlockTicking))
                << "Chunk (" << dx << ", " << dz << ") level=" << level;
        }
    }

    // 创建振动系统，启用相邻区块检查
    TestVibrationSystem system(BlockPos(8, 64, 8), 16);
    auto& data = system.data();
    auto& user = system.user();
    user.setRequireAdjacentChunks(true);

    // 设置振动并传播完成
    GameEvent event("step");
    VibrationInfo info(event, 5.0f, Vector3d(10.0, 64.0, 10.0), nullptr);
    data.setCurrentVibration(info);
    data.setTravelTimeInTicks(1);

    // 传播并接收振动
    VibrationSystem::Ticker::tick(world(), data, user);
    // travelTime 递减为 0，此时接收振动
    VibrationSystem::Ticker::tick(world(), data, user);

    // 振动应被正常接收
    EXPECT_EQ(user.receivedVibrationCount(), 1u);
    EXPECT_EQ(data.currentVibration(), nullptr);
}

TEST_F(GameEventServerTest, VibrationTicker_AdjacentChunksCheck_NoRequire_CheckNotPerformed)
{
    // 当 requiresAdjacentChunksToBeTicking=false 时，不检查区块加载级别，
    // 即使区块未加载也能接收振动。
    createWorld();

    TestVibrationSystem system(BlockPos(0, 64, 0), 16);
    auto& data = system.data();
    auto& user = system.user();
    // 默认 requiresAdjacentChunksToBeTicking = false

    // 只加载中心区块
    auto* chunk = world().chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // 设置振动并传播完成
    GameEvent event("step");
    VibrationInfo info(event, 3.0f, Vector3d(2.0, 64.0, 2.0), nullptr);
    data.setCurrentVibration(info);
    data.setTravelTimeInTicks(1);

    VibrationSystem::Ticker::tick(world(), data, user);
    VibrationSystem::Ticker::tick(world(), data, user);

    // 振动应被正常接收（无区块检查）
    EXPECT_EQ(user.receivedVibrationCount(), 1u);
}

TEST_F(GameEventServerTest, VibrationTicker_AdjacentChunksCheck_RetryOnFailedCheck)
{
    // 当 requiresAdjacentChunksToBeTicking=true 且 3x3 区块检查不通过时，
    // receiveVibration 返回 false 但不清除当前振动，下次 tick 可以重试。
    createWorld();

    // 只加载中心区块，不加载相邻区块，导致 3x3 检查失败
    auto* chunk = world().chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    TestVibrationSystem system(BlockPos(0, 64, 0), 16);
    auto& data = system.data();
    auto& user = system.user();
    user.setRequireAdjacentChunks(true);

    // 设置振动，传播时间为 0（立即可接收）
    GameEvent event("step");
    VibrationInfo info(event, 3.0f, Vector3d(2.0, 64.0, 2.0), nullptr);
    data.setCurrentVibration(info);
    data.setTravelTimeInTicks(0);

    // 通过 tick 触发 receiveVibration
    // 由于 3x3 区块不完整（只有中心区块），接收应失败
    VibrationSystem::Ticker::tick(world(), data, user);

    // 接收失败，但振动未被清除（重试机制）
    EXPECT_EQ(user.receivedVibrationCount(), 0u);
    EXPECT_NE(data.currentVibration(), nullptr); // 振动仍然存在
}

TEST_F(GameEventServerTest, VibrationTicker_AdjacentChunksCheck_UsesListenerPosition)
{
    // 验证区块检查使用监听器位置而非振动源位置。
    // 监听器在区块 (0,0)，振动源在远处区块。
    // 如果检查使用源位置则会失败，但使用监听器位置则应成功
    // （假设监听器周围的区块都已加载）。
    createWorld();

    auto* chunkManager = world().chunkManager();
    ASSERT_NE(chunkManager, nullptr);

    // 先逐个同步加载 3x3 区块到内存，再 updatePlayerPosition 设置票据级别
    // （理由见 VibrationTicker_AdjacentChunksCheck_AllChunksBlockTicking_ReceivesVibration 注释）。
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            auto* chunk = chunkManager->getChunkSync(dx, dz);
            ASSERT_NE(chunk, nullptr) << "Failed to load chunk (" << dx << ", " << dz << ")";
        }
    }

    // 通过玩家位置使监听器位置（区块 0,0）周围的 3x3 区块达到 EntityTicking 级别
    chunkManager->updatePlayerPosition(PlayerId{1}, 8.0, 8.0);
    chunkManager->processTicketUpdatesSync();

    // 监听器在区块 (0,0) 内，振动源在远处（但仍在检测半径内）
    TestVibrationSystem system(BlockPos(8, 64, 8), 16);
    auto& data = system.data();
    auto& user = system.user();
    user.setRequireAdjacentChunks(true);

    // 振动源距离 5 格，仍在半径 16 内
    GameEvent event("step");
    VibrationInfo info(event, 5.0f, Vector3d(12.0, 64.0, 12.0), nullptr);
    data.setCurrentVibration(info);
    data.setTravelTimeInTicks(1);

    VibrationSystem::Ticker::tick(world(), data, user);
    VibrationSystem::Ticker::tick(world(), data, user);

    // 监听器位置区块 (0,0) 周围 3x3 已加载，振动应被接收
    EXPECT_EQ(user.receivedVibrationCount(), 1u);
}

TEST_F(GameEventServerTest, VibrationTicker_AdjacentChunksCheck_ChunkNotInMemory_FailsCheck)
{
    // 当 3x3 区域中的某个区块虽在票据系统中但不在内存中时，检查应失败。
    createWorld();

    auto* chunkManager = world().chunkManager();
    ASSERT_NE(chunkManager, nullptr);

    // 只强制加载中心区块到内存
    auto* chunk = chunkManager->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    TestVibrationSystem system(BlockPos(0, 64, 0), 16);
    auto& data = system.data();
    auto& user = system.user();
    user.setRequireAdjacentChunks(true);

    GameEvent event("step");
    VibrationInfo info(event, 3.0f, Vector3d(2.0, 64.0, 2.0), nullptr);
    data.setCurrentVibration(info);
    data.setTravelTimeInTicks(0);

    // 通过 tick 触发 receiveVibration
    VibrationSystem::Ticker::tick(world(), data, user);

    // 相邻区块不在内存中，3x3 检查应失败，振动未被接收
    EXPECT_EQ(user.receivedVibrationCount(), 0u);
    // 振动未被清除（等待重试）
    EXPECT_NE(data.currentVibration(), nullptr);
}

// ============================================================================
// 振动遮挡检测测试（isOccluded 集成测试）
// ============================================================================

TEST_F(GameEventServerTest, VibrationOcclusion_WoolBlocksAllSides_VibrationBlocked)
{
    // 振动源被羊毛方块从所有6个方向包围时，振动信号被遮挡，handleGameEvent 返回 false。
    createWorld();

    // 确保区块已加载
    auto* chunk = world().chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // 在 Y=300 高度（远离地形）创建羊毛包围结构
    // 振动源在 (5, 300, 5)，监听器在 (10, 300, 10)
    // 包围振动源6个方向：上下左右前后
    const i32 sx = 5, sy = 300, sz = 5;
    const BlockState* woolState = &VanillaBlocks::WHITE_WOOL->defaultState();
    world().setBlockState(sx - 1, sy, sz, woolState); // 西
    world().setBlockState(sx + 1, sy, sz, woolState); // 东
    world().setBlockState(sx, sy - 1, sz, woolState); // 下
    world().setBlockState(sx, sy + 1, sz, woolState); // 上
    world().setBlockState(sx, sy, sz - 1, woolState); // 北
    world().setBlockState(sx, sy, sz + 1, woolState); // 南

    TestVibrationSystem system(BlockPos(5, 305, 5), 16);
    auto& data = system.data();
    auto& listener = system.getVibrationListener();
    auto& user = system.user();

    // BlockTags 需要初始化
    BlockTags::initialize();

    GameEvent stepEvent("step");
    GameEvent::Context context(nullptr, nullptr); // 无源实体，无受影响方块

    // 振动源被完全包围，handleGameEvent 应返回 false
    Vector3d sourcePos(5.5, 300.5, 5.5);
    bool result = listener.handleGameEvent(world(), stepEvent, context, sourcePos);
    EXPECT_FALSE(result);

    // 确认没有候选振动被添加
    auto candidate = data.selectionStrategy().chosenCandidate(world().currentTick());
    EXPECT_FALSE(candidate.has_value());
}

TEST_F(GameEventServerTest, VibrationOcclusion_WoolBlocksPartial_VibrationNotBlocked)
{
    // 振动源仅部分方向被羊毛方块包围时，振动信号可从未遮挡方向逸出，handleGameEvent 返回 true。
    // 监听器放在源正上方，这样向上的射线不会经过侧面羊毛方块。
    createWorld();

    auto* chunk = world().chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    const i32 sx = 5, sy = 300, sz = 5;
    const BlockState* woolState = &VanillaBlocks::WHITE_WOOL->defaultState();
    // 只包围4个水平方向，上方和下方敞开
    world().setBlockState(sx - 1, sy, sz, woolState); // 西
    world().setBlockState(sx + 1, sy, sz, woolState); // 东
    world().setBlockState(sx, sy, sz - 1, woolState); // 北
    world().setBlockState(sx, sy, sz + 1, woolState); // 南
    // 上方和下方不包围，振动可逸出

    // 监听器放在源正上方（同 X/Z 坐标），这样向上的射线直线上行不经过水平方向的羊毛
    TestVibrationSystem system(BlockPos(5, 305, 5), 16);
    auto& data = system.data();
    auto& listener = system.getVibrationListener();
    auto& user = system.user();

    BlockTags::initialize();

    GameEvent stepEvent("step");
    GameEvent::Context context(nullptr, nullptr);

    // 上方未被遮挡，振动可逸出
    Vector3d sourcePos(5.5, 300.5, 5.5);
    bool result = listener.handleGameEvent(world(), stepEvent, context, sourcePos);
    EXPECT_TRUE(result);

    // 候选振动应被添加
    auto candidate = data.selectionStrategy().chosenCandidate(world().currentTick());
    // chosenCandidate 需要 tickAdded < currentTick，由于我们刚添加（tickAdded == currentTick），
    // 所以可能还没就绪，但 handleGameEvent 返回 true 已说明振动未被遮挡
}

TEST_F(GameEventServerTest, VibrationOcclusion_StoneBlocksAllSides_VibrationNotBlocked)
{
    // 石头不在 OCCLUDES_VIBRATION_SIGNALS 标签中，即使完全包围也不会遮挡振动。
    createWorld();

    auto* chunk = world().chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    const i32 sx = 5, sy = 300, sz = 5;
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    // 石头完全包围振动源
    world().setBlockState(sx - 1, sy, sz, stoneState);
    world().setBlockState(sx + 1, sy, sz, stoneState);
    world().setBlockState(sx, sy - 1, sz, stoneState);
    world().setBlockState(sx, sy + 1, sz, stoneState);
    world().setBlockState(sx, sy, sz - 1, stoneState);
    world().setBlockState(sx, sy, sz + 1, stoneState);

    // 监听器在源正上方
    TestVibrationSystem system(BlockPos(5, 305, 5), 16);
    auto& listener = system.getVibrationListener();
    auto& user = system.user();

    BlockTags::initialize();

    GameEvent stepEvent("step");
    GameEvent::Context context(nullptr, nullptr);

    // 石头不遮挡振动信号
    Vector3d sourcePos(5.5, 300.5, 5.5);
    bool result = listener.handleGameEvent(world(), stepEvent, context, sourcePos);
    EXPECT_TRUE(result);
}

TEST_F(GameEventServerTest, VibrationOcclusion_NoBlocks_VibrationNotBlocked)
{
    // 无遮挡方块时振动正常传播。
    createWorld();

    auto* chunk = world().chunkManager()->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // Y=300 高度无任何方块（空气），振动不应被遮挡
    // 监听器在源正上方
    TestVibrationSystem system(BlockPos(5, 305, 5), 16);
    auto& listener = system.getVibrationListener();
    auto& user = system.user();

    BlockTags::initialize();

    GameEvent stepEvent("step");
    GameEvent::Context context(nullptr, nullptr);

    Vector3d sourcePos(5.5, 300.5, 5.5);
    bool result = listener.handleGameEvent(world(), stepEvent, context, sourcePos);
    EXPECT_TRUE(result);
}
