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

#include "BlockRegistry.hpp"

#include "common/util/assert/AssertMacros.hpp"
#include "world/block/Block.hpp"
#include "world/block/registry/HarvestToolTable.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace mc {

BlockRegistry& BlockRegistry::instance()
{
    static BlockRegistry instance;
    return instance;
}

void BlockRegistry::applyHarvestToolTable()
{
    // 把原版的两组数据写回方块属性：
    //   1. requiresCorrectToolForDrops —— 原版注册时硬编码，这里按实测快照校正；
    //   2. harvestTool / harvestLevel   —— 来自数据包 mineable/<tool> 与 needs_*_tool 标签。
    // 手写注册值长期与这两组数据无声漂移（首次 can_harvest 差分查出 411 个方块里
    // 有 462 处判定错误），因此统一以数据为准覆盖。
    std::size_t requiresCount = 0;
    for (auto& [id, block] : m_blocks) {
        block->m_requiresTool = block_registry::HarvestToolTable::requiresCorrectToolForDrops(id);
        if (block->m_requiresTool) {
            ++requiresCount;
        }

        // 只有需要工具的方块才有「用什么工具、哪一档」可言；其余方块保留零值，
        // 避免 isToolEffective 之类的旧判定被无关数据污染。
        const block_registry::HarvestToolRuleSet* rules = block_registry::HarvestToolTable::find(id);
        if (!block->m_requiresTool || rules == nullptr || rules->count == 0) {
            block->m_harvestTool = HarvestTool::None;
            block->m_harvestLevel = 0;
            continue;
        }
        if (rules->count > 1) {
            const std::string message =
                fmt::format("block {} belongs to {} mineable/<tool> tags, but Block stores a single harvest tool",
                    id.toString(),
                    static_cast<int>(rules->count));
            MC_ASSERT_RELEASE_MSG(false, message.c_str());
        }
        block->m_harvestTool = rules->entries[0].tool;
        block->m_harvestLevel = rules->entries[0].level;
    }

    // 反向自检：需要工具的方块通常应在某个 mineable 标签里有对应工具。
    // 但原版确实存在少数例外（如 command_block / chain_command_block：要求正确工具、
    // 却不在任何 mineable 标签里，生存中被判定为「任何工具都挖不动」），因此这里只告警。
    for (auto& [id, block] : m_blocks) {
        if (!block->m_requiresTool) {
            continue;
        }
        if (block_registry::HarvestToolTable::find(id) == nullptr) {
            spdlog::debug("BlockRegistry: block {} requires the correct tool but is in no mineable/<tool> tag "
                          "(vanilla behaves the same: nothing can harvest it)",
                id.toString());
        }
    }
    spdlog::info("BlockRegistry: harvest rules applied ({} blocks require the correct tool)", requiresCount);
}

} // namespace mc
