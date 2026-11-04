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
 * @brief 差分能力：潮涌核心框架等级（`ConduitBlockEntity.updateShape`）。
 *
 * `list`：一组框架布局
 * `run` ：一行 -> { "frameCount": i32, "active": bool, "eyeOpen": bool, "killReady": bool }
 *
 * ## 原版算法（ConduitBlockEntity.java:127-162）
 *
 * ```java
 * for (i,j,k in -1..1) if (!level.isWaterAt(pos.offset(i,j,k))) return false;   // 3x3x3 全水
 *
 * for (j1,k1,l1 in -2..2) {
 *     int i2=|j1|, l=|k1|, i1=|l1|;
 *     if ((i2>1||l>1||i1>1) && (j1==0&&(l==2||i1==2) || k1==0&&(i2==2||i1==2) || l1==0&&(i2==2||l==2)))
 *         if (state.is(VALID_BLOCKS)) list.add(pos);
 * }
 * return list.size() >= 16;
 * ```
 *
 * 三个要点：
 *
 * 1. **中心 3x3x3 必须全是水**，任一格不是水就立刻失效，框架数完全不看；
 * 2. 只认**框架环**，不是 5x5x5 的整个外壳。环是「至少一个分量绝对值 > 1 且某个分量为 0」，
 *    共 **42 格**；外壳有 98 格。混淆会让 16 / 42 两个门槛全部失真；
 * 3. 门槛有两档：`MIN_ACTIVE_SIZE = 16` 激活，`MIN_KILL_SIZE = 42` 才能攻击敌对生物
 *    （`list.size() >= 42`）。恰好 42 格时三档全满足。
 *
 * ## 单元格式
 *
 * `<drained>;<ring bitmap>`
 *
 * - `drained`：`0` 表示中心 3x3x3 全是水，`1` 表示其中有一格不是水（模拟中心被抽空前的
 *   破坏）。为 1 时整块直接失效；
 * - `ring bitmap`：42 个字符，按环的枚举顺序，`1` 表示该格是合法框架方块、`0` 表示不是。
 *   长度必须是 42。
 */

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/world/blockentity/processing/ConduitEntity.hpp"

#include <array>
#include <cstdlib>

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 环上 42 格的枚举顺序，与 `countFrameBlocks` 的三重循环一致。
std::vector<std::array<i32, 3>> ringOffsets()
{
    std::vector<std::array<i32, 3>> offsets;
    for (i32 dx = -2; dx <= 2; ++dx) {
        for (i32 dy = -2; dy <= 2; ++dy) {
            for (i32 dz = -2; dz <= 2; ++dz) {
                const i32 adx = std::abs(dx);
                const i32 ady = std::abs(dy);
                const i32 adz = std::abs(dz);
                if (adx <= 1 && ady <= 1 && adz <= 1) {
                    continue;
                }
                const bool isFrame = (dx == 0 && (ady == 2 || adz == 2)) || (dy == 0 && (adx == 2 || adz == 2)) ||
                    (dz == 0 && (adx == 2 || ady == 2));
                if (isFrame) {
                    offsets.push_back({dx, dy, dz});
                }
            }
        }
    }
    return offsets;
}

std::string repeat(char ch, std::size_t count)
{
    return std::string(count, ch);
}

/**
 * 场景表：`<drained>;<bitmap>`。
 *
 * 用 helper 生成常见形状，避免手写 42 个字符出错。
 */
std::vector<std::string> buildScenes()
{
    std::vector<std::string> scenes;

    // 全满（42 格）
    scenes.push_back("0;" + repeat('1', 42));
    // 全空
    scenes.push_back("0;" + repeat('0', 42));
    // 少一格（41 格）：仍满足 42 门槛？不满足，但满足 16
    scenes.push_back("0;" + repeat('1', 41) + "0");
    // 恰好 16 格
    scenes.push_back("0;" + repeat('1', 16) + repeat('0', 26));
    // 恰好 15 格（不激活）
    scenes.push_back("0;" + repeat('1', 15) + repeat('0', 27));
    // 恰好 17 格（激活）
    scenes.push_back("0;" + repeat('1', 17) + repeat('0', 25));
    // 中心不全是水 -> 无论框架多满都失效
    scenes.push_back("1;" + repeat('1', 42));
    scenes.push_back("1;" + repeat('0', 42));

    return scenes;
}

void writeList(FILE* out)
{
    for (const std::string& scene : buildScenes()) {
        std::fprintf(out, "%s\n", scene.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const std::size_t separator = unit.find(';');
    if (separator == std::string::npos) {
        throw std::runtime_error("conduit_frame_level: 单元应为 'drained;bitmap'：" + unit);
    }
    const bool drained = unit.substr(0, separator) == "1";
    const std::string bitmap = unit.substr(separator + 1);

    const std::vector<std::array<i32, 3>> offsets = ringOffsets();
    if (bitmap.size() != offsets.size()) {
        throw std::runtime_error("conduit_frame_level: bitmap 长度应为 " + std::to_string(offsets.size()) + "，实际 " +
            std::to_string(bitmap.size()));
    }

    // 中心 3x3x3 全是水，除非 drained。
    auto isWater = [drained](i32 dx, i32 dy, i32 dz) -> bool {
        if (!drained) {
            return true;
        }
        // drained 时让 (0,0,0) 不是水。
        return !(dx == 0 && dy == 0 && dz == 0);
    };

    auto isFrameBlock = [&bitmap, &offsets](i32 dx, i32 dy, i32 dz) -> bool {
        for (std::size_t i = 0; i < offsets.size(); ++i) {
            if (offsets[i][0] == dx && offsets[i][1] == dy && offsets[i][2] == dz) {
                return bitmap[i] == '1';
            }
        }
        return false;
    };

    const i32 frameCount = blockentity::ConduitEntity::countFrameBlocks(0, 0, 0, isWater, isFrameBlock);

    const bool active = frameCount >= blockentity::ConduitEntity::MIN_FRAME_BLOCKS;
    const bool eyeOpen = frameCount >= blockentity::ConduitEntity::EYE_OPEN_FRAME_BLOCKS;

    return json{{"unit", unit},
        {"frameCount", frameCount},
        {"active", active},
        {"eyeOpen", eyeOpen},
        {"killReady", frameCount >= 42}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(conduit_frame_level, &writeList, &runLine)

} // namespace mc::diff
