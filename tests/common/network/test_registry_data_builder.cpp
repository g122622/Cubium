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

// Phase3 阶段3：RegistryDataBuilder 单元测试。
// buildConfigurationRegistryData() 是纯自由函数，无依赖（不碰注册表），可独立回归。
// 8 个注册表 + 条目数 3/10/9/38/10/17/9/27（与 RegistryDataBuilder.cpp 实现一致），
// 每条 data=nullopt（声明客户端已知，依赖 SelectKnownPacks 命中 minecraft:core）。
// buildServerKnownPacks() 返单包 minecraft:core 1.21.11。

#include "server/network/RegistryDataBuilder.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace mc::server::net;
using namespace mc::network::ir::configuration;

TEST(RegistryDataBuilder, ReturnsEightRegistries)
{
    auto registries = buildConfigurationRegistryData();
    ASSERT_EQ(registries.size(), 8u);
    EXPECT_EQ(registries[0].registryKey, "minecraft:dimension_type");
    EXPECT_EQ(registries[1].registryKey, "minecraft:biome");
    EXPECT_EQ(registries[2].registryKey, "minecraft:chat_type");
    EXPECT_EQ(registries[3].registryKey, "minecraft:damage_type");
    EXPECT_EQ(registries[4].registryKey, "minecraft:trim_material");
    EXPECT_EQ(registries[5].registryKey, "minecraft:trim_pattern");
    EXPECT_EQ(registries[6].registryKey, "minecraft:wolf_variant");
    EXPECT_EQ(registries[7].registryKey, "minecraft:painting_variant");
}

TEST(RegistryDataBuilder, EntryCountsMatchVanilla)
{
    auto registries = buildConfigurationRegistryData();
    ASSERT_EQ(registries.size(), 8u);
    // 计数对齐 RegistryDataBuilder.cpp 的 initializer_list 大小。
    EXPECT_EQ(registries[0].entries.size(), 3u);  // dimension_type: overworld/nether/end
    EXPECT_EQ(registries[1].entries.size(), 10u); // biome: 10 代表性条目
    EXPECT_EQ(registries[2].entries.size(), 9u);  // chat_type: 9 类型
    EXPECT_EQ(registries[3].entries.size(), 39u); // damage_type: 39 类型
    EXPECT_EQ(registries[4].entries.size(), 10u); // trim_material: 10 材质
    EXPECT_EQ(registries[5].entries.size(), 17u); // trim_pattern: 17 纹样
    EXPECT_EQ(registries[6].entries.size(), 9u);  // wolf_variant: 9 变种
    EXPECT_EQ(registries[7].entries.size(), 27u); // painting_variant: 27 画作
}

TEST(RegistryDataBuilder, AllEntriesAreKnownNoNbt)
{
    // Phase4 策略：所有条目 data=nullopt，声明客户端已知（命中 minecraft:core 后合法）。
    auto registries = buildConfigurationRegistryData();
    for (const auto& reg : registries) {
        for (const auto& entry : reg.entries) {
            EXPECT_FALSE(entry.id.empty()) << "registry " << reg.registryKey << " 含空 id";
            EXPECT_EQ(entry.data, std::nullopt)
                << "registry " << reg.registryKey << " entry " << entry.id << " 应为 data=nullopt";
        }
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
