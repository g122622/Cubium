#include <gtest/gtest.h>

#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentRegistry.hpp"

using namespace mc;
using namespace mc::mod::bedrock::addon;

// ============================================================================
// BlockComponentRegistry 单元测试
// ============================================================================

class BlockComponentRegistryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 每个测试前清空注册表，避免测试间互相影响
        BlockComponentRegistry::instance().clear();
    }

    void TearDown() override { BlockComponentRegistry::instance().clear(); }

    /**
     * @brief 创建一个简单的方块组件，包含onStepOn回调
     */
    BlockCustomComponent makeStepOnComponent(const std::string& name = "test:step_on",
        std::function<void(BlockComponentStepOnEvent&, const CustomComponentParameters&)> callback = nullptr)
    {
        BlockCustomComponent comp;
        comp.name = name;
        if (callback) {
            comp.onStepOn = std::move(callback);
        } else {
            comp.onStepOn = [](BlockComponentStepOnEvent& event, const CustomComponentParameters&) {
                // 默认空回调
            };
        }
        return comp;
    }

    /**
     * @brief 创建包含多个回调的组件
     */
    BlockCustomComponent makeMultiCallbackComponent()
    {
        BlockCustomComponent comp;
        comp.name = "test:multi";
        comp.onStepOn = [](BlockComponentStepOnEvent&, const CustomComponentParameters&) {};
        comp.onStepOff = [](BlockComponentStepOffEvent&, const CustomComponentParameters&) {};
        comp.onPlace = [](BlockComponentOnPlaceEvent&, const CustomComponentParameters&) {};
        comp.onBreak = [](BlockComponentBreakEvent&, const CustomComponentParameters&) {};
        comp.onPlayerBreak = [](BlockComponentPlayerBreakEvent&, const CustomComponentParameters&) {};
        comp.onPlayerInteract = [](BlockComponentPlayerInteractEvent&, const CustomComponentParameters&) {};
        comp.beforeOnPlayerPlace = [](BlockComponentPlayerPlaceBeforeEvent&, const CustomComponentParameters&) {};
        comp.onEntityFallOn = [](BlockComponentEntityFallOnEvent&, const CustomComponentParameters&) {};
        comp.onRandomTick = [](BlockComponentRandomTickEvent&, const CustomComponentParameters&) {};
        comp.onTick = [](BlockComponentTickEvent&, const CustomComponentParameters&) {};
        comp.onEntity = [](BlockComponentEntityEvent&, const CustomComponentParameters&) {};
        comp.onRedstoneUpdate = [](BlockComponentRedstoneUpdateEvent&, const CustomComponentParameters&) {};
        comp.onBlockStateChange = [](BlockComponentBlockStateChangeEvent&, const CustomComponentParameters&) {};
        return comp;
    }
};

// ========== 注册测试 ==========

TEST_F(BlockComponentRegistryTest, RegisterSingleComponent)
{
    auto& reg = BlockComponentRegistry::instance();
    auto comp = makeStepOnComponent("test:pressure_plate");

    reg.registerComponent("minecraft:stone", std::move(comp));

    EXPECT_EQ(reg.componentCount("minecraft:stone"), 1u);
    EXPECT_TRUE(reg.hasStepOnCallback("minecraft:stone"));
}

TEST_F(BlockComponentRegistryTest, RegisterMultipleComponentsForSameBlock)
{
    auto& reg = BlockComponentRegistry::instance();

    BlockCustomComponent comp1;
    comp1.name = "test:comp1";
    comp1.onStepOn = [](BlockComponentStepOnEvent&, const CustomComponentParameters&) {};

    BlockCustomComponent comp2;
    comp2.name = "test:comp2";
    comp2.onStepOff = [](BlockComponentStepOffEvent&, const CustomComponentParameters&) {};

    reg.registerComponent("minecraft:stone", std::move(comp1));
    reg.registerComponent("minecraft:stone", std::move(comp2));

    EXPECT_EQ(reg.componentCount("minecraft:stone"), 2u);
    EXPECT_TRUE(reg.hasStepOnCallback("minecraft:stone"));
    EXPECT_TRUE(reg.hasStepOffCallback("minecraft:stone"));
}

TEST_F(BlockComponentRegistryTest, RegisterComponentsForDifferentBlocks)
{
    auto& reg = BlockComponentRegistry::instance();

    BlockCustomComponent comp1;
    comp1.name = "test:comp1";
    comp1.onStepOn = [](BlockComponentStepOnEvent&, const CustomComponentParameters&) {};

    BlockCustomComponent comp2;
    comp2.name = "test:comp2";
    comp2.onBreak = [](BlockComponentBreakEvent&, const CustomComponentParameters&) {};

    reg.registerComponent("minecraft:stone", std::move(comp1));
    reg.registerComponent("minecraft:dirt", std::move(comp2));

    EXPECT_EQ(reg.registeredBlockTypeCount(), 2u);
    EXPECT_TRUE(reg.hasStepOnCallback("minecraft:stone"));
    EXPECT_FALSE(reg.hasStepOnCallback("minecraft:dirt"));
    EXPECT_TRUE(reg.hasBreakCallback("minecraft:dirt"));
    EXPECT_FALSE(reg.hasBreakCallback("minecraft:stone"));
}

// ========== 标志位查询测试 ==========

TEST_F(BlockComponentRegistryTest, HasCallbackReturnsFalseForUnregisteredBlock)
{
    auto& reg = BlockComponentRegistry::instance();
    EXPECT_FALSE(reg.hasStepOnCallback("minecraft:unregistered"));
    EXPECT_FALSE(reg.hasPlaceCallback("minecraft:unregistered"));
    EXPECT_EQ(reg.componentCount("minecraft:unregistered"), 0u);
}

TEST_F(BlockComponentRegistryTest, AllCallbackFlagsWork)
{
    auto& reg = BlockComponentRegistry::instance();
    auto comp = makeMultiCallbackComponent();
    reg.registerComponent("minecraft:test_block", std::move(comp));

    EXPECT_TRUE(reg.hasStepOnCallback("minecraft:test_block"));
    EXPECT_TRUE(reg.hasStepOffCallback("minecraft:test_block"));
    EXPECT_TRUE(reg.hasPlaceCallback("minecraft:test_block"));
    EXPECT_TRUE(reg.hasBreakCallback("minecraft:test_block"));
    EXPECT_TRUE(reg.hasPlayerBreakCallback("minecraft:test_block"));
    EXPECT_TRUE(reg.hasPlayerInteractCallback("minecraft:test_block"));
    EXPECT_TRUE(reg.hasPlayerPlaceBeforeCallback("minecraft:test_block"));
    EXPECT_TRUE(reg.hasEntityFallOnCallback("minecraft:test_block"));
    EXPECT_TRUE(reg.hasRandomTickCallback("minecraft:test_block"));
    EXPECT_TRUE(reg.hasTickCallback("minecraft:test_block"));
    EXPECT_TRUE(reg.hasEntityCallback("minecraft:test_block"));
    EXPECT_TRUE(reg.hasRedstoneUpdateCallback("minecraft:test_block"));
    EXPECT_TRUE(reg.hasBlockStateChangeCallback("minecraft:test_block"));
}

// ========== 派发测试 ==========

TEST_F(BlockComponentRegistryTest, DispatchStepOnCallsCallback)
{
    auto& reg = BlockComponentRegistry::instance();
    bool callbackCalled = false;

    BlockCustomComponent comp;
    comp.name = "test:step_on";
    comp.onStepOn = [&](BlockComponentStepOnEvent& event, const CustomComponentParameters&) {
        callbackCalled = true;
        EXPECT_EQ(event.blockTypeId, "minecraft:pressure_plate");
        EXPECT_EQ(event.blockX, 10);
        EXPECT_EQ(event.blockY, 64);
        EXPECT_EQ(event.blockZ, -5);
        EXPECT_TRUE(event.entityId.has_value());
        EXPECT_EQ(event.entityId.value(), 42u);
    };

    reg.registerComponent("minecraft:pressure_plate", std::move(comp));

    BlockComponentStepOnEvent event;
    event.blockTypeId = "minecraft:pressure_plate";
    event.blockX = 10;
    event.blockY = 64;
    event.blockZ = -5;
    event.entityId = 42u;

    bool dispatched = reg.dispatchStepOn("minecraft:pressure_plate", event);
    EXPECT_TRUE(dispatched);
    EXPECT_TRUE(callbackCalled);
}

TEST_F(BlockComponentRegistryTest, DispatchReturnsFalseForUnregisteredBlock)
{
    auto& reg = BlockComponentRegistry::instance();
    BlockComponentStepOnEvent event;
    event.blockTypeId = "minecraft:unregistered";

    bool dispatched = reg.dispatchStepOn("minecraft:unregistered", event);
    EXPECT_FALSE(dispatched);
}

TEST_F(BlockComponentRegistryTest, DispatchCallsAllRegisteredComponents)
{
    auto& reg = BlockComponentRegistry::instance();
    int callCount = 0;

    BlockCustomComponent comp1;
    comp1.name = "test:comp1";
    comp1.onStepOn = [&](BlockComponentStepOnEvent&, const CustomComponentParameters&) { callCount++; };

    BlockCustomComponent comp2;
    comp2.name = "test:comp2";
    comp2.onStepOn = [&](BlockComponentStepOnEvent&, const CustomComponentParameters&) { callCount++; };

    reg.registerComponent("minecraft:stone", std::move(comp1));
    reg.registerComponent("minecraft:stone", std::move(comp2));

    BlockComponentStepOnEvent event;
    bool dispatched = reg.dispatchStepOn("minecraft:stone", event);
    EXPECT_TRUE(dispatched);
    EXPECT_EQ(callCount, 2);
}

TEST_F(BlockComponentRegistryTest, DispatchPlayerPlaceBeforeCanCancel)
{
    auto& reg = BlockComponentRegistry::instance();

    BlockCustomComponent comp;
    comp.name = "test:cancel_place";
    comp.beforeOnPlayerPlace = [](BlockComponentPlayerPlaceBeforeEvent& event, const CustomComponentParameters&) {
        event.cancel = true;
    };

    reg.registerComponent("minecraft:test_block", std::move(comp));

    BlockComponentPlayerPlaceBeforeEvent event;
    event.blockTypeId = "minecraft:test_block";
    event.cancel = false;

    bool dispatched = reg.dispatchPlayerPlaceBefore("minecraft:test_block", event);
    EXPECT_TRUE(dispatched);
    EXPECT_TRUE(event.cancel);
}

TEST_F(BlockComponentRegistryTest, DispatchTickEvent)
{
    auto& reg = BlockComponentRegistry::instance();
    bool tickCallbackCalled = false;

    BlockCustomComponent comp;
    comp.name = "test:tick";
    comp.onTick = [&](BlockComponentTickEvent& event, const CustomComponentParameters&) {
        tickCallbackCalled = true;
        EXPECT_EQ(event.blockTypeId, "minecraft:test_block");
    };

    reg.registerComponent("minecraft:test_block", std::move(comp));

    BlockComponentTickEvent event;
    event.blockTypeId = "minecraft:test_block";
    event.blockX = 0;
    event.blockY = 0;
    event.blockZ = 0;

    bool dispatched = reg.dispatchTick("minecraft:test_block", event);
    EXPECT_TRUE(dispatched);
    EXPECT_TRUE(tickCallbackCalled);
}

TEST_F(BlockComponentRegistryTest, DispatchEntityFallOnEvent)
{
    auto& reg = BlockComponentRegistry::instance();
    f32 receivedFallDistance = 0.0f;

    BlockCustomComponent comp;
    comp.name = "test:fall_on";
    comp.onEntityFallOn = [&](BlockComponentEntityFallOnEvent& event, const CustomComponentParameters&) {
        receivedFallDistance = event.fallDistance;
    };

    reg.registerComponent("minecraft:slime_block", std::move(comp));

    BlockComponentEntityFallOnEvent event;
    event.blockTypeId = "minecraft:slime_block";
    event.fallDistance = 5.5f;

    reg.dispatchEntityFallOn("minecraft:slime_block", event);
    EXPECT_FLOAT_EQ(receivedFallDistance, 5.5f);
}

// ========== 注销测试 ==========

TEST_F(BlockComponentRegistryTest, UnregisterComponent)
{
    auto& reg = BlockComponentRegistry::instance();
    auto comp = makeStepOnComponent("test:step_on");
    reg.registerComponent("minecraft:stone", std::move(comp));

    EXPECT_TRUE(reg.hasStepOnCallback("minecraft:stone"));
    EXPECT_EQ(reg.componentCount("minecraft:stone"), 1u);

    size_t removed = reg.unregisterComponent("minecraft:stone", "test:step_on");
    EXPECT_EQ(removed, 1u);
    EXPECT_FALSE(reg.hasStepOnCallback("minecraft:stone"));
    EXPECT_EQ(reg.componentCount("minecraft:stone"), 0u);
}

TEST_F(BlockComponentRegistryTest, UnregisterAllComponentsForBlock)
{
    auto& reg = BlockComponentRegistry::instance();

    BlockCustomComponent comp1;
    comp1.name = "test:comp1";
    comp1.onStepOn = [](BlockComponentStepOnEvent&, const CustomComponentParameters&) {};

    BlockCustomComponent comp2;
    comp2.name = "test:comp2";
    comp2.onStepOff = [](BlockComponentStepOffEvent&, const CustomComponentParameters&) {};

    reg.registerComponent("minecraft:stone", std::move(comp1));
    reg.registerComponent("minecraft:stone", std::move(comp2));
    EXPECT_EQ(reg.componentCount("minecraft:stone"), 2u);

    reg.unregisterAll("minecraft:stone");
    EXPECT_EQ(reg.componentCount("minecraft:stone"), 0u);
    EXPECT_FALSE(reg.hasStepOnCallback("minecraft:stone"));
    EXPECT_FALSE(reg.hasStepOffCallback("minecraft:stone"));
}

// ========== 清空测试 ==========

TEST_F(BlockComponentRegistryTest, ClearRemovesAllComponents)
{
    auto& reg = BlockComponentRegistry::instance();

    BlockCustomComponent comp1;
    comp1.name = "test:comp1";
    comp1.onStepOn = [](BlockComponentStepOnEvent&, const CustomComponentParameters&) {};

    BlockCustomComponent comp2;
    comp2.name = "test:comp2";
    comp2.onBreak = [](BlockComponentBreakEvent&, const CustomComponentParameters&) {};

    reg.registerComponent("minecraft:stone", std::move(comp1));
    reg.registerComponent("minecraft:dirt", std::move(comp2));

    EXPECT_EQ(reg.registeredBlockTypeCount(), 2u);

    reg.clear();

    EXPECT_EQ(reg.registeredBlockTypeCount(), 0u);
    EXPECT_EQ(reg.componentCount("minecraft:stone"), 0u);
    EXPECT_EQ(reg.componentCount("minecraft:dirt"), 0u);
}

// ========== 统计测试 ==========

TEST_F(BlockComponentRegistryTest, RegisteredBlockTypeCount)
{
    auto& reg = BlockComponentRegistry::instance();
    EXPECT_EQ(reg.registeredBlockTypeCount(), 0u);

    BlockCustomComponent comp;
    comp.name = "test:comp";
    comp.onStepOn = [](BlockComponentStepOnEvent&, const CustomComponentParameters&) {};

    reg.registerComponent("minecraft:stone", std::move(comp));
    EXPECT_EQ(reg.registeredBlockTypeCount(), 1u);

    BlockCustomComponent comp2;
    comp2.name = "test:comp2";
    comp2.onStepOn = [](BlockComponentStepOnEvent&, const CustomComponentParameters&) {};
    reg.registerComponent("minecraft:dirt", std::move(comp2));
    EXPECT_EQ(reg.registeredBlockTypeCount(), 2u);
}

// ========== 事件数据测试 ==========

TEST_F(BlockComponentRegistryTest, StepOffEventCarriesEntityId)
{
    auto& reg = BlockComponentRegistry::instance();
    std::optional<u64> receivedEntityId;

    BlockCustomComponent comp;
    comp.name = "test:step_off";
    comp.onStepOff = [&](BlockComponentStepOffEvent& event, const CustomComponentParameters&) {
        receivedEntityId = event.entityId;
    };

    reg.registerComponent("minecraft:test_block", std::move(comp));

    BlockComponentStepOffEvent event;
    event.blockTypeId = "minecraft:test_block";
    event.entityId = 123u;

    reg.dispatchStepOff("minecraft:test_block", event);
    EXPECT_TRUE(receivedEntityId.has_value());
    EXPECT_EQ(receivedEntityId.value(), 123u);
}

TEST_F(BlockComponentRegistryTest, PlayerBreakEventCarriesPlayerId)
{
    auto& reg = BlockComponentRegistry::instance();
    std::optional<u64> receivedPlayerId;

    BlockCustomComponent comp;
    comp.name = "test:player_break";
    comp.onPlayerBreak = [&](BlockComponentPlayerBreakEvent& event, const CustomComponentParameters&) {
        receivedPlayerId = event.playerId;
    };

    reg.registerComponent("minecraft:test_block", std::move(comp));

    BlockComponentPlayerBreakEvent event;
    event.blockTypeId = "minecraft:test_block";
    event.playerId = 456u;

    reg.dispatchPlayerBreak("minecraft:test_block", event);
    EXPECT_TRUE(receivedPlayerId.has_value());
    EXPECT_EQ(receivedPlayerId.value(), 456u);
}

TEST_F(BlockComponentRegistryTest, BreakEventWithoutEntitySource)
{
    auto& reg = BlockComponentRegistry::instance();
    std::optional<u64> receivedEntitySourceId;

    BlockCustomComponent comp;
    comp.name = "test:break";
    comp.onBreak = [&](BlockComponentBreakEvent& event, const CustomComponentParameters&) {
        receivedEntitySourceId = event.entitySourceId;
    };

    reg.registerComponent("minecraft:test_block", std::move(comp));

    BlockComponentBreakEvent event;
    event.blockTypeId = "minecraft:test_block";
    // 不设置entitySourceId，应该为std::nullopt

    reg.dispatchBreak("minecraft:test_block", event);
    EXPECT_FALSE(receivedEntitySourceId.has_value());
}

TEST_F(BlockComponentRegistryTest, EntityEventCarriesSourceId)
{
    auto& reg = BlockComponentRegistry::instance();
    u64 receivedSourceId = 0;

    BlockCustomComponent comp;
    comp.name = "test:entity";
    comp.onEntity = [&](BlockComponentEntityEvent& event, const CustomComponentParameters&) {
        receivedSourceId = event.entitySourceId;
    };

    reg.registerComponent("minecraft:test_block", std::move(comp));

    BlockComponentEntityEvent event;
    event.blockTypeId = "minecraft:test_block";
    event.entitySourceId = 789u;

    reg.dispatchEntity("minecraft:test_block", event);
    EXPECT_EQ(receivedSourceId, 789u);
}

TEST_F(BlockComponentRegistryTest, RedstoneUpdateEventCarriesPowerLevels)
{
    auto& reg = BlockComponentRegistry::instance();
    i32 receivedPower = -1;
    i32 receivedPrevPower = -1;

    BlockCustomComponent comp;
    comp.name = "test:redstone";
    comp.onRedstoneUpdate = [&](BlockComponentRedstoneUpdateEvent& event, const CustomComponentParameters&) {
        receivedPower = event.powerLevel;
        receivedPrevPower = event.previousPowerLevel;
    };

    reg.registerComponent("minecraft:test_block", std::move(comp));

    BlockComponentRedstoneUpdateEvent event;
    event.blockTypeId = "minecraft:test_block";
    event.powerLevel = 15;
    event.previousPowerLevel = 0;
    event.firstUpdate = true;

    reg.dispatchRedstoneUpdate("minecraft:test_block", event);
    EXPECT_EQ(receivedPower, 15);
    EXPECT_EQ(receivedPrevPower, 0);
}

TEST_F(BlockComponentRegistryTest, ParametersPassedToCallback)
{
    auto& reg = BlockComponentRegistry::instance();
    bool paramsReceived = false;

    BlockCustomComponent comp;
    comp.name = "test:params";
    comp.onStepOn = [&](BlockComponentStepOnEvent&, const CustomComponentParameters& params) {
        // 验证参数被传递（即使是空参数）
        paramsReceived = true;
    };

    reg.registerComponent("minecraft:test_block", std::move(comp));

    BlockComponentStepOnEvent event;
    event.blockTypeId = "minecraft:test_block";
    reg.dispatchStepOn("minecraft:test_block", event);
    EXPECT_TRUE(paramsReceived);
}
