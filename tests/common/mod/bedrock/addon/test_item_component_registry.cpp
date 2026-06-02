#include <gtest/gtest.h>

#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentRegistry.hpp"

using namespace mc;
using namespace mc::mod::bedrock::addon;

// ============================================================================
// ItemComponentRegistry 单元测试
// ============================================================================

class ItemComponentRegistryTest : public ::testing::Test {
protected:
    void SetUp() override { ItemComponentRegistry::instance().clear(); }

    void TearDown() override { ItemComponentRegistry::instance().clear(); }

    ItemCustomComponent makeUseComponent(const std::string& name = "test:use",
        std::function<void(ItemComponentUseEvent&, const CustomComponentParameters&)> callback = nullptr)
    {
        ItemCustomComponent comp;
        comp.name = name;
        if (callback) {
            comp.onUse = std::move(callback);
        } else {
            comp.onUse = [](ItemComponentUseEvent&, const CustomComponentParameters&) {};
        }
        return comp;
    }

    ItemCustomComponent makeAllCallbacksComponent()
    {
        ItemCustomComponent comp;
        comp.name = "test:all";
        comp.onUse = [](ItemComponentUseEvent&, const CustomComponentParameters&) {};
        comp.onUseOn = [](ItemComponentUseOnEvent&, const CustomComponentParameters&) {};
        comp.onHitEntity = [](ItemComponentHitEntityEvent&, const CustomComponentParameters&) {};
        comp.onMineBlock = [](ItemComponentMineBlockEvent&, const CustomComponentParameters&) {};
        comp.onBeforeDurabilityDamage = [](ItemComponentBeforeDurabilityDamageEvent&,
                                            const CustomComponentParameters&) {};
        comp.onCompleteUse = [](ItemComponentCompleteUseEvent&, const CustomComponentParameters&) {};
        comp.onConsume = [](ItemComponentConsumeEvent&, const CustomComponentParameters&) {};
        return comp;
    }
};

// ========== 注册测试 ==========

TEST_F(ItemComponentRegistryTest, RegisterSingleComponent)
{
    auto& reg = ItemComponentRegistry::instance();
    auto comp = makeUseComponent("test:diamond_sword_use");
    reg.registerComponent("minecraft:diamond_sword", std::move(comp));

    EXPECT_EQ(reg.componentCount("minecraft:diamond_sword"), 1u);
    EXPECT_TRUE(reg.hasUseCallback("minecraft:diamond_sword"));
}

TEST_F(ItemComponentRegistryTest, RegisterMultipleComponentsForSameItem)
{
    auto& reg = ItemComponentRegistry::instance();

    ItemCustomComponent comp1;
    comp1.name = "test:comp1";
    comp1.onUse = [](ItemComponentUseEvent&, const CustomComponentParameters&) {};

    ItemCustomComponent comp2;
    comp2.name = "test:comp2";
    comp2.onHitEntity = [](ItemComponentHitEntityEvent&, const CustomComponentParameters&) {};

    reg.registerComponent("minecraft:diamond_sword", std::move(comp1));
    reg.registerComponent("minecraft:diamond_sword", std::move(comp2));

    EXPECT_EQ(reg.componentCount("minecraft:diamond_sword"), 2u);
    EXPECT_TRUE(reg.hasUseCallback("minecraft:diamond_sword"));
    EXPECT_TRUE(reg.hasHitEntityCallback("minecraft:diamond_sword"));
}

TEST_F(ItemComponentRegistryTest, RegisterComponentsForDifferentItems)
{
    auto& reg = ItemComponentRegistry::instance();

    ItemCustomComponent comp1;
    comp1.name = "test:comp1";
    comp1.onUse = [](ItemComponentUseEvent&, const CustomComponentParameters&) {};

    ItemCustomComponent comp2;
    comp2.name = "test:comp2";
    comp2.onConsume = [](ItemComponentConsumeEvent&, const CustomComponentParameters&) {};

    reg.registerComponent("minecraft:diamond_sword", std::move(comp1));
    reg.registerComponent("minecraft:apple", std::move(comp2));

    EXPECT_EQ(reg.registeredItemTypeCount(), 2u);
    EXPECT_TRUE(reg.hasUseCallback("minecraft:diamond_sword"));
    EXPECT_FALSE(reg.hasUseCallback("minecraft:apple"));
    EXPECT_TRUE(reg.hasConsumeCallback("minecraft:apple"));
    EXPECT_FALSE(reg.hasConsumeCallback("minecraft:diamond_sword"));
}

// ========== 标志位查询测试 ==========

TEST_F(ItemComponentRegistryTest, HasCallbackReturnsFalseForUnregisteredItem)
{
    auto& reg = ItemComponentRegistry::instance();
    EXPECT_FALSE(reg.hasUseCallback("minecraft:unregistered"));
    EXPECT_FALSE(reg.hasHitEntityCallback("minecraft:unregistered"));
    EXPECT_EQ(reg.componentCount("minecraft:unregistered"), 0u);
}

TEST_F(ItemComponentRegistryTest, AllCallbackFlagsWork)
{
    auto& reg = ItemComponentRegistry::instance();
    auto comp = makeAllCallbacksComponent();
    reg.registerComponent("minecraft:test_item", std::move(comp));

    EXPECT_TRUE(reg.hasUseCallback("minecraft:test_item"));
    EXPECT_TRUE(reg.hasUseOnCallback("minecraft:test_item"));
    EXPECT_TRUE(reg.hasHitEntityCallback("minecraft:test_item"));
    EXPECT_TRUE(reg.hasMineBlockCallback("minecraft:test_item"));
    EXPECT_TRUE(reg.hasBeforeDurabilityDamageCallback("minecraft:test_item"));
    EXPECT_TRUE(reg.hasCompleteUseCallback("minecraft:test_item"));
    EXPECT_TRUE(reg.hasConsumeCallback("minecraft:test_item"));
}

// ========== 派发测试 ==========

TEST_F(ItemComponentRegistryTest, DispatchUseEvent)
{
    auto& reg = ItemComponentRegistry::instance();
    bool callbackCalled = false;

    ItemCustomComponent comp;
    comp.name = "test:use";
    comp.onUse = [&](ItemComponentUseEvent& event, const CustomComponentParameters&) {
        callbackCalled = true;
        EXPECT_EQ(event.itemTypeId, "minecraft:ender_pearl");
        EXPECT_EQ(event.sourceId, 42u);
    };

    reg.registerComponent("minecraft:ender_pearl", std::move(comp));

    ItemComponentUseEvent event;
    event.itemTypeId = "minecraft:ender_pearl";
    event.sourceId = 42u;

    bool dispatched = reg.dispatchUse("minecraft:ender_pearl", event);
    EXPECT_TRUE(dispatched);
    EXPECT_TRUE(callbackCalled);
}

TEST_F(ItemComponentRegistryTest, DispatchReturnsFalseForUnregisteredItem)
{
    auto& reg = ItemComponentRegistry::instance();
    ItemComponentUseEvent event;
    event.itemTypeId = "minecraft:unregistered";

    bool dispatched = reg.dispatchUse("minecraft:unregistered", event);
    EXPECT_FALSE(dispatched);
}

TEST_F(ItemComponentRegistryTest, DispatchCallsAllRegisteredComponents)
{
    auto& reg = ItemComponentRegistry::instance();
    int callCount = 0;

    ItemCustomComponent comp1;
    comp1.name = "test:comp1";
    comp1.onUse = [&](ItemComponentUseEvent&, const CustomComponentParameters&) { callCount++; };

    ItemCustomComponent comp2;
    comp2.name = "test:comp2";
    comp2.onUse = [&](ItemComponentUseEvent&, const CustomComponentParameters&) { callCount++; };

    reg.registerComponent("minecraft:apple", std::move(comp1));
    reg.registerComponent("minecraft:apple", std::move(comp2));

    ItemComponentUseEvent event;
    event.itemTypeId = "minecraft:apple";
    bool dispatched = reg.dispatchUse("minecraft:apple", event);
    EXPECT_TRUE(dispatched);
    EXPECT_EQ(callCount, 2);
}

TEST_F(ItemComponentRegistryTest, DispatchUseOnEvent)
{
    auto& reg = ItemComponentRegistry::instance();
    i32 receivedFace = -1;

    ItemCustomComponent comp;
    comp.name = "test:use_on";
    comp.onUseOn = [&](ItemComponentUseOnEvent& event, const CustomComponentParameters&) { receivedFace = event.face; };

    reg.registerComponent("minecraft:flint_and_steel", std::move(comp));

    ItemComponentUseOnEvent event;
    event.itemTypeId = "minecraft:flint_and_steel";
    event.sourceId = 1u;
    event.blockX = 10;
    event.blockY = 20;
    event.blockZ = 30;
    event.face = 1; // Up

    reg.dispatchUseOn("minecraft:flint_and_steel", event);
    EXPECT_EQ(receivedFace, 1);
}

TEST_F(ItemComponentRegistryTest, DispatchHitEntityEvent)
{
    auto& reg = ItemComponentRegistry::instance();
    u64 receivedHitEntityId = 0;
    u64 receivedAttackingEntityId = 0;

    ItemCustomComponent comp;
    comp.name = "test:hit_entity";
    comp.onHitEntity = [&](ItemComponentHitEntityEvent& event, const CustomComponentParameters&) {
        receivedHitEntityId = event.hitEntityId;
        receivedAttackingEntityId = event.attackingEntityId;
    };

    reg.registerComponent("minecraft:diamond_sword", std::move(comp));

    ItemComponentHitEntityEvent event;
    event.itemTypeId = "minecraft:diamond_sword";
    event.attackingEntityId = 100u;
    event.hitEntityId = 200u;

    reg.dispatchHitEntity("minecraft:diamond_sword", event);
    EXPECT_EQ(receivedHitEntityId, 200u);
    EXPECT_EQ(receivedAttackingEntityId, 100u);
}

TEST_F(ItemComponentRegistryTest, DispatchBeforeDurabilityDamageCanMutate)
{
    auto& reg = ItemComponentRegistry::instance();

    ItemCustomComponent comp;
    comp.name = "test:durability";
    comp.onBeforeDurabilityDamage = [](ItemComponentBeforeDurabilityDamageEvent& event,
                                        const CustomComponentParameters&) {
        // 将耐久伤害从2改为0（不消耗耐久）
        event.durabilityDamage = 0;
    };

    reg.registerComponent("minecraft:golden_sword", std::move(comp));

    ItemComponentBeforeDurabilityDamageEvent event;
    event.itemTypeId = "minecraft:golden_sword";
    event.attackingEntityId = 1u;
    event.hitEntityId = 2u;
    event.durabilityDamage = 2;

    reg.dispatchBeforeDurabilityDamage("minecraft:golden_sword", event);
    EXPECT_EQ(event.durabilityDamage, 0);
}

TEST_F(ItemComponentRegistryTest, DispatchBeforeDurabilityDamageNoCallbackKeepsOriginal)
{
    auto& reg = ItemComponentRegistry::instance();

    // 注册一个没有onBeforeDurabilityDamage回调的组件
    ItemCustomComponent comp;
    comp.name = "test:no_durability";
    comp.onUse = [](ItemComponentUseEvent&, const CustomComponentParameters&) {};
    reg.registerComponent("minecraft:stone_sword", std::move(comp));

    ItemComponentBeforeDurabilityDamageEvent event;
    event.itemTypeId = "minecraft:stone_sword";
    event.durabilityDamage = 3;

    // 没有onBeforeDurabilityDamage回调，dispatch应该返回false，damage不变
    bool dispatched = reg.dispatchBeforeDurabilityDamage("minecraft:stone_sword", event);
    EXPECT_FALSE(dispatched);
    EXPECT_EQ(event.durabilityDamage, 3);
}

TEST_F(ItemComponentRegistryTest, DispatchMineBlockEvent)
{
    auto& reg = ItemComponentRegistry::instance();
    bool callbackCalled = false;

    ItemCustomComponent comp;
    comp.name = "test:mine_block";
    comp.onMineBlock = [&](ItemComponentMineBlockEvent& event, const CustomComponentParameters&) {
        callbackCalled = true;
        EXPECT_EQ(event.itemTypeId, "minecraft:diamond_pickaxe");
        EXPECT_EQ(event.blockX, 5);
        EXPECT_EQ(event.blockY, 10);
        EXPECT_EQ(event.blockZ, 15);
    };

    reg.registerComponent("minecraft:diamond_pickaxe", std::move(comp));

    ItemComponentMineBlockEvent event;
    event.itemTypeId = "minecraft:diamond_pickaxe";
    event.sourceId = 1u;
    event.blockX = 5;
    event.blockY = 10;
    event.blockZ = 15;

    reg.dispatchMineBlock("minecraft:diamond_pickaxe", event);
    EXPECT_TRUE(callbackCalled);
}

TEST_F(ItemComponentRegistryTest, DispatchCompleteUseEvent)
{
    auto& reg = ItemComponentRegistry::instance();
    i32 receivedDuration = 0;

    ItemCustomComponent comp;
    comp.name = "test:complete_use";
    comp.onCompleteUse = [&](ItemComponentCompleteUseEvent& event, const CustomComponentParameters&) {
        receivedDuration = event.useDuration;
    };

    reg.registerComponent("minecraft:apple", std::move(comp));

    ItemComponentCompleteUseEvent event;
    event.itemTypeId = "minecraft:apple";
    event.sourceId = 1u;
    event.useDuration = 32;

    reg.dispatchCompleteUse("minecraft:apple", event);
    EXPECT_EQ(receivedDuration, 32);
}

TEST_F(ItemComponentRegistryTest, DispatchConsumeEvent)
{
    auto& reg = ItemComponentRegistry::instance();
    bool callbackCalled = false;

    ItemCustomComponent comp;
    comp.name = "test:consume";
    comp.onConsume = [&](ItemComponentConsumeEvent& event, const CustomComponentParameters&) {
        callbackCalled = true;
        EXPECT_EQ(event.itemTypeId, "minecraft:golden_apple");
    };

    reg.registerComponent("minecraft:golden_apple", std::move(comp));

    ItemComponentConsumeEvent event;
    event.itemTypeId = "minecraft:golden_apple";
    event.sourceId = 1u;

    reg.dispatchConsume("minecraft:golden_apple", event);
    EXPECT_TRUE(callbackCalled);
}

// ========== 注销测试 ==========

TEST_F(ItemComponentRegistryTest, UnregisterComponent)
{
    auto& reg = ItemComponentRegistry::instance();
    auto comp = makeUseComponent("test:use");
    reg.registerComponent("minecraft:diamond_sword", std::move(comp));

    EXPECT_TRUE(reg.hasUseCallback("minecraft:diamond_sword"));
    EXPECT_EQ(reg.componentCount("minecraft:diamond_sword"), 1u);

    size_t removed = reg.unregisterComponent("minecraft:diamond_sword", "test:use");
    EXPECT_EQ(removed, 1u);
    EXPECT_FALSE(reg.hasUseCallback("minecraft:diamond_sword"));
    EXPECT_EQ(reg.componentCount("minecraft:diamond_sword"), 0u);
}

TEST_F(ItemComponentRegistryTest, UnregisterAllComponentsForItem)
{
    auto& reg = ItemComponentRegistry::instance();

    ItemCustomComponent comp1;
    comp1.name = "test:comp1";
    comp1.onUse = [](ItemComponentUseEvent&, const CustomComponentParameters&) {};

    ItemCustomComponent comp2;
    comp2.name = "test:comp2";
    comp2.onHitEntity = [](ItemComponentHitEntityEvent&, const CustomComponentParameters&) {};

    reg.registerComponent("minecraft:diamond_sword", std::move(comp1));
    reg.registerComponent("minecraft:diamond_sword", std::move(comp2));
    EXPECT_EQ(reg.componentCount("minecraft:diamond_sword"), 2u);

    reg.unregisterAll("minecraft:diamond_sword");
    EXPECT_EQ(reg.componentCount("minecraft:diamond_sword"), 0u);
    EXPECT_FALSE(reg.hasUseCallback("minecraft:diamond_sword"));
    EXPECT_FALSE(reg.hasHitEntityCallback("minecraft:diamond_sword"));
}

// ========== 清空测试 ==========

TEST_F(ItemComponentRegistryTest, ClearRemovesAllComponents)
{
    auto& reg = ItemComponentRegistry::instance();

    ItemCustomComponent comp1;
    comp1.name = "test:comp1";
    comp1.onUse = [](ItemComponentUseEvent&, const CustomComponentParameters&) {};

    ItemCustomComponent comp2;
    comp2.name = "test:comp2";
    comp2.onConsume = [](ItemComponentConsumeEvent&, const CustomComponentParameters&) {};

    reg.registerComponent("minecraft:diamond_sword", std::move(comp1));
    reg.registerComponent("minecraft:apple", std::move(comp2));

    EXPECT_EQ(reg.registeredItemTypeCount(), 2u);

    reg.clear();

    EXPECT_EQ(reg.registeredItemTypeCount(), 0u);
    EXPECT_EQ(reg.componentCount("minecraft:diamond_sword"), 0u);
    EXPECT_EQ(reg.componentCount("minecraft:apple"), 0u);
}

// ========== 统计测试 ==========

TEST_F(ItemComponentRegistryTest, RegisteredItemTypeCount)
{
    auto& reg = ItemComponentRegistry::instance();
    EXPECT_EQ(reg.registeredItemTypeCount(), 0u);

    ItemCustomComponent comp;
    comp.name = "test:comp";
    comp.onUse = [](ItemComponentUseEvent&, const CustomComponentParameters&) {};
    reg.registerComponent("minecraft:diamond_sword", std::move(comp));
    EXPECT_EQ(reg.registeredItemTypeCount(), 1u);

    ItemCustomComponent comp2;
    comp2.name = "test:comp2";
    comp2.onUse = [](ItemComponentUseEvent&, const CustomComponentParameters&) {};
    reg.registerComponent("minecraft:apple", std::move(comp2));
    EXPECT_EQ(reg.registeredItemTypeCount(), 2u);
}

// ========== 参数传递测试 ==========

TEST_F(ItemComponentRegistryTest, ParametersPassedToCallback)
{
    auto& reg = ItemComponentRegistry::instance();
    bool paramsReceived = false;

    ItemCustomComponent comp;
    comp.name = "test:params";
    comp.onUse = [&](ItemComponentUseEvent&, const CustomComponentParameters& params) { paramsReceived = true; };

    reg.registerComponent("minecraft:test_item", std::move(comp));

    ItemComponentUseEvent event;
    event.itemTypeId = "minecraft:test_item";
    reg.dispatchUse("minecraft:test_item", event);
    EXPECT_TRUE(paramsReceived);
}

// ========== 事件数据完整性测试 ==========

TEST_F(ItemComponentRegistryTest, UseOnEventCarriesBlockPosition)
{
    auto& reg = ItemComponentRegistry::instance();
    i32 receivedX = 0, receivedY = 0, receivedZ = 0;

    ItemCustomComponent comp;
    comp.name = "test:use_on";
    comp.onUseOn = [&](ItemComponentUseOnEvent& event, const CustomComponentParameters&) {
        receivedX = event.blockX;
        receivedY = event.blockY;
        receivedZ = event.blockZ;
    };

    reg.registerComponent("minecraft:bone_meal", std::move(comp));

    ItemComponentUseOnEvent event;
    event.itemTypeId = "minecraft:bone_meal";
    event.blockX = -100;
    event.blockY = 64;
    event.blockZ = 200;

    reg.dispatchUseOn("minecraft:bone_meal", event);
    EXPECT_EQ(receivedX, -100);
    EXPECT_EQ(receivedY, 64);
    EXPECT_EQ(receivedZ, 200);
}

TEST_F(ItemComponentRegistryTest, BeforeDurabilityDamageEventCarriesEntityIds)
{
    auto& reg = ItemComponentRegistry::instance();
    u64 receivedAttacker = 0, receivedHit = 0;

    ItemCustomComponent comp;
    comp.name = "test:durability";
    comp.onBeforeDurabilityDamage = [&](ItemComponentBeforeDurabilityDamageEvent& event,
                                        const CustomComponentParameters&) {
        receivedAttacker = event.attackingEntityId;
        receivedHit = event.hitEntityId;
        event.durabilityDamage = 0; // 防止耐久消耗
    };

    reg.registerComponent("minecraft:unbreaking_sword", std::move(comp));

    ItemComponentBeforeDurabilityDamageEvent event;
    event.itemTypeId = "minecraft:unbreaking_sword";
    event.attackingEntityId = 111u;
    event.hitEntityId = 222u;
    event.durabilityDamage = 1;

    reg.dispatchBeforeDurabilityDamage("minecraft:unbreaking_sword", event);
    EXPECT_EQ(receivedAttacker, 111u);
    EXPECT_EQ(receivedHit, 222u);
    EXPECT_EQ(event.durabilityDamage, 0);
}
