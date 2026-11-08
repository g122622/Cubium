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
 * @file block_sound_type.cpp
 * @brief 差分能力：方块声音组（破坏 / 踩踏 / 放置 / 击打 / 坠落 五个音效事件 + 音量 + 音调）。
 *
 * `list`：全部方块 ID（另有饰纹陶罐的裂纹状态，见下）
 * `run` ：{ "block": "minecraft:stone" }
 *         → { "ok": true, "break": "minecraft:block.stone.break", "step": "...",
 *             "place": ..., "hit": ..., "fall": ..., "volume": 1.0, "pitch": 1.0 }
 *
 * ## 为什么值得比
 *
 * 声音组是「方块 → 五个音效事件 + 一对音量音调」的映射。它不参与任何游戏逻辑，因此
 * 错了不会崩溃、不会影响平衡——只会让玩家听到不该有的声音：木头踩出石头声、羊毛
 * 摔出金属声、陶罐碎了却还是普通的破碎声。这种错误只能靠「逐个方块比对」发现，人工
 * 听遍一千多个方块不现实。
 *
 * ## 单元粒度：默认状态 + 少数状态相关方块
 *
 * 原版声音组绝大多数挂在方块属性上、与状态无关，所以单元按方块 ID 走默认状态即可。
 * 但 `DecoratedPotBlock#getSoundType(BlockState)` 会按 `CRACKED` 切换，Cubium 此前
 * 完全没有这条链路（无参的 `getSoundType()` 拿不到状态），本能力顺带补齐。
 * 这类状态相关的方块以 `minecraft:decorated_pot#cracked=true` 的形式额外列举。
 */
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/resource/ResourceLocation.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/base/sound/BlockSoundType.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/state/BlockState.hpp"

#include "probe.hpp"
#include "vanilla_datapack.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 状态相关的额外单元：方块 ID + 状态后缀 + 状态定制函数。
struct StateVariant {
    std::string unit;
    const char* blockId;
    /// 在方块的默认状态上叠加变体属性；`state` 可能被替换为另一个状态对象。
    void (*apply)(const BlockState*& state);
};

void applyCracked(const BlockState*& state)
{
    state = &state->with(BlockStateProperties::CRACKED(), true);
}

const std::vector<StateVariant>& stateVariants()
{
    static const std::vector<StateVariant> s_variants = {
        {.unit = "minecraft:decorated_pot#cracked=true", .blockId = "minecraft:decorated_pot", .apply = &applyCracked},
    };
    return s_variants;
}

/// 全部单元：方块 ID（默认状态）在前，状态相关变体在后，均排序。
[[nodiscard]] const std::vector<std::string>& units()
{
    static std::vector<std::string> s_units;

    static bool s_initialised = false;
    if (!s_initialised) {
        s_initialised = true;

        datapack::initializeRegistries();

        BlockRegistry::instance().forEachBlock([](Block& block) {
            s_units.push_back(block.blockLocation().toString());
        });
        for (const auto& variant : stateVariants()) {
            s_units.push_back(variant.unit);
        }
        std::sort(s_units.begin(), s_units.end());
        s_units.erase(std::unique(s_units.begin(), s_units.end()), s_units.end());
    }
    return s_units;
}

void writeList(FILE* out)
{
    for (const auto& id : units()) {
        std::fprintf(out, "%s\n", id.c_str());
    }
}

/**
 * @brief 把 f32 转成「能原样往返」的最短十进制文本，再按数值写进 JSON。
 *
 * `0.3f` 直接以 float 提升成 double 会得到 `0.30000001192092896`，而原版 oracle 那边
 * Gson 写的是 `0.3`——两者其实是同一个 float，只是十进制写法不同。这里先把 f32 收窄到
 * 最短往返表示（`%.9g` 截掉尾部噪声，再 round-trip 校验），得到与原版一致的写法。
 */
[[nodiscard]] double canonicalFloat(f32 value)
{
    char buffer[32];
    for (int precision = 6; precision <= 9; ++precision) {
        std::snprintf(buffer, sizeof(buffer), "%.*g", precision, static_cast<double>(value));
        if (std::strtof(buffer, nullptr) == value) {
            break;
        }
    }
    return std::strtod(buffer, nullptr);
}

/// 解析单元，得到「方块 + 状态」。未知方块返回 nullptr。
const BlockState* resolveUnitState(const std::string& unit)
{
    const size_t hashPos = unit.find('#');
    const std::string blockId = unit.substr(0, hashPos);

    const Block* block = BlockRegistry::instance().getBlock(ResourceLocation(blockId));
    if (block == nullptr) {
        return nullptr;
    }

    // 状态对象由方块自己持有（`with()` 返回的是同一池里的另一个状态），探针不复制。
    const BlockState* state = &block->defaultState();
    if (hashPos != std::string::npos) {
        for (const auto& variant : stateVariants()) {
            if (variant.unit == unit) {
                variant.apply(state);
                break;
            }
        }
    }
    return state;
}

std::string runLine(std::string_view inputLine)
{
    static_cast<void>(units());

    const json input = json::parse(inputLine);
    const std::string unit = input.at("block").get<std::string>();

    const BlockState* state = resolveUnitState(unit);
    if (state == nullptr) {
        return json{{"block", unit}, {"ok", false}, {"error", "unknown block"}}.dump();
    }

    const BlockSoundType& sound = state->getSoundType();
    return json{{"block", unit},
        {"ok", true},
        {"break", sound.getBreakSound().toString()},
        {"step", sound.getStepSound().toString()},
        {"place", sound.getPlaceSound().toString()},
        {"hit", sound.getHitSound().toString()},
        {"fall", sound.getFallSound().toString()},
        {"volume", canonicalFloat(sound.getVolume())},
        {"pitch", canonicalFloat(sound.getPitch())}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(block_sound_type, &writeList, &runLine)

} // namespace mc::diff
