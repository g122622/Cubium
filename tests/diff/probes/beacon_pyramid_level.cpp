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
 * @brief 差分能力：信标金字塔等级（`BeaconBlockEntity.updateBase`）。
 *
 * `list`：一组金字塔布局 × 世界底部高度
 * `run` ：一行 -> { "level": i32 }
 *
 * ## 原版算法（BeaconBlockEntity.java:198-224）
 *
 * ```java
 * int i = 0;
 * for (int j = 1; j <= 4; i = j++) {
 *     int k = y - j;                              // 第 j 层的 Y
 *     if (k < level.getMinY()) break;             // 低于世界底即停止
 *     boolean flag = true;
 *     for (int l = x - j; l <= x + j && flag; l++)
 *         for (int i1 = z - j; i1 <= z + j; i1++)
 *             if (!getBlockState(new BlockPos(l, k, i1)).is(BlockTags.BEACON_BASE_BLOCKS)) {
 *                 flag = false; break;
 *             }
 *     if (!flag) break;
 * }
 * return i;
 * ```
 *
 * 要点：
 *
 * 1. 第 j 层是 **(2j+1)×(2j+1) 的整片**，不是只有边框——任何一格不是底座方块整层就失效；
 * 2. **必须连续**：第 2 层破了，即使第 3 层完整也只算 1 层（循环一 break 就结束）；
 * 3. **低于世界底部即停止**。世界底是维度参数而非常量，主世界 -64、下界与末地 0；
 * 4. 只看信标正下方，与信标自身所在格的方块无关。
 *
 * ## 单元格式
 *
 * `minY;layer1;layer2;layer3;layer4`
 *
 * - `minY`：世界最低建筑高度；
 * - `layerN`：第 N 层的方块布局，从左到右、从上到下逐格，`.` 表示空气、`#` 表示底座方块。
 *   长度必须是 `(2N+1)^2`，下标 `row*(2N+1)+col` 对应偏移 `(col-N, -N, row-N)`。
 *
 * 这个格式让「整层缺一格」「层间不连续」「世界底截断」这三类错误都能精确表达。
 */

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/world/blockentity/processing/BeaconEntity.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

std::vector<std::string> splitSemicolon(std::string_view text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == ';') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

/**
 * 场景表：`minY;layer1;layer2;layer3;layer4`。
 *
 * 空字符串表示该层不参与（不会发生——上面几层先失效就 break 了）。
 */
// 模拟「3x3 全实心」等布局用的字面量。
constexpr const char* L1 = "#########";                                                 // 3x3
constexpr const char* L2 = "#########################";                                 // 5x5
constexpr const char* L3 = "###########################################";               // 7x7
constexpr const char* L4 = "#########################################################"; // 9x9

/**
 * 场景表：`minY;layer1;layer2;layer3;layer4`（**恰好 5 段**）。
 *
 * 不需要参与的层留空串，但仍要保留分号占位。
 */
std::vector<std::string> buildScenes()
{
    std::vector<std::string> scenes;
    auto add = [&scenes](const std::string& minY,
                   const std::string& l1,
                   const std::string& l2,
                   const std::string& l3,
                   const std::string& l4) {
        scenes.push_back(minY + ";" + l1 + ";" + l2 + ";" + l3 + ";" + l4);
    };

    // 完全无底座
    add("-64", "", "", "", "");
    // 只有第 1 层
    add("-64", L1, "", "", "");
    // 第 1 层缺中间一格
    add("-64", "###.#.###", "", "", "");
    // 第 1 层缺角
    add("-64", ".###.####", "", "", "");
    // 1 + 2 层完整
    add("-64", L1, L2, "", "");
    // 1..3 层完整
    add("-64", L1, L2, L3, "");
    // 1..4 层完整
    add("-64", L1, L2, L3, L4);
    // 1 层好、2 层缺一格、3 层完整 -> 只能是 1
    add("-64", L1, "#########.###############", L3, "");
    // 世界底截断：minY=-1，信标 y=0，只有第 1 层（y=-1）够得着
    add("-1", L1, "", "", "");
    // 世界底截断：minY=-3，1 层(y=-1) 与 2 层(y=-2) 都在范围内
    add("-3", L1, L2, "", "");
    // 世界底截断：minY=-2，第 3 层(y=-3) 越界
    add("-2", L1, L2, L3, "");

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
    const std::vector<std::string> parts = splitSemicolon(unit);
    if (parts.size() != 5) {
        throw std::runtime_error("beacon_pyramid_level: 单元应为 5 段（minY + 4 层）：" + unit);
    }

    const i32 minY = std::stoi(parts[0]);

    // 把每层布局摊平成「坐标 -> 是否底座方块」的集合。
    std::vector<std::string> layers(parts.begin() + 1, parts.end());

    // 信标站在 y，第 j 层在 y-j。这里固定信标在 (0, 0, 0)，层 Y 就用 -j。
    constexpr i32 BEACON_Y = 0;

    auto isBaseBlock = [&layers](i32 x, i32 y, i32 z) -> bool {
        const i32 level = BEACON_Y - y;
        if (level < 1 || level > 4) {
            return false;
        }
        const std::string& layout = layers[static_cast<std::size_t>(level - 1)];
        const i32 side = 2 * level + 1;
        if (static_cast<i32>(layout.size()) != side * side) {
            return false;
        }
        const i32 row = z + level;
        const i32 col = x + level;
        if (row < 0 || row >= side || col < 0 || col >= side) {
            return false;
        }
        return layout[static_cast<std::size_t>(row * side + col)] == '#';
    };

    const i32 level = blockentity::BeaconEntity::computeLevel(0, BEACON_Y, 0, minY, isBaseBlock);
    return json{{"unit", unit}, {"level", level}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("beacon_pyramid_level", &writeList, &runLine)

} // namespace mc::diff
