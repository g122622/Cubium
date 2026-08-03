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

#include "common/world/blockentity/JavaBlockEntityTypeIdMap.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"

#include <spdlog/spdlog.h>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace mc {

// ============================================================================
// 单例
// ============================================================================

JavaBlockEntityTypeIdMap& JavaBlockEntityTypeIdMap::instance()
{
    static JavaBlockEntityTypeIdMap s_instance;
    return s_instance;
}

// ============================================================================
// vanilla 1.21.11 block_entity_type 注册顺序（BlockEntityType.java 静态字段声明顺序）
// registry id = 下标。提取自 1.21.11 反编译源码 BuiltInRegistries.BLOCK_ENTITY_TYPE 注册顺序。
// 该注册表未由本项目 RegistryDataBuilder 同步，真 Java 客户端用内置 core 包，id 即此顺序。
// ============================================================================
static constexpr std::array<std::string_view, 49> kVanillaBlockEntityTypeNames = {
    "minecraft:furnace",                 // 0
    "minecraft:chest",                   // 1
    "minecraft:trapped_chest",           // 2
    "minecraft:ender_chest",             // 3
    "minecraft:jukebox",                 // 4
    "minecraft:dispenser",               // 5
    "minecraft:dropper",                 // 6
    "minecraft:sign",                    // 7
    "minecraft:hanging_sign",            // 8
    "minecraft:mob_spawner",             // 9
    "minecraft:creaking_heart",          // 10
    "minecraft:piston",                  // 11
    "minecraft:brewing_stand",           // 12
    "minecraft:enchanting_table",        // 13
    "minecraft:end_portal",              // 14
    "minecraft:beacon",                  // 15
    "minecraft:skull",                   // 16
    "minecraft:daylight_detector",       // 17
    "minecraft:hopper",                  // 18
    "minecraft:comparator",              // 19
    "minecraft:banner",                  // 20
    "minecraft:structure_block",         // 21
    "minecraft:end_gateway",             // 22
    "minecraft:command_block",           // 23
    "minecraft:shulker_box",             // 24
    "minecraft:bed",                     // 25
    "minecraft:conduit",                 // 26
    "minecraft:barrel",                  // 27
    "minecraft:smoker",                  // 28
    "minecraft:blast_furnace",           // 29
    "minecraft:lectern",                 // 30
    "minecraft:bell",                    // 31
    "minecraft:jigsaw",                  // 32
    "minecraft:campfire",                // 33
    "minecraft:beehive",                 // 34
    "minecraft:sculk_sensor",            // 35
    "minecraft:calibrated_sculk_sensor", // 36
    "minecraft:sculk_catalyst",          // 37
    "minecraft:sculk_shrieker",          // 38
    "minecraft:chiseled_bookshelf",      // 39
    "minecraft:shelf",                   // 40
    "minecraft:brushable_block",         // 41
    "minecraft:decorated_pot",           // 42
    "minecraft:crafter",                 // 43
    "minecraft:trial_spawner",           // 44
    "minecraft:vault",                   // 45
    "minecraft:test_block",              // 46
    "minecraft:test_instance_block",     // 47
    "minecraft:copper_golem_statue",     // 48
};

// ============================================================================
// 公开接口
// ============================================================================

Result<void> JavaBlockEntityTypeIdMap::initialize()
{
    m_initialized = false;
    m_nameToId.clear();
    m_idToType.clear();

    // name → registry id
    m_nameToId.reserve(kVanillaBlockEntityTypeNames.size());
    for (u32 i = 0; i < kVanillaBlockEntityTypeNames.size(); ++i) {
        m_nameToId[std::string(kVanillaBlockEntityTypeNames[i])] = i;
    }

    // 遍历项目所有 BlockEntityType 枚举值（Unknown..Count-1），经 blockEntityTypeToId 取
    // ResourceLocation 查 vanilla 表，建 registry id → BlockEntityType 反向映射。
    // 正向（BlockEntityType → id）在 toJavaRegistryId 里即时查 m_nameToId，无需缓存。
    size_t matched = 0;
    size_t missing = 0;
    for (int raw = static_cast<int>(BlockEntityType::Unknown) + 1; raw < static_cast<int>(BlockEntityType::Count);
        ++raw) {
        const auto type = static_cast<BlockEntityType>(raw);
        const ResourceLocation rl = blockEntityTypeToId(type);
        const std::string key = rl.toString();
        if (const auto it = m_nameToId.find(key); it != m_nameToId.end()) {
            m_idToType[it->second] = type;
            ++matched;
        } else {
            ++missing;
            spdlog::warn("JavaBlockEntityTypeIdMap: no vanilla registry id for block entity type {} ({})",
                static_cast<int>(type),
                key);
        }
    }

    spdlog::info("JavaBlockEntityTypeIdMap: matched {} block entity types, {} missing", matched, missing);

    m_initialized = true;
    return {};
}

u32 JavaBlockEntityTypeIdMap::toJavaRegistryId(BlockEntityType type) const
{
    if (!m_initialized) {
        spdlog::warn("JavaBlockEntityTypeIdMap: not initialized, returning furnace(0)");
        return 0;
    }
    const ResourceLocation rl = blockEntityTypeToId(type);
    const std::string key = rl.toString();
    if (const auto it = m_nameToId.find(key); it != m_nameToId.end()) {
        return it->second;
    }
    spdlog::warn(
        "JavaBlockEntityTypeIdMap: toJavaRegistryId miss for block entity type {} ({})", static_cast<int>(type), key);
    return 0; // furnace 兜底
}

BlockEntityType JavaBlockEntityTypeIdMap::fromJavaRegistryId(u32 javaRegistryId) const
{
    if (!m_initialized) {
        return BlockEntityType::Unknown;
    }
    if (const auto it = m_idToType.find(javaRegistryId); it != m_idToType.end()) {
        return it->second;
    }
    spdlog::warn("JavaBlockEntityTypeIdMap: fromJavaRegistryId miss for javaRegistryId={}", javaRegistryId);
    return BlockEntityType::Unknown;
}

} // namespace mc
