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

#include "common/TempDirHelper.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include "common/mod/bedrock/addon/binding/ScriptGameObjects.hpp"
#include "common/mod/bedrock/addon/content/entity/events/AddonEntityEvents.hpp"
#include "common/mod/bedrock/addon/content/entity/events/EntityEventProgram.hpp"
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPlugin.hpp"

#include "common/entity/ai/pathfinding/FlyNodeEvaluator.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/AgeableEntity.hpp" // is_baby → setChild/isChild 断言
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/serialization/EntityDeserializer.hpp"
#include "common/entity/serialization/components/ComponentSerializerRegistry.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/mod/bedrock/addon/content/entity/definition/AddonEntityDefinition.hpp"
#include "common/mod/bedrock/addon/content/entity/filters/EntityFilter.hpp"
#include "common/mod/bedrock/addon/content/entity/state/AddonEntityComponents.hpp"
#include "common/mod/bedrock/addon/content/entity/state/EntityAddonState.hpp"
#include "common/mod/bedrock/addon/content/entity/state/timer/EntityTimerState.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptManager.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPack.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPackList.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <gtest/gtest.h>

using namespace mc::mod::bedrock::addon;

namespace {
constexpr const char* _PACK = "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa";
constexpr const char* _OTHER = "bbbbbbbb-bbbb-4bbb-8bbb-bbbbbbbbbbbb";

nlohmann::json _definition(const std::string& identifier)
{
    return {{"format_version", "1.21.0"},
        {"minecraft:entity",
            {{"description",
                 {{"identifier", identifier},
                     {"is_summonable", true},
                     {"properties", {{"test:count", {{"type", "int"}, {"range", {0, 100}}, {"default", 3}}}}}}},
                {"components",
                    {{"minecraft:collision_box", {{"width", 0.8}, {"height", 1.4}}},
                        {"minecraft:physics", {{"has_gravity", false}, {"has_collision", true}}},
                        {"minecraft:fire_immune", nlohmann::json::object()}}}}}};
}

class AddonEntityDefinitionTest : public testing::Test {
protected:
    void SetUp() override
    {
        root = mc::test::makeUniqueTestDir("addon-entity-definitions");
        ASSERT_TRUE(manager.initialize().success());
    }

    void TearDown() override
    {
        manager.shutdown();
        mc::test::removeTestDir(root);
    }

    std::filesystem::path pack(const std::string& folder, const std::string& uuid, std::optional<std::string> script)
    {
        const auto directory = root / folder;
        std::filesystem::create_directories(directory / "entities/nested");
        nlohmann::json manifest = {{"format_version", 2},
            {"header", {{"name", folder}, {"uuid", uuid}, {"version", {1, 0, 0}}, {"min_engine_version", {1, 21, 90}}}},
            {"modules",
                {{{"type", "data"}, {"uuid", "cccccccc-cccc-4ccc-8ccc-cccccccccccc"}, {"version", {1, 0, 0}}}}}};
        if (script) {
            manifest["modules"].push_back({{"type", "script"},
                {"uuid", "dddddddd-dddd-4ddd-8ddd-dddddddddddd"},
                {"version", {1, 0, 0}},
                {"entry", "scripts/main.js"},
                {"language", "javascript"}});
            manifest["dependencies"] = {{{"module_name", "@minecraft/server"}, {"version", "2.0.0"}}};
            std::filesystem::create_directories(directory / "scripts");
            std::ofstream(directory / "scripts/main.js") << *script;
        }
        std::ofstream(directory / "manifest.json") << manifest.dump();
        return directory;
    }

    void write(const std::filesystem::path& path, const nlohmann::json& document)
    {
        std::ofstream(path) << document.dump();
    }

    std::unique_ptr<mc::Entity> create(const std::string& id)
    {
        const auto* type = mc::entity::EntityRegistry::instance().getType(id);
        EXPECT_NE(type, nullptr);
        return type != nullptr ? type->create(nullptr, entities) : nullptr;
    }

    std::filesystem::path root;
    mc::ecs::EntityRegistry entities{"addon-defined-entities"};
    ScriptManager manager;
};
} // namespace

TEST_F(AddonEntityDefinitionTest, PushableDrivesIsPushableAndPistonReaction)
{
    // is_pushable 决定实体能否被其他实体推动（同时影响实体碰撞与方块放置判定）；
    // is_pushable_by_piston 为 false 时活塞扫过应跳过该实体（PushReaction::Ignore）。
    auto definition = _definition("addon_test:anchor");
    definition["minecraft:entity"]["components"]["minecraft:pushable"] = {
        {"is_pushable", false}, {"is_pushable_by_piston", false}};

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/anchor.json", definition);
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();

    auto actor = create("addon_test:anchor");
    ASSERT_NE(actor, nullptr);
    EXPECT_FALSE(actor->isPushable());
    EXPECT_EQ(actor->getPushReaction(), mc::PushReaction::Ignore);
}

TEST_F(AddonEntityDefinitionTest, PushableDefaultsKeepVanillaBehavior)
{
    // 字面上等价于缺省值的配置必须与不写该组件一致：is_pushable=true 仍可推，
    // is_pushable_by_piston=true 不返回 Ignore。这条用例防止实现把默认值写反。
    auto definition = _definition("addon_test:pushy");
    definition["minecraft:entity"]["components"]["minecraft:pushable"] = {
        {"is_pushable", true}, {"is_pushable_by_piston", true}};

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/pushy.json", definition);
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();

    auto actor = create("addon_test:pushy");
    ASSERT_NE(actor, nullptr);
    EXPECT_TRUE(actor->isPushable());
    EXPECT_NE(actor->getPushReaction(), mc::PushReaction::Ignore);
}

TEST_F(AddonEntityDefinitionTest, PushableRejectsNonBoolean)
{
    auto definition = _definition("addon_test:bad_pushable");
    definition["minecraft:entity"]["components"]["minecraft:pushable"] = {{"is_pushable", "no"}};

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/bad_pushable.json", definition);
    EXPECT_TRUE(manager.loadPacks("", root.string()).failed());
    EXPECT_FALSE(mc::entity::EntityRegistry::instance().hasType("addon_test:bad_pushable"));
}

TEST_F(AddonEntityDefinitionTest, PushableAppliesToLivingMobsToo)
{
    // pushable 同时适用于非生物 actor 与生物 mob。带 minecraft:health 的定义会走 _AddonMob，
    // 这里确认生物分支也读到了同一组件（两条实现分别位于两个类，漏一个就会静默失效）。
    auto definition = _definition("addon_test:heavy_mob");
    definition["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    definition["minecraft:entity"]["components"]["minecraft:pushable"] = {
        {"is_pushable", false}, {"is_pushable_by_piston", false}};

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/heavy_mob.json", definition);
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();

    auto mob = create("addon_test:heavy_mob");
    ASSERT_NE(mob, nullptr);
    ASSERT_NE(dynamic_cast<mc::LivingEntity*>(mob.get()), nullptr) << "带 health 的定义应创建生物";
    EXPECT_FALSE(mob->isPushable());
    EXPECT_EQ(mob->getPushReaction(), mc::PushReaction::Ignore);
}

TEST_F(AddonEntityDefinitionTest, AttackDamageAcceptsThreeShapes)
{
    // 官方文档给出三种承载形态，基岩原版实体定义里三种都出现过，只认一种会让合法 addon 加载失败。
    // 同时验证区间取中值写入 attack_damage 属性。
    struct Case {
        const char* file;
        const char* id;
        nlohmann::json damage;
        mc::f64 expected;
    };

    const std::array<Case, 3> cases{{
        {"attacker_number", "addon_test:attacker_number", 5, 5.0},
        {"attacker_array", "addon_test:attacker_array", nlohmann::json::array({4, 8}), 6.0},
        {"attacker_range", "addon_test:attacker_range", {{"range_min", 3}, {"range_max", 7}}, 5.0},
    }};

    const auto directory = pack("data", _PACK, std::nullopt);
    for (const auto& testCase : cases) {
        auto definition = _definition(testCase.id);
        definition["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
        definition["minecraft:entity"]["components"]["minecraft:attack"] = {{"damage", testCase.damage}};
        write(directory / (std::string("entities/") + testCase.file + ".json"), definition);
    }

    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();

    for (const auto& testCase : cases) {
        auto entity = create(testCase.id);
        ASSERT_NE(entity, nullptr) << testCase.id;
        auto* living = dynamic_cast<mc::LivingEntity*>(entity.get());
        ASSERT_NE(living, nullptr) << testCase.id;
        EXPECT_DOUBLE_EQ(
            living->getAttributeValue(mc::entity::attribute::Attributes::ATTACK_DAMAGE, 0.0), testCase.expected)
            << testCase.id;
    }
}

TEST_F(AddonEntityDefinitionTest, AttackDamageRejectsMalformedShapes)
{
    // 形态不认识必须报错，不能静默当成 0 或默认值——那会让"攻击不疼"变成无法定位的问题。
    const std::array<nlohmann::json, 3> invalid{
        nlohmann::json::array({1, 2, 3}), // 数组元素个数不对
        nlohmann::json{{"range_min", 1}}, // 缺 range_max
        "five",                           // 既非数字也非数组/对象
    };

    for (const auto& damage : invalid) {
        auto definition = _definition("addon_test:bad_attack");
        definition["minecraft:entity"]["components"]["minecraft:attack"] = {{"damage", damage}};
        EXPECT_TRUE(AddonEntityDefinition::parse(definition).failed()) << damage.dump();
    }
}

TEST_F(AddonEntityDefinitionTest, AttackEffectFieldsAreTypeChecked)
{
    // effect_name 必须是非空字符串；duration / amplifier 必须是非负数字/整数。
    const std::array<nlohmann::json, 3> invalid{
        nlohmann::json{{"damage", 3}, {"effect_name", ""}},
        nlohmann::json{{"damage", 3}, {"effect_name", "minecraft:poison"}, {"effect_duration", -1}},
        nlohmann::json{{"damage", 3}, {"effect_name", "minecraft:poison"}, {"effect_amplifier", -1}},
    };

    for (const auto& component : invalid) {
        auto definition = _definition("addon_test:bad_effect");
        definition["minecraft:entity"]["components"]["minecraft:attack"] = component;
        EXPECT_TRUE(AddonEntityDefinition::parse(definition).failed()) << component.dump();
    }
}

TEST_F(AddonEntityDefinitionTest, NameableDrivesNameTagRenaming)
{
    // allow_name_tag_renaming 为 false 时命名牌对该实体无效（NameTagItem 查询 isNameableByTag）。
    // 默认（不写该组件、或写 true）必须保持"可改名"，否则正常 addon 实体会被误锁。
    auto refusing = _definition("addon_test:unnameable");
    refusing["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    refusing["minecraft:entity"]["components"]["minecraft:nameable"] = {{"allow_name_tag_renaming", false}};

    auto accepting = _definition("addon_test:nameable");
    accepting["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    accepting["minecraft:entity"]["components"]["minecraft:nameable"] = {{"allow_name_tag_renaming", true}};

    auto implicit = _definition("addon_test:nameable_default");
    implicit["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/unnameable.json", refusing);
    write(directory / "entities/nameable.json", accepting);
    write(directory / "entities/nameable_default.json", implicit);
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();

    auto refusingEntity = create("addon_test:unnameable");
    auto* refusingMob = dynamic_cast<mc::MobEntity*>(refusingEntity.get());
    ASSERT_NE(refusingMob, nullptr);
    EXPECT_FALSE(refusingMob->isNameableByTag());

    auto acceptingEntity = create("addon_test:nameable");
    auto* acceptingMob = dynamic_cast<mc::MobEntity*>(acceptingEntity.get());
    ASSERT_NE(acceptingMob, nullptr);
    EXPECT_TRUE(acceptingMob->isNameableByTag());

    auto implicitEntity = create("addon_test:nameable_default");
    auto* implicitMob = dynamic_cast<mc::MobEntity*>(implicitEntity.get());
    ASSERT_NE(implicitMob, nullptr);
    EXPECT_TRUE(implicitMob->isNameableByTag());
}

TEST_F(AddonEntityDefinitionTest, NameableRejectsNonBoolean)
{
    auto definition = _definition("addon_test:bad_nameable");
    definition["minecraft:entity"]["components"]["minecraft:nameable"] = {{"always_show", "yes"}};

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/bad_nameable.json", definition);
    EXPECT_TRUE(manager.loadPacks("", root.string()).failed());
    EXPECT_FALSE(mc::entity::EntityRegistry::instance().hasType("addon_test:bad_nameable"));
}

// minecraft:inventory 此前落在"未实现"分支，含该组件的 addon 实体定义直接加载失败。
// 本轮接入解析与字段校验（实体持有真实库存容器仍是 TODO，见定义处说明）。
// minecraft:leashable 此前落在"未实现组件"分支，含该组件的 addon 实体定义直接加载失败。
// 本轮接入解析与字段校验，并让 _AddonMob::canBeLeashed 消费它（声明即可被拴）。
// minecraft:breathable 此前落在"未实现组件"分支，含该组件的 addon 实体定义直接加载失败。
// 原版 schema 同时给 snake_case 与 camelCase 两套键，两种都必须接受。
// minecraft:healable 此前落在"未实现组件"分支，含该组件的 addon 实体定义直接加载失败。
// minecraft:rideable 此前落在"未实现组件"分支，含该组件的 addon 实体定义直接加载失败。
// 本轮接入解析与字段校验，并让 seat_count 驱动 getMaxPassengers。
// minecraft:burns_in_daylight 此前落在"未实现组件"分支，含该组件的 addon 实体定义直接加载失败。
// minecraft:experience_reward 此前落在"未实现组件"分支，含该组件的 addon 实体定义直接加载失败。
// minecraft:can_fly 此前落在"未实现组件"分支；它同时暴露了 FlyNodeEvaluator 从未被接入。
TEST_F(AddonEntityDefinitionTest, CanFlyComponentIsAcceptedAndSwapsNavigator)
{
    auto flyer = _definition("addon_test:flyer");
    flyer["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    flyer["minecraft:entity"]["components"]["minecraft:can_fly"] = nlohmann::json::object();

    auto walker = _definition("addon_test:walker");
    walker["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/flyer.json", flyer);
    write(directory / "entities/walker.json", walker);
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();

    // 两者都必须有导航器（基类默认创建地面导航器）；声明 can_fly 后应被替换成飞行求值器。
    auto flyerEntity = create("addon_test:flyer");
    ASSERT_NE(flyerEntity, nullptr);
    auto* flyerMob = dynamic_cast<mc::MobEntity*>(flyerEntity.get());
    ASSERT_NE(flyerMob, nullptr);
    ASSERT_NE(flyerMob->navigator(), nullptr) << "an addon mob must always have a navigator";

    auto walkerEntity = create("addon_test:walker");
    ASSERT_NE(walkerEntity, nullptr);
    auto* walkerMob = dynamic_cast<mc::MobEntity*>(walkerEntity.get());
    ASSERT_NE(walkerMob, nullptr);
    ASSERT_NE(walkerMob->navigator(), nullptr);

    // 导航器都是 GroundPathNavigator，差别在内层求值器：飞行用 FlyNodeEvaluator。
    // 用 dynamic_cast 判断而不是比较 typeid 名称（后者是实现定义字符串，不可靠）。
    auto* flyerEvaluator = flyerMob->navigator()->nodeEvaluator();
    auto* walkerEvaluator = walkerMob->navigator()->nodeEvaluator();
    ASSERT_NE(flyerEvaluator, nullptr);
    ASSERT_NE(walkerEvaluator, nullptr);
    EXPECT_NE(dynamic_cast<mc::entity::ai::pathfinding::FlyNodeEvaluator*>(flyerEvaluator), nullptr)
        << "can_fly must install the flying node evaluator";
    EXPECT_EQ(dynamic_cast<mc::entity::ai::pathfinding::FlyNodeEvaluator*>(walkerEvaluator), nullptr)
        << "an addon mob without can_fly must keep the ground evaluator";
}

TEST_F(AddonEntityDefinitionTest, ExperienceRewardComponentIsAccepted)
{
    // 三种写法都要认：字面数字、Molang 字符串、{expression,version} 对象。
    nlohmann::json rewards = {{"on_death", 5}, {"on_bred", "Math.Random(1,7)"}};
    auto plain = _definition("addon_test:xp_plain");
    plain["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    plain["minecraft:entity"]["components"]["minecraft:experience_reward"] = rewards;

    auto objectForm = _definition("addon_test:xp_object");
    objectForm["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    objectForm["minecraft:entity"]["components"]["minecraft:experience_reward"] = {
        {"on_death", {{"expression", "query.last_hit_by_player ? 10 : 0"}, {"version", 12}}}};

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/xp_plain.json", plain);
    write(directory / "entities/xp_object.json", objectForm);

    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
}

TEST_F(AddonEntityDefinitionTest, ExperienceRewardRejectsInvalidFields)
{
    struct Case {
        nlohmann::json component;
        const char* label;
    };

    std::vector<Case> invalid;
    invalid.push_back({{{("on_death"), -3}}, "negative on_death"});
    invalid.push_back({{{("on_death"), ""}}, "empty expression string"});
    invalid.push_back({{{("on_death"), {{"version", 12}}}}, "object form without expression"});
    invalid.push_back({{{("on_death"), {{"expression", "5"}, {"unknow_key", 1}}}}, "unknown object field"});
    invalid.push_back({{{("on_death"), nlohmann::json::array()}}, "array is not a valid form"});

    for (const auto& testCase : invalid) {
        auto definition = _definition("addon_test:bad_xp");
        definition["minecraft:entity"]["components"]["minecraft:experience_reward"] = testCase.component;
        EXPECT_TRUE(AddonEntityDefinition::parse(definition).failed()) << testCase.label;
    }
}

// on_death 必须在死亡时求值并驱动 experienceValue；未声明该组件的实体不带经验。
TEST_F(AddonEntityDefinitionTest, ExperienceRewardDrivesDeathExperience)
{
    auto numeric = _definition("addon_test:xp_numeric");
    numeric["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    numeric["minecraft:entity"]["components"]["minecraft:experience_reward"] = {{"on_death", 7}};

    auto none = _definition("addon_test:xp_none");
    none["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/xp_numeric.json", numeric);
    write(directory / "entities/xp_none.json", none);
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();

    auto rewarded = create("addon_test:xp_numeric");
    ASSERT_NE(rewarded, nullptr);
    auto* rewardedMob = dynamic_cast<mc::MobEntity*>(rewarded.get());
    ASSERT_NE(rewardedMob, nullptr);
    EXPECT_EQ(rewardedMob->experienceValue(), 0) << "experience must not be granted before death";
    // 直接触发经验掉落：数值型 on_death 应写入 experienceValue。
    rewardedMob->dropExperience();
    EXPECT_EQ(rewardedMob->experienceValue(), 7) << "numeric on_death must drive the experience value";

    auto plain = create("addon_test:xp_none");
    ASSERT_NE(plain, nullptr);
    auto* plainMob = dynamic_cast<mc::MobEntity*>(plain.get());
    ASSERT_NE(plainMob, nullptr);
    plainMob->dropExperience();
    EXPECT_EQ(plainMob->experienceValue(), 0)
        << "without minecraft:experience_reward the entity must carry no experience";
}

TEST_F(AddonEntityDefinitionTest, ExperienceRewardEvaluatesEntityQueries)
{
    // 用"else 分支非零"的表达式区分两种失败模式：
    //   - 查询被正确解析并求值（未被玩家击中 → 0）→ 取 else 分支 5；
    //   - 表达式编译失败 → 整条奖励回退 0。
    // 若断言写成 `? 10 : 0`，两种模式都得到 0，测试会失去判别力。
    auto hitByPlayer = _definition("addon_test:xp_query");
    hitByPlayer["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    hitByPlayer["minecraft:entity"]["components"]["minecraft:experience_reward"] = {
        {"on_death", "query.last_hit_by_player ? 10 : 5"}};

    // 尚未实现的查询按基岩语义取 0，同样落到 else 分支，不能拖垮整条奖励。
    auto unimplemented = _definition("addon_test:xp_query_unknown");
    unimplemented["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    unimplemented["minecraft:entity"]["components"]["minecraft:experience_reward"] = {
        {"on_death", "query.not_implemented ? 10 : 5"}};

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/xp_query.json", hitByPlayer);
    write(directory / "entities/xp_query_unknown.json", unimplemented);
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();

    auto queried = create("addon_test:xp_query");
    ASSERT_NE(queried, nullptr);
    auto* queriedMob = dynamic_cast<mc::MobEntity*>(queried.get());
    ASSERT_NE(queriedMob, nullptr);
    queriedMob->dropExperience();
    EXPECT_EQ(queriedMob->experienceValue(), 5)
        << "query.last_hit_by_player must resolve to 0 for an entity never hit by a player";

    auto unknown = create("addon_test:xp_query_unknown");
    ASSERT_NE(unknown, nullptr);
    auto* unknownMob = dynamic_cast<mc::MobEntity*>(unknown.get());
    ASSERT_NE(unknownMob, nullptr);
    unknownMob->dropExperience();
    EXPECT_EQ(unknownMob->experienceValue(), 5)
        << "an unimplemented query must evaluate to 0 instead of failing the whole reward";
}

TEST_F(AddonEntityDefinitionTest, BurnsInDaylightComponentIsAccepted)
{
    for (const auto& [id, slot] :
        std::array<std::pair<const char*, const char*>, 5>{std::pair{"addon_test:burn_head", "slot.armor.head"},
            std::pair{"addon_test:burn_body", "slot.armor.body"},
            std::pair{"addon_test:burn_chest", "slot.armor.chest"},
            std::pair{"addon_test:burn_mainhand", "slot.weapon.mainhand"},
            std::pair{"addon_test:burn_bare", "head"}}) {
        auto definition = _definition(id);
        definition["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
        definition["minecraft:entity"]["components"]["minecraft:burns_in_daylight"] = {{"protection_slot", slot}};
        EXPECT_TRUE(AddonEntityDefinition::parse(definition).success()) << slot;
    }

    // 官方示例的写法就是空对象（默认头盔位保护）。
    auto minimal = _definition("addon_test:burn_default");
    minimal["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    minimal["minecraft:entity"]["components"]["minecraft:burns_in_daylight"] = nlohmann::json::object();
    EXPECT_TRUE(AddonEntityDefinition::parse(minimal).success());
}

TEST_F(AddonEntityDefinitionTest, BurnsInDaylightRejectsUnknownProtectionSlot)
{
    auto definition = _definition("addon_test:bad_burn");
    definition["minecraft:entity"]["components"]["minecraft:burns_in_daylight"] = {
        {"protection_slot", "slot.armor.tail"}};
    EXPECT_TRUE(AddonEntityDefinition::parse(definition).failed())
        << "an unknown protection slot must be rejected rather than silently ignored";

    auto wrongType = _definition("addon_test:bad_burn_type");
    wrongType["minecraft:entity"]["components"]["minecraft:burns_in_daylight"] = {{"protection_slot", 3}};
    EXPECT_TRUE(AddonEntityDefinition::parse(wrongType).failed());
}

// 声明该组件就必须日光燃烧；防护槽位穿了装备则免疫（基岩默认头盔位）。
TEST_F(AddonEntityDefinitionTest, BurnsInDaylightDrivesSunBurningAndHelmetProtection)
{
    auto burner = _definition("addon_test:sunburner");
    burner["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    burner["minecraft:entity"]["components"]["minecraft:burns_in_daylight"] = nlohmann::json::object();

    auto plain = _definition("addon_test:no_sunburn");
    plain["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/sunburner.json", burner);
    write(directory / "entities/no_sunburn.json", plain);
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();

    auto burning = create("addon_test:sunburner");
    ASSERT_NE(burning, nullptr);
    auto* burningMob = dynamic_cast<mc::MobEntity*>(burning.get());
    ASSERT_NE(burningMob, nullptr);
    EXPECT_TRUE(burningMob->shouldBurnInDaylight()) << "declaring the component must enable sun burning";

    // 默认 protection_slot 是头盔位：戴上头盔后必须免疫。
    const auto* helmet = mc::Items::DIAMOND_HELMET;
    if (helmet != nullptr) {
        burningMob->setEquipment(mc::EquipmentSlot::Head, mc::ItemStack(*helmet, 1));
        EXPECT_FALSE(burningMob->shouldBurnInDaylight())
            << "a helmet in the default protection slot must prevent burning";
    }

    auto plainEntity = create("addon_test:no_sunburn");
    ASSERT_NE(plainEntity, nullptr);
    auto* plainMob = dynamic_cast<mc::MobEntity*>(plainEntity.get());
    ASSERT_NE(plainMob, nullptr);
    EXPECT_FALSE(plainMob->shouldBurnInDaylight()) << "without the component an addon entity must not burn in daylight";
}

TEST_F(AddonEntityDefinitionTest, RideableComponentIsAccepted)
{
    auto full = _definition("addon_test:rideable_mob");
    full["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    full["minecraft:entity"]["components"]["minecraft:rideable"] = {{"seat_count", 2},
        {"controlling_seat", 0},
        {"crouching_skip_interact", false},
        {"family_types", nlohmann::json::array({"player"})},
        {"interact_text", "action.interact.mount"},
        {"dismount_mode", "on_top_center"},
        {"pull_in_entities", true},
        {"rider_can_interact", true}};

    auto minimal = _definition("addon_test:rideable_default");
    minimal["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    minimal["minecraft:entity"]["components"]["minecraft:rideable"] = nlohmann::json::object();

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/rideable_mob.json", full);
    write(directory / "entities/rideable_default.json", minimal);

    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
}

TEST_F(AddonEntityDefinitionTest, RideableComponentRejectsInvalidFields)
{
    struct Case {
        nlohmann::json component;
        const char* label;
    };

    const std::array<Case, 4> invalid{{
        {{{"seat_count", 0}}, "zero seat_count is self-contradictory"},
        {{{"seat_count", "two"}}, "non-integer seat_count"},
        {{{"family_types", "player"}}, "non-array family_types"},
        {{{"dismount_mode", "sideways"}}, "unknown dismount_mode"},
    }};

    for (const auto& testCase : invalid) {
        auto definition = _definition("addon_test:bad_rideable");
        definition["minecraft:entity"]["components"]["minecraft:rideable"] = testCase.component;
        EXPECT_TRUE(AddonEntityDefinition::parse(definition).failed()) << testCase.label;
    }
}

// seat_count 必须驱动 getMaxPassengers；未声明 rideable 的实体不得接受乘客。
TEST_F(AddonEntityDefinitionTest, RideableDrivesPassengerCapacity)
{
    auto twoSeats = _definition("addon_test:two_seater");
    twoSeats["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    twoSeats["minecraft:entity"]["components"]["minecraft:rideable"] = {{"seat_count", 3}};

    auto noRide = _definition("addon_test:not_rideable");
    noRide["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/two_seater.json", twoSeats);
    write(directory / "entities/not_rideable.json", noRide);
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();

    auto seater = create("addon_test:two_seater");
    ASSERT_NE(seater, nullptr);
    EXPECT_EQ(seater->getMaxPassengers(), 3) << "seat_count must drive the passenger capacity";
    EXPECT_TRUE(seater->couldAcceptPassenger()) << "declaring minecraft:rideable must allow passengers";

    auto noRideEntity = create("addon_test:not_rideable");
    ASSERT_NE(noRideEntity, nullptr);
    EXPECT_FALSE(noRideEntity->couldAcceptPassenger())
        << "an addon entity without minecraft:rideable must not accept passengers";
}

TEST_F(AddonEntityDefinitionTest, HealableComponentIsAccepted)
{
    auto full = _definition("addon_test:healable_mob");
    full["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 20}, {"max", 20}};
    nlohmann::json cookieEffect = {{"name", "poison"}, {"duration", 100}};
    nlohmann::json cookie = {
        {"item", "minecraft:cookie"}, {"heal_amount", 0}, {"effects", nlohmann::json::array({cookieEffect})}};
    nlohmann::json hay = {{"item", "minecraft:hay_block"}, {"heal_amount", 10}};
    nlohmann::json wheat = {{"item", "minecraft:wheat"}, {"heal_amount", 2}};
    full["minecraft:entity"]["components"]["minecraft:healable"] = {{"force_use", true},
        {"filters", {{"test", "is_riding"}, {"operator", "!="}, {"value", true}}},
        {"items", nlohmann::json::array({wheat, hay, cookie})}};

    auto minimal = _definition("addon_test:healable_default");
    minimal["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 20}, {"max", 20}};
    minimal["minecraft:entity"]["components"]["minecraft:healable"] = nlohmann::json::object();

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/healable_mob.json", full);
    write(directory / "entities/healable_default.json", minimal);

    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
}

TEST_F(AddonEntityDefinitionTest, HealableComponentRejectsInvalidFields)
{
    struct Case {
        nlohmann::json component;
        const char* label;
    };

    const std::array<Case, 4> invalid{{
        {{{"items", "wheat"}}, "non-array items"},
        {{{"items", nlohmann::json::array({{{"heal_amount", 2}}})}}, "entry missing item"},
        {{{"items", nlohmann::json::array({{{"item", "minecraft:wheat"}, {"heal_amount", -1}}})}},
            "negative heal_amount"},
        {{{"force_use", "yes"}}, "non-boolean force_use"},
    }};

    for (const auto& testCase : invalid) {
        auto definition = _definition("addon_test:bad_healable");
        definition["minecraft:entity"]["components"]["minecraft:healable"] = testCase.component;
        EXPECT_TRUE(AddonEntityDefinition::parse(definition).failed()) << testCase.label;
    }
}

// 喂食治疗是服务端行为：声明 healable 的生物被喂对应物品时必须回血，且物品被消耗。
TEST_F(AddonEntityDefinitionTest, HealableRestoresHealthFromMatchingItem)
{
    if (mc::Items::WHEAT == nullptr || mc::Items::DIRT == nullptr) {
        GTEST_SKIP() << "item registry not initialized";
    }

    auto definition = _definition("addon_test:fed_mob");
    definition["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 20}, {"max", 20}};
    definition["minecraft:entity"]["components"]["minecraft:healable"] = {
        {"items", nlohmann::json::array({{{"item", "minecraft:wheat"}, {"heal_amount", 5}}})}};

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/fed_mob.json", definition);
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();

    auto entity = create("addon_test:fed_mob");
    ASSERT_NE(entity, nullptr);
    auto* mob = dynamic_cast<mc::MobEntity*>(entity.get());
    ASSERT_NE(mob, nullptr);

    // 掉血到 10，喂小麦应回到 15。
    mob->setHealth(10.0f);
    mc::ItemStack wheat(mc::Items::WHEAT, 3);
    mc::Player player(mc::EntityInstanceId(9001), "Feeder", entities);
    ASSERT_TRUE(mob->tryHealFromItem(wheat, player));
    EXPECT_FLOAT_EQ(mob->health(), 15.0f);
    EXPECT_EQ(wheat.getCount(), 2) << "survival feeding must consume one item";

    // 不匹配的物品不治疗，也不消耗。
    mc::ItemStack dirt(mc::Items::DIRT, 1);
    EXPECT_FALSE(mob->tryHealFromItem(dirt, player));
    EXPECT_EQ(dirt.getCount(), 1);

    // 满血时不治疗（force_use 默认 false）。
    mob->setHealth(20.0f);
    mc::ItemStack spare(mc::Items::WHEAT, 1);
    EXPECT_FALSE(mob->tryHealFromItem(spare, player));
    EXPECT_EQ(spare.getCount(), 1) << "no consumption when already at full health";
}

TEST_F(AddonEntityDefinitionTest, BreathableComponentIsAccepted)
{
    auto snakeCase = _definition("addon_test:breather_snake");
    snakeCase["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    snakeCase["minecraft:entity"]["components"]["minecraft:breathable"] = {
        {"total_supply", 15}, {"suffocate_time", 0}, {"breathes_water", true}, {"generates_bubbles", false}};

    auto camelCase = _definition("addon_test:breather_camel");
    camelCase["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    camelCase["minecraft:entity"]["components"]["minecraft:breathable"] = {{"totalSupply", 15}, {"suffocateTime", 0}};

    auto minimal = _definition("addon_test:breather_default");
    minimal["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    minimal["minecraft:entity"]["components"]["minecraft:breathable"] = nlohmann::json::object();

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/breather_snake.json", snakeCase);
    write(directory / "entities/breather_camel.json", camelCase);
    write(directory / "entities/breather_default.json", minimal);

    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
}

TEST_F(AddonEntityDefinitionTest, BreathableComponentRejectsInvalidFields)
{
    struct Case {
        nlohmann::json component;
        const char* label;
    };

    const std::array<Case, 4> invalid{{
        {{{"total_supply", -1}}, "negative total_supply"},
        {{{"total_supply", "long"}}, "non-numeric total_supply"},
        {{{"breathes_water", "yes"}}, "non-boolean breathes_water"},
        {{{"breathe_blocks", "stone"}}, "non-array breathe_blocks"},
    }};

    for (const auto& testCase : invalid) {
        auto definition = _definition("addon_test:bad_breather");
        definition["minecraft:entity"]["components"]["minecraft:breathable"] = testCase.component;
        EXPECT_TRUE(AddonEntityDefinition::parse(definition).failed()) << testCase.label;
    }
}

// total_supply 的单位是秒，项目空气量按 tick 计；15 秒应换算为 300（vanilla 默认值）。
TEST_F(AddonEntityDefinitionTest, BreathableDrivesMaxAirAndUnderwaterBreathing)
{
    auto definition = _definition("addon_test:breathing_mob");
    definition["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    definition["minecraft:entity"]["components"]["minecraft:breathable"] = {
        {"total_supply", 15}, {"breathes_water", true}};

    auto camel = _definition("addon_test:breathing_camel_mob");
    camel["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    camel["minecraft:entity"]["components"]["minecraft:breathable"] = {{"totalSupply", 5}};

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/breathing_mob.json", definition);
    write(directory / "entities/breathing_camel_mob.json", camel);
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();

    auto entity = create("addon_test:breathing_mob");
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->maxAir(), 300) << "15 seconds must convert to 300 ticks";
    auto* living = dynamic_cast<mc::LivingEntity*>(entity.get());
    ASSERT_NE(living, nullptr);
    EXPECT_TRUE(living->canBreatheUnderwater()) << "breathes_water=true must allow underwater breathing";

    // camelCase 键同样要能驱动换算，否则原版实体的写法会被静默忽略。
    auto camelEntity = create("addon_test:breathing_camel_mob");
    ASSERT_NE(camelEntity, nullptr);
    EXPECT_EQ(camelEntity->maxAir(), 100) << "5 seconds must convert to 100 ticks";
}

TEST_F(AddonEntityDefinitionTest, LeashableComponentIsAccepted)
{
    auto full = _definition("addon_test:leashable_mob");
    full["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    full["minecraft:entity"]["components"]["minecraft:leashable"] = {{"can_be_cut", false},
        {"can_be_stolen", true},
        {"soft_distance", 4},
        {"hard_distance", 6},
        {"max_distance", 10},
        {"unleash_on_removal", false},
        {"on_unleash_interact_only", true},
        {"presets", nlohmann::json::array({{{"hard_distance", 7}}})}};

    // 官方文档给所有字段都定了默认值，空对象写法同样必须接受。
    auto minimal = _definition("addon_test:leashable_default");
    minimal["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    minimal["minecraft:entity"]["components"]["minecraft:leashable"] = nlohmann::json::object();

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/leashable_mob.json", full);
    write(directory / "entities/leashable_default.json", minimal);

    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
}

TEST_F(AddonEntityDefinitionTest, LeashableComponentRejectsInvalidFields)
{
    struct Case {
        nlohmann::json component;
        const char* label;
    };

    const std::array<Case, 4> invalid{{
        {{{"soft_distance", "near"}}, "non-integer soft_distance"},
        {{{"max_distance", -1}}, "negative max_distance"},
        {{{"can_be_cut", "yes"}}, "non-boolean can_be_cut"},
        {{{"presets", "one"}}, "non-array presets"},
    }};

    for (const auto& testCase : invalid) {
        auto definition = _definition("addon_test:bad_leashable");
        definition["minecraft:entity"]["components"]["minecraft:leashable"] = testCase.component;
        EXPECT_TRUE(AddonEntityDefinition::parse(definition).failed()) << testCase.label;
    }
}

// 声明 leashable 的 addon 生物必须可以被拴；未声明时沿用基类判断（敌对生物不可拴）。
TEST_F(AddonEntityDefinitionTest, LeashableDrivesCanBeLeashed)
{
    auto definition = _definition("addon_test:leashed_mob");
    definition["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    definition["minecraft:entity"]["components"]["minecraft:leashable"] = nlohmann::json::object();

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/leashed_mob.json", definition);
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();

    auto entity = create("addon_test:leashed_mob");
    ASSERT_NE(entity, nullptr);
    auto* mob = dynamic_cast<mc::MobEntity*>(entity.get());
    ASSERT_NE(mob, nullptr);
    EXPECT_TRUE(mob->canBeLeashed()) << "declaring minecraft:leashable must make the mob leasable";
}

TEST_F(AddonEntityDefinitionTest, InventoryComponentIsAccepted)
{
    // 一次性把「全字段」与「空对象（全默认）」两个定义都写入同一个数据包再统一加载：
    // 同一 manager 上重复 loadPacks 会触发重复注册，无法区分是解析失败还是装载冲突。
    auto full = _definition("addon_test:container_mob");
    full["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    full["minecraft:entity"]["components"]["minecraft:inventory"] = {{"inventory_size", 27},
        {"container_type", "container"},
        {"private", true},
        {"can_be_siphoned_from", true},
        {"restrict_to_owner", false},
        {"additional_slots_per_strength", 1}};

    auto minimal = _definition("addon_test:container_default");
    minimal["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 10}, {"max", 10}};
    minimal["minecraft:entity"]["components"]["minecraft:inventory"] = nlohmann::json::object();

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/container_mob.json", full);
    write(directory / "entities/container_default.json", minimal);

    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
}

TEST_F(AddonEntityDefinitionTest, InventoryComponentRejectsInvalidFields)
{
    struct Case {
        nlohmann::json component;
        const char* label;
    };

    const std::array<Case, 4> invalid{{
        {{{"inventory_size", "many"}}, "non-integer inventory_size"},
        {{{"inventory_size", -1}}, "negative inventory_size"},
        {{{"private", "yes"}}, "non-boolean private"},
        {{{"container_type", ""}}, "empty container_type"},
    }};

    for (const auto& testCase : invalid) {
        auto definition = _definition("addon_test:bad_container");
        definition["minecraft:entity"]["components"]["minecraft:inventory"] = testCase.component;
        EXPECT_TRUE(AddonEntityDefinition::parse(definition).failed()) << testCase.label;
    }
}

TEST_F(AddonEntityDefinitionTest, InstantDespawnRemoveChildEntitiesIsAccepted)
{
    // 此前 remove_child_entities 为 true 会让整个实体定义加载失败（解析期返回 Unsupported）。
    // 现在它被接受，并由 AddonEntityComponents::despawnIfRequested 消费：discard 自身前，
    // 把拴在本实体身上的子实体（leashHolderUuid == 本实体 uuid）一并 discard。
    //
    // 子实体清理需要真实 IWorld 才能枚举周边实体，而本夹具用 world=nullptr 建实体，
    // 因此这里只守"定义能被接受且实体可创建"这一段；实体侧行为由 tick 路径保证。
    auto definition = _definition("addon_test:despawner");
    definition["minecraft:entity"]["components"]["minecraft:instant_despawn"] = {{"remove_child_entities", true}};

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/despawner.json", definition);
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();

    auto actor = create("addon_test:despawner");
    ASSERT_NE(actor, nullptr);
    const auto* components = actor->tryGetComponent<mc::mod::bedrock::addon::AddonEntityComponents>();
    ASSERT_NE(components, nullptr);
    EXPECT_TRUE(components->has("minecraft:instant_despawn"));
}

TEST_F(AddonEntityDefinitionTest, InstantDespawnRemoveChildEntitiesRejectsNonBoolean)
{
    // 放开的是"行为未实现"，不是类型校验：字符串形态仍必须被拒绝。
    auto definition = _definition("addon_test:bad_despawner");
    definition["minecraft:entity"]["components"]["minecraft:instant_despawn"] = {{"remove_child_entities", "yes"}};

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/bad_despawner.json", definition);
    EXPECT_TRUE(manager.loadPacks("", root.string()).failed());
    EXPECT_FALSE(mc::entity::EntityRegistry::instance().hasType("addon_test:bad_despawner"));
}

TEST_F(AddonEntityDefinitionTest, PhysicsPushTowardsClosestSpaceIsAccepted)
{
    // 此前 push_towards_closest_space 为 true 会让整个实体定义加载失败（解析期直接返回
    // Unsupported），合法 addon 用不了这个字段。现在它被接受，并由 _AddonMob 的脱困推挤
    // 消费（卡进方块时移到最近空位）。
    auto definition = _definition("addon_test:pusher");
    definition["minecraft:entity"]["components"]["minecraft:physics"]["push_towards_closest_space"] = true;

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/pusher.json", definition);
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();

    auto actor = create("addon_test:pusher");
    ASSERT_NE(actor, nullptr);
    // 该字段只影响"卡住时是否脱困"，不改变无重力/碰撞这类静态物理状态。
    EXPECT_TRUE(actor->hasNoGravity());
    EXPECT_FALSE(actor->noClip());
}

TEST_F(AddonEntityDefinitionTest, PhysicsPushTowardsClosestSpaceRejectsNonBoolean)
{
    // 字段本身仍必须是布尔值：这里只放开"实现了行为"，不放过类型错误。
    auto definition = _definition("addon_test:bad_pusher");
    definition["minecraft:entity"]["components"]["minecraft:physics"]["push_towards_closest_space"] = "true";

    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/bad_pusher.json", definition);
    EXPECT_TRUE(manager.loadPacks("", root.string()).failed());
    EXPECT_FALSE(mc::entity::EntityRegistry::instance().hasType("addon_test:bad_pusher"));
}

TEST_F(AddonEntityDefinitionTest, DataOnlyPackInstallsPropertiesDimensionsAndPhysics)
{
    const auto directory = pack("data", _PACK, std::nullopt);
    write(directory / "entities/nested/actor.json", _definition("addon_test:actor"));
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
    EXPECT_EQ(manager.pluginManager().pluginCount(), 0u);
    auto actor = create("addon_test:actor");
    ASSERT_NE(actor, nullptr);
    EXPECT_EQ(dynamic_cast<mc::LivingEntity*>(actor.get()), nullptr);
    EXPECT_FLOAT_EQ(actor->width(), 0.8f);
    EXPECT_FLOAT_EQ(actor->height(), 1.4f);
    EXPECT_TRUE(actor->hasNoGravity());
    EXPECT_FALSE(actor->noClip());
    EXPECT_TRUE(actor->isImmuneToFire());
    const auto* state = actor->tryGetComponent<EntityAddonState>();
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->getProperty("test:count"), nlohmann::json(3));
    EXPECT_EQ(actor->entityType()->name(), "addon_test:actor");
}

// description.is_spawnable 此前只校验类型就丢弃，运行时读不到。现在必须如实保存：
// 缺省为 false，显式 true 时为 true，且与 is_summonable 相互独立。
// minecraft:is_baby / minecraft:can_climb 是纯标记组件（无字段），必须被接受而不是当作未知组件
// 拒绝——否则用了它们的整个行为包都加载不了。is_baby 对 living 型实体会真正调 setChild。
TEST_F(AddonEntityDefinitionTest, MarkerComponentsAreAcceptedAndIsBabyTakesEffect)
{
    auto definition = _definition("addon_test:markers");
    definition["minecraft:entity"]["components"]["minecraft:is_baby"] = nlohmann::json::object();
    definition["minecraft:entity"]["components"]["minecraft:can_climb"] = nlohmann::json::object();
    definition["minecraft:entity"]["components"]["minecraft:movement"] = {{"value", 0.3}};
    // living 判定要求 movement + health 同时存在（仅有 movement 会被判为"无生命的移动 actor"而拒绝）。
    definition["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 20}, {"max", 20}};
    auto parsed = AddonEntityDefinition::parse(definition);
    ASSERT_TRUE(parsed.success()) << parsed.error().message();

    // 带 minecraft:movement 的 definition 是 living 型，生成后应当是幼体。
    ASSERT_TRUE(parsed.value().isLiving());
    auto shared = std::make_shared<const AddonEntityDefinition>(parsed.value());
    auto entity = AddonEntityDefinition::create(shared, nullptr, entities, {});
    ASSERT_NE(entity, nullptr);
    const auto* ageable = dynamic_cast<mc::AgeableEntity*>(entity.get());
    ASSERT_NE(ageable, nullptr) << "living addon mobs must be AgeableEntity so is_baby can apply";
    EXPECT_TRUE(ageable->isChild());
}

// is_baby 写在非 living 实体上不报错（只是无效果），基岩侧也不会因此拒绝行为包。
// minecraft:knockback_resistance.value 必须落到击退抗性属性上，且只接受字面量数字：
// range 对象 / Molang 当前没有"每 tick 求值"的通路，放任通过会导致组件被收下却不生效。
// minecraft:cannot_be_attacked 是纯标记组件，解析后实体必须是无敌的（否则组件等于被忽略）。
// minecraft:follow_range.value 必须落到追击范围属性上（实体寻路/目标搜索读的就是它）。
TEST_F(AddonEntityDefinitionTest, FollowRangeIsAppliedToAttribute)
{
    auto definition = _definition("addon_test:follow_range");
    definition["minecraft:entity"]["components"]["minecraft:movement"] = {{"value", 0.3}};
    definition["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 20}, {"max", 20}};
    definition["minecraft:entity"]["components"]["minecraft:follow_range"] = {{"value", 48}, {"max", 64}};
    auto parsed = AddonEntityDefinition::parse(definition);
    ASSERT_TRUE(parsed.success()) << parsed.error().message();

    auto shared = std::make_shared<const AddonEntityDefinition>(parsed.value());
    auto entity = AddonEntityDefinition::create(shared, nullptr, entities, {});
    ASSERT_NE(entity, nullptr);
    const auto* living = dynamic_cast<mc::LivingEntity*>(entity.get());
    ASSERT_NE(living, nullptr);
    EXPECT_DOUBLE_EQ(living->attributes().getValue(mc::entity::attribute::Attributes::FOLLOW_RANGE), 48.0);

    // 动态形式必须被明确拒绝，与 knockback_resistance 保持同一策略。
    auto dynamicValue = _definition("addon_test:follow_range_dynamic");
    dynamicValue["minecraft:entity"]["components"]["minecraft:follow_range"] = {{"value", "math.random(16, 48)"}};
    const auto rejected = AddonEntityDefinition::parse(dynamicValue);
    ASSERT_TRUE(rejected.failed());
    EXPECT_EQ(rejected.error().code(), mc::ErrorCode::Unsupported);
}

TEST_F(AddonEntityDefinitionTest, CannotBeAttackedMakesEntityInvulnerable)
{
    auto definition = _definition("addon_test:invulnerable");
    definition["minecraft:entity"]["components"]["minecraft:cannot_be_attacked"] = nlohmann::json::object();
    auto parsed = AddonEntityDefinition::parse(definition);
    ASSERT_TRUE(parsed.success()) << parsed.error().message();

    auto shared = std::make_shared<const AddonEntityDefinition>(parsed.value());
    auto entity = AddonEntityDefinition::create(shared, nullptr, entities, {});
    ASSERT_NE(entity, nullptr);
    EXPECT_TRUE(entity->isInvulnerable()) << "cannot_be_attacked must make the entity invulnerable";

    // 没写该组件的实体必须保持可被攻击，避免"一律无敌"的过度实现。
    auto plain = AddonEntityDefinition::parse(_definition("addon_test:attackable"));
    ASSERT_TRUE(plain.success()) << plain.error().message();
    auto plainShared = std::make_shared<const AddonEntityDefinition>(plain.value());
    auto plainEntity = AddonEntityDefinition::create(plainShared, nullptr, entities, {});
    ASSERT_NE(plainEntity, nullptr);
    EXPECT_FALSE(plainEntity->isInvulnerable());
}

TEST_F(AddonEntityDefinitionTest, KnockbackResistanceIsAppliedAndRequiresLiteralNumber)
{
    auto definition = _definition("addon_test:knockback");
    definition["minecraft:entity"]["components"]["minecraft:knockback_resistance"] = {{"value", 0.75}};
    // 击退抗性挂在 LivingEntity 上，因此需要 movement + health 让定义被判为 living 型。
    definition["minecraft:entity"]["components"]["minecraft:movement"] = {{"value", 0.3}};
    definition["minecraft:entity"]["components"]["minecraft:health"] = {{"value", 20}, {"max", 20}};
    auto parsed = AddonEntityDefinition::parse(definition);
    ASSERT_TRUE(parsed.success()) << parsed.error().message();

    auto shared = std::make_shared<const AddonEntityDefinition>(parsed.value());
    auto entity = AddonEntityDefinition::create(shared, nullptr, entities, {});
    ASSERT_NE(entity, nullptr);
    const auto* living = dynamic_cast<mc::LivingEntity*>(entity.get());
    ASSERT_NE(living, nullptr);
    EXPECT_DOUBLE_EQ(living->attributes().getValue(mc::entity::attribute::Attributes::KNOCKBACK_RESISTANCE), 0.75);

    // 动态形式必须被明确拒绝，而不是静默收下。
    auto dynamicValue = _definition("addon_test:knockback_dynamic");
    dynamicValue["minecraft:entity"]["components"]["minecraft:knockback_resistance"] = {
        {"value", {{"range_min", 0.1}, {"range_max", 0.9}}}};
    const auto rejected = AddonEntityDefinition::parse(dynamicValue);
    ASSERT_TRUE(rejected.failed());
    EXPECT_EQ(rejected.error().code(), mc::ErrorCode::Unsupported);
}

TEST_F(AddonEntityDefinitionTest, IsBabyOnNonLivingIsAcceptedWithoutEffect)
{
    auto definition = _definition("addon_test:baby_actor");
    definition["minecraft:entity"]["components"]["minecraft:is_baby"] = nlohmann::json::object();
    auto parsed = AddonEntityDefinition::parse(definition);
    ASSERT_TRUE(parsed.success()) << parsed.error().message();
    ASSERT_FALSE(parsed.value().isLiving());
    auto shared = std::make_shared<const AddonEntityDefinition>(parsed.value());
    EXPECT_NE(AddonEntityDefinition::create(shared, nullptr, entities, {}), nullptr);
}

TEST_F(AddonEntityDefinitionTest, SpawnableAndSummonableFlagsAreParsedIndependently)
{
    auto neither = AddonEntityDefinition::parse(_definition("addon_test:neither"));
    ASSERT_TRUE(neither.success()) << neither.error().message();
    EXPECT_FALSE(neither.value().canSpawn());
    EXPECT_TRUE(neither.value().canSummon()) << "fixture sets is_summonable=true";

    auto spawnOnly = _definition("addon_test:spawn_only");
    spawnOnly["minecraft:entity"]["description"]["is_spawnable"] = true;
    spawnOnly["minecraft:entity"]["description"]["is_summonable"] = false;
    auto parsed = AddonEntityDefinition::parse(spawnOnly);
    ASSERT_TRUE(parsed.success()) << parsed.error().message();
    EXPECT_TRUE(parsed.value().canSpawn());
    EXPECT_FALSE(parsed.value().canSummon());
}

TEST_F(AddonEntityDefinitionTest, LivingComponentsInitializeHealthAndMovement)
{
    const auto directory = pack("living", _PACK, std::nullopt);
    auto definition = _definition("addon_test:living");
    definition["minecraft:entity"]["components"]["minecraft:health"] = {{"max", 64}, {"value", 40}};
    definition["minecraft:entity"]["components"]["minecraft:movement"] = {{"value", 0.35}};
    write(directory / "entities/living.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:living");
    auto* living = dynamic_cast<mc::LivingEntity*>(entity.get());
    ASSERT_NE(living, nullptr);
    EXPECT_FLOAT_EQ(living->maxHealth(), 64);
    EXPECT_FLOAT_EQ(living->health(), 40);
    EXPECT_DOUBLE_EQ(living->attributes().getValue(mc::entity::attribute::Attributes::MOVEMENT_SPEED, -1), 0.35);
}

TEST_F(AddonEntityDefinitionTest, EntityDeserializerInstallsSchemaBeforeRestoringProperties)
{
    mc::entity::serialization::components::ComponentSerializerRegistry::instance().registerAll();
    const auto directory = pack("saved", _PACK, std::nullopt);
    write(directory / "entities/saved.json", _definition("addon_test:saved"));
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto original = create("addon_test:saved");
    ASSERT_NE(original, nullptr);
    auto& state = original->getOrAddComponent<EntityAddonState>();
    ASSERT_TRUE(state.setProperty("test:count", 42).success());
    ASSERT_TRUE(state.dynamicProperties().set(_PACK, "flag", true).success());
    const auto expected = state.save();
    mc::nbt::tags::compound_tag snapshot;
    snapshot.put("id", original->getTypeId());
    original->writeToNBT(snapshot);
    auto direct = mc::entity::serialization::EntityDeserializer::deserialize(snapshot, entities);
    ASSERT_TRUE(direct.success()) << direct.error().message();
    auto directEntity = direct.value();
    ASSERT_NE(directEntity, nullptr);
    ASSERT_EQ(directEntity->getOrAddComponent<EntityAddonState>().save(), expected);
    auto saved = mc::entity::serialization::EntityDeserializer::serializeToBinary(*original);
    ASSERT_TRUE(saved.success()) << saved.error().message();
    auto restored = mc::entity::serialization::EntityDeserializer::deserializeFromBinary(saved.value(), entities);
    ASSERT_TRUE(restored.success()) << restored.error().message();
    // Result<unique_ptr<T>>::value() 会转移所有权，必须先持有实体再借用组件。
    auto restoredEntity = restored.value();
    ASSERT_NE(restoredEntity, nullptr);
    const auto* restoredState = restoredEntity->tryGetComponent<EntityAddonState>();
    ASSERT_NE(restoredState, nullptr);
    EXPECT_EQ(restoredEntity->getTypeId(), "addon_test:saved");
    EXPECT_EQ(restoredState->save(), expected);
    EXPECT_EQ(restoredState->getProperty("test:count"), nlohmann::json(42));
    EXPECT_EQ(restoredState->dynamicProperties().get(_PACK, "flag"), nlohmann::json(true));
}

TEST_F(AddonEntityDefinitionTest, InvalidDefinitionDoesNotRegisterEarlierValidFiles)
{
    const auto directory = pack("invalid", _PACK, std::nullopt);
    write(directory / "entities/a-valid.json", _definition("addon_test:uncommitted"));
    auto invalid = _definition("addon_test:broken");
    invalid["minecraft:entity"]["components"]["minecraft:physics"]["has_gravity"] = "false";
    write(directory / "entities/z-invalid.json", invalid);
    EXPECT_TRUE(manager.loadPacks("", root.string()).failed());
    EXPECT_FALSE(mc::entity::EntityRegistry::instance().hasType("addon_test:uncommitted"));
    EXPECT_FALSE(mc::entity::EntityRegistry::instance().hasType("addon_test:broken"));
}

TEST_F(AddonEntityDefinitionTest, ScriptFailureRollsBackOnlyNewContent)
{
    const auto first = pack("existing", _PACK, std::nullopt);
    write(first / "entities/existing.json", _definition("addon_test:existing"));
    ASSERT_TRUE(manager.packList()->addPack(first.string(), true, 0).success());
    ASSERT_TRUE(manager.loadPacks("", "").success());
    const auto* existing = mc::entity::EntityRegistry::instance().getType("addon_test:existing");
    const auto broken = pack("broken", _OTHER, std::string("invalid javascript ???"));
    write(broken / "entities/new.json", _definition("addon_test:rollback"));
    ASSERT_TRUE(manager.packList()->addPack(broken.string(), true, 0).success());
    EXPECT_TRUE(manager.loadPacks("", "").failed());
    EXPECT_FALSE(mc::entity::EntityRegistry::instance().hasType("addon_test:rollback"));
    EXPECT_EQ(mc::entity::EntityRegistry::instance().getType("addon_test:existing"), existing);
}

TEST_F(AddonEntityDefinitionTest, ReloadRetiresLookupButKeepsLiveEntityDefinition)
{
    const auto directory = pack("reload", _PACK, std::nullopt);
    auto definition = _definition("addon_test:reload");
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto oldEntity = create("addon_test:reload");
    ASSERT_NE(oldEntity, nullptr);
    const auto* oldType = oldEntity->entityType();
    const auto oldId = oldType->id();
    manager.reload();
    EXPECT_EQ(mc::entity::EntityRegistry::instance().getType("addon_test:reload"), nullptr);
    EXPECT_EQ(mc::entity::EntityRegistry::instance().getTypeById(oldId), nullptr);
    EXPECT_EQ(oldType->name(), "addon_test:reload");
    EXPECT_FLOAT_EQ(oldEntity->width(), 0.8f);
    definition["minecraft:entity"]["components"]["minecraft:collision_box"]["width"] = 2;
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto current = create("addon_test:reload");
    ASSERT_NE(current, nullptr);
    EXPECT_FLOAT_EQ(current->width(), 2);
    EXPECT_FLOAT_EQ(oldEntity->width(), 0.8f);
    EXPECT_NE(current->entityType(), oldType);
    EXPECT_FALSE(mc::entity::EntityRegistry::instance().unregisterType("addon_test:reload", oldType));
    const auto registered = mc::entity::EntityRegistry::instance().getAllTypes();
    EXPECT_EQ(std::find(registered.begin(), registered.end(), oldType), registered.end());
}

TEST_F(AddonEntityDefinitionTest, HigherPriorityPackOverridesTheSameIdentifier)
{
    const auto low = pack("low", _PACK, std::nullopt);
    const auto high = pack("high", _OTHER, std::nullopt);
    auto lowDefinition = _definition("addon_test:priority");
    auto highDefinition = lowDefinition;
    highDefinition["minecraft:entity"]["description"]["properties"]["test:count"]["default"] = 9;
    write(low / "entities/entity.json", lowDefinition);
    write(high / "entities/entity.json", highDefinition);
    ASSERT_TRUE(manager.packList()->addPack(low.string(), true, 0).success());
    ASSERT_TRUE(manager.packList()->addPack(high.string(), true, 100).success());
    ASSERT_TRUE(manager.loadPacks("", "").success());
    auto entity = create("addon_test:priority");
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(9));
}

TEST_F(AddonEntityDefinitionTest, DuplicateDefinitionsWithinOnePackAreRejected)
{
    const auto directory = pack("duplicate", _PACK, std::nullopt);
    write(directory / "entities/a.json", _definition("addon_test:duplicate"));
    write(directory / "entities/b.json", _definition("addon_test:duplicate"));
    EXPECT_TRUE(manager.loadPacks("", root.string()).failed());
    EXPECT_FALSE(mc::entity::EntityRegistry::instance().hasType("addon_test:duplicate"));
}

TEST_F(AddonEntityDefinitionTest, UnsupportedComponentsAndEventSectionsAreExplicitFailures)
{
    auto definition = _definition("addon_test:unsupported");
    // 用一个确实尚未支持的组件作样本：inventory / leashable / breathable / healable / rideable / burns_in_daylight
    // 都已接入。
    definition["minecraft:entity"]["components"]["minecraft:damage_sensor"] = nlohmann::json::array();
    auto result = AddonEntityDefinition::parse(definition);
    ASSERT_TRUE(result.failed());
    EXPECT_EQ(result.error().code(), mc::ErrorCode::Unsupported);
    definition = _definition("addon_test:unsupported");
    definition["minecraft:entity"]["events"]["test:change"] = {
        {"set_property", {{"test:count", "variable.counter + 1"}}}};
    result = AddonEntityDefinition::parse(definition);
    ASSERT_TRUE(result.failed());
    EXPECT_EQ(result.error().code(), mc::ErrorCode::Unsupported);
}

// 组件数值的三种合法写法都要接受：字面数字、范围对象（range_min/range_max）、Molang 表达式。
// 基岩 minecraft:health 等组件的 value/max 均支持后两种形式，缺了它们合法行为包会被挡在门外。
TEST_F(AddonEntityDefinitionTest, RangeObjectAndMolangComponentValuesAreAccepted)
{
    auto definition = _definition("addon_test:value_forms");
    definition["minecraft:entity"]["components"]["minecraft:health"] = {
        {"max", {{"range_min", 40}, {"range_max", 60}}}, {"value", {{"range_max", 30}}}};
    definition["minecraft:entity"]["components"]["minecraft:movement"] = {{"value", "math.random(0.1, 0.3)"}};
    EXPECT_TRUE(AddonEntityDefinition::parse(definition).success());
}

// 范围对象本身写错时必须报 InvalidData，而不是静默接受：
// 未知字段、非数字边界、缺两个边界、range_min > range_max 都要拒绝。
TEST_F(AddonEntityDefinitionTest, MalformedRangeObjectsAreRejected)
{
    std::vector<nlohmann::json> invalid;
    auto base = _definition("addon_test:bad_range");

    auto broken = base;
    broken["minecraft:entity"]["components"]["minecraft:health"] = {
        {"value", {{"range_min", 10}, {"range_max", 20}, {"x", 1}}}};
    invalid.push_back(broken);

    broken = base;
    broken["minecraft:entity"]["components"]["minecraft:health"] = {{"value", {{"range_min", "ten"}}}};
    invalid.push_back(broken);

    broken = base;
    broken["minecraft:entity"]["components"]["minecraft:health"] = {{"value", nlohmann::json::object()}};
    invalid.push_back(broken);

    broken = base;
    broken["minecraft:entity"]["components"]["minecraft:health"] = {{"value", {{"range_min", 20}, {"range_max", 10}}}};
    invalid.push_back(broken);

    for (const auto& document : invalid) {
        const auto result = AddonEntityDefinition::parse(document);
        ASSERT_TRUE(result.failed());
        EXPECT_EQ(result.error().code(), mc::ErrorCode::InvalidData) << result.error().message();
    }
}

TEST_F(AddonEntityDefinitionTest, MalformedSchemasAndComponentValuesAreRejected)
{
    auto definition = _definition("addon_test:invalid");
    std::vector<nlohmann::json> invalid;
    invalid.push_back(nlohmann::json::array());
    auto broken = definition;
    broken["minecraft:entity"]["description"]["identifier"] = "invalid identifier";
    invalid.push_back(broken);
    broken = definition;
    broken["minecraft:entity"]["description"]["properties"]["test:count"]["default"] = 101;
    invalid.push_back(broken);
    broken = definition;
    broken["minecraft:entity"]["components"]["minecraft:collision_box"]["height"] =
        std::numeric_limits<double>::infinity();
    invalid.push_back(broken);
    broken = definition;
    broken["minecraft:entity"]["components"] = nlohmann::json::array();
    invalid.push_back(broken);
    broken = definition;
    broken["minecraft:entity"]["events"] = nlohmann::json::array();
    invalid.push_back(broken);
    for (const auto& document : invalid) {
        EXPECT_TRUE(AddonEntityDefinition::parse(document).failed());
    }
}

TEST_F(AddonEntityDefinitionTest, ResourceEnumerationIsSortedAndRejectsParentTraversal)
{
    const auto directory = pack("paths", _PACK, std::nullopt);
    write(directory / "entities/z.json", _definition("addon_test:path_z"));
    write(directory / "entities/nested/a.json", _definition("addon_test:path_a"));
    ASSERT_TRUE(manager.packList()->addPack(directory.string(), true, 0).success());
    auto* behavior = manager.packList()->getPackByUuid(_PACK);
    ASSERT_NE(behavior, nullptr);
    auto files = behavior->listResources("entities", ".json");
    ASSERT_TRUE(files.success()) << files.error().message();
    EXPECT_EQ(files.value(), (std::vector<std::string>{"entities/nested/a.json", "entities/z.json"}));
    EXPECT_TRUE(behavior->listResources("../", ".json").failed());
    EXPECT_TRUE(behavior->listResources(directory.string(), ".json").failed());
    auto missing = behavior->listResources("missing", ".json");
    ASSERT_TRUE(missing.success());
    EXPECT_TRUE(missing.value().empty());
}

TEST_F(AddonEntityDefinitionTest, PropertyEventsExecuteSequenceAndSelfTriggersAtTickBoundary)
{
    const auto directory = pack("events", _PACK, std::nullopt);
    auto definition = _definition("addon_test:events");
    definition["minecraft:entity"]["events"] = {{"test:inner", {{"set_property", {{"test:count", 7}}}}},
        {"test:outer",
            {{"sequence",
                {{{"set_property", {{"test:count", 5}}}},
                    {{"trigger", {{"event", "test:inner"}, {"target", "self"}}}},
                    {{"set_property", {{"test:count", 11}}}}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:events");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    ASSERT_NE(events, nullptr);
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    ASSERT_TRUE(events->trigger(*entity, "test:outer").success());
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(11));
}

TEST_F(AddonEntityDefinitionTest, EventNumericValuesClampWhileScriptWritesRejectOutOfRange)
{
    const auto directory = pack("clamp", _PACK, std::nullopt);
    auto definition = _definition("addon_test:clamp");
    definition["minecraft:entity"]["events"] = {{"test:high", {{"set_property", {{"test:count", 1000}}}}},
        {"test:low", {{"set_property", {{"test:count", -1000}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:clamp");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    EXPECT_TRUE(state.setProperty("test:count", 1000).failed());
    ASSERT_TRUE(events->trigger(*entity, "test:high").success());
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(100));
    ASSERT_TRUE(events->trigger(*entity, "test:low").success());
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(0));
}

TEST_F(AddonEntityDefinitionTest, WeightedRandomizeExecutesOnePositiveWeightBranch)
{
    const auto directory = pack("random", _PACK, std::nullopt);
    auto definition = _definition("addon_test:random");
    definition["minecraft:entity"]["events"]["test:random"] = {{"randomize",
        {{{"weight", 0}, {"set_property", {{"test:count", 99}}}},
            {{"weight", 1}, {"set_property", {{"test:count", 4}}}},
            {{"weight", 2}, {"set_property", {{"test:count", 8}}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:random");
    ASSERT_NE(entity, nullptr);
    entity->getRandom().setSeed(123);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    int first = 0;
    int second = 0;
    for (int index = 0; index < 100; ++index) {
        ASSERT_TRUE(events->trigger(*entity, "test:random").success());
        entity->tick();
        const auto value = state.getProperty("test:count");
        EXPECT_TRUE(value == nlohmann::json(4) || value == nlohmann::json(8));
        first += value == nlohmann::json(4);
        second += value == nlohmann::json(8);
    }
    EXPECT_GT(first, 0);
    EXPECT_GT(second, 0);
}

TEST_F(AddonEntityDefinitionTest, RecursiveFailureDoesNotCommitAnyPendingWritesOrNotifications)
{
    const auto directory = pack("recursive", _PACK, std::nullopt);
    auto definition = _definition("addon_test:recursive");
    definition["minecraft:entity"]["events"]["test:loop"] = {
        {"sequence", {{{"set_property", {{"test:count", 90}}}}, {{"trigger", "test:loop"}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:recursive");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    const auto before = state.save();
    EXPECT_TRUE(events->trigger(*entity, "test:loop").failed());
    EXPECT_TRUE(events->trigger(*entity, "test:missing").failed());
    EXPECT_EQ(state.save(), before);
    EXPECT_FALSE(manager.eventBus().hasPendingAfterEvents());
}

TEST_F(AddonEntityDefinitionTest, MalformedEventActionsAndReferencesFailBeforeRegistration)
{
    const auto properties = _definition("addon_test:validation")["minecraft:entity"]["description"]["properties"];
    const std::vector<nlohmann::json> invalid = {{{"test:event", {{"trigger", "test:missing"}}}},
        {{"test:event", {{"sequence", nlohmann::json::object()}}}},
        {{"test:event", {{"randomize", nlohmann::json::array()}}}},
        {{"test:event", {{"randomize", {{{"weight", -1}}}}}}},
        {{"test:event", {{"randomize", {{{"weight", 0}}}}}}},
        {{"test:event", {{"set_property", {{"test:missing", true}}}}}},
        {{"test:event", {{"set_property", {{"test:count", true}}}}}},
        {{"test:event", {{"trigger", {{"event", "test:event"}, {"target", "other"}}}}}}};
    for (const auto& events : invalid) {
        EXPECT_TRUE(EntityEventProgram::compile(events, properties, nlohmann::json::object()).failed())
            << events.dump();
    }
}

TEST_F(AddonEntityDefinitionTest, SpawnEventsAreOnceOnlyAndAreNotReplayedByRestoreFactories)
{
    const auto directory = pack("spawn-events", _PACK, std::nullopt);
    auto definition = _definition("addon_test:spawn_events");
    definition["minecraft:entity"]["events"]["minecraft:entity_spawned"] = {{"set_property", {{"test:count", 8}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    // nullptr 世界是统一反序列化工厂入口，不能重新触发 entity_spawned。
    auto entity = create("addon_test:spawn_events");
    ASSERT_NE(entity, nullptr);
    entity->tick();
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
    auto compiled = EntityEventProgram::compile(definition["minecraft:entity"]["events"],
        definition["minecraft:entity"]["description"]["properties"],
        nlohmann::json::object());
    ASSERT_TRUE(compiled.success());
    auto program = std::make_shared<const EntityEventProgram>(std::move(compiled.value()));
    AddonEntityEvents spawned(program, {}, true);
    spawned.onTick(*entity);
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(8));
    ASSERT_TRUE(state.setProperty("test:count", 2).success());
    entity->tick();
    spawned.onTick(*entity);
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(2));
}

TEST_F(AddonEntityDefinitionTest, ScriptTriggerEventPublishesFilteredDeferredSnapshots)
{
    const auto directory = pack("script-events", _PACK, std::string(R"(
        import { world } from '@minecraft/server';
        globalThis.events = [];
        world.afterEvents.dataDrivenEntityTrigger.subscribe(event => {
            events.push({name: event.eventId, type: event.entity.typeId, modifiers: event.getModifiers().length});
        }, {eventTypes: ['test:go'], entityTypes: ['addon_test:script_events']});
    )"));
    auto definition = _definition("addon_test:script_events");
    definition["minecraft:entity"]["events"] = {
        {"test:go", {{"set_property", {{"test:count", 9}}}}}, {"test:ignored", nlohmann::json::object()}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:script_events");
    ASSERT_NE(entity, nullptr);
    auto* plugin = manager.pluginManager().getPlugin(_PACK);
    ASSERT_NE(plugin, nullptr);
    auto* context = plugin->context();
    ASSERT_NE(context, nullptr);
    auto& bindings = context->bindingContext();
    void* global = bindings.getGlobalObject();
    void* subject = wrapEntity(bindings, entity.get());
    bindings.setProperty(global, "subject", subject);
    bindings.releaseValue(subject);
    bindings.releaseValue(global);
    auto triggered = context->evaluate("subject.triggerEvent('test:go'); subject.triggerEvent('test:ignored'); if "
                                       "(events.length) throw Error('event was synchronous');",
        "trigger.js",
        EvalFlags::None);
    ASSERT_TRUE(triggered.success()) << triggered.errorMessage();
    manager.eventBus().tick();
    auto checked = context->evaluate("if (events.length !== 1 || events[0].name !== 'test:go' || events[0].modifiers "
                                     "!== 0) throw Error('wrong filtered event');",
        "check.js",
        EvalFlags::None);
    ASSERT_TRUE(checked.success()) << checked.errorMessage();
    entity->tick();
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(9));
}

TEST_F(AddonEntityDefinitionTest, EventsOutliveScriptShutdownWithoutKeepingTheBusAlive)
{
    const auto directory = pack("shutdown-events", _PACK, std::nullopt);
    auto definition = _definition("addon_test:shutdown_events");
    definition["minecraft:entity"]["events"]["test:change"] = {{"set_property", {{"test:count", 6}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:shutdown_events");
    ASSERT_NE(entity, nullptr);
    manager.shutdown();
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    ASSERT_NE(events, nullptr);
    ASSERT_TRUE(events->trigger(*entity, "test:change").success());
    entity->tick();
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(6));
}

TEST_F(AddonEntityDefinitionTest, ComponentGroupsChangeDimensionsAtTickBoundaryAndRemoveWithoutFallback)
{
    const auto directory = pack("groups", _PACK, std::nullopt);
    auto definition = _definition("addon_test:groups");
    definition["minecraft:entity"]["component_groups"]["large"] = {
        {"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    definition["minecraft:entity"]["events"] = {{"test:add", {{"add", {{"component_groups", {"large"}}}}}},
        {"test:remove", {{"remove", {{"component_groups", {"large"}}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:groups");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    ASSERT_TRUE(events->trigger(*entity, "test:add").success());
    EXPECT_FLOAT_EQ(entity->width(), 0.8f);
    entity->tick();
    EXPECT_FLOAT_EQ(entity->width(), 2);
    EXPECT_FLOAT_EQ(entity->height(), 3);
    ASSERT_TRUE(events->trigger(*entity, "test:remove").success());
    entity->tick();
    EXPECT_FLOAT_EQ(entity->width(), 0);
    EXPECT_FLOAT_EQ(entity->height(), 0);
}

TEST_F(AddonEntityDefinitionTest, OverlappingGroupRemovalDoesNotReactivateAnotherActiveGroup)
{
    const auto directory = pack("overlapping", _PACK, std::nullopt);
    auto definition = _definition("addon_test:overlapping");
    auto& groups = definition["minecraft:entity"]["component_groups"];
    groups["first"] = {{"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    groups["second"] = {{"minecraft:collision_box", {{"width", 4}, {"height", 5}}}};
    definition["minecraft:entity"]["events"] = {{"test:first", {{"add", {{"component_groups", {"first"}}}}}},
        {"test:second", {{"add", {{"component_groups", {"second"}}}}}},
        {"test:remove_first", {{"remove", {{"component_groups", {"first"}}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:overlapping");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    ASSERT_TRUE(events->trigger(*entity, "test:first").success());
    ASSERT_TRUE(events->trigger(*entity, "test:second").success());
    entity->tick();
    EXPECT_FLOAT_EQ(entity->width(), 4);
    ASSERT_TRUE(events->trigger(*entity, "test:remove_first").success());
    entity->tick();
    EXPECT_FLOAT_EQ(entity->width(), 0);
    const auto state = entity->tryGetComponent<AddonEntityComponents>()->save();
    EXPECT_EQ(state["active"], nlohmann::json::array({"second"}));
    EXPECT_FALSE(state["sources"].contains("minecraft:collision_box"));
    ASSERT_TRUE(events->trigger(*entity, "test:second").success());
    entity->tick();
    EXPECT_FLOAT_EQ(entity->width(), 4);
}

TEST_F(AddonEntityDefinitionTest, ReaddingAnActiveHealthGroupReinitializesItsValues)
{
    const auto directory = pack("health-group", _PACK, std::nullopt);
    auto definition = _definition("addon_test:health_group");
    definition["minecraft:entity"]["components"]["minecraft:health"] = {{"max", 32}, {"value", 24}};
    definition["minecraft:entity"]["component_groups"]["strong"] = {
        {"minecraft:health", {{"max", 128}, {"value", 96}}}};
    definition["minecraft:entity"]["events"]["test:strong"] = {{"add", {{"component_groups", {"strong"}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:health_group");
    auto* living = dynamic_cast<mc::LivingEntity*>(entity.get());
    ASSERT_NE(living, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    ASSERT_TRUE(events->trigger(*entity, "test:strong").success());
    components->applyPending(*entity);
    EXPECT_FLOAT_EQ(living->maxHealth(), 128);
    EXPECT_FLOAT_EQ(living->health(), 96);
    living->setHealth(50);
    ASSERT_TRUE(events->trigger(*entity, "test:strong").success());
    components->applyPending(*entity);
    EXPECT_FLOAT_EQ(living->health(), 96);
}

TEST_F(AddonEntityDefinitionTest, GroupPhysicsAndFireImmunityRemainPerInstance)
{
    const auto directory = pack("physics-group", _PACK, std::nullopt);
    auto definition = _definition("addon_test:physics_group");
    definition["minecraft:entity"]["components"].erase("minecraft:fire_immune");
    definition["minecraft:entity"]["component_groups"]["ghost"] = {
        {"minecraft:physics", {{"has_gravity", false}, {"has_collision", false}}},
        {"minecraft:fire_immune", nlohmann::json::object()}};
    definition["minecraft:entity"]["events"] = {{"test:ghost", {{"add", {{"component_groups", {"ghost"}}}}}},
        {"test:normal", {{"remove", {{"component_groups", {"ghost"}}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto first = create("addon_test:physics_group");
    auto second = create("addon_test:physics_group");
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    ASSERT_TRUE(first->tryGetComponent<AddonEntityEvents>()->trigger(*first, "test:ghost").success());
    first->tick();
    EXPECT_TRUE(first->noClip());
    EXPECT_TRUE(first->isImmuneToFire());
    EXPECT_FALSE(second->noClip());
    EXPECT_FALSE(second->isImmuneToFire());
    ASSERT_TRUE(first->tryGetComponent<AddonEntityEvents>()->trigger(*first, "test:normal").success());
    first->tick();
    EXPECT_FALSE(first->isImmuneToFire());
}

TEST_F(AddonEntityDefinitionTest, SavedGroupsRestoreActualSourcesWithoutRecombiningDefinitions)
{
    mc::entity::serialization::components::ComponentSerializerRegistry::instance().registerAll();
    const auto directory = pack("save-groups", _PACK, std::nullopt);
    auto definition = _definition("addon_test:save_groups");
    definition["minecraft:entity"]["component_groups"]["large"] = {
        {"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    definition["minecraft:entity"]["events"] = {{"test:add", {{"add", {{"component_groups", {"large"}}}}}},
        {"test:remove", {{"remove", {{"component_groups", {"large"}}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:save_groups");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    ASSERT_TRUE(events->trigger(*entity, "test:add").success());
    entity->tick();
    ASSERT_TRUE(events->trigger(*entity, "test:remove").success());
    entity->tick();
    auto saved = mc::entity::serialization::EntityDeserializer::serializeToBinary(*entity);
    ASSERT_TRUE(saved.success()) << saved.error().message();
    auto loaded = mc::entity::serialization::EntityDeserializer::deserializeFromBinary(saved.value(), entities);
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
    auto restored = loaded.value();
    ASSERT_NE(restored, nullptr);
    EXPECT_FLOAT_EQ(restored->width(), 0);
    EXPECT_EQ(restored->tryGetComponent<AddonEntityComponents>()->save(),
        entity->tryGetComponent<AddonEntityComponents>()->save());
}

TEST_F(AddonEntityDefinitionTest, SavedPendingGroupChangesApplyAfterRestoration)
{
    mc::entity::serialization::components::ComponentSerializerRegistry::instance().registerAll();
    const auto directory = pack("pending-groups", _PACK, std::nullopt);
    auto definition = _definition("addon_test:pending_groups");
    definition["minecraft:entity"]["component_groups"]["large"] = {
        {"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    definition["minecraft:entity"]["events"]["test:add"] = {{"add", {{"component_groups", {"large"}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:pending_groups");
    ASSERT_NE(entity, nullptr);
    ASSERT_TRUE(entity->tryGetComponent<AddonEntityEvents>()->trigger(*entity, "test:add").success());
    auto saved = mc::entity::serialization::EntityDeserializer::serializeToBinary(*entity);
    ASSERT_TRUE(saved.success());
    auto loaded = mc::entity::serialization::EntityDeserializer::deserializeFromBinary(saved.value(), entities);
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
    auto restored = loaded.value();
    ASSERT_NE(restored, nullptr);
    EXPECT_FLOAT_EQ(restored->width(), 0.8f);
    restored->tick();
    EXPECT_FLOAT_EQ(restored->width(), 2);
}

TEST_F(AddonEntityDefinitionTest, GroupMaximumHealthRestoresBeforeSavedCurrentHealth)
{
    mc::entity::serialization::components::ComponentSerializerRegistry::instance().registerAll();
    const auto directory = pack("saved-health-group", _PACK, std::nullopt);
    auto definition = _definition("addon_test:saved_health_group");
    definition["minecraft:entity"]["components"]["minecraft:health"] = {{"max", 32}, {"value", 24}};
    definition["minecraft:entity"]["component_groups"]["strong"] = {
        {"minecraft:health", {{"max", 128}, {"value", 96}}}};
    definition["minecraft:entity"]["events"]["test:add"] = {{"add", {{"component_groups", {"strong"}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:saved_health_group");
    auto* living = dynamic_cast<mc::LivingEntity*>(entity.get());
    ASSERT_NE(living, nullptr);
    ASSERT_TRUE(entity->tryGetComponent<AddonEntityEvents>()->trigger(*entity, "test:add").success());
    entity->tryGetComponent<AddonEntityComponents>()->applyPending(*entity);
    living->setHealth(70);
    auto saved = mc::entity::serialization::EntityDeserializer::serializeToBinary(*entity);
    ASSERT_TRUE(saved.success());
    auto loaded = mc::entity::serialization::EntityDeserializer::deserializeFromBinary(saved.value(), entities);
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
    auto restored = loaded.value();
    auto* restoredLiving = dynamic_cast<mc::LivingEntity*>(restored.get());
    ASSERT_NE(restoredLiving, nullptr);
    EXPECT_FLOAT_EQ(restoredLiving->maxHealth(), 128);
    EXPECT_FLOAT_EQ(restoredLiving->health(), 70);
}

TEST_F(AddonEntityDefinitionTest, UnknownGroupReferencesAndCorruptSnapshotsPreserveState)
{
    const auto directory = pack("invalid-groups", _PACK, std::nullopt);
    auto definition = _definition("addon_test:invalid_groups");
    definition["minecraft:entity"]["component_groups"]["large"] = {
        {"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    auto invalid = definition;
    invalid["minecraft:entity"]["events"]["test:invalid"] = {{"add", {{"component_groups", {"missing"}}}}};
    EXPECT_TRUE(AddonEntityDefinition::parse(invalid).failed());
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:invalid_groups");
    ASSERT_NE(entity, nullptr);
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    const auto before = components->save();
    auto broken = before;
    broken["sources"]["minecraft:collision_box"] = "missing";
    EXPECT_TRUE(components->load(*entity, broken).failed());
    EXPECT_EQ(components->save(), before);
    broken = before;
    broken["pending"].push_back({{"operation", "invalid"}, {"group", "large"}});
    EXPECT_TRUE(components->load(*entity, broken).failed());
    EXPECT_EQ(components->save(), before);
    EXPECT_FLOAT_EQ(entity->width(), 0.8f);
}

TEST_F(AddonEntityDefinitionTest, RecursiveGroupEventFailureQueuesNoComponentChanges)
{
    const auto directory = pack("group-cycle", _PACK, std::nullopt);
    auto definition = _definition("addon_test:group_cycle");
    definition["minecraft:entity"]["component_groups"]["large"] = {
        {"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    definition["minecraft:entity"]["events"]["test:loop"] = {
        {"sequence", {{{"add", {{"component_groups", {"large"}}}}}, {{"trigger", "test:loop"}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:group_cycle");
    ASSERT_NE(entity, nullptr);
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    const auto before = components->save();
    EXPECT_TRUE(entity->tryGetComponent<AddonEntityEvents>()->trigger(*entity, "test:loop").failed());
    EXPECT_EQ(components->save(), before);
}

TEST_F(AddonEntityDefinitionTest, ScriptModifiersDescribeGroupChangesAndInactiveHealthIsAbsent)
{
    const auto directory = pack("script-groups", _PACK, std::string(R"(
        import { world } from '@minecraft/server';
        globalThis.modifiers = [];
        world.afterEvents.dataDrivenEntityTrigger.subscribe(event => {
            modifiers = event.getModifiers();
        }, {eventTypes: ['test:add']});
    )"));
    auto definition = _definition("addon_test:script_groups");
    definition["minecraft:entity"]["component_groups"]["health"] = {{"minecraft:health", {{"max", 64}, {"value", 40}}}};
    definition["minecraft:entity"]["events"]["test:add"] = {{"add", {{"component_groups", {"health"}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:script_groups");
    ASSERT_NE(entity, nullptr);
    auto* context = manager.pluginManager().getPlugin(_PACK)->context();
    auto& bindings = context->bindingContext();
    void* global = bindings.getGlobalObject();
    void* subject = wrapEntity(bindings, entity.get());
    bindings.setProperty(global, "subject", subject);
    bindings.releaseValue(subject);
    bindings.releaseValue(global);
    auto check = context->evaluate("if (subject.getComponent('minecraft:health') !== undefined) throw Error('inactive "
                                   "health was exposed'); subject.triggerEvent('test:add');",
        "group-trigger.js",
        EvalFlags::None);
    ASSERT_TRUE(check.success()) << check.errorMessage();
    manager.eventBus().tick();
    check = context->evaluate("if (modifiers.length !== 1 || modifiers[0].addedComponentGroups[0] !== 'health' || "
                              "modifiers[0].removedComponentGroups.length) throw Error('wrong modifiers');",
        "group-modifiers.js",
        EvalFlags::None);
    ASSERT_TRUE(check.success()) << check.errorMessage();
    entity->tryGetComponent<AddonEntityComponents>()->applyPending(*entity);
    check = context->evaluate(
        "if (subject.getComponent('minecraft:health').currentValue !== 40) throw Error('active health missing');",
        "group-health.js",
        EvalFlags::None);
    ASSERT_TRUE(check.success()) << check.errorMessage();
}

TEST_F(AddonEntityDefinitionTest, MolangPropertyQueriesReadCommittedStateWithinTheSameTick)
{
    const auto directory = pack("molang-events", _PACK, std::nullopt);
    auto definition = _definition("addon_test:molang_events");
    definition["minecraft:entity"]["events"]["test:increment"] = {
        {"set_property", {{"test:count", "q.property('test:count') + 1"}}}};
    write(directory / "entities/entity.json", definition);
    const auto loaded = manager.loadPacks("", root.string());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
    auto entity = create("addon_test:molang_events");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    ASSERT_TRUE(events->trigger(*entity, "test:increment").success());
    ASSERT_TRUE(events->trigger(*entity, "test:increment").success());
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(4));
    ASSERT_TRUE(events->trigger(*entity, "test:increment").success());
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(5));
}

TEST_F(AddonEntityDefinitionTest, MaidTimerAndProgressExpressionsUseMolangArithmetic)
{
    const auto directory = pack("maid-expressions", _PACK, std::nullopt);
    auto definition = _definition("addon_test:maid_expressions");
    auto& properties = definition["minecraft:entity"]["description"]["properties"];
    properties["thlm:time"] = {{"type", "int"}, {"range", {0, 1000}}, {"default", 10}};
    properties["thlm:total_time"] = {{"type", "int"}, {"range", {0, 1000}}, {"default", 20}};
    properties["thlm:progress"] = {{"type", "float"}, {"range", {0, 1}}, {"default", 0}};
    definition["minecraft:entity"]["events"]["test:update"] = {{"set_property",
        {{"thlm:time", "q.property('thlm:time')+1"},
            {"thlm:progress", "q.property('thlm:time')/q.property('thlm:total_time')"}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:maid_expressions");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    ASSERT_TRUE(events->trigger(*entity, "test:update").success());
    entity->tick();
    const auto& state = entity->getOrAddComponent<EntityAddonState>();
    EXPECT_EQ(state.getProperty("thlm:time"), nlohmann::json(11));
    EXPECT_EQ(state.getProperty("thlm:progress"), nlohmann::json(0.5));
}

TEST_F(AddonEntityDefinitionTest, MolangBooleanAndEnumResultsMatchTheirPropertyTypes)
{
    const auto directory = pack("typed-expressions", _PACK, std::nullopt);
    auto definition = _definition("addon_test:typed_expressions");
    auto& properties = definition["minecraft:entity"]["description"]["properties"];
    properties["test:enabled"] = {{"type", "bool"}, {"default", false}};
    properties["test:mode"] = {{"type", "enum"}, {"values", {"idle", "work"}}, {"default", "idle"}};
    definition["minecraft:entity"]["events"]["test:update"] = {{"set_property",
        {{"test:enabled", "q.property('test:count') > 2"},
            {"test:mode", "q.property('test:mode') == 'idle' ? 'work' : 'idle'"}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:typed_expressions");
    ASSERT_NE(entity, nullptr);
    ASSERT_TRUE(entity->tryGetComponent<AddonEntityEvents>()->trigger(*entity, "test:update").success());
    entity->tick();
    const auto& state = entity->getOrAddComponent<EntityAddonState>();
    EXPECT_EQ(state.getProperty("test:enabled"), nlohmann::json(true));
    EXPECT_EQ(state.getProperty("test:mode"), nlohmann::json("work"));
}

TEST_F(AddonEntityDefinitionTest, MolangRuntimeFailureCommitsNeitherPropertiesNorComponentGroups)
{
    const auto directory = pack("expression-failure", _PACK, std::nullopt);
    auto definition = _definition("addon_test:expression_failure");
    definition["minecraft:entity"]["component_groups"]["large"] = {
        {"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    definition["minecraft:entity"]["events"]["test:fail"] = {{"sequence",
        {{{"add", {{"component_groups", {"large"}}}}, {"set_property", {{"test:count", 90}}}},
            {{"set_property", {{"test:count", "math.sqrt(-1)"}}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:expression_failure");
    ASSERT_NE(entity, nullptr);
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    const auto beforeState = state.save();
    const auto beforeComponents = components->save();
    EXPECT_TRUE(entity->tryGetComponent<AddonEntityEvents>()->trigger(*entity, "test:fail").failed());
    EXPECT_EQ(state.save(), beforeState);
    EXPECT_EQ(components->save(), beforeComponents);
    EXPECT_FALSE(manager.eventBus().hasPendingAfterEvents());
}

TEST_F(AddonEntityDefinitionTest, FactoryEvaluatesDefaultExpressionsBeforeEntityUse)
{
    const auto directory = pack("default-expressions", _PACK, std::nullopt);
    auto definition = _definition("addon_test:default_expressions");
    auto& properties = definition["minecraft:entity"]["description"]["properties"];
    properties["test:count"]["default"] = "math.random_integer(7, 7)";
    properties["test:flag"] = {{"type", "bool"}, {"default", "math.sin(90)"}};
    properties["test:mode"] = {{"type", "enum"}, {"values", {"idle", "work"}}, {"default", "'work'"}};
    definition["minecraft:entity"]["events"]["test:increment"] = {
        {"set_property", {{"test:count", "q.property('test:count') + 1"}}}};
    write(directory / "entities/entity.json", definition);
    const auto result = manager.loadPacks("", root.string());
    ASSERT_TRUE(result.success()) << result.error().message();
    auto entity = create("addon_test:default_expressions");
    ASSERT_NE(entity, nullptr);
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(7));
    EXPECT_EQ(state.getProperty("test:flag"), nlohmann::json(true));
    EXPECT_EQ(state.getProperty("test:mode"), nlohmann::json("work"));
    ASSERT_TRUE(entity->tryGetComponent<AddonEntityEvents>()->trigger(*entity, "test:increment").success());
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(8));
    EXPECT_EQ(state.resetProperty("test:count"), nlohmann::json(7));
}

TEST_F(AddonEntityDefinitionTest, SavedPropertyValuesOverrideExpressionDefaultsDuringRestore)
{
    mc::entity::serialization::components::ComponentSerializerRegistry::instance().registerAll();
    const auto directory = pack("saved-defaults", _PACK, std::nullopt);
    auto definition = _definition("addon_test:saved_defaults");
    definition["minecraft:entity"]["description"]["properties"]["test:count"]["default"] = "math.random_integer(7, 7)";
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:saved_defaults");
    ASSERT_NE(entity, nullptr);
    ASSERT_TRUE(entity->getOrAddComponent<EntityAddonState>().setProperty("test:count", 42).success());
    auto saved = mc::entity::serialization::EntityDeserializer::serializeToBinary(*entity);
    ASSERT_TRUE(saved.success());
    auto loaded = mc::entity::serialization::EntityDeserializer::deserializeFromBinary(saved.value(), entities);
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
    auto restored = loaded.value();
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(42));
}

TEST_F(AddonEntityDefinitionTest, InvalidDefaultExpressionEvaluationRejectsEntityCreation)
{
    const auto directory = pack("invalid-default", _PACK, std::nullopt);
    auto definition = _definition("addon_test:invalid_default");
    definition["minecraft:entity"]["description"]["properties"]["test:count"]["default"] = "math.sqrt(-1)";
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    const auto* type = mc::entity::EntityRegistry::instance().getType("addon_test:invalid_default");
    ASSERT_NE(type, nullptr);
    EXPECT_EQ(type->create(nullptr, entities), nullptr);
}

TEST_F(AddonEntityDefinitionTest, EventFiltersReadCommittedPropertiesAndGateGroupChanges)
{
    const auto directory = pack("filtered-event", _PACK, std::nullopt);
    auto definition = _definition("addon_test:filtered_event");
    definition["minecraft:entity"]["component_groups"]["large"] = {
        {"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    definition["minecraft:entity"]["events"]["test:conditional"] = {
        {"filters", {{"test", "int_property"}, {"domain", "test:count"}, {"operator", ">"}, {"value", 5}}},
        {"add", {{"component_groups", {"large"}}}},
        {"set_property", {{"test:count", 9}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:filtered_event");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    ASSERT_TRUE(state.setProperty("test:count", 6).success());
    ASSERT_TRUE(events->trigger(*entity, "test:conditional").success());
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(6));
    EXPECT_FLOAT_EQ(entity->width(), 0.8f);
    ASSERT_TRUE(events->trigger(*entity, "test:conditional").success());
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(9));
    EXPECT_FLOAT_EQ(entity->width(), 2);
}

TEST_F(AddonEntityDefinitionTest, FamilyFiltersUseTheCurrentlyActiveFamilyComponent)
{
    const auto directory = pack("family-event", _PACK, std::nullopt);
    auto definition = _definition("addon_test:family_event");
    definition["minecraft:entity"]["components"]["minecraft:type_family"] = {{"family", {"maid", "mob"}}};
    definition["minecraft:entity"]["component_groups"]["worker"] = {
        {"minecraft:type_family", {{"family", {"worker"}}}}};
    definition["minecraft:entity"]["events"] = {{"test:activate", {{"add", {{"component_groups", {"worker"}}}}}},
        {"test:deactivate", {{"remove", {{"component_groups", {"worker"}}}}}},
        {"test:work",
            {{"filters", {{"test", "is_family"}, {"value", "worker"}}}, {"set_property", {{"test:count", 66}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:family_event");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    ASSERT_TRUE(events->trigger(*entity, "test:activate").success());
    ASSERT_TRUE(events->trigger(*entity, "test:work").success());
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
    ASSERT_TRUE(events->trigger(*entity, "test:work").success());
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(66));
    ASSERT_TRUE(events->trigger(*entity, "test:deactivate").success());
    entity->tick();
    auto originalFamily = EntityFilter::compile({{"test", "is_family"}, {"value", "maid"}});
    ASSERT_TRUE(originalFamily.success());
    auto matches = originalFamily.value().matches(
        EntityFilterContext{*entity, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});
    ASSERT_TRUE(matches.success());
    EXPECT_FALSE(matches.value());
}

TEST_F(AddonEntityDefinitionTest, EventFiltersReceiveExplicitOtherEntityContext)
{
    const auto directory = pack("interaction-filter", _PACK, std::nullopt);
    auto definition = _definition("addon_test:interaction_filter");
    definition["minecraft:entity"]["events"]["test:interact"] = {
        {"filters", {{"test", "int_property"}, {"subject", "other"}, {"domain", "test:count"}, {"value", 99}}},
        {"set_property", {{"test:count", 12}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:interaction_filter");
    auto other = create("addon_test:interaction_filter");
    ASSERT_NE(entity, nullptr);
    ASSERT_NE(other, nullptr);
    ASSERT_TRUE(other->getOrAddComponent<EntityAddonState>().setProperty("test:count", 99).success());
    other->tick();
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    ASSERT_TRUE(events->trigger(*entity, "test:interact").success());
    entity->tick();
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(3));
    ASSERT_TRUE(events
            ->trigger(
                EntityFilterContext{*entity, other.get(), nullptr, nullptr, nullptr, nullptr, nullptr}, "test:interact")
            .success());
    entity->tick();
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(12));
}

TEST_F(AddonEntityDefinitionTest, FilterEvaluationFailureAbortsTheWholeEventTransaction)
{
    const auto directory = pack("failed-filter", _PACK, std::nullopt);
    auto definition = _definition("addon_test:failed_filter");
    definition["minecraft:entity"]["component_groups"]["large"] = {
        {"minecraft:collision_box", {{"width", 2}, {"height", 3}}}};
    definition["minecraft:entity"]["events"]["test:fail"] = {{"sequence",
        {{{"add", {{"component_groups", {"large"}}}}, {"set_property", {{"test:count", 90}}}},
            {{"filters", {{"test", "is_family"}, {"subject", "other"}, {"value", "unmapped-native-family"}}},
                {"set_property", {{"test:count", 80}}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:failed_filter");
    ASSERT_NE(entity, nullptr);
    mc::Entity other(mc::EntityInstanceId(2), nullptr, entities);
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    const auto before = components->save();
    EXPECT_TRUE(entity->tryGetComponent<AddonEntityEvents>()
            ->trigger(EntityFilterContext{*entity, &other, nullptr, nullptr, nullptr, nullptr, nullptr}, "test:fail")
            .failed());
    EXPECT_EQ(components->save(), before);
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(3));
    EXPECT_FALSE(manager.eventBus().hasPendingAfterEvents());
}

TEST_F(AddonEntityDefinitionTest, FixedOneShotTimerFiresAfterItsDurationAndAppliesEventNextTick)
{
    const auto directory = pack("one-shot", _PACK, std::nullopt);
    auto definition = _definition("addon_test:one_shot");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {
        {"time", 0.1}, {"looping", false}, {"time_down_event", "test:fire"}};
    definition["minecraft:entity"]["events"]["test:fire"] = {
        {"set_property", {{"test:count", "q.property('test:count')+1"}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:one_shot");
    ASSERT_NE(entity, nullptr);
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(4));
    for (int index = 0; index < 8; ++index) {
        entity->tick();
    }
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(4));
    EXPECT_EQ(entity->tryGetComponent<AddonEntityComponents>()->save()["timer"]["state"], "stopped");
}

TEST_F(AddonEntityDefinitionTest, LoopingTimerContinuesToAdvanceThePropertyEventChain)
{
    const auto directory = pack("loop-timer", _PACK, std::nullopt);
    auto definition = _definition("addon_test:loop_timer");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {
        {"time", 0.05}, {"time_down_event", {{"event", "test:fire"}, {"target", "self"}}}};
    definition["minecraft:entity"]["events"]["test:fire"] = {
        {"set_property", {{"test:count", "q.property('test:count')+1"}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:loop_timer");
    ASSERT_NE(entity, nullptr);
    for (int index = 0; index < 5; ++index) {
        entity->tick();
    }
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(7));
    const auto timer = entity->tryGetComponent<AddonEntityComponents>()->save()["timer"];
    EXPECT_EQ(timer["state"], "running");
    EXPECT_EQ(timer["remaining"], 1);
}

TEST_F(AddonEntityDefinitionTest, TimerGroupReadditionRestartsAndRemovalStopsTheTimer)
{
    const auto directory = pack("timer-group", _PACK, std::nullopt);
    auto definition = _definition("addon_test:timer_group");
    definition["minecraft:entity"]["component_groups"]["clock"] = {
        {"minecraft:timer", {{"time", 0.15}, {"looping", false}, {"time_down_event", "test:fire"}}}};
    definition["minecraft:entity"]["events"] = {{"test:start", {{"add", {{"component_groups", {"clock"}}}}}},
        {"test:stop", {{"remove", {{"component_groups", {"clock"}}}}}},
        {"test:fire", {{"set_property", {{"test:count", "q.property('test:count')+1"}}}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:timer_group");
    ASSERT_NE(entity, nullptr);
    auto* events = entity->tryGetComponent<AddonEntityEvents>();
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    ASSERT_TRUE(events->trigger(*entity, "test:start").success());
    entity->tick();
    entity->tick();
    ASSERT_TRUE(events->trigger(*entity, "test:start").success());
    entity->tick();
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
    entity->tick();
    entity->tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(4));
    ASSERT_TRUE(events->trigger(*entity, "test:start").success());
    entity->tick();
    ASSERT_TRUE(events->trigger(*entity, "test:stop").success());
    for (int index = 0; index < 6; ++index) {
        entity->tick();
    }
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(4));
    EXPECT_EQ(entity->tryGetComponent<AddonEntityComponents>()->save()["timer"]["state"], "absent");
}

TEST_F(AddonEntityDefinitionTest, SavedTimerRemainingTicksAreRestoredWithoutRestartingTheInterval)
{
    mc::entity::serialization::components::ComponentSerializerRegistry::instance().registerAll();
    const auto directory = pack("saved-timer", _PACK, std::nullopt);
    auto definition = _definition("addon_test:saved_timer");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {
        {"time", 0.25}, {"looping", false}, {"time_down_event", "test:fire"}};
    definition["minecraft:entity"]["events"]["test:fire"] = {{"set_property", {{"test:count", 8}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:saved_timer");
    ASSERT_NE(entity, nullptr);
    entity->tick();
    entity->tick();
    EXPECT_EQ(entity->tryGetComponent<AddonEntityComponents>()->save()["timer"]["remaining"], 3);
    auto saved = mc::entity::serialization::EntityDeserializer::serializeToBinary(*entity);
    ASSERT_TRUE(saved.success());
    auto loaded = mc::entity::serialization::EntityDeserializer::deserializeFromBinary(saved.value(), entities);
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
    auto restored = loaded.value();
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->tryGetComponent<AddonEntityComponents>()->save()["timer"]["remaining"], 3);
    for (int index = 0; index < 3; ++index) {
        restored->tick();
    }
    EXPECT_EQ(restored->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(3));
    restored->tick();
    EXPECT_EQ(restored->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(8));
}

TEST_F(AddonEntityDefinitionTest, CompletedOneShotTimerDoesNotFireAgainAfterRestore)
{
    mc::entity::serialization::components::ComponentSerializerRegistry::instance().registerAll();
    const auto directory = pack("completed-timer", _PACK, std::nullopt);
    auto definition = _definition("addon_test:completed_timer");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {
        {"time", 0}, {"looping", false}, {"time_down_event", "test:fire"}};
    definition["minecraft:entity"]["events"]["test:fire"] = {
        {"set_property", {{"test:count", "q.property('test:count')+1"}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:completed_timer");
    ASSERT_NE(entity, nullptr);
    entity->tick();
    entity->tick();
    auto saved = mc::entity::serialization::EntityDeserializer::serializeToBinary(*entity);
    ASSERT_TRUE(saved.success());
    auto loaded = mc::entity::serialization::EntityDeserializer::deserializeFromBinary(saved.value(), entities);
    ASSERT_TRUE(loaded.success());
    auto restored = loaded.value();
    ASSERT_NE(restored, nullptr);
    for (int index = 0; index < 6; ++index) {
        restored->tick();
    }
    EXPECT_EQ(restored->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(4));
    EXPECT_EQ(restored->tryGetComponent<AddonEntityComponents>()->save()["timer"]["state"], "stopped");
}

TEST_F(AddonEntityDefinitionTest, WeightedTimerChoicesIgnoreZeroWeights)
{
    const auto directory = pack("weighted-timer", _PACK, std::nullopt);
    auto definition = _definition("addon_test:weighted_timer");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {
        {"random_time_choices", {{{"weight", 0}, {"value", 100}}, {{"weight", 1}, {"value", 0.1}}}},
        {"looping", false},
        {"time_down_event", "test:fire"}};
    definition["minecraft:entity"]["events"]["test:fire"] = {{"set_property", {{"test:count", 8}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:weighted_timer");
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->tryGetComponent<AddonEntityComponents>()->save()["timer"]["interval"], 2);
    for (int index = 0; index < 3; ++index) {
        entity->tick();
    }
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(8));
}

TEST_F(AddonEntityDefinitionTest, NonRandomLoopReusesItsChosenInterval)
{
    const auto directory = pack("fixed-random-timer", _PACK, std::nullopt);
    auto definition = _definition("addon_test:fixed_random_timer");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {
        {"time", {0.1, 0.3}}, {"randomInterval", false}, {"time_down_event", "test:fire"}};
    definition["minecraft:entity"]["events"]["test:fire"] = nlohmann::json::object();
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:fixed_random_timer");
    ASSERT_NE(entity, nullptr);
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    const auto interval = components->save()["timer"]["interval"];
    EXPECT_GE(interval.get<int>(), 2);
    EXPECT_LE(interval.get<int>(), 6);
    for (int index = 0; index < 40; ++index) {
        entity->tick();
        EXPECT_EQ(components->save()["timer"]["interval"], interval);
    }
}

TEST_F(AddonEntityDefinitionTest, TimerEventReferenceFiltersGateTheExpiryAction)
{
    const auto directory = pack("filtered-timer", _PACK, std::nullopt);
    auto definition = _definition("addon_test:filtered_timer");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {{"time", 0.05},
        {"time_down_event",
            {{"event", "test:fire"},
                {"filters", {{"test", "int_property"}, {"domain", "test:count"}, {"operator", ">"}, {"value", 5}}}}}};
    definition["minecraft:entity"]["events"]["test:fire"] = {{"set_property", {{"test:count", 8}}}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:filtered_timer");
    ASSERT_NE(entity, nullptr);
    entity->tick();
    entity->tick();
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(3));
    ASSERT_TRUE(entity->getOrAddComponent<EntityAddonState>().setProperty("test:count", 6).success());
    entity->tick();
    entity->tick();
    EXPECT_EQ(entity->getOrAddComponent<EntityAddonState>().getProperty("test:count"), nlohmann::json(8));
}

TEST_F(AddonEntityDefinitionTest, InvalidTimerDefinitionsAndUnknownEventsAreRejected)
{
    const std::vector<nlohmann::json> timers = {{{"time", -1}, {"time_down_event", "test:fire"}},
        {{"time", {2, 1}}, {"time_down_event", "test:fire"}},
        {{"time", {1}}, {"time_down_event", "test:fire"}},
        {{"time", 1}, {"looping", "true"}, {"time_down_event", "test:fire"}},
        {{"time", 1}, {"random_time_choices", {{{"value", 2}}}}, {"time_down_event", "test:fire"}},
        {{"random_time_choices", {{{"value", 2}, {"weight", 0}}}}, {"time_down_event", "test:fire"}},
        {{"time", 1}, {"time_down_event", {{"event", "test:fire"}, {"target", "other"}}}},
        {{"time", 1}}};
    for (const auto& timer : timers) {
        EXPECT_TRUE(EntityTimerState::validateDefinition(timer).failed()) << timer.dump();
    }
    auto definition = _definition("addon_test:unknown_timer_event");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {
        {"time", 1}, {"time_down_event", "test:missing"}};
    EXPECT_TRUE(AddonEntityDefinition::parse(definition).failed());
}

TEST_F(AddonEntityDefinitionTest, InvalidSavedTimerDoesNotMutateLiveComponentState)
{
    const auto directory = pack("corrupt-timer", _PACK, std::nullopt);
    auto definition = _definition("addon_test:corrupt_timer");
    definition["minecraft:entity"]["components"]["minecraft:timer"] = {{"time", 1}, {"time_down_event", "test:fire"}};
    definition["minecraft:entity"]["events"]["test:fire"] = nlohmann::json::object();
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());
    auto entity = create("addon_test:corrupt_timer");
    ASSERT_NE(entity, nullptr);
    auto* components = entity->tryGetComponent<AddonEntityComponents>();
    const auto before = components->save();
    auto corrupt = before;
    corrupt["timer"]["remaining"] = 1000;
    EXPECT_TRUE(components->load(*entity, corrupt).failed());
    EXPECT_EQ(components->save(), before);
    corrupt = before;
    corrupt["timer"]["state"] = "stopped";
    EXPECT_TRUE(components->load(*entity, corrupt).failed());
    EXPECT_EQ(components->save(), before);
}

TEST_F(AddonEntityDefinitionTest, NameableDefaultTriggerFiresOnNamingChangeOnly)
{
    // minecraft:nameable.default_trigger 表示"该实体被命名时触发一次指定事件"，
    // 与具体名字无关。三条边界都要守住：
    //   1) 初次命名要触发；
    //   2) 换成另一个名字要再触发一次；
    //   3) 改回同一个名字、以及清除名字都不触发。
    const auto directory = pack("nameable", _PACK, std::nullopt);
    auto definition = _definition("addon_test:nameable");
    definition["minecraft:entity"]["events"] = {{"test:on_name", {{"set_property", {{"test:count", 42}}}}}};
    definition["minecraft:entity"]["components"]["minecraft:nameable"] = {
        {"allow_name_tag_renaming", true}, {"default_trigger", "test:on_name"}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());

    auto entity = create("addon_test:nameable");
    ASSERT_NE(entity, nullptr);
    auto& state = entity->getOrAddComponent<EntityAddonState>();
    ASSERT_EQ(state.getProperty("test:count"), nlohmann::json(3));

    entity->setCustomName("First");
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(42));

    // 同名重复设置不重复触发：把属性改回默认值后仍应保持默认，说明没有再派发。
    state.setProperty("test:count", nlohmann::json(3));
    entity->setCustomName("First");
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));

    // 换成另一个名字要再次触发。
    entity->setCustomName("Second");
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(42));

    // 清除名字不触发。
    state.setProperty("test:count", nlohmann::json(3));
    entity->setCustomName("");
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
}

TEST_F(AddonEntityDefinitionTest, NameableWithoutDefaultTriggerLeavesNameUnchanged)
{
    // 未声明 default_trigger 时命名只改名字，不应出错也不应影响其它属性状态。
    const auto directory = pack("nameable_plain", _PACK, std::nullopt);
    auto definition = _definition("addon_test:nameable_plain");
    definition["minecraft:entity"]["components"]["minecraft:nameable"] = {{"allow_name_tag_renaming", true}};
    write(directory / "entities/entity.json", definition);
    ASSERT_TRUE(manager.loadPacks("", root.string()).success());

    auto entity = create("addon_test:nameable_plain");
    ASSERT_NE(entity, nullptr);
    entity->setCustomName("Plain");
    EXPECT_EQ(entity->customNameText(), std::string("Plain"));
}
