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

#include "common/network/backend/java/mappings/JavaItemIdMap.hpp"

#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/network/backend/java/generated/java_item_table.gen.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>

namespace mc::network::backend::java {

// ============================================================================
// 单例
// ============================================================================

JavaItemIdMap& JavaItemIdMap::instance()
{
    static JavaItemIdMap s_instance;
    return s_instance;
}

// ============================================================================
// 内部辅助
// ============================================================================

/**
 * @brief 项目用 1.16.5 旧名注册、vanilla 1.21.11 已改名/删除的 item 别名
 *
 * vanilla 1.21.11 重命名了部分 item，项目仍按旧名注册（参考 1.16.5）。这些 item 若不别名，
 * initialize() 在 vanilla 表查不到 → 兜底 air → 真客户端挖这些方块掉 air（无掉落）。
 * 别名把项目旧名映射到 vanilla 1.21.11 现名，使 lookupVanillaId 命中正确 vanilla id。
 *
 * 注：tripwire 在 vanilla 1.21.11 无对应 item（绊线方块无物品形态，只能由 tripwire_hook
 * 放置/破坏），故不别名，保持 air 兜底（与 vanilla 行为一致：破坏绊线不掉物品）。
 */
static const std::unordered_map<std::string, std::string>& itemLocationAliases()
{
    static const std::unordered_map<std::string, std::string> kAliases = {
        {"minecraft:grass", "minecraft:short_grass"},             // 旧名 grass → 1.21.11 short_grass(202)
        {"minecraft:lapis_lazuli_dye", "minecraft:lapis_lazuli"}, // 旧名 → 1.21.11 lapis_lazuli(900)
    };
    return kAliases;
}

/**
 * @brief 在预生成排序表里二分查找 item name 对应的 Java registry id
 *
 * 表项按 name 字典序排序,name 以 '\0' 结尾存于扁平字符串池。
 * @return 找到返回 vanilla id,找不到返回 nullopt。
 */
static std::optional<u32> lookupVanillaId(const std::string& itemLocation)
{
    using generated::itemIdEntries;
    using generated::itemIdEntriesCount;
    using generated::ItemIdEntry;
    using generated::itemNamePool;

    const auto* entries = itemIdEntries();
    const size_t count = itemIdEntriesCount();
    if (entries == nullptr || count == 0) {
        return std::nullopt;
    }
    const char* pool = itemNamePool();

    // 先查别名表：项目旧名 → vanilla 1.21.11 现名。
    const std::string& effective = [&]() -> const std::string& {
        if (const auto it = itemLocationAliases().find(itemLocation); it != itemLocationAliases().end()) {
            return it->second;
        }
        return itemLocation;
    }();

    // lower_bound:找第一个 entry.name >= target(用 strcmp 比较 \0 结尾的 name)。
    const auto cmp = [pool](const ItemIdEntry& entry, const std::string& target) {
        return std::strcmp(pool + entry.nameOffset, target.c_str()) < 0;
    };
    const auto* it = std::lower_bound(entries, entries + count, effective, cmp);
    if (it == entries + count) {
        return std::nullopt;
    }
    if (std::strcmp(pool + it->nameOffset, effective.c_str()) != 0) {
        return std::nullopt; // 不相等,未命中
    }
    return it->vanillaId;
}

// ============================================================================
// 公开接口
// ============================================================================

Result<void> JavaItemIdMap::initialize()
{
    m_initialized = false;
    m_nameToJava.clear();
    m_javaToInternal.clear();
    m_airInternalId = 0;

    if (generated::itemIdEntriesCount() == 0) {
        return Error(ErrorCode::InvalidData, "JavaItemIdMap: generated item table is empty");
    }

    // 取项目 air 的真实内部 id 作所有 miss 的兜底（含 toJavaRegistryId 反查不到的旧名 item、
    // fromJavaRegistryId 越界 id）。项目 ItemId 0 是无效占位（ItemRegistry 保留 ID 0），
    // air 实际内部 id ≥1；若直接返回 0，getItem(0) 返 nullptr → fromItemStackView 走
    // InvalidItem 错误路径，真客户端收任意越界/未知 id 即崩。故须用 air 真实内部 id 兜底。
    if (const ::mc::Item* airItem = ::mc::ItemRegistry::instance().getItem(::mc::ResourceLocation("minecraft:air"));
        airItem != nullptr) {
        m_airInternalId = airItem->itemId();
    }

    size_t matched = 0;
    size_t fallback = 0;
    ::mc::ItemRegistry::instance().forEachItem([&](::mc::Item& item) {
        const std::string name = item.itemLocation().toString();
        if (auto javaId = lookupVanillaId(name)) {
            m_nameToJava[name] = *javaId;
            // 反向:vanilla id → 项目内部 ItemId。后注册的同 vanilla id 覆盖前者(一般唯一)。
            m_javaToInternal[*javaId] = item.itemId();
            ++matched;
        } else {
            ++fallback;
            spdlog::warn("JavaItemIdMap: item '{}' (internal id={}) has no Java registry entry, "
                         "will fall back to air on wire",
                name,
                item.itemId());
        }
    });

    // 确保 air(vanilla id 0)的反向映射指向项目 air 内部 id:即便项目 air item 名命中失败,
    // 也要让 fromJavaRegistryId(0) 返回项目 air 的内部 id 而非未初始化的 0 占位。
    m_javaToInternal.try_emplace(0, m_airInternalId);

    spdlog::info("JavaItemIdMap: matched {} items, {} fell back to air", matched, fallback);

    m_initialized = true;
    return {};
}

u32 JavaItemIdMap::toJavaRegistryId(const ::mc::Item& item) const
{
    return toJavaRegistryId(item.itemLocation().toString());
}

u32 JavaItemIdMap::toJavaRegistryId(std::string_view itemLocation) const
{
    if (!m_initialized) {
        // 防御:漏初始化时自动建表(幂等),避免全发 id 0。
        (void)const_cast<JavaItemIdMap*>(this)->initialize();
    }
    const std::string key(itemLocation);
    if (const auto it = m_nameToJava.find(key); it != m_nameToJava.end()) {
        return it->second;
    }
    spdlog::warn("JavaItemIdMap: toJavaRegistryId miss for item {}", itemLocation);
    return 0; // air 兜底
}

::mc::ItemId JavaItemIdMap::fromJavaRegistryId(u32 javaRegistryId) const
{
    if (!m_initialized) {
        return m_airInternalId; // air 的项目内部 id
    }
    if (const auto it = m_javaToInternal.find(javaRegistryId); it != m_javaToInternal.end()) {
        return it->second;
    }
    spdlog::warn("JavaItemIdMap: fromJavaRegistryId miss for javaRegistryId={}", javaRegistryId);
    return m_airInternalId; // air 兜底（项目 air 真实内部 id，非 0 占位）
}

} // namespace mc::network::backend::java
