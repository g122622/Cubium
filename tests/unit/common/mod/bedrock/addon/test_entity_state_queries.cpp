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
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/entity/entities/monster/basic/CreeperEntity.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/passive/basic/MooshroomEntity.hpp"
#include "common/entity/entities/passive/horse/HorseEntity.hpp"
#include "common/entity/entities/passive/special/FoxEntity.hpp"
#include "common/entity/entities/passive/tamable/WolfEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/vehicle/ChestBoatEntity.hpp"
#include "common/entity/entities/vehicle/MinecartEntity.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/entity/inventory/container/ChestContainer.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include "common/mod/bedrock/addon/binding/ScriptGameObjects.hpp"
#include "common/mod/bedrock/addon/content/entity/definition/AddonEntityDefinition.hpp"
#include "common/mod/bedrock/addon/content/entity/filters/EntityFilter.hpp"
#include "common/mod/bedrock/addon/content/entity/query/EntityStateQuery.hpp"
#include "common/mod/bedrock/addon/content/entity/state/AddonEntityComponents.hpp"
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/core/IScriptRuntime.hpp"
#include "common/mod/bedrock/addon/modules/MinecraftModuleFactory.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include <limits>
#include <memory>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::mod::bedrock::addon;

namespace {
class AddonEntityStateQueryTest : public testing::Test {
protected:
    void SetUp() override
    {
        Items::ensureInitialized();
        item = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stick"));
        ASSERT_NE(item, nullptr);
    }

    void check(const EntityFilterContext& context, const nlohmann::json& source, bool expected)
    {
        auto filter = EntityFilter::compile(source);
        ASSERT_TRUE(filter.success()) << filter.error().message();
        auto result = filter.value().matches(context);
        ASSERT_TRUE(result.success()) << result.error().message();
        EXPECT_EQ(result.value(), expected) << source.dump();
    }

    ecs::EntityRegistry registry{"addon-entity-state-query"};
    const Item* item = nullptr;
};

nlohmann::json _variantDefinition()
{
    return {{"format_version", "1.21.0"},
        {"minecraft:entity",
            {{"description", {{"identifier", "test:variant"}, {"is_summonable", true}}},
                {"components",
                    {{"minecraft:variant", {{"value", 2}}}, {"minecraft:mark_variant", nlohmann::json::object()}}},
                {"component_groups",
                    {{"changed",
                        {{"minecraft:variant", {{"value", 7}}}, {"minecraft:mark_variant", {{"value", 3}}}}}}}}}};
}

/** @brief 取出 Molang 查询结果里的数值，未取到时报错并返回 0，便于用 ASSERT 风格逐项断言。 */
f32 _molangNumber(const std::optional<MolangValue>& value)
{
    EXPECT_TRUE(value.has_value()) << "query must be implemented";
    if (!value.has_value()) {
        return 0.0f;
    }
    EXPECT_TRUE(std::holds_alternative<f32>(*value)) << "query must resolve to a number";
    return std::holds_alternative<f32>(*value) ? std::get<f32>(*value) : 0.0f;
}
} // namespace

TEST_F(AddonEntityStateQueryTest, HandDomainsAndUnqualifiedItemNamesReadActualEquipment)
{
    Player player(EntityInstanceId(1), "Equipment", registry);
    Entity caller(EntityInstanceId(2), nullptr, registry);
    EntityFilterContext context{caller, &player, nullptr, nullptr, nullptr, nullptr, nullptr};
    player.setEquipment(EquipmentSlot::OffHand, ItemStack(*item, 1));
    check(context, {{"test", "has_equipment"}, {"subject", "other"}, {"domain", "hand"}, {"value", "stick"}}, true);
    check(
        context, {{"test", "has_equipment"}, {"subject", "other"}, {"domain", "main_hand"}, {"value", "stick"}}, false);
    check(context, {{"test", "has_equipment"}, {"subject", "other"}, {"value", "minecraft:stick"}}, true);
    player.setEquipment(EquipmentSlot::OffHand, ItemStack::EMPTY);
    check(context,
        {{"test", "has_equipment"}, {"subject", "other"}, {"domain", "hand"}, {"value", "stick"}, {"operator", "not"}},
        true);
    player.setEquipment(EquipmentSlot::MainHand, ItemStack(*item, 1));
    check(
        context, {{"test", "has_equipment"}, {"subject", "other"}, {"domain", "main_hand"}, {"value", "stick"}}, true);
}

TEST_F(AddonEntityStateQueryTest, ArmorBodyAndInventoryDomainsDoNotOverlap)
{
    LivingEntity entity(EntityInstanceId(1), nullptr, registry);
    EntityFilterContext context{entity, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    for (const auto& [name, slot] : {std::pair{"head", EquipmentSlot::Head},
             std::pair{"torso", EquipmentSlot::Chest},
             std::pair{"leg", EquipmentSlot::Legs},
             std::pair{"feet", EquipmentSlot::Feet}}) {
        entity.setEquipment(slot, ItemStack(*item, 1));
        check(context, {{"test", "has_equipment"}, {"domain", name}, {"value", "stick"}}, true);
        check(context, {{"test", "has_equipment"}, {"domain", "armor"}, {"value", "stick"}}, true);
        check(context, {{"test", "has_equipment"}, {"domain", "body"}, {"value", "stick"}}, false);
        entity.setEquipment(slot, ItemStack::EMPTY);
    }
    entity.setEquipment(EquipmentSlot::Body, ItemStack(*item, 1));
    check(context, {{"test", "has_equipment"}, {"domain", "body"}, {"value", "stick"}}, true);
    check(context, {{"test", "has_equipment"}, {"domain", "armor"}, {"value", "stick"}}, false);

    Player player(EntityInstanceId(2), "Inventory", registry);
    context.other = &player;
    player.setEquipment(EquipmentSlot::Head, ItemStack(*item, 1));
    player.setEquipment(EquipmentSlot::OffHand, ItemStack(*item, 1));
    const nlohmann::json inventoryFilter = {
        {"test", "has_equipment"}, {"subject", "other"}, {"domain", "inventory"}, {"value", "stick"}};
    check(context, inventoryFilter, false);
    player.inventory().setItem(PlayerInventory::HOTBAR_SIZE, ItemStack(*item, 1));
    check(context, inventoryFilter, true);
    player.inventory().setItem(PlayerInventory::HOTBAR_SIZE, ItemStack::EMPTY);
    check(context, inventoryFilter, false);
}

// 狐狸叼着的物品放在主手槽，inventory 域必须能查到它。
TEST_F(AddonEntityStateQueryTest, FoxMouthItemIsQueryableThroughInventoryDomain)
{
    FoxEntity fox(EntityInstanceId(7), registry);
    EntityFilterContext context{fox, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    const nlohmann::json inventoryFilter = {{"test", "has_equipment"}, {"domain", "inventory"}, {"value", "stick"}};

    // 未叼物品时查不到。
    check(context, inventoryFilter, false);

    // 狐狸把叼着的物品放在主手槽，因此装备到主手后 inventory 域就应能查到。
    fox.setEquipment(EquipmentSlot::MainHand, ItemStack(*item, 1));
    check(context, inventoryFilter, true);

    fox.setEquipment(EquipmentSlot::MainHand, ItemStack::EMPTY);
    check(context, inventoryFilter, false);
}

TEST_F(AddonEntityStateQueryTest, OwnerFilterUsesCallerOwnershipAndCanonicalUuid)
{
    WolfEntity tameable(EntityInstanceId(1), registry);
    Entity owner(EntityInstanceId(2), nullptr, registry);
    Entity stranger(EntityInstanceId(3), nullptr, registry);
    owner.setUuid("12345678-1234-4234-8234-123456789abc");
    tameable.setOwnerId(util::uuidFromString(owner.uuid()));
    EntityFilterContext context{tameable, &owner, nullptr, nullptr, nullptr, nullptr, nullptr};
    check(context, {{"test", "is_owner"}, {"subject", "other"}}, true);
    check(context, {{"test", "is_owner"}}, false);
    context.other = &stranger;
    check(context, {{"test", "is_owner"}, {"subject", "other"}, {"value", false}}, true);
    context.other = nullptr;
    check(context, {{"test", "is_owner"}, {"subject", "other"}, {"value", false}}, false);
    context.other = &owner;
    tameable.clearOwner();
    check(context, {{"test", "is_owner"}, {"subject", "other"}}, false);

    HorseEntity horse(EntityInstanceId(4), registry);
    horse.setOwnerUuid("12345678123442348234123456789abc");
    EntityFilterContext horseContext{horse, &owner, nullptr, nullptr, nullptr, nullptr, nullptr};
    check(horseContext, {{"test", "is_owner"}, {"subject", "other"}}, true);
    horse.clearOwnerUuid();
    check(horseContext, {{"test", "is_owner"}, {"subject", "other"}}, false);
}

TEST_F(AddonEntityStateQueryTest, ContainerFilterFollowsRealMenuOpenAndClose)
{
    Player player(EntityInstanceId(1), "Container", registry);
    EntityFilterContext context{player, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    check(context, {{"test", "has_container_open"}, {"value", "false"}}, true);
    CraftingInventory inventory(3, 3);
    blockentity::ChestContainer menu(1, &player.inventory(), &inventory, 1);
    player.setOpenContainerMenu(&menu);
    check(context, {{"test", "has_container_open"}}, true);
    player.clearOpenContainerMenu();
    check(context, {{"test", "has_container_open"}}, false);
}

TEST_F(AddonEntityStateQueryTest, NativeMarkerComponentsFollowTamingAndChargingTransitions)
{
    WolfEntity tameable(EntityInstanceId(1), registry);
    EntityFilterContext context{tameable, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    check(context, {{"test", "has_component"}, {"value", "is_tamed"}, {"operator", "not"}}, true);
    tameable.setTamed(true);
    check(context, {{"test", "has_component"}, {"value", "minecraft:is_tamed"}}, true);
    tameable.setTamed(false);
    check(context, {{"test", "has_component"}, {"value", "is_tamed"}}, false);
    CreeperEntity creeper(EntityInstanceId(2), registry);
    context.other = &creeper;
    check(context, {{"test", "has_component"}, {"subject", "other"}, {"value", "is_charged"}}, false);
    creeper.setPowered(true);
    check(context, {{"test", "has_component"}, {"subject", "other"}, {"value", "is_charged"}}, true);
}

// 原生标记组件（fire_immune / is_baby / is_sleeping / is_leashed / is_riding / is_sneaking）
// 的存在条件必须读实体真实状态，而不是恒 false 或报 Unsupported。
TEST_F(AddonEntityStateQueryTest, NativeStateComponentsFollowEntityState)
{
    AgeableEntity mob(EntityInstanceId(9), registry);
    mob.registerAttributes();
    EntityFilterContext context{mob, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

    // 幼体状态：新建为非幼体，setChild 后组件才存在。
    check(context, {{"test", "has_component"}, {"value", "minecraft:is_baby"}, {"operator", "not"}}, true);
    mob.setChild(true);
    check(context, {{"test", "has_component"}, {"value", "minecraft:is_baby"}}, true);

    // 睡眠状态同理。
    check(context, {{"test", "has_component"}, {"value", "minecraft:is_sleeping"}, {"operator", "not"}}, true);

    // 攀爬与骑乘默认都不成立。
    check(context, {{"test", "has_component"}, {"value", "minecraft:is_riding"}, {"operator", "not"}}, true);
    check(context, {{"test", "has_component"}, {"value", "minecraft:is_leashed"}, {"operator", "not"}}, true);

    // fire_immune 是实体固有属性，普通生物不免疫火焰。
    check(context, {{"test", "has_component"}, {"value", "minecraft:fire_immune"}, {"operator", "not"}}, true);
}

// 原版实体的固有组件：physics / collision_box / type_family / nameable 对任何实体都存在，
// 而 attack / breathable 只有 LivingEntity 具备，leashable 只有 MobEntity 具备。
// 这组判定不依赖数据包，与 addon 实体走 components->has 的路径不同。
TEST_F(AddonEntityStateQueryTest, NativeIntrinsicComponentsAreReportedPerEntityCapability)
{
    Entity plain(EntityInstanceId(1), nullptr, registry);
    LivingEntity living(EntityInstanceId(2), nullptr, registry);

    for (const char* universal :
        {"minecraft:physics", "minecraft:collision_box", "minecraft:type_family", "minecraft:nameable"}) {
        const auto present = EntityStateQuery::hasComponent(plain, universal);
        ASSERT_TRUE(present.success()) << universal << ": " << present.error().message();
        EXPECT_TRUE(present.value()) << universal << " 对任何实体都应存在";
    }

    // attack / breathable 依赖 LivingEntity 的属性表与氧气机制
    for (const char* livingOnly : {"minecraft:attack", "minecraft:breathable"}) {
        const auto onPlain = EntityStateQuery::hasComponent(plain, livingOnly);
        ASSERT_TRUE(onPlain.success()) << livingOnly;
        EXPECT_FALSE(onPlain.value()) << livingOnly << " 不应出现在非生物实体上";

        const auto onLiving = EntityStateQuery::hasComponent(living, livingOnly);
        ASSERT_TRUE(onLiving.success()) << livingOnly;
        EXPECT_TRUE(onLiving.value()) << livingOnly << " 应出现在生物实体上";
    }

    // leashable：非生物没有拴绳状态机
    const auto leashedPlain = EntityStateQuery::hasComponent(plain, "minecraft:leashable");
    ASSERT_TRUE(leashedPlain.success());
    EXPECT_FALSE(leashedPlain.value());
}

// 哞菇的红/棕是主变种（基岩 minecraft:variant），不能用 mark_variant 承载——
// mark_variant 是"附加变种"，语义不同。脚本按官方组件名读取时必须拿到 variant。
TEST_F(AddonEntityStateQueryTest, MooshroomColorIsReportedAsVariantNotMarkVariant)
{
    MooshroomEntity mooshroom(EntityInstanceId(1), registry);

    const auto variantPresent = EntityStateQuery::hasComponent(mooshroom, "minecraft:variant");
    ASSERT_TRUE(variantPresent.success()) << variantPresent.error().message();
    EXPECT_TRUE(variantPresent.value());

    const auto markVariantPresent = EntityStateQuery::hasComponent(mooshroom, "minecraft:mark_variant");
    EXPECT_TRUE(markVariantPresent.failed());

    // 默认红色 = 0，翻转后为棕色 = 1。
    auto red = EntityStateQuery::variant(mooshroom, "minecraft:variant");
    ASSERT_TRUE(red.success()) << red.error().message();
    EXPECT_EQ(red.value(), 0);

    mooshroom.setMooshroomType(MooshroomEntity::MooshroomType::Brown);
    auto brown = EntityStateQuery::variant(mooshroom, "minecraft:variant");
    ASSERT_TRUE(brown.success()) << brown.error().message();
    EXPECT_EQ(brown.value(), 1);
}

TEST_F(AddonEntityStateQueryTest, VariantsFollowGroupsAndNbtRoundTripWithoutRestoringRemovedBase)
{
    auto parsed = AddonEntityDefinition::parse(_variantDefinition());
    ASSERT_TRUE(parsed.success()) << parsed.error().message();
    auto definition = std::make_shared<const AddonEntityDefinition>(std::move(parsed.value()));
    auto entity = AddonEntityDefinition::create(definition, nullptr, registry, {});
    ASSERT_NE(entity, nullptr);
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    ASSERT_NE(components, nullptr);
    EntityFilterContext context{*entity, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    check(context, {{"test", "is_variant"}, {"value", "2"}}, true);
    check(context, {{"test", "is_mark_variant"}, {"value", 0}}, true);
    components->queue({{true, "changed"}});
    check(context, {{"test", "is_variant"}, {"value", 2}}, true);
    components->applyPending(*entity);
    check(context, {{"test", "is_variant"}, {"operator", ">="}, {"value", 7}}, true);
    check(context, {{"test", "is_mark_variant"}, {"value", 3}}, true);
    components->queue({{false, "changed"}});
    auto binaryShape = mc::nbt::jsonToNbt(components->save());
    auto restored = AddonEntityDefinition::create(definition, nullptr, registry, {});
    auto* restoredComponents = restored->tryGetComponent<AddonEntityComponents>();
    ASSERT_TRUE(restoredComponents->load(*restored, mc::nbt::nbtToJson(*binaryShape)).success());
    EXPECT_EQ(EntityStateQuery::variant(*restored, "minecraft:variant").value(), 7);
    restoredComponents->applyPending(*restored);
    EXPECT_FALSE(restoredComponents->has("minecraft:variant"));
    EXPECT_EQ(EntityStateQuery::variant(*restored, "minecraft:variant").value(), 0);
}

TEST_F(AddonEntityStateQueryTest, InvalidVariantComponentsAndFilterInputsAreRejected)
{
    for (const auto& value : {nlohmann::json(1.5),
             nlohmann::json("1"),
             nlohmann::json(true),
             nlohmann::json(2147483648LL),
             nlohmann::json(-2147483649LL)}) {
        auto definition = _variantDefinition();
        definition["minecraft:entity"]["components"]["minecraft:variant"]["value"] = value;
        EXPECT_TRUE(AddonEntityDefinition::parse(definition).failed()) << value.dump();
    }
    for (const nlohmann::json source : {nlohmann::json{{"test", "is_variant"}, {"value", 1.2}},
             nlohmann::json{{"test", "is_mark_variant"}, {"value", 2147483648LL}},
             nlohmann::json{{"test", "has_equipment"}, {"domain", "off_hand"}, {"value", "stick"}},
             nlohmann::json{{"test", "has_equipment"}, {"domain", 1}, {"value", "stick"}},
             nlohmann::json{{"test", "has_equipment"}, {"value", ""}}}) {
        EXPECT_TRUE(EntityFilter::compile(source).failed()) << source.dump();
    }
}

TEST_F(AddonEntityStateQueryTest, ScriptVariantReferenceReadsCurrentGroupAndRejectsRemovedComponent)
{
    auto parsed = AddonEntityDefinition::parse(_variantDefinition());
    ASSERT_TRUE(parsed.success());
    auto definition = std::make_shared<const AddonEntityDefinition>(std::move(parsed.value()));
    auto entity = AddonEntityDefinition::create(definition, nullptr, registry, {});
    auto engine = createScriptEngine();
    ASSERT_TRUE(engine->initialize());
    auto context = engine->runtime().createContext(
        ContextConfig{Capabilities{}, std::nullopt, DEFAULT_CONTEXT_MEMORY_BYTES, DEFAULT_CONTEXT_STACK_SIZE_BYTES});
    ASSERT_NE(context, nullptr);
    MinecraftModuleFactory factory;
    ASSERT_TRUE(factory.registerBindings(*context));
    auto& binding = context->bindingContext();
    const auto expose = [&](const char* name, Entity* entity) {
        void* global = binding.getGlobalObject();
        void* wrapper = wrapEntity(binding, entity);
        binding.setProperty(global, name, wrapper);
        binding.releaseValue(wrapper);
        binding.releaseValue(global);
    };
    expose("subject", entity.get());
    auto result = context->evaluate(R"(
        globalThis.variant = subject.getComponent('minecraft:variant');
        globalThis.mark = subject.getComponent('mark_variant');
        if (variant.value !== 2 || mark.value !== 0) throw Error('initial variant');
    )",
        "variant.js",
        EvalFlags::None);
    ASSERT_TRUE(result.success()) << result.errorMessage();
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    components->queue({{true, "changed"}});
    components->applyPending(*entity);
    result = context->evaluate(
        "if (variant.value !== 7 || mark.value !== 3) throw Error('stale variant');", "variant.js", EvalFlags::None);
    ASSERT_TRUE(result.success()) << result.errorMessage();
    components->queue({{false, "changed"}});
    components->applyPending(*entity);
    result = context->evaluate(R"(
        if (subject.getComponent('variant') !== undefined) throw Error('removed variant returned');
        let rejected = false;
        try { variant.value; } catch (error) { rejected = true; }
        if (!rejected) throw Error('removed component reference accepted');
    )",
        "variant.js",
        EvalFlags::None);
    ASSERT_TRUE(result.success()) << result.errorMessage();

    WolfEntity native(EntityInstanceId(2), registry);
    expose("native", &native);
    result = context->evaluate("if (native.getComponent('is_tamed') !== undefined) throw Error('untamed marker');",
        "tamed.js",
        EvalFlags::None);
    ASSERT_TRUE(result.success()) << result.errorMessage();
    native.setTamed(true);
    result = context->evaluate("if (!native.getComponent('is_tamed') || native.getComponent('is_tamed').value !== "
                               "undefined) throw Error('tamed marker');",
        "tamed.js",
        EvalFlags::None);
    ASSERT_TRUE(result.success()) << result.errorMessage();
    native.setTamed(false);
    result =
        context->evaluate("if (native.getComponent('is_tamed') !== undefined) throw Error('untamed marker remained');",
            "tamed.js",
            EvalFlags::None);
    ASSERT_TRUE(result.success()) << result.errorMessage();
    context.reset();
    engine->shutdown();
}

// 载具容器（箱子矿车/漏斗矿车/箱子船）的 inventory 域查询：
// 这些实体都提供了不修改库存的只读访问接口，has_equipment 的 inventory 域应当能查到。
TEST_F(AddonEntityStateQueryTest, VehicleContainerInventoryIsQueryable)
{
    entity::ChestMinecartEntity minecart(EntityInstanceId(1), registry);
    minecart.setInventoryItem(3, ItemStack(*item, 1));

    Entity caller(EntityInstanceId(2), nullptr, registry);
    EntityFilterContext minecartContext{caller, &minecart, nullptr, nullptr, nullptr, nullptr, nullptr};
    check(minecartContext,
        {{"test", "has_equipment"}, {"subject", "other"}, {"domain", "inventory"}, {"value", "minecraft:stick"}},
        true);
    check(minecartContext,
        {{"test", "has_equipment"}, {"subject", "other"}, {"domain", "inventory"}, {"value", "minecraft:diamond"}},
        false);

    entity::ChestBoatEntity boat(entity::BoatEntity::Type::OAK, registry);
    boat.setInventoryItem(0, ItemStack(*item, 1));
    EntityFilterContext boatContext{caller, &boat, nullptr, nullptr, nullptr, nullptr, nullptr};
    check(boatContext,
        {{"test", "has_equipment"}, {"subject", "other"}, {"domain", "inventory"}, {"value", "minecraft:stick"}},
        true);
}

// 载具也承载库存，因此必须报告 minecraft:inventory 组件；否则会出现
// "组件说不存在、inventory 域却查得到"的自相矛盾，依赖 has_component 的过滤条件会误判。
TEST_F(AddonEntityStateQueryTest, VehicleReportsInventoryComponent)
{
    entity::ChestMinecartEntity minecart(EntityInstanceId(1), registry);
    Entity caller(EntityInstanceId(2), nullptr, registry);

    EntityFilterContext minecartContext{caller, &minecart, nullptr, nullptr, nullptr, nullptr, nullptr};
    check(minecartContext, {{"test", "has_component"}, {"subject", "other"}, {"value", "minecraft:inventory"}}, true);

    entity::ChestBoatEntity boat(entity::BoatEntity::Type::OAK, registry);
    EntityFilterContext boatContext{caller, &boat, nullptr, nullptr, nullptr, nullptr, nullptr};
    check(boatContext, {{"test", "has_component"}, {"subject", "other"}, {"value", "minecraft:inventory"}}, true);

    // 不承载库存的实体不应报告该组件：接口以 Unsupported 表达"该实体类型没有映射"，
    // 而不是返回 false，因此这里直接断言返回值的形态。
    Entity plain(EntityInstanceId(3), nullptr, registry);
    auto plainResult = EntityStateQuery::hasComponent(plain, "minecraft:inventory");
    EXPECT_FALSE(plainResult.success());
    EXPECT_EQ(plainResult.error().code(), ErrorCode::Unsupported);
}

// entity.onGround / isSneaking / isRiding / isOnFire 这些状态查询必须读真实实体状态，
// 而不是恒返回 0；这里断言新建实体的默认值即可暴露"接线错了导致永远是 0"的实现。
TEST_F(AddonEntityStateQueryTest, MolangQueriesReportEntityPhysicsState)
{
    Entity plain(EntityInstanceId(1), nullptr, registry);
    for (const char* query : {"query.is_on_ground", "query.is_sneaking", "query.is_riding", "query.is_on_fire"}) {
        EXPECT_EQ(_molangNumber(EntityStateQuery::molangQuery(plain, query)), 0.0f) << query;
    }

    plain.setOnGround(true);
    EXPECT_EQ(_molangNumber(EntityStateQuery::molangQuery(plain, "query.is_on_ground")), 1.0f);

    // 潜行状态只有 Player 提供了设置入口，用玩家来验证查询真的读了 isSneaking()。
    Player player(EntityInstanceId(2), "Sneaking", registry);
    EXPECT_EQ(_molangNumber(EntityStateQuery::molangQuery(player, "query.is_sneaking")), 0.0f);
    player.setSneaking(true);
    EXPECT_EQ(_molangNumber(EntityStateQuery::molangQuery(player, "query.is_sneaking")), 1.0f);
}

// health / max_health 必须取 LivingEntity 的实际数值，且非生物实体上取 0（而不是 nullopt）。
TEST_F(AddonEntityStateQueryTest, MolangHealthQueriesReadLivingEntityValues)
{
    LivingEntity living(EntityInstanceId(1), nullptr, registry);
    living.registerAttributes();
    EXPECT_FLOAT_EQ(_molangNumber(EntityStateQuery::molangQuery(living, "query.health")), living.health());
    EXPECT_FLOAT_EQ(_molangNumber(EntityStateQuery::molangQuery(living, "query.max_health")), living.maxHealth());

    Entity plain(EntityInstanceId(2), nullptr, registry);
    EXPECT_EQ(_molangNumber(EntityStateQuery::molangQuery(plain, "query.health")), 0.0f);
    EXPECT_EQ(_molangNumber(EntityStateQuery::molangQuery(plain, "query.max_health")), 0.0f);
}

// equipment_count 统计真正装有物品的装备槽，鞍槽不属于基岩定义的"装备"。
TEST_F(AddonEntityStateQueryTest, MolangEquipmentCountCountsOccupiedEquipmentSlots)
{
    LivingEntity living(EntityInstanceId(1), nullptr, registry);
    EXPECT_EQ(_molangNumber(EntityStateQuery::molangQuery(living, "query.equipment_count")), 0.0f);

    living.setEquipment(EquipmentSlot::MainHand, ItemStack(*item, 1));
    EXPECT_EQ(_molangNumber(EntityStateQuery::molangQuery(living, "query.equipment_count")), 1.0f);
    living.setEquipment(EquipmentSlot::Head, ItemStack(*item, 1));
    living.setEquipment(EquipmentSlot::Body, ItemStack(*item, 1));
    EXPECT_EQ(_molangNumber(EntityStateQuery::molangQuery(living, "query.equipment_count")), 3.0f);

    // 鞍槽不计入。
    living.setEquipment(EquipmentSlot::Saddle, ItemStack(*item, 1));
    EXPECT_EQ(_molangNumber(EntityStateQuery::molangQuery(living, "query.equipment_count")), 3.0f);

    living.setEquipment(EquipmentSlot::Head, ItemStack::EMPTY);
    EXPECT_EQ(_molangNumber(EntityStateQuery::molangQuery(living, "query.equipment_count")), 2.0f);
}

// is_baby 只对 AgeableEntity 有意义；其余实体恒为成年（0），不能返回 nullopt。
TEST_F(AddonEntityStateQueryTest, MolangBabyQueryFollowsAgeableState)
{
    AgeableEntity mob(EntityInstanceId(1), registry);
    mob.registerAttributes();
    EXPECT_EQ(_molangNumber(EntityStateQuery::molangQuery(mob, "query.is_baby")), 0.0f);
    mob.setChild(true);
    EXPECT_EQ(_molangNumber(EntityStateQuery::molangQuery(mob, "query.is_baby")), 1.0f);

    Entity plain(EntityInstanceId(2), nullptr, registry);
    EXPECT_EQ(_molangNumber(EntityStateQuery::molangQuery(plain, "query.is_baby")), 0.0f);
}

// "已实现但该实体不适用"（取 0）与"尚未实现"（nullopt）必须区分开，
// 否则上层会把不适用也记成"查询未实现"，产生误导性日志。
TEST_F(AddonEntityStateQueryTest, MolangQuerySeparatesUnimplementedFromNotApplicable)
{
    Entity plain(EntityInstanceId(1), nullptr, registry);
    EXPECT_TRUE(EntityStateQuery::molangQuery(plain, "query.health").has_value())
        << "an implemented query must resolve even when the entity does not support it";
    EXPECT_FALSE(EntityStateQuery::molangQuery(plain, "query.has_target").has_value())
        << "an unimplemented query must report nullopt";
}

// 按能力推导的组件（movement / knockback_resistance / follow_range /
// despawn / persistent / rideable）必须跟着实体能力走，而不是一律报 Unsupported。
TEST_F(AddonEntityStateQueryTest, CapabilityDerivedComponentsFollowLivingAndMobSplit)
{
    Entity plain(EntityInstanceId(1), nullptr, registry);
    LivingEntity living(EntityInstanceId(2), nullptr, registry);
    ZombieEntity mob(EntityInstanceId(3), registry);

    // 生物属性表相关：只有 LivingEntity 有属性表。
    for (const char* livingOnly : {"minecraft:movement", "minecraft:knockback_resistance"}) {
        const auto onPlain = EntityStateQuery::hasComponent(plain, livingOnly);
        ASSERT_TRUE(onPlain.success()) << livingOnly;
        EXPECT_FALSE(onPlain.value()) << livingOnly << " must not appear on a non-living entity";

        const auto onLiving = EntityStateQuery::hasComponent(living, livingOnly);
        ASSERT_TRUE(onLiving.success()) << livingOnly;
        EXPECT_TRUE(onLiving.value()) << livingOnly << " must appear on a living entity";
    }

    // 生物行为相关（追击范围 / 自然消失 / 持久化）：只有 MobEntity 有。
    for (const char* mobOnly : {"minecraft:follow_range", "minecraft:despawn", "minecraft:persistent"}) {
        const auto onLiving = EntityStateQuery::hasComponent(living, mobOnly);
        ASSERT_TRUE(onLiving.success()) << mobOnly;
        EXPECT_FALSE(onLiving.value()) << mobOnly << " must not appear on a plain living entity";

        const auto onMob = EntityStateQuery::hasComponent(mob, mobOnly);
        ASSERT_TRUE(onMob.success()) << mobOnly;
        EXPECT_TRUE(onMob.value()) << mobOnly << " must appear on a mob";
    }
}

// rideable 必须与骑乘硬门槛 Entity::couldAcceptPassenger() 保持一致。
// 不能写死 true，否则脚本会对本来拒绝乘客的实体报"可骑"。
TEST_F(AddonEntityStateQueryTest, RideableMirrorsCouldAcceptPassenger)
{
    Entity plain(EntityInstanceId(1), nullptr, registry);

    const auto onPlain = EntityStateQuery::hasComponent(plain, "minecraft:rideable");
    ASSERT_TRUE(onPlain.success()) << onPlain.error().message();
    EXPECT_EQ(onPlain.value(), plain.couldAcceptPassenger()) << "rideable must mirror Entity::couldAcceptPassenger()";
}
