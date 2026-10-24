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

#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"
#include "common/mod/bedrock/addon/content/item/ItemAddonProperties.hpp"
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/core/IScriptRuntime.hpp"
#include "common/mod/bedrock/addon/modules/MinecraftModuleFactory.hpp"
#include "common/network/backend/java/mappings/JavaItemIdMap.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <memory>
#include <string>
#include <gtest/gtest.h>

using namespace mc::mod::bedrock::addon;

namespace {
class AddonItemPropertiesTest : public testing::Test {
protected:
    void SetUp() override
    {
        mc::Items::ensureInitialized();
        sword = mc::ItemRegistry::instance().getItem(mc::ResourceLocation("minecraft:diamond_sword"));
        stick = mc::ItemRegistry::instance().getItem(mc::ResourceLocation("minecraft:stick"));
        ASSERT_NE(sword, nullptr);
        ASSERT_NE(stick, nullptr);
    }

    const mc::Item* sword = nullptr;
    const mc::Item* stick = nullptr;
};

class AddonItemPropertiesBindingTest : public AddonItemPropertiesTest {
protected:
    void SetUp() override
    {
        AddonItemPropertiesTest::SetUp();
        engine = createScriptEngine();
        ASSERT_TRUE(engine->initialize());
        subject = mc::ItemStack(*sword, 1);
        first = createContext("first-pack");
        second = createContext("second-pack");
        ASSERT_NE(first, nullptr);
        ASSERT_NE(second, nullptr);
    }

    void TearDown() override
    {
        second.reset();
        first.reset();
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
        const auto itemId = binding.classRegistry().classIdByName("ItemStack");
        void* item = ScriptObjectRegistry::wrap(
            binding, itemId, binding.classRegistry().proto(itemId), &subject, false, "ItemStack", nullptr, 0, nullptr);
        binding.setProperty(global, "subject", item);
        binding.releaseValue(item);
        const auto containerId = binding.classRegistry().classIdByName("Container");
        void* container = ScriptObjectRegistry::wrap(binding,
            containerId,
            binding.classRegistry().proto(containerId),
            static_cast<mc::IInventory*>(&inventory),
            false,
            "Container",
            nullptr,
            0,
            nullptr);
        binding.setProperty(global, "inventory", container);
        binding.releaseValue(container);
        binding.releaseValue(global);
        return context;
    }

    void check(IScriptContext& context, const std::string& source)
    {
        const auto result = context.evaluateModule(source, "item-properties-" + std::to_string(nextModule++) + ".js");
        ASSERT_TRUE(result.success()) << result.errorMessage();
    }

    mc::ItemStack subject;
    mc::CraftingInventory inventory{2, 2};
    std::unique_ptr<IScriptEngine> engine;
    std::unique_ptr<IScriptContext> first;
    std::unique_ptr<IScriptContext> second;
    int nextModule = 0;
};
} // namespace

TEST_F(AddonItemPropertiesTest, NbtRoundTripPreservesEveryTypeAndOtherCustomData)
{
    mc::ItemStack item(*sword, 1);
    item.getOrCreateTag()["existing"] = "kept";
    AddonDynamicProperties values;
    ASSERT_TRUE(values
            .setMany("first-pack",
                {{"flag", true}, {"number", 1.5}, {"text", "女仆"}, {"position", {{"x", 1}, {"y", 2}, {"z", 3}}}})
            .success());
    ASSERT_TRUE(values.set("second-pack", "flag", false).success());
    ASSERT_TRUE(ItemAddonProperties::write(item, values).success());
    mc::nbt::tags::compound_tag saved;
    item.toNbt(saved);
    auto restored = mc::ItemStack::fromNbt(saved);
    ASSERT_TRUE(restored.success()) << restored.error().message();
    auto loaded = ItemAddonProperties::read(restored.value());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
    EXPECT_EQ(loaded.value().save(), values.save());
    EXPECT_TRUE(loaded.value().get("first-pack", "flag")->is_boolean());
    ASSERT_NE(restored.value().getTag(), nullptr);
    EXPECT_EQ((*restored.value().getTag())["existing"], "kept");
}

TEST_F(AddonItemPropertiesTest, WireComponentRoundTripPreservesDynamicPropertyTypes)
{
    ASSERT_TRUE(mc::network::backend::java::JavaItemIdMap::instance().initialize().success());
    mc::ItemStack item(*sword, 1);
    AddonDynamicProperties values;
    ASSERT_TRUE(values
            .setMany("pack",
                {{"flag", false}, {"number", 9}, {"text", "女仆"}, {"vector", {{"x", 1.5}, {"y", 2}, {"z", -3}}}})
            .success());
    ASSERT_TRUE(ItemAddonProperties::write(item, values).success());
    const auto wire = mc::network::ir::toItemStackView(item);
    const auto restored = mc::network::ir::fromItemStackView(wire);
    ASSERT_TRUE(restored.success()) << restored.error().message();
    auto loaded = ItemAddonProperties::read(restored.value());
    ASSERT_TRUE(loaded.success()) << loaded.error().message();
    EXPECT_EQ(loaded.value().save(), values.save());
}

TEST_F(AddonItemPropertiesTest, CopyAndContainerTransfersHaveIndependentValueState)
{
    mc::ItemStack item(*sword, 1);
    AddonDynamicProperties values;
    ASSERT_TRUE(values.set("pack", "owner", "original").success());
    ASSERT_TRUE(ItemAddonProperties::write(item, values).success());
    mc::CraftingInventory inventory(2, 2);
    inventory.setItem(0, item);
    auto copied = item.copy();
    ASSERT_TRUE(values.set("pack", "owner", "changed").success());
    ASSERT_TRUE(ItemAddonProperties::write(copied, values).success());
    auto originalValues = ItemAddonProperties::read(item);
    ASSERT_TRUE(originalValues.success());
    EXPECT_EQ(originalValues.value().get("pack", "owner"), nlohmann::json("original"));
    auto moved = inventory.removeItemNoUpdate(0);
    auto movedValues = ItemAddonProperties::read(moved);
    ASSERT_TRUE(movedValues.success());
    EXPECT_EQ(movedValues.value().save(), originalValues.value().save());
    inventory.setItem(1, copied);
    EXPECT_EQ(ItemAddonProperties::read(inventory.getItem(1)).value().get("pack", "owner"), nlohmann::json("changed"));
}

TEST_F(AddonItemPropertiesTest, EmptyAndStackableItemsRejectWritesWithoutAllocation)
{
    mc::ItemStack empty;
    mc::ItemStack stackable(*stick, 1);
    AddonDynamicProperties values;
    ASSERT_TRUE(values.set("pack", "flag", true).success());
    EXPECT_TRUE(ItemAddonProperties::read(empty).failed());
    EXPECT_TRUE(ItemAddonProperties::write(empty, values).failed());
    EXPECT_TRUE(ItemAddonProperties::write(stackable, values).failed());
    EXPECT_FALSE(stackable.hasTag());
    auto loaded = ItemAddonProperties::read(stackable);
    ASSERT_TRUE(loaded.success());
    EXPECT_TRUE(loaded.value().save().empty());
}

TEST_F(AddonItemPropertiesTest, DeletionRemovesOnlyTheAddonData)
{
    mc::ItemStack item(*sword, 1);
    item.getOrCreateTag()["other"] = 42;
    AddonDynamicProperties values;
    ASSERT_TRUE(values.set("pack", "flag", true).success());
    ASSERT_TRUE(ItemAddonProperties::write(item, values).success());
    values.clear("pack");
    ASSERT_TRUE(ItemAddonProperties::write(item, values).success());
    ASSERT_NE(item.getTag(), nullptr);
    EXPECT_EQ(*item.getTag(), nlohmann::json({{"other", 42}}));
    item.removeChildTag("other");
    EXPECT_FALSE(item.hasTag());
}

TEST_F(AddonItemPropertiesTest, MalformedSavedPropertiesAreReported)
{
    mc::ItemStack item(*sword, 1);
    item.getOrCreateTag()["cubium:addon_dynamic_properties"] = {
        {"pack", {{"bad", {{"kind", "boolean"}, {"value", 9}}}}}};
    const auto before = item.toJson();
    EXPECT_TRUE(ItemAddonProperties::read(item).failed());
    EXPECT_EQ(item.toJson(), before);
}

TEST_F(AddonItemPropertiesBindingTest, PackIsolationAndCloneIndependence)
{
    check(*first, R"(
        subject.setDynamicProperties({same: 'first', flag: true, vector: {x: 1, y: 2, z: 3}});
        globalThis.copied = subject.clone();
        copied.setDynamicProperty('same', 'copy');
        if (subject.getDynamicProperty('same') !== 'first') throw Error('clone shared data');
        if (copied.getDynamicProperty('same') !== 'copy') throw Error('clone write missing');
    )");
    check(*second, R"(
        if (subject.getDynamicProperty('same') !== undefined) throw Error('cross-pack read');
        subject.setDynamicProperty('same', 'second');
        subject.clearDynamicProperties();
    )");
    check(*first, R"(
        if (subject.getDynamicProperty('same') !== 'first') throw Error('cross-pack clear');
        if (subject.getDynamicPropertyIds().length !== 3) throw Error('missing keys');
        subject.setDynamicProperties({same: undefined, flag: null});
        subject.setDynamicProperty('vector');
        if (subject.getDynamicPropertyIds().length) throw Error('deletion failed');
    )");
}

TEST_F(AddonItemPropertiesBindingTest, ContainerItemsRemainCopiesUntilExplicitlyWrittenBack)
{
    check(*first, R"(
        subject.setDynamicProperty('state', 'original');
        inventory.setItem(0, subject);
        const copy = inventory.getItem(0);
        copy.setDynamicProperty('state', 'changed');
        if (inventory.getItem(0).getDynamicProperty('state') !== 'original') throw Error('copy changed container');
        inventory.setItem(0, copy);
        if (inventory.getItem(0).getDynamicProperty('state') !== 'changed') throw Error('writeback lost state');
        if (subject.getDynamicProperty('state') !== 'original') throw Error('container changed source');
    )");
}

TEST_F(AddonItemPropertiesBindingTest, StackableItemsAndInvalidValuesRejectWithoutChangingData)
{
    check(*first, R"(
        import { ItemStack } from '@minecraft/server';
        const stackable = new ItemStack('minecraft:stick', 1);
        const operations = [() => stackable.setDynamicProperty('flag', true),
            () => stackable.setDynamicProperties({flag: true}),
            () => subject.setDynamicProperties({good: true, bad: 'x'.repeat(32768)}),
            () => subject.setDynamicProperty('bad', NaN),
            () => subject.setDynamicProperty('bad', {x: 1, y: 2}),
            () => subject.setDynamicProperty.call({}, 'bad', 1)];
        for (const operation of operations) {
            let caught = false;
            try { operation(); } catch { caught = true; }
            if (!caught) throw Error('invalid item write was accepted');
        }
        if (subject.getDynamicPropertyIds().length || stackable.getDynamicPropertyIds().length) throw Error('invalid data committed');
    )");
    EXPECT_FALSE(subject.hasTag());
}

TEST_F(AddonItemPropertiesBindingTest, ReentrantGettersPreserveInnerWritesAndOuterIntent)
{
    check(*first, R"(
        subject.setDynamicProperty('same', 1);
        subject.setDynamicProperties({
            get same() { subject.setDynamicProperty('same', 2); subject.setDynamicProperty('inner', true); return 1; },
            outer: true
        });
        if (subject.getDynamicProperty('same') !== 1) throw Error('outer intent was lost');
        if (subject.getDynamicProperty('inner') !== true) throw Error('inner write was overwritten');
        if (subject.getDynamicProperty('outer') !== true) throw Error('outer write missing');
        subject.setDynamicProperty('vector', {get x() { subject.setDynamicProperty('nested', true); return 1; }, y:2, z:3});
        if (subject.getDynamicProperty('nested') !== true) throw Error('vector getter write was overwritten');
    )");
}

TEST_F(AddonItemPropertiesBindingTest, GetterExceptionsPreserveOriginalValueAndExceptionIdentity)
{
    check(*first, R"(
        subject.setDynamicProperty('saved', true);
        const sentinel = {reason: 'getter failed'};
        let caught = false;
        try { subject.setDynamicProperties({good: true, get saved() { throw sentinel; }}); }
        catch (error) { if (error !== sentinel) throw Error('exception changed'); caught = true; }
        if (!caught) throw Error('exception swallowed');
        if (subject.getDynamicProperty('saved') !== true) throw Error('saved value changed');
        if (subject.getDynamicProperty('good') !== undefined) throw Error('partial commit');
    )");
}
