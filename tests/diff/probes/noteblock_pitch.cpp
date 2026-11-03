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
 * @brief 差分能力：音符盒的调音（音符值 0..24 的音高与循环）。
 *
 * `list`：25 个单元 `note=<0..24>`
 * `run` ：一行 -> { "pitch": f32 位模式, "note": 当前音符,
 *                  "cycled": 循环一步后的音符, "wrapped": 是否回绕 }
 *
 * ## 三件事一起比
 *
 * 1. **pitch**：原版 `NoteBlock#getPitchFromNote(note)` = `2^((note-12)/12)`。
 *    这是**浮点计算**，必须按 f32 位模式比——`std::pow` 与 Java 的 `Math.pow` 都是
 *    正确舍入的，但中间用 f32 还是 f64 会让结果差 1 ulp。差 1 ulp 人耳听不出来，
 *    但会让音乐作品的包络计算逐帧漂移，且这类差异只有位级比对才能发现。
 * 2. **note**：`getNote(state)` 读 `NOTE_0_24` 属性。
 * 3. **cycled / wrapped**：右键音符盒会 `cycle(NOTE)`。原版由属性取值范围 `0..24`
 *    决定回绕点（`cycle` 到 24 之后回到 0），自己写 `(note + 1) % 25` 也等价，
 *    但一旦常量写错（比如写成 24 或 16），音阶就少了一档或在中途断掉，
 *    表现为「调到最高音时突然跳回最低音」。
 *
 * ## 抄错的后果
 *
 * 音高是第一类错误：整首曲子的音阶全部偏一个半音，或者八度关系错乱（12 个音符
 * 才翻倍的规律被打破），听起来像走调。回绕错则是「某些音根本弹不出来」。
 *
 * f32 位模式用 `std::bit_cast<u32>` 输出为 `0x%08x`，避免 JSON 序列化的十进制舍入。
 */

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/redstone/NoteBlock.hpp"

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

/// 音符盒方块的单元名，`list` 用它之外的 25 个音符值作单元
constexpr i32 NOTE_COUNT = 25;

const Block* noteBlock()
{
    return BlockRegistry::instance().getBlock(ResourceLocation("minecraft:note_block"));
}

void writeList(FILE* out)
{
    ensureInitialized();
    for (i32 note = 0; note < NOTE_COUNT; ++note) {
        std::fprintf(out, "note=%d\n", note);
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    // 单元形如 "note=<n>"
    i32 note = -1;
    if (unit.rfind("note=", 0) == 0) {
        note = std::stoi(unit.substr(5));
    }
    if (note < 0 || note >= NOTE_COUNT) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    const Block* block = noteBlock();
    if (block == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    const BlockState& base = block->defaultState();
    const BlockState withNote = blocks::NoteBlock::withNote(base, note);
    const BlockState cycled = blocks::NoteBlock::cycleNote(withNote);

    const f32 pitch = blocks::NoteBlock::calculatePitch(note);
    const u32 pitchBits = std::bit_cast<u32>(pitch);
    char bitsBuffer[16];
    std::snprintf(bitsBuffer, sizeof(bitsBuffer), "0x%08x", pitchBits);

    const i32 readBack = blocks::NoteBlock::getNote(withNote);
    const i32 cycledNote = blocks::NoteBlock::getNote(cycled);

    return json{{"unit", unit},
        {"pitch", std::string(bitsBuffer)},
        {"note", readBack},
        {"cycled", cycledNote},
        {"wrapped", cycledNote == 0}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("noteblock_pitch", &writeList, &runLine)

} // namespace mc::diff
