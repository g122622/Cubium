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

#include "common/entity/core/Entity.hpp"
#include "common/entity/ecs/context/EntityRegistry.hpp"
#include "common/entity/serialization/components/ComponentSerializerRegistry.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include "common/mod/bedrock/addon/binding/ScriptGameObjects.hpp"
#include "common/mod/bedrock/addon/content/entity/state/EntityAddonState.hpp"
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/core/IScriptRuntime.hpp"
#include "common/mod/bedrock/addon/modules/MinecraftModuleFactory.hpp"

#include <limits>
#include <memory>
#include <string>
#include <vector>
#include <gtest/gtest.h>

using namespace mc::mod::bedrock::addon;

namespace {

nlohmann::json _schema()
{
    return {{"test:enabled", {{"type", "bool"}, {"default", false}, {"client_sync", true}}},
        {"test:count", {{"type", "int"}, {"range", {0, 100}}, {"default", 3}}},
        {"test:speed", {{"type", "float"}, {"range", {-10.0, 10.0}}, {"default", 1.5}}},
        {"test:mode", {{"type", "enum"}, {"values", {"Idle", "Working_2"}}, {"default", "Idle"}}}};
}

class _TickEntity final : public mc::Entity {
public:
    explicit _TickEntity(mc::ecs::EntityRegistry& registry)
        : Entity(mc::EntityInstanceId(1), nullptr, registry)
    {}

    // 保留真实 Entity::tick，只去掉依赖物理世界的基础更新。
    void baseTick() override {}
};

class AddonEntityPropertiesBindingTest : public testing::Test {
protected:
    void SetUp() override
    {
        engine = createScriptEngine();
        ASSERT_TRUE(engine->initialize());
        subject = std::make_unique<_TickEntity>(registry);
        ASSERT_TRUE(subject->getOrAddComponent<EntityAddonState>().defineProperties(_schema()).success());
        first = createContext("pack-first");
        second = createContext("pack-second");
        ASSERT_NE(first, nullptr);
        ASSERT_NE(second, nullptr);
    }

    void TearDown() override
    {
        second.reset();
        first.reset();
        subject.reset();
        engine->shutdown();
    }

    std::unique_ptr<IScriptContext> createContext(const std::string& pack)
    {
        auto context = engine->runtime().createContext(ContextConfig{
            Capabilities{}, std::nullopt, DEFAULT_CONTEXT_MEMORY_BYTES, DEFAULT_CONTEXT_STACK_SIZE_BYTES});
        if (!context) {
            return nullptr;
        }
        auto& binding = context->bindingContext();
        binding.setPackId(pack);
        MinecraftModuleFactory factory;
        EXPECT_TRUE(factory.registerBindings(*context));
        void* global = binding.getGlobalObject();
        void* wrapped = wrapEntity(binding, subject.get());
        binding.setProperty(global, "subject", wrapped);
        binding.releaseValue(wrapped);
        binding.releaseValue(global);
        return context;
    }

    void check(IScriptContext& context, const std::string& source)
    {
        const auto result = context.evaluate(source, "entity-properties.js", EvalFlags::None);
        ASSERT_TRUE(result.success()) << result.errorMessage();
    }

    mc::ecs::EntityRegistry registry{"addon-entity-properties"};
    std::unique_ptr<IScriptEngine> engine;
    std::unique_ptr<_TickEntity> subject;
    std::unique_ptr<IScriptContext> first;
    std::unique_ptr<IScriptContext> second;
};

} // namespace

TEST(AddonEntityStateTest, DeclaredPropertiesUseDefaultsAndApplyOnlyAtEntityTick)
{
    mc::ecs::EntityRegistry registry("addon-property-tick");
    _TickEntity entity(registry);
    auto& state = entity.getOrAddComponent<EntityAddonState>();
    ASSERT_TRUE(state.defineProperties(_schema()).success());
    EXPECT_EQ(state.getProperty("test:enabled"), nlohmann::json(false));
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
    EXPECT_EQ(state.getProperty("test:speed"), nlohmann::json(1.5));
    EXPECT_EQ(state.getProperty("test:mode"), nlohmann::json("Idle"));
    EXPECT_FALSE(state.getProperty("test:missing").has_value());

    ASSERT_TRUE(state.setProperty("test:count", 10).success());
    ASSERT_TRUE(state.setProperty("test:count", 20).success());
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
    entity.tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(20));
    EXPECT_FALSE(state.applyPendingProperties());
    ASSERT_TRUE(state.setProperty("test:count", 20).success());
    EXPECT_FALSE(state.applyPendingProperties());

    EXPECT_EQ(state.resetProperty("test:count"), nlohmann::json(3));
    EXPECT_FALSE(state.resetProperty("test:missing").has_value());
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(20));
    entity.tick();
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(3));
}

TEST(AddonEntityStateTest, InvalidWritesPreserveThePreviousPendingValue)
{
    EntityAddonState state;
    ASSERT_TRUE(state.defineProperties(_schema()).success());
    ASSERT_TRUE(state.setProperty("test:count", 12).success());
    const std::vector<nlohmann::json> invalid = {true,
        "5",
        nullptr,
        -1,
        101,
        1.25,
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::quiet_NaN()};
    for (const auto& value : invalid) {
        EXPECT_TRUE(state.setProperty("test:count", value).failed()) << value.dump();
    }
    EXPECT_TRUE(state.setProperty("test:enabled", 1).failed());
    EXPECT_TRUE(state.setProperty("test:speed", 11).failed());
    EXPECT_TRUE(state.setProperty("test:mode", "unknown").failed());
    EXPECT_TRUE(state.setProperty("test:missing", true).failed());
    EXPECT_TRUE(state.applyPendingProperties());
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(12));
}

TEST(AddonEntityStateTest, InvalidDefinitionsDoNotReplaceExistingState)
{
    EntityAddonState state;
    ASSERT_TRUE(state.defineProperties(_schema()).success());
    ASSERT_TRUE(state.setProperty("test:count", 22).success());
    std::vector<nlohmann::json> invalid;
    invalid.push_back(nlohmann::json::array());
    auto malformed = _schema();
    malformed["test:count"]["range"] = {0.5, 10};
    invalid.push_back(malformed);
    malformed["test:count"]["range"] = {0, 2147483648.0};
    invalid.push_back(malformed);
    malformed["test:count"]["range"] = {10, 0};
    invalid.push_back(malformed);
    malformed["test:count"]["range"] = {0};
    invalid.push_back(malformed);
    malformed = _schema();
    malformed["test:enabled"]["client_sync"] = 1;
    invalid.push_back(malformed);
    malformed = _schema();
    malformed["test:mode"]["values"] = {"Idle", "Idle"};
    invalid.push_back(malformed);
    malformed["test:mode"]["values"] = {"Idle", "2invalid"};
    invalid.push_back(malformed);
    malformed["test:mode"]["values"] = {"Idle", "bad-name"};
    invalid.push_back(malformed);
    malformed["test:mode"]["values"] = {"Idle", std::string(33, 'a')};
    invalid.push_back(malformed);
    malformed["test:mode"]["values"] = {"Idle", 42};
    invalid.push_back(malformed);
    malformed = _schema();
    malformed["test:mode"]["values"] = nlohmann::json::array({"Idle"});
    for (int index = 0; index < 16; ++index) {
        malformed["test:mode"]["values"].push_back("Value" + std::to_string(index));
    }
    invalid.push_back(malformed);
    malformed = nlohmann::json::object();
    for (int index = 0; index < 33; ++index) {
        malformed["test:p" + std::to_string(index)] = {{"type", "bool"}, {"default", false}};
    }
    invalid.push_back(malformed);
    invalid.push_back({{"no_namespace", {{"type", "bool"}, {"default", true}}}});
    invalid.push_back({{"test:value", {{"type", "unknown"}, {"default", 0}}}});
    invalid.push_back({{"test:value", {{"type", "bool"}}}});

    for (const auto& definition : invalid) {
        EXPECT_TRUE(state.defineProperties(definition).failed()) << definition.dump();
    }
    EXPECT_TRUE(state.applyPendingProperties());
    EXPECT_EQ(state.getProperty("test:count"), nlohmann::json(22));
}

TEST(AddonEntityStateTest, IntegerTypeBoundsAreNotRoundedThroughFloat)
{
    EntityAddonState state;
    ASSERT_TRUE(state.defineProperties({{"test:value", {{"type", "int"}, {"default", 0}}}}).success());
    EXPECT_TRUE(state.setProperty("test:value", 2147483647.0).success());
    EXPECT_TRUE(state.setProperty("test:value", 2147483648.0).failed());
    EXPECT_TRUE(state.setProperty("test:value", -2147483648.0).success());
    EXPECT_TRUE(state.setProperty("test:value", -2147483649.0).failed());
}

TEST(AddonEntityStateTest, DynamicPropertiesAreImmediateAndIsolatedByPack)
{
    EntityAddonState state;
    const nlohmann::json vector = {{"x", 1.25}, {"y", -2}, {"z", 3}};
    ASSERT_TRUE(state.dynamicProperties()
            .setMany("first", {{"flag", true}, {"number", 42.5}, {"text", "女仆"}, {"position", vector}})
            .success());
    ASSERT_TRUE(state.dynamicProperties().set("second", "flag", false).success());
    EXPECT_EQ(state.dynamicProperties().get("first", "flag"), nlohmann::json(true));
    EXPECT_EQ(state.dynamicProperties().get("second", "flag"), nlohmann::json(false));
    EXPECT_FALSE(state.dynamicProperties().get("second", "number").has_value());
    EXPECT_EQ(state.dynamicProperties().get("first", "position"), vector);
    EXPECT_EQ(state.dynamicProperties().ids("first"), (std::vector<std::string>{"flag", "number", "position", "text"}));

    ASSERT_TRUE(state.dynamicProperties().set("first", "number", nullptr).success());
    EXPECT_FALSE(state.dynamicProperties().get("first", "number").has_value());
    state.dynamicProperties().clear("first");
    EXPECT_TRUE(state.dynamicProperties().ids("first").empty());
    EXPECT_EQ(state.dynamicProperties().get("second", "flag"), nlohmann::json(false));
}

TEST(AddonEntityStateTest, DynamicPropertyBoundaryValidationIsAtomic)
{
    EntityAddonState state;
    ASSERT_TRUE(state.dynamicProperties().set("pack", "text", std::string(32767, 'a')).success());
    const std::vector<nlohmann::json> invalid = {std::string(32768, 'a'),
        nlohmann::json::array({1, 2, 3}),
        {{"x", 1}, {"y", 2}},
        {{"x", 1}, {"y", 2}, {"z", "3"}},
        {{"x", 1}, {"y", 2}, {"z", std::numeric_limits<double>::infinity()}},
        {{"x", 1}, {"y", 2}, {"z", 3}, {"w", 4}},
        std::numeric_limits<double>::quiet_NaN()};
    for (const auto& value : invalid) {
        EXPECT_TRUE(state.dynamicProperties().set("pack", "text", value).failed());
    }
    EXPECT_EQ(state.dynamicProperties().get("pack", "text"), nlohmann::json(std::string(32767, 'a')));
    EXPECT_TRUE(state.dynamicProperties().set("", "key", 1).failed());
    EXPECT_TRUE(state.dynamicProperties().set("pack", "", 1).failed());
    EXPECT_TRUE(state.dynamicProperties()
            .setMany("pack", {{"a-valid", true}, {"z-invalid", std::string(32768, 'a')}})
            .failed());
    EXPECT_FALSE(state.dynamicProperties().get("pack", "a-valid").has_value());
}

TEST(AddonEntityStateTest, SavedStateIncludesPendingChangesWithoutApplyingThem)
{
    EntityAddonState original;
    ASSERT_TRUE(original.defineProperties(_schema()).success());
    ASSERT_TRUE(original.setProperty("test:enabled", true).success());
    ASSERT_TRUE(original.setProperty("test:mode", "Working_2").success());
    ASSERT_TRUE(original.dynamicProperties().set("pack", "flag", true).success());
    EntityAddonState restored;
    ASSERT_TRUE(restored.defineProperties(_schema()).success());
    ASSERT_TRUE(restored.load(original.save()).success());
    EXPECT_EQ(original.getProperty("test:enabled"), nlohmann::json(false));
    EXPECT_EQ(restored.getProperty("test:enabled"), nlohmann::json(true));
    EXPECT_EQ(restored.getProperty("test:mode"), nlohmann::json("Working_2"));
    EXPECT_EQ(restored.dynamicProperties().get("pack", "flag"), nlohmann::json(true));
}

TEST(AddonEntityStateTest, MalformedSavedStateDoesNotPartiallyReplaceLiveState)
{
    EntityAddonState state;
    ASSERT_TRUE(state.defineProperties(_schema()).success());
    ASSERT_TRUE(state.setProperty("test:count", 17).success());
    ASSERT_TRUE(state.dynamicProperties().set("pack", "flag", true).success());
    const auto before = state.save();
    auto invalid = before;
    invalid["dynamic"]["pack"]["z-broken"] = {{"kind", "number"}, {"value", "bad"}};
    EXPECT_TRUE(state.load(invalid).failed());
    EXPECT_EQ(state.save(), before);
    invalid = before;
    invalid["properties"]["test:count"]["value"] = 500;
    EXPECT_TRUE(state.load(invalid).failed());
    EXPECT_EQ(state.save(), before);
    EXPECT_TRUE(state.load(nlohmann::json::array()).failed());
    EXPECT_EQ(state.save(), before);
}

TEST(AddonEntityStateTest, EntityNbtRoundTripPreservesAllPropertyTypes)
{
    mc::entity::serialization::components::ComponentSerializerRegistry::instance().registerAll();
    mc::ecs::EntityRegistry registry("addon-property-save");
    _TickEntity original(registry);
    auto& state = original.getOrAddComponent<EntityAddonState>();
    ASSERT_TRUE(state.defineProperties(_schema()).success());
    ASSERT_TRUE(state.setProperty("test:enabled", true).success());
    ASSERT_TRUE(state.setProperty("test:count", 42).success());
    ASSERT_TRUE(state.setProperty("test:speed", 3.5).success());
    ASSERT_TRUE(state.setProperty("test:mode", "Working_2").success());
    ASSERT_TRUE(state.dynamicProperties()
            .setMany("first",
                {{"flag", false}, {"n", 12.5}, {"text", "女仆"}, {"vector", {{"x", -1.5}, {"y", 64}, {"z", 20}}}})
            .success());
    ASSERT_TRUE(state.dynamicProperties().set("second", "flag", true).success());
    mc::nbt::tags::compound_tag tag;
    original.writeToNBT(tag);
    EXPECT_TRUE(tag.value.contains("CubiumAddonState"));

    _TickEntity restored(registry);
    auto& restoredState = restored.getOrAddComponent<EntityAddonState>();
    ASSERT_TRUE(restoredState.defineProperties(_schema()).success());
    const auto result = restored.readFromNBT(tag);
    ASSERT_TRUE(result.success()) << result.error().message();
    EXPECT_EQ(restoredState.save(), state.save());
    ASSERT_TRUE(restoredState.getProperty("test:enabled").has_value());
    EXPECT_TRUE(restoredState.getProperty("test:enabled")->is_boolean());
    EXPECT_TRUE(restoredState.dynamicProperties().get("first", "flag")->is_boolean());
    EXPECT_TRUE(restoredState.dynamicProperties().get("first", "n")->is_number());
    EXPECT_EQ(restoredState.dynamicProperties().get("first", "text"), nlohmann::json("女仆"));
}

TEST_F(AddonEntityPropertiesBindingTest, ScriptWritesAndResetObserveTickBoundary)
{
    check(*first, R"(
        subject.setProperty('test:count', 20);
        if (subject.getProperty('test:count') !== 3) throw Error('write was not deferred');
        subject.setDynamicProperty('instant', true);
        if (subject.getDynamicProperty('instant') !== true) throw Error('dynamic write was deferred');
    )");
    subject->tick();
    check(*first, R"(
        if (subject.getProperty('test:count') !== 20) throw Error('write was not applied');
        if (subject.resetProperty('test:count') !== 3) throw Error('reset returned wrong default');
        if (subject.getProperty('test:count') !== 20) throw Error('reset was not deferred');
        if (subject.resetProperty('test:missing') !== undefined) throw Error('missing reset');
    )");
    subject->tick();
    check(*first, "if (subject.getProperty('test:count') !== 3) throw Error('reset was not applied');");
}

TEST_F(AddonEntityPropertiesBindingTest, ScriptContextsCannotReadOrClearAnotherPack)
{
    check(*first, R"(
        subject.setDynamicProperties({same: 'first', position: {x: 1, y: 2, z: 3}, flag: false});
        if (subject.getDynamicPropertyIds().length !== 3) throw Error('missing keys');
    )");
    check(*second, R"(
        if (subject.getDynamicProperty('same') !== undefined) throw Error('cross-pack read');
        subject.setDynamicProperty('same', 'second');
        subject.clearDynamicProperties();
        if (subject.getDynamicPropertyIds().length !== 0) throw Error('clear failed');
    )");
    check(*first, R"(
        if (subject.getDynamicProperty('same') !== 'first') throw Error('cross-pack clear');
        if (subject.getDynamicProperty('position').z !== 3) throw Error('vector roundtrip');
        subject.setDynamicProperty('same');
        subject.setDynamicProperty('position', null);
        subject.setDynamicProperties({flag: undefined});
        if (subject.getDynamicPropertyIds().length !== 0) throw Error('deletion failed');
    )");
}

TEST_F(AddonEntityPropertiesBindingTest, ScriptRejectsInvalidValuesWithoutPartialWrites)
{
    check(*first, R"(
        const invalid = [() => subject.setProperty('test:count', 1.5),
            () => subject.setProperty('test:count', 101),
            () => subject.setProperty('test:enabled', 1),
            () => subject.setDynamicProperty('bad', NaN),
            () => subject.setDynamicProperty('bad', {x: 1, y: 2}),
            () => subject.setDynamicProperties({good: true, bad: 'a'.repeat(32768)})];
        for (const write of invalid) {
            let rejected = false;
            try { write(); } catch { rejected = true; }
            if (!rejected) throw Error('invalid value was accepted');
        }
        if (subject.getDynamicProperty('good') !== undefined) throw Error('partial batch write');
    )");
}

TEST_F(AddonEntityPropertiesBindingTest, DestroyedEntityRejectsEveryPropertyMethod)
{
    subject.reset();
    check(*first, R"(
        const operations = [() => subject.getProperty('test:count'),
            () => subject.setProperty('test:count', 1), () => subject.resetProperty('test:count'),
            () => subject.getDynamicProperty('x'), () => subject.setDynamicProperty('x', true),
            () => subject.setDynamicProperties({x: true}), () => subject.getDynamicPropertyIds(),
            () => subject.clearDynamicProperties()];
        for (const operation of operations) {
            let rejected = false;
            try { operation(); } catch { rejected = true; }
            if (!rejected) throw Error('destroyed entity remained accessible');
        }
    )");
}
