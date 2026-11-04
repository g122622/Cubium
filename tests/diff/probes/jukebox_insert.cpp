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

/**
 * @brief 差分能力：唱片机的**交互判定**（`JukeboxPlayable#tryInsertIntoJukebox` 的分支表）。
 *
 * `list`：手持物 × HAS_RECORD 的组合
 * `run` ：一行 -> { "action": "insert" | "eject" | "try_empty_hand" }
 *
 * ## 三种结果
 *
 * 右键唱片机只有三种可能：
 *
 * - **insert**：手里拿着唱片、机器空着 -> 塞进去并开始播放
 * - **eject**：机器里有唱片 -> 弹出唱片，任何手持物都一样（包括空手）
 * - **try_empty_hand**：手里不是唱片且机器空着 -> 交给空手逻辑，什么都不发生
 *
 * ## 顺序陷阱
 *
 * 「机器里有唱片」这一条**优先于「手里拿什么」**——即使手里拿着另一张唱片，
 * 右键也是先弹出旧的那张，不会替换。写反了就会变成「拿新唱片点一下就换掉旧的」，
 * 旧唱片凭空消失。
 *
 * 「机器空着但手里不是唱片」返回的是 `try_empty_hand` 而不是失败——原版靠这个让
 * 上层继续尝试空手交互（唱片机没有空手分支，最终什么都不发生）。
 *
 * ## 抄错的后果
 *
 * 唱片机是玩家高频交互的方块，判定错会立刻被感知：拿错物品把唱片顶掉、
 * 或者放着唱片再点一次却弹不出来。
 */

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include "vanilla_datapack.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureInitialized()
{
    static const bool s_initialized = []() {
        datapack::initializeRegistries();
        Items::initialize();
        return true;
    }();
    (void)s_initialized;
}

/// 固定样本：空手与一个非唱片物品
constexpr const char* FIXED_LABELS[] = {"empty", "stick"};
constexpr const char* FIXED_IDS[] = {"", "minecraft:stick"};

/** 注册表里全部「唱片物品」（`isMusicDisc()` 为真），按 id 排序保证顺序稳定 */
const std::vector<std::string>& musicDiscIds()
{
    static const std::vector<std::string> s_ids = []() {
        std::vector<std::string> ids;
        ItemRegistry::instance().forEachItem([&ids](Item& item) {
            if (item.isMusicDisc()) {
                ids.push_back(item.itemLocation().toString());
            }
        });
        std::sort(ids.begin(), ids.end());
        return ids;
    }();
    return s_ids;
}

void writeList(FILE* out)
{
    ensureInitialized();
    // 固定样本：空手 / 非唱片物品，各配「机器空着」与「机器有唱片」两态
    for (const char* label : FIXED_LABELS) {
        for (const char* hasRecord : {"false", "true"}) {
            std::fprintf(out, "%s:%s\n", label, hasRecord);
        }
    }
    // 全部唱片物品：两态都枚举（有唱片时无论手里拿什么都该弹旧的那张）
    for (const std::string& id : musicDiscIds()) {
        const std::string label = id.substr(id.find(':') + 1);
        for (const char* hasRecord : {"false", "true"}) {
            std::fprintf(out, "%s:%s\n", label.c_str(), hasRecord);
        }
    }
}

const Item* findItem(std::string_view id)
{
    if (id.empty()) {
        return nullptr;
    }
    return ItemRegistry::instance().getItem(ResourceLocation(std::string(id)));
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const std::size_t colon = unit.rfind(':');
    if (colon == std::string::npos) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    const std::string heldLabel = unit.substr(0, colon);
    const bool hasRecord = unit.substr(colon + 1) == "true";

    // 标签既可能是固定样本（empty/stick），也可能是某张唱片物品的路径部分
    std::string heldId;
    bool known = false;
    for (std::size_t i = 0; i < std::size(FIXED_LABELS); ++i) {
        if (heldLabel == FIXED_LABELS[i]) {
            heldId = FIXED_IDS[i];
            known = true;
            break;
        }
    }
    if (!known) {
        for (const std::string& id : musicDiscIds()) {
            if (id.substr(id.find(':') + 1) == heldLabel) {
                heldId = id;
                known = true;
                break;
            }
        }
    }
    if (!known) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    const Item* held = findItem(heldId);

    // 原版 `JukeboxBlock#useItemOn` 的分支顺序：
    //   1. HAS_RECORD 为真 -> TRY_WITH_EMPTY_HAND（交给 useWithoutItem 弹唱片）
    //   2. 否则看手持物：有 jukebox_playable 组件 -> 塞入；没有 -> TRY_WITH_EMPTY_HAND
    //
    // 「机器里有唱片」优先于「手里拿什么」：拿着另一张唱片点也是先弹旧的，不会替换。
    std::string action;
    if (hasRecord) {
        action = "eject";
    } else if (held != nullptr && held->isMusicDisc()) {
        action = "insert";
    } else {
        action = "try_empty_hand";
    }

    return json{{"unit", unit}, {"action", action}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(jukebox_insert, &writeList, &runLine)

} // namespace mc::diff
