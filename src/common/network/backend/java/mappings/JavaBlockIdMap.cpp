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

#include "common/network/backend/java/mappings/JavaBlockIdMap.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/backend/java/generated/java_block_id_table.gen.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mc::network::backend::java {

// ============================================================================
// 单例
// ============================================================================

JavaBlockIdMap& JavaBlockIdMap::instance()
{
    static JavaBlockIdMap s_instance;
    return s_instance;
}

// ============================================================================
// 内部辅助
// ============================================================================

/**
 * @brief 在预生成排序表里二分查找 block name 对应的 Java registry id
 *
 * 表项按 name 字典序排序,name 以 '\0' 结尾存于扁平字符串池。
 * @return 找到返回 vanilla id,找不到返回 nullopt。
 */
static std::optional<u32> lookupVanillaId(const std::string& blockLocation)
{
    using generated::blockIdEntries;
    using generated::blockIdEntriesCount;
    using generated::BlockIdEntry;
    using generated::blockNamePool;

    const auto* entries = blockIdEntries();
    const size_t count = blockIdEntriesCount();
    if (entries == nullptr || count == 0) {
        return std::nullopt;
    }
    const char* pool = blockNamePool();

    // lower_bound:找第一个 entry.name >= target(用 strcmp 比较 \0 结尾的 name)。
    const auto cmp = [pool](const BlockIdEntry& entry, const std::string& target) {
        return std::strcmp(pool + entry.nameOffset, target.c_str()) < 0;
    };
    const auto* it = std::lower_bound(entries, entries + count, blockLocation, cmp);
    if (it == entries + count) {
        return std::nullopt;
    }
    if (std::strcmp(pool + it->nameOffset, blockLocation.c_str()) != 0) {
        return std::nullopt; // 不相等,未命中
    }
    return it->vanillaId;
}

// ============================================================================
// 公开接口
// ============================================================================

Result<void> JavaBlockIdMap::initialize()
{
    m_initialized = false;
    m_nameToJava.clear();
    m_javaToInternal.clear();
    m_internalToJava.clear();

    if (generated::blockIdEntriesCount() == 0) {
        return Error(ErrorCode::InvalidData, "JavaBlockIdMap: generated block id table is empty");
    }

    // 探测 vanilla id 上界,预分配反向稠密数组(下标 = vanilla id,0..maxId 连续)。
    // miss 兜底 0(air):反向数组默认填 0,即项目 air 的内部 blockId(air 内部 id 也是 0)。
    size_t maxId = 0;
    const auto* entries = generated::blockIdEntries();
    const size_t count = generated::blockIdEntriesCount();
    for (size_t i = 0; i < count; ++i) {
        if (entries[i].vanillaId > maxId) {
            maxId = entries[i].vanillaId;
        }
    }
    m_javaToInternal.assign(maxId + 1, 0); // 0 = air 兜底

    size_t matched = 0;
    size_t fallback = 0;
    size_t maxInternalId = 0;
    // 命中的 (internalBlockId, javaId) 暂存,待正向稠密表预分配后批量填入。
    struct Pending {
        u32 internalBlockId;
        u32 javaId;
    };
    std::vector<Pending> pendings;
    ::mc::Block::forEachBlock([&](::mc::Block& block) {
        if (block.blockId() > maxInternalId) {
            maxInternalId = block.blockId();
        }
        const std::string name = block.blockLocation().toString();
        if (auto javaId = lookupVanillaId(name)) {
            m_nameToJava[name] = *javaId;
            // 反向:vanilla id → 项目内部 blockId。后注册的同 vanilla id 覆盖前者(一般唯一)。
            if (*javaId < m_javaToInternal.size()) {
                m_javaToInternal[*javaId] = block.blockId();
            }
            pendings.push_back({block.blockId(), *javaId});
            ++matched;
        } else {
            ++fallback;
            spdlog::warn("JavaBlockIdMap: block '{}' (internal id={}) has no Java registry entry, "
                         "will fall back to air on wire",
                name,
                block.blockId());
        }
    });

    // 正向稠密表:内部 blockId → vanilla id。下标 = 内部 blockId(连续稠密 0..maxInternalId)。
    // miss 兜底 0(air)。
    m_internalToJava.assign(maxInternalId + 1, 0);
    for (const auto& p : pendings) {
        if (p.internalBlockId < m_internalToJava.size()) {
            m_internalToJava[p.internalBlockId] = p.javaId;
        }
    }

    spdlog::info("JavaBlockIdMap: matched {} blocks, {} fell back to air", matched, fallback);

    m_initialized = true;
    return {};
}

u32 JavaBlockIdMap::toJavaRegistryId(const ::mc::Block& block) const
{
    return toJavaRegistryId(block.blockLocation().toString());
}

u32 JavaBlockIdMap::toJavaRegistryId(u32 internalBlockId) const
{
    if (!m_initialized) {
        // 防御:漏初始化时自动建表(幂等),避免全发 id 0。
        (void)const_cast<JavaBlockIdMap*>(this)->initialize();
    }
    if (internalBlockId < m_internalToJava.size()) {
        return m_internalToJava[internalBlockId];
    }
    spdlog::warn("JavaBlockIdMap: toJavaRegistryId(internalBlockId={}) out of range", internalBlockId);
    return 0; // air 兜底
}

u32 JavaBlockIdMap::toJavaRegistryId(std::string_view blockLocation) const
{
    if (!m_initialized) {
        // 防御:漏初始化时自动建表(幂等),避免全发 id 0。
        (void)const_cast<JavaBlockIdMap*>(this)->initialize();
    }
    const std::string key(blockLocation);
    if (const auto it = m_nameToJava.find(key); it != m_nameToJava.end()) {
        return it->second;
    }
    spdlog::warn("JavaBlockIdMap: toJavaRegistryId miss for block {}", blockLocation);
    return 0; // air 兜底
}

u32 JavaBlockIdMap::fromJavaRegistryId(u32 javaRegistryId) const
{
    if (!m_initialized) {
        return 0; // air 兜底
    }
    if (javaRegistryId < m_javaToInternal.size()) {
        return m_javaToInternal[javaRegistryId];
    }
    spdlog::warn("JavaBlockIdMap: fromJavaRegistryId miss for javaRegistryId={}", javaRegistryId);
    return 0; // air 兜底
}

} // namespace mc::network::backend::java
