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

#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/mod/bedrock/addon/content/entity/filters/EntityFilter.hpp"
#include "common/mod/bedrock/addon/content/entity/filters/base/VanillaEntityFamilies.hpp"
#include "common/mod/bedrock/addon/content/entity/state/EntityAddonState.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <limits>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::mod::bedrock::addon;

namespace {
class _FilterWorld final : public IWorld {
public:
    _FilterWorld()
        : m_ticks(*this)
    {}

    const BlockState* getBlockState(i32, i32, i32) const override
    {
        return nullptr;
    }

    bool setBlockState(i32, i32, i32, const BlockState*) override
    {
        return false;
    }

    const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return nullptr;
    }

    bool isWithinWorldBounds(i32, i32, i32) const override
    {
        return true;
    }

    i32 getHeight(i32, i32) const override
    {
        return world::SEA_LEVEL;
    }

    u8 getBlockLight(i32, i32, i32) const override
    {
        return 0;
    }

    u8 getSkyLight(i32, i32, i32) const override
    {
        return 15;
    }

    bool hasBlockCollision(const AxisAlignedBB&) const override
    {
        return false;
    }

    std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override
    {
        return {};
    }

    bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override
    {
        return false;
    }

    std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }

    std::vector<Entity*> getPlayers() const override
    {
        return players;
    }

    PhysicsEngine* physicsEngine() override
    {
        return nullptr;
    }

    const PhysicsEngine* physicsEngine() const override
    {
        return nullptr;
    }

    world::tick::TickManager& tickManager() override
    {
        return m_ticks;
    }

    const world::tick::TickManager& tickManager() const override
    {
        return m_ticks;
    }

    DimensionId dimension() const override
    {
        return 0;
    }

    u64 seed() const override
    {
        return 0;
    }

    u64 currentTick() const override
    {
        return 0;
    }

    i64 dayTime() const override
    {
        return time;
    }

    bool isClientSide() const override
    {
        return false;
    }

    bool isHardcore() const override
    {
        return false;
    }

    Difficulty difficulty() const override
    {
        return Difficulty::Normal;
    }

    math::IRandom& getRandom() override
    {
        return m_random;
    }

    const math::IRandom& getRandom() const override
    {
        return m_random;
    }

    world::border::WorldBorder& worldBorder() override
    {
        return m_border;
    }

    const world::border::WorldBorder& worldBorder() const override
    {
        return m_border;
    }

    bool isRaining() const override
    {
        return raining;
    }

    bool isThundering() const override
    {
        return thundering;
    }

    std::vector<Entity*> players;
    i64 time = 1000;
    bool raining = false;
    bool thundering = false;

private:
    world::tick::TickManager m_ticks;
    math::Random m_random{123};
    world::border::WorldBorder m_border;
};

class EntityFilterTest : public testing::Test {
protected:
    void SetUp() override
    {
        entity.setTypeId("test:filter");
        auto result = entity.getOrAddComponent<EntityAddonState>().defineProperties(
            {{"test:count", {{"type", "int"}, {"range", {0, 100}}, {"default", 7}}},
                {"test:ratio", {{"type", "float"}, {"range", {0, 1}}, {"default", 0.1}}},
                {"test:enabled", {{"type", "bool"}, {"default", true}}},
                {"test:mode", {{"type", "enum"}, {"values", {"idle", "work", "sleep"}}, {"default", "work"}}}});
        ASSERT_TRUE(result.success());
    }

    void check(const nlohmann::json& source, bool expected)
    {
        auto filter = EntityFilter::compile(source);
        ASSERT_TRUE(filter.success()) << filter.error().message() << source.dump();
        auto matches = filter.value().matches(context);
        ASSERT_TRUE(matches.success()) << matches.error().message();
        EXPECT_EQ(matches.value(), expected) << source.dump();
    }

    ecs::EntityRegistry registry{"entity-filter-test"};
    Entity entity{EntityInstanceId(1), nullptr, registry};
    EntityFilterContext context{entity, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
};
} // namespace

TEST_F(EntityFilterTest, NumericPropertyComparisonsAndOfficialOperatorAliases)
{
    for (const auto* operation : {"equals", "=", "==", ">=", "<="}) {
        check({{"test", "int_property"}, {"domain", "test:count"}, {"operator", operation}, {"value", "7"}}, true);
    }
    for (const auto* operation : {"not", "<>", "!=", ">", "<"}) {
        check({{"test", "int_property"}, {"domain", "test:count"}, {"operator", operation}, {"value", 7}}, false);
    }
    check({{"test", "int_property"}, {"domain", "test:count"}, {"operator", ">"}, {"value", 5}}, true);
    check({{"test", "float_property"}, {"domain", "test:ratio"}, {"value", 0.1}}, true);
    check({{"test", "int_property"}, {"domain", "test:ratio"}, {"value", 0}}, false);
}

TEST_F(EntityFilterTest, BooleanDefaultsEnumOrderAndPropertyExistence)
{
    check({{"test", "bool_property"}, {"domain", "test:enabled"}}, true);
    check({{"test", "bool_property"}, {"domain", "test:enabled"}, {"value", "false"}}, false);
    check({{"test", "enum_property"}, {"domain", "test:mode"}, {"value", "work"}}, true);
    check({{"test", "enum_property"}, {"domain", "test:mode"}, {"operator", "<"}, {"value", "sleep"}}, true);
    check({{"test", "has_property"}, {"value", "test:count"}}, true);
    check({{"test", "has_property"}, {"value", "test:missing"}, {"operator", "not"}}, true);
    check({{"test", "int_property"}, {"domain", "test:missing"}, {"value", 7}, {"operator", "not"}}, false);
}

TEST_F(EntityFilterTest, LogicalCollectionsAndEmptyCollections)
{
    check(nlohmann::json::object(), true);
    check({{"all_of", nlohmann::json::array()}}, true);
    check({{"any_of", nlohmann::json::array()}}, false);
    check({{"none_of", nlohmann::json::array()}}, true);
    const nlohmann::json present = {{"test", "has_property"}, {"value", "test:count"}};
    const nlohmann::json missing = {{"test", "has_property"}, {"value", "test:missing"}};
    check({{"all_of", {present, missing}}}, false);
    check({{"any_of", {present, missing}}}, true);
    check({{"none_of", {missing}}}, true);
}

TEST_F(EntityFilterTest, CollectionsShortCircuitUnsupportedNativeMappings)
{
    const nlohmann::json present = {{"test", "has_property"}, {"value", "test:count"}};
    const nlohmann::json missing = {{"test", "has_property"}, {"value", "test:missing"}};
    const nlohmann::json unsupported = {{"test", "is_family"}, {"value", "unmapped-native-family"}};
    check({{"any_of", {present, unsupported}}}, true);
    check({{"all_of", {missing, unsupported}}}, false);
}

TEST_F(EntityFilterTest, ExplicitSubjectsUseTheProvidedParticipant)
{
    Entity other(EntityInstanceId(2), nullptr, registry);
    ASSERT_TRUE(other.getOrAddComponent<EntityAddonState>()
            .defineProperties({{"test:count", {{"type", "int"}, {"range", {0, 100}}, {"default", 20}}}})
            .success());
    const nlohmann::json filter = {
        {"test", "int_property"}, {"subject", "other"}, {"domain", "test:count"}, {"value", 20}};
    check(filter, false);
    context.other = &other;
    check(filter, true);
    context.target = &other;
    check({{"test", "int_property"}, {"subject", "target"}, {"domain", "test:count"}, {"value", 20}}, true);
    check({{"test", "bool_property"}, {"subject", "parent"}, {"domain", "test:enabled"}, {"value", false}}, false);
}

TEST_F(EntityFilterTest, TypeTagAndBasicEntityStateTests)
{
    ASSERT_TRUE(entity.addTag("maid-worker"));
    check({{"test", "is_type"}, {"value", "test:filter"}}, true);
    check({{"test", "has_tag"}, {"value", "maid-worker"}}, true);
    check({{"test", "has_tag"}, {"value", "missing"}, {"operator", "not"}}, true);
    check({{"test", "is_alive"}}, true);
    check({{"test", "is_sneaking"}, {"value", false}}, true);
    check({{"test", "has_target"}, {"value", false}}, true);
}

// 维度与高度型过滤：in_nether / in_overworld 读实体维度，is_altitude 读脚底方块 Y。
TEST_F(EntityFilterTest, DimensionAndAltitudeFiltersFollowEntityState)
{
    // 裸 Entity 默认在主世界。
    check({{"test", "in_overworld"}, {"value", true}}, true);
    check({{"test", "in_nether"}, {"value", false}}, true);

    entity.setDimension(DimensionManager::NETHER);
    check({{"test", "in_nether"}, {"value", true}}, true);
    check({{"test", "in_overworld"}, {"value", false}}, true);

    // is_altitude 比较的是方块 Y（向下取整），不是浮点高度。
    entity.setDimension(DimensionManager::OVERWORLD);
    entity.setPosition(0.0f, 64.7f, 0.0f);
    check({{"test", "is_altitude"}, {"operator", "=="}, {"value", 64}}, true);
    check({{"test", "is_altitude"}, {"operator", ">"}, {"value", 63}}, true);
    check({{"test", "is_altitude"}, {"operator", "<"}, {"value", 64}}, false);
}

// has_nametag / on_ladder 必须跟随实体真实状态，而不是恒 false。
TEST_F(EntityFilterTest, NameTagAndLadderFiltersFollowEntityState)
{
    check({{"test", "has_nametag"}, {"value", false}}, true);
    entity.setCustomName("Filtered");
    check({{"test", "has_nametag"}, {"value", true}}, true);

    // 裸 Entity 不在梯子上；攀爬状态由 setLastClimbPos 表达（isOnLadder 读该状态）。
    check({{"test", "on_ladder"}, {"value", false}}, true);
}

// 实体状态型过滤（is_baby / on_fire / is_sleeping / is_leashed / is_riding / rider_count）。
// 关键断言：非生物实体上这些测试必须恒为 false，而不是报错——基岩侧对"实体没有该概念"
// 的处理就是条件不成立，若返回 Unsupported 会让整包加载失败。
TEST_F(EntityFilterTest, EntityStateFiltersAreFalseOnNonLivingEntity)
{
    // 裸 Entity 既不是 LivingEntity 也不是 MobEntity，也没有乘客。
    check({{"test", "is_baby"}, {"value", false}}, true);
    check({{"test", "is_sleeping"}, {"value", false}}, true);
    check({{"test", "is_leashed"}, {"value", false}}, true);
    check({{"test", "is_riding"}, {"value", false}}, true);
    check({{"test", "rider_count"}, {"value", 0}}, true);
    check({{"test", "rider_count"}, {"value", 1}}, false);
}

// is_baby 必须读真实的幼体状态：AgeableEntity::isChild() 为 true 时才成立。
TEST_F(EntityFilterTest, IsBabyReflectsAgeableChildState)
{
    AgeableEntity mob(EntityInstanceId(3), registry);
    mob.registerAttributes();
    EntityFilterContext mobContext{mob, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

    const nlohmann::json babyFilter = {{"test", "is_baby"}, {"value", true}};
    auto filter = EntityFilter::compile(babyFilter);
    ASSERT_TRUE(filter.success()) << filter.error().message();

    auto notBaby = filter.value().matches(mobContext);
    ASSERT_TRUE(notBaby.success()) << notBaby.error().message();
    EXPECT_FALSE(notBaby.value()) << "a freshly created mob must not be a baby";

    mob.setChild(true);
    auto isBaby = filter.value().matches(mobContext);
    ASSERT_TRUE(isBaby.success()) << isBaby.error().message();
    EXPECT_TRUE(isBaby.value()) << "is_baby must follow AgeableEntity::setChild";
}

// on_fire / is_riding 在生物实体上也要能读到真实状态。
TEST_F(EntityFilterTest, FireAndRidingFiltersFollowEntityState)
{
    CreatureEntity mob(EntityInstanceId(4), registry);
    mob.registerAttributes();
    EntityFilterContext mobContext{mob, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

    const nlohmann::json fireFilter = {{"test", "on_fire"}, {"value", true}};
    auto onFire = EntityFilter::compile(fireFilter);
    ASSERT_TRUE(onFire.success()) << onFire.error().message();
    auto before = onFire.value().matches(mobContext);
    ASSERT_TRUE(before.success());
    EXPECT_FALSE(before.value()) << "freshly created mob is not on fire";

    mob.setRemainingFireTicks(100);
    auto after = onFire.value().matches(mobContext);
    ASSERT_TRUE(after.success());
    EXPECT_TRUE(after.value()) << "on_fire must follow the fire timer";
}

TEST_F(EntityFilterTest, DaytimeWeatherAndNearestPlayerUseWorldState)
{
    _FilterWorld world;
    entity.setWorld(&world);
    check({{"test", "is_daytime"}}, true);
    world.time = 13000;
    check({{"test", "is_daytime"}, {"value", false}}, true);
    check({{"test", "weather"}, {"value", "clear"}}, true);
    world.raining = true;
    check({{"test", "weather"}, {"value", "precipitation"}}, true);
    world.thundering = true;
    check({{"test", "weather"}, {"value", "thunderstorm"}}, true);
    Entity nearest(EntityInstanceId(2), &world, registry);
    nearest.setPosition(3, 4, 0);
    world.players.push_back(&nearest);
    check({{"test", "distance_to_nearest_player"}, {"operator", "<="}, {"value", 5}}, true);
    check({{"test", "distance_to_nearest_player"}, {"operator", "<"}, {"value", 5}}, false);
    world.players.clear();
    entity.setWorld(nullptr);
}

TEST_F(EntityFilterTest, MalformedAndUnsupportedFiltersAreExplicitFailures)
{
    const std::vector<nlohmann::json> invalid = {nlohmann::json::array(),
        {{"test", 7}},
        {{"test", "not_implemented"}, {"value", true}},
        {{"test", "int_property"}, {"value", 7}},
        {{"test", "int_property"}, {"domain", "test:count"}, {"value", 1.5}},
        {{"test", "int_property"}, {"domain", "test:count"}, {"value", "7x"}},
        {{"test", "bool_property"}, {"domain", "test:enabled"}, {"value", 2}},
        {{"test", "has_property"}, {"value", "test:count"}, {"operator", "unknown"}},
        {{"test", "is_alive"}, {"subject", "block"}},
        {{"any_of", nlohmann::json::object()}},
        {{"all_of", nlohmann::json::array()}, {"value", true}}};
    for (const auto& source : invalid) {
        EXPECT_TRUE(EntityFilter::compile(source).failed()) << source.dump();
    }
}

TEST_F(EntityFilterTest, NativeMappingGapsReturnErrorsRatherThanFalseMatches)
{
    auto filter = EntityFilter::compile({{"test", "is_family"}, {"value", "unmapped-native-family"}});
    ASSERT_TRUE(filter.success());
    auto result = filter.value().matches(context);
    ASSERT_TRUE(result.failed());
    EXPECT_EQ(result.error().code(), ErrorCode::Unsupported);
}

TEST_F(EntityFilterTest, FilterNestingAndSizeAreBounded)
{
    nlohmann::json source = {{"test", "has_property"}, {"value", "test:count"}};
    for (int index = 0; index < 80; ++index) {
        source = {{"all_of", {source}}};
    }
    EXPECT_TRUE(EntityFilter::compile(source).failed());
    source = {{"any_of", nlohmann::json::array()}};
    for (int index = 0; index < 4100; ++index) {
        source["any_of"].push_back(nlohmann::json::object());
    }
    EXPECT_TRUE(EntityFilter::compile(source).failed());
}

TEST_F(EntityFilterTest, NativeFamilyQueriesUseOfficialBaseDefinitions)
{
    entity.setTypeId("minecraft:cow");
    check({{"test", "is_family"}, {"value", "cow"}}, true);
    check({{"test", "is_family"}, {"value", "mob"}}, true);
    check({{"test", "is_family"}, {"value", "monster"}}, false);
    entity.setTypeId("minecraft:zombie");
    check({{"test", "is_family"}, {"value", "undead"}}, true);
    check({{"test", "is_family"}, {"value", "monster"}}, true);
    check({{"test", "is_family"}, {"value", "player"}}, false);
    entity.setTypeId("minecraft:player");
    check({{"test", "is_family"}, {"value", "player"}}, true);
    check({{"test", "is_family"}, {"value", "mob"}}, false);
}

TEST_F(EntityFilterTest, UnknownFamilyOnKnownNativeTypeIsAValidFalseMatch)
{
    entity.setTypeId("minecraft:cow");
    check({{"test", "is_family"}, {"value", "unrelated-family"}}, false);
    check({{"test", "is_family"}, {"value", "unrelated-family"}, {"operator", "not"}}, true);
    EXPECT_FALSE(vanillaEntityFamilyMatch("test:missing", "mob").has_value());
}

TEST_F(EntityFilterTest, GroundAndFluidTestsReadActualEntityState)
{
    entity.setOnGround(false);
    check({{"test", "on_ground"}}, false);
    entity.setOnGround(true);
    check({{"test", "on_ground"}}, true);
    entity.setInWater(false);
    entity.setInLava(false);
    check({{"test", "in_water_or_rain"}}, false);
    entity.setInWater(true);
    check({{"test", "in_water"}}, true);
    check({{"test", "in_water_or_rain"}}, true);
    entity.setInWater(false);
    entity.setInLava(true);
    check({{"test", "in_water"}, {"value", false}}, true);
    check({{"test", "in_lava"}}, true);
}

TEST_F(EntityFilterTest, SingleConditionArrayWrappingPreservesTheConditionAndNestingBudget)
{
    const nlohmann::json present = {{"test", "has_property"}, {"value", "test:count"}};
    const nlohmann::json missing = {{"test", "has_property"}, {"value", "test:missing"}};
    check(nlohmann::json::array({present}), true);
    check(nlohmann::json::array({missing}), false);
    nlohmann::json nested = present;
    for (int depth = 0; depth < 80; ++depth) {
        nested = nlohmann::json::array({nested});
    }
    EXPECT_TRUE(EntityFilter::compile(nested).failed());
    auto ambiguous = EntityFilter::compile(nlohmann::json::array({present, missing}));
    ASSERT_TRUE(ambiguous.failed());
    EXPECT_EQ(ambiguous.error().code(), ErrorCode::Unsupported);
}

// is_temperature_value：把所在生物群系温度与给定值（0.0 最冷 ~ 1.0 最热）比较。
// 编译期必须接受该测试，并拒绝超出 [0, 1] 的取值——否则写错范围的过滤器会静默永不命中。
TEST_F(EntityFilterTest, TemperatureValueIsCompiledAndRangeChecked)
{
    auto valid = EntityFilter::compile({{"test", "is_temperature_value"}, {"value", 0.5}});
    ASSERT_TRUE(valid.success()) << valid.error().message();

    // 取值域是 [0, 1]，越界必须报错而不是静默当成 0/1
    EXPECT_TRUE(EntityFilter::compile({{"test", "is_temperature_value"}, {"value", -0.1}}).failed());
    EXPECT_TRUE(EntityFilter::compile({{"test", "is_temperature_value"}, {"value", 1.5}}).failed());

    // 边界值本身合法
    EXPECT_TRUE(EntityFilter::compile({{"test", "is_temperature_value"}, {"value", 0.0}}).success());
    EXPECT_TRUE(EntityFilter::compile({{"test", "is_temperature_value"}, {"value", 1.0}}).success());
}

// 实体未绑定世界时无法查生物群系：必须报 Unsupported，而不是把温度当成 0 参与比较
// （那会让所有温度过滤器在无世界实体上恒等于"0 度"，静默给出错误结论）。
TEST_F(EntityFilterTest, TemperatureValueRequiresAWorldBoundEntity)
{
    auto filter = EntityFilter::compile({{"test", "is_temperature_value"}, {"value", 0.5}});
    ASSERT_TRUE(filter.success()) << filter.error().message();

    const auto matches = filter.value().matches(context);
    ASSERT_TRUE(matches.failed());
    EXPECT_EQ(matches.error().code(), ErrorCode::Unsupported);
}

// has_component 要走 EntityStateQuery 的统一查询入口，
// 而不是被 "兑底走距离比较" 的隐式分支吾掉。
TEST_F(EntityFilterTest, HasComponentQueriesTheUnifiedStateQuery)
{
    // 普通 Entity 既没有 AddonEntityComponents，也不是 LivingEntity：
    // minecraft:health 在它上不存在。
    check({{"test", "has_component"}, {"value", "minecraft:health"}}, false);
    check({{"test", "has_component"}, {"value", "minecraft:health"}, {"operator", "not"}}, true);
    // fire_immune 是实体固有属性，普通实体默认不免火。
    check({{"test", "has_component"}, {"value", "minecraft:fire_immune"}}, false);
}

// 白名单里未实现的 test 不得再被静默当成 distance_to_nearest_player。
TEST_F(EntityFilterTest, UnimplementedTestIsRejectedInsteadOfSilentlyComparedAsDistance)
{
    // 这些名字不在 _validate 白名单里，应当在编译期就报错。
    EXPECT_TRUE(EntityFilter::compile({{"test", "definitely_not_a_real_test"}, {"value", 1}}).failed());
}
