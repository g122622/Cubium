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

#include "common/network/backend/java/mappings/JavaBlockStateIdMap.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/backend/java/generated/java_block_state_table.gen.hpp"
#include "common/util/property/IProperty.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>
#include <map>
#include <optional>
#include <string>

namespace mc::network::backend::java {

// ============================================================================
// 单例
// ============================================================================

JavaBlockStateIdMap& JavaBlockStateIdMap::instance()
{
    static JavaBlockStateIdMap s_instance;
    return s_instance;
}

// ============================================================================
// 内部辅助
// ============================================================================

/// 构造 (name, properties) → 查表用的统一键。
/// 形如 "minecraft:acacia_button|face=floor,facing=north,powered=true",
/// properties 按 name 字母序排列,与烘焙脚本 buildLookupKey 完全一致。
static std::string buildLookupKey(const std::string& name, const std::map<std::string, std::string>& props)
{
    std::string key = name;
    key.push_back('|');
    bool first = true;
    for (const auto& [k, v] : props) {
        if (!first) {
            key.push_back(',');
        }
        first = false;
        key += k;
        key.push_back('=');
        key += v;
    }
    return key;
}

/// 从 BlockState 导出 properties: name → value 字符串(用 IProperty::valueToString)。
static std::map<std::string, std::string> exportStateProperties(const ::mc::BlockState& state)
{
    std::map<std::string, std::string> props;
    for (const auto& entry : state.values()) {
        if (entry.property == nullptr) {
            continue;
        }
        props[entry.property->name()] = entry.property->valueToString(entry.valueIndex);
    }
    return props;
}

/**
 * @brief 在预生成排序表里二分查找 key 对应的 Java globalId
 *
 * @return 找到返回 globalId,找不到返回 nullopt。
 */
static std::optional<u32> lookupGlobalId(const std::string& key)
{
    using generated::blockStateIdEntries;
    using generated::blockStateIdEntriesCount;
    using generated::BlockStateIdEntry;
    using generated::blockStateKeyPool;

    const auto* entries = blockStateIdEntries();
    const size_t count = blockStateIdEntriesCount();
    if (entries == nullptr || count == 0) {
        return std::nullopt;
    }
    const char* pool = blockStateKeyPool();

    // lower_bound:找第一个 entry.key >= key(用 strcmp 比较 \0 结尾的 key)。
    const auto cmp = [pool](const BlockStateIdEntry& entry, const std::string& target) {
        return std::strcmp(pool + entry.keyOffset, target.c_str()) < 0;
    };
    const auto* it = std::lower_bound(entries, entries + count, key, cmp);
    if (it == entries + count) {
        return std::nullopt;
    }
    if (std::strcmp(pool + it->keyOffset, key.c_str()) != 0) {
        return std::nullopt; // 不相等,未命中
    }
    return it->globalId;
}

// ============================================================================
// 公开接口
// ============================================================================

Result<void> JavaBlockStateIdMap::initialize()
{
    m_initialized = false;
    m_toJava.clear();
    m_fromJava.clear();

    if (generated::blockStateIdEntriesCount() == 0) {
        return Error(ErrorCode::InvalidData, "JavaBlockStateIdMap: generated block state table is empty");
    }

    // 探测 globalId 上界以预分配 m_fromJava。遍历一次取最大 globalId。
    // globalId 连续 0..max,故 m_fromJava 大小 = max+1。
    u32 maxGlobalId = 0;
    const auto* genEntries = generated::blockStateIdEntries();
    const size_t genCount = generated::blockStateIdEntriesCount();
    for (size_t i = 0; i < genCount; ++i) {
        maxGlobalId = std::max(maxGlobalId, genEntries[i].globalId);
    }
    m_fromJava.assign(static_cast<size_t>(maxGlobalId) + 1, 0); // 兜底 air=0

    size_t matched = 0;
    size_t missing = 0;
    ::mc::Block::forEachBlockState([&](const ::mc::BlockState& state) {
        std::map<std::string, std::string> props = exportStateProperties(state);
        const std::string key = buildLookupKey(state.blockLocation().toString(), props);
        if (auto javaId = lookupGlobalId(key)) {
            const u32 internalId = state.stateId();
            if (internalId >= m_toJava.size()) {
                m_toJava.resize(static_cast<size_t>(internalId) + 1, 0); // 兜底 air=0
            }
            m_toJava[internalId] = *javaId;
            if (*javaId < m_fromJava.size()) {
                m_fromJava[*javaId] = internalId;
            }
            ++matched;
        } else {
            ++missing;
            spdlog::warn("JavaBlockStateIdMap: no Java globalId for internal state {} ({})", state.stateId(), key);
        }
    });

    // 主动 shrink_to_fit,释放预留多余容量,常驻内存最小化。
    m_toJava.shrink_to_fit();
    m_fromJava.shrink_to_fit();

    spdlog::info("JavaBlockStateIdMap: matched {} states, {} missing Java globalId (from {} generated entries)",
        matched,
        missing,
        genCount);

    m_initialized = true;
    return {};
}

u32 JavaBlockStateIdMap::toJavaGlobalId(u32 internalStateId) const
{
    if (!m_initialized) {
        spdlog::warn("JavaBlockStateIdMap: not initialized, returning air(0)");
        return 0;
    }
    if (internalStateId < m_toJava.size()) {
        return m_toJava[internalStateId];
    }
    spdlog::warn("JavaBlockStateIdMap: toJavaGlobalId miss for internal stateId={}", internalStateId);
    return 0; // air 兜底
}

u32 JavaBlockStateIdMap::fromJavaGlobalId(u32 javaGlobalId) const
{
    if (!m_initialized) {
        return 0;
    }
    if (javaGlobalId < m_fromJava.size()) {
        return m_fromJava[javaGlobalId];
    }
    spdlog::warn("JavaBlockStateIdMap: fromJavaGlobalId miss for javaGlobalId={}", javaGlobalId);
    return 0; // air 兜底
}

} // namespace mc::network::backend::java
