/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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

// RegistryDataBuilder 单元测试。
// buildConfigurationRegistryData() 发送 23 个 RegistryDataLoader.SYNCHRONIZED_REGISTRIES
// （1.21.11）。除 enchantment 外所有条目 data=nullopt（声明客户端已知，依赖 SelectKnownPacks
// 命中 minecraft:core）。enchantment 发【内联 NBT】（见 EnchantmentNbtBuilder），但本测试
// 不注册 datapack 源也不初始化 ItemTags，故 enchantment 条目经缓存路径返回空列表
// （buildEnchantmentRegistryEntries 在 datapack 源未注册时记 error 并返回空），不影响对
// 其余 22 个注册表的结构断言。enchantment 内联 NBT 的正确性由 test_enchantment_nbt_builder
// 专项覆盖。
// buildConfigurationUpdateTags() 发 timeline（4 tag）+ dialog（2 空 tag）。
// buildServerKnownPacks() 返单包 minecraft:core 1.21.11。

#include "server/network/RegistryDataBuilder.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

using namespace mc::server::net;
using namespace mc::network::ir::configuration;

namespace {
bool hasRegistry(const std::vector<RegistryData>& registries, const std::string& key)
{
    return std::any_of(
        registries.begin(), registries.end(), [&](const RegistryData& r) { return r.registryKey == key; });
}
} // namespace

TEST(RegistryDataBuilder, SendsAllSynchronizedRegistries)
{
    auto registries = buildConfigurationRegistryData();
    // 对齐 Java 1.21.11 RegistryDataLoader.SYNCHRONIZED_REGISTRIES：23 个动态注册表。
    ASSERT_EQ(registries.size(), 23u);
    EXPECT_EQ(registries[0].registryKey, "minecraft:dimension_type");
    EXPECT_EQ(registries[1].registryKey, "minecraft:biome");
    // 关键注册表存在性（enchantment/dialog/timeline 是本任务修复目标）。
    EXPECT_TRUE(hasRegistry(registries, "minecraft:enchantment"));
    EXPECT_TRUE(hasRegistry(registries, "minecraft:dialog"));
    EXPECT_TRUE(hasRegistry(registries, "minecraft:timeline"));
    EXPECT_TRUE(hasRegistry(registries, "minecraft:painting_variant"));
    EXPECT_TRUE(hasRegistry(registries, "minecraft:damage_type"));
    EXPECT_TRUE(hasRegistry(registries, "minecraft:banner_pattern"));
}

TEST(RegistryDataBuilder, NonEnchantmentEntriesAreKnownNoNbt)
{
    // 除 enchantment 外所有条目 data=nullopt（声明客户端已知）。enchantment 发内联 NBT，
    // 其 data 形态由专项测试覆盖；此处跳过 enchantment 注册表。
    auto registries = buildConfigurationRegistryData();
    for (const auto& reg : registries) {
        if (reg.registryKey == "minecraft:enchantment") {
            continue;
        }
        ASSERT_FALSE(reg.entries.empty()) << "registry " << reg.registryKey << " 不应为空";
        for (const auto& entry : reg.entries) {
            EXPECT_FALSE(entry.id.empty()) << "registry " << reg.registryKey << " 含空 id";
            EXPECT_EQ(entry.data, std::nullopt)
                << "registry " << reg.registryKey << " entry " << entry.id << " 应为 data=nullopt";
        }
    }
}

TEST(RegistryDataBuilder, DialogRegistryHasThreeEntries)
{
    auto registries = buildConfigurationRegistryData();
    auto it = std::find_if(registries.begin(), registries.end(), [](const RegistryData& r) {
        return r.registryKey == "minecraft:dialog";
    });
    ASSERT_NE(it, registries.end());
    // dialog: quick_actions / custom_options / server_links（对齐 1.21.11 vanilla datapack）。
    EXPECT_EQ(it->entries.size(), 3u);
    // dialog 仍走 data=nullopt（客户端从本地 core 包加载），其 tag 由 UpdateTags 绑定。
    for (const auto& entry : it->entries) {
        EXPECT_EQ(entry.data, std::nullopt);
    }
}

TEST(RegistryDataBuilder, UpdateTagsSendsTimelineAndDialog)
{
    auto tags = buildConfigurationUpdateTags();
    ASSERT_EQ(tags.size(), 2u);
    // timeline: 4 tag（universal/in_overworld/in_nether/in_end）
    auto timelineIt = std::find_if(
        tags.begin(), tags.end(), [](const TagRegistry& t) { return t.registryKey == "minecraft:timeline"; });
    ASSERT_NE(timelineIt, tags.end());
    EXPECT_EQ(timelineIt->tags.size(), 4u);
    // dialog: 2 空 tag（pause_screen_additions/quick_actions，空 id 列表触发 bindTag 绑定）
    auto dialogIt = std::find_if(
        tags.begin(), tags.end(), [](const TagRegistry& t) { return t.registryKey == "minecraft:dialog"; });
    ASSERT_NE(dialogIt, tags.end());
    EXPECT_EQ(dialogIt->tags.size(), 2u);
    for (const auto& tag : dialogIt->tags) {
        EXPECT_TRUE(tag.elementIds.empty()) << "dialog tag " << tag.tagName << " 应为空 id 列表";
    }
}

TEST(RegistryDataBuilder, KnownPacksIsCoreOnly)
{
    auto packs = buildServerKnownPacks();
    ASSERT_EQ(packs.size(), 1u);
    EXPECT_EQ(packs[0].ns, "minecraft");
    EXPECT_EQ(packs[0].id, "core");
    EXPECT_EQ(packs[0].version, "1.21.11");
}
