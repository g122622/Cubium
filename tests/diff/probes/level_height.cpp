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
 * @brief 差分能力：世界高度范围与分区索引（`LevelHeightAccessor`，纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.level.LevelHeightAccessor`（全部是 `default` 方法）：
 *
 * ```java
 * int getHeight();
 * int getMinY();
 *
 * default int getMaxY() { return this.getMinY() + this.getHeight() - 1; }
 * default int getMinSectionY() { return SectionPos.blockToSectionCoord(this.getMinY()); }
 * default int getMaxSectionY() { return SectionPos.blockToSectionCoord(this.getMaxY()); }
 * default int getSectionsCount() { return this.getMaxSectionY() - this.getMinSectionY() + 1; }
 *
 * default boolean isInsideBuildHeight(int y) {
 *     return y >= this.getMinY() && y <= this.getMaxY();
 * }
 *
 * default boolean isOutsideBuildHeight(int y) {
 *     return y < this.getMinY() || y > this.getMaxY();
 * }
 *
 * default int getSectionIndex(int y) {
 *     return this.getSectionIndexFromSectionY(SectionPos.blockToSectionCoord(y));
 * }
 *
 * default int getSectionIndexFromSectionY(int sectionY) {
 *     return sectionY - this.getMinSectionY();
 * }
 *
 * default int getSectionYFromSectionIndex(int index) {
 *     return index + this.getMinSectionY();
 * }
 *
 * static LevelHeightAccessor create(int minY, int height) { ... }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`getMaxY()` 是「含」的上界：`minY + height - 1`。**
 *
 *    主世界 `minY = -64`、`height = 384` → `maxY = -64 + 384 - 1 = 319`。
 *    合法 Y 是 `-64..319` 共 **384** 个值。若写成 `minY + height`（不加 `-1`），`maxY` 会多
 *    出 1 格，导致顶端多放行一个方块。
 *
 * 2. **`isInsideBuildHeight` 与 `isOutsideBuildHeight` 是严格互补的。**
 *
 *    前者用 `y >= minY && y <= maxY`，后者用 `y < minY || y > maxY`（德摩根律）。两者
 *    **对任何整数输入都恰好一真一假**，不存在「都不成立」的空隙。
 *
 * 3. **`isOutsideBuildHeight(BlockPos)` 走 `getY()`，与整数重载完全等价。**
 *
 *    重载看似做额外的事，实际只取 Y 分量——X 与 Z 完全不参与判定。
 *
 * 4. **分区索引从 `minSectionY` 起算，不是从 0。**
 *
 *    ```java
 *    getSectionIndexFromSectionY(sectionY) = sectionY - getMinSectionY();
 *    ```
 *
 *    主世界 `minY = -64` → `minSectionY = -64 >> 4 = -4`，`maxY = 319` →
 *    `maxSectionY = 319 >> 4 = 19`，`sectionsCount = 19 - (-4) + 1 = 24`。
 *
 *    **负的 `minY` 会让 `minSectionY` 为负**，所以索引不能直接用 `sectionY`，必须减去
 *    `minSectionY` 才能落到 `0..sectionsCount-1`。
 *
 *    注意 `319 >> 4 = 19` 而不是 `19.9375` 的截断问题——`319 / 16 = 19`，且坐标为正所以
 *    算术右移与整除一致。
 *
 * 单元格式：`<mode>|<minY>|<height>|<参数>`
 * - `range|<minY>|<height>` —— 输出 `maxY,minSectionY,maxSectionY,sectionsCount`
 * - `inside|<minY>|<height>|<y>` —— 输出 `true`/`false`
 * - `outside|<minY>|<height>|<y>` —— 输出 `true`/`false`
 * - `both|<minY>|<height>|<y>` —— 同时输出两者，验证互补性
 * - `sectionIndex|<minY>|<height>|<y>` —— 方块 Y 所在的分区索引
 * - `sectionYToIndex|<minY>|<height>|<sectionY>` —— `getSectionIndexFromSectionY`
 * - `indexToSectionY|<minY>|<height>|<index>` —— `getSectionYFromSectionIndex`
 *
 * TODO: 未覆盖 `LevelHeightAccessor.isOutsideBuildHeight(BlockPos)` 的重载形式（等价于整数
 * 版本），以及 `Level` 实现类里对 `getSectionsCount` 的其他使用点。
 */
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 原版 `SectionPos.blockToSectionCoord(int)`：算术右移，负数向下取整。
i32 blockToSectionCoord(i32 blockCoord)
{
    return blockCoord >> 4;
}

/// 原版 `LevelHeightAccessor` 的纯计算部分。
struct HeightAccessor {
    i32 minY;
    i32 height;

    i32 getMinY() const
    {
        return minY;
    }

    i32 getHeight() const
    {
        return height;
    }

    i32 getMaxY() const
    {
        return getMinY() + getHeight() - 1;
    }

    i32 getMinSectionY() const
    {
        return blockToSectionCoord(getMinY());
    }

    i32 getMaxSectionY() const
    {
        return blockToSectionCoord(getMaxY());
    }

    i32 getSectionsCount() const
    {
        return getMaxSectionY() - getMinSectionY() + 1;
    }

    bool isInsideBuildHeight(i32 y) const
    {
        return y >= getMinY() && y <= getMaxY();
    }

    bool isOutsideBuildHeight(i32 y) const
    {
        return y < getMinY() || y > getMaxY();
    }

    i32 getSectionIndexFromSectionY(i32 sectionY) const
    {
        return sectionY - getMinSectionY();
    }

    i32 getSectionIndex(i32 y) const
    {
        return getSectionIndexFromSectionY(blockToSectionCoord(y));
    }

    i32 getSectionYFromSectionIndex(i32 index) const
    {
        return index + getMinSectionY();
    }
};

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // 范围：主世界（minY=-64, height=384）
        "range|-64|384",
        // 范围：下界（minY=0, height=256）
        "range|0|256",
        // 范围：末地（minY=0, height=256）
        "range|0|256",
        // 范围：height 为 16 的整数倍
        "range|0|16",
        "range|-16|16",
        "range|-16|32",
        // 范围：height 不是 16 的倍数
        "range|0|17",
        "range|5|17",
        // 范围：负 minY 非 16 的倍数
        "range|-5|100",
        // inside：主世界的边界
        "inside|-64|384|-64",
        "inside|-64|384|319",
        "inside|-64|384|-65",
        "inside|-64|384|320",
        "inside|-64|384|0",
        "inside|-64|384|63",
        "inside|-64|384|64",
        // outside：同上四点
        "outside|-64|384|-64",
        "outside|-64|384|319",
        "outside|-64|384|-65",
        "outside|-64|384|320",
        // 互补性
        "both|-64|384|-64",
        "both|-64|384|-65",
        "both|-64|384|0",
        "both|0|256|256",
        "both|0|256|255",
        "both|-5|17|-6",
        "both|-5|17|11",
        // 分区索引
        "sectionIndex|-64|384|-64",
        "sectionIndex|-64|384|-1",
        "sectionIndex|-64|384|0",
        "sectionIndex|-64|384|15",
        "sectionIndex|-64|384|16",
        "sectionIndex|-64|384|319",
        "sectionIndex|0|256|0",
        "sectionIndex|0|256|-1",
        // sectionY <-> index
        "sectionYToIndex|-64|384|-4",
        "sectionYToIndex|-64|384|0",
        "sectionYToIndex|-64|384|19",
        "sectionYToIndex|0|256|0",
        "indexToSectionY|-64|384|0",
        "indexToSectionY|-64|384|23",
        "indexToSectionY|0|256|0",
    };
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::vector<std::string> splitPipe(const std::string& text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == '|') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

std::string boolText(bool value)
{
    return value ? "true" : "false";
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 3) {
        throw std::runtime_error("level_height: 单元至少三段：" + unit);
    }

    const HeightAccessor accessor{std::stoi(parts[1]), std::stoi(parts[2])};
    const std::string mode = parts[0];

    if (mode == "range" && parts.size() == 3) {
        const std::string value = std::to_string(accessor.getMaxY()) + "," + std::to_string(accessor.getMinSectionY()) +
            "," + std::to_string(accessor.getMaxSectionY()) + "," + std::to_string(accessor.getSectionsCount());
        return json{{"unit", unit}, {"value", value}}.dump();
    }
    if (mode == "inside" && parts.size() == 4) {
        const bool result = accessor.isInsideBuildHeight(std::stoi(parts[3]));
        return json{{"unit", unit}, {"value", boolText(result)}}.dump();
    }
    if (mode == "outside" && parts.size() == 4) {
        const bool result = accessor.isOutsideBuildHeight(std::stoi(parts[3]));
        return json{{"unit", unit}, {"value", boolText(result)}}.dump();
    }
    if (mode == "both" && parts.size() == 4) {
        const i32 y = std::stoi(parts[3]);
        const std::string value = std::string("inside=") + boolText(accessor.isInsideBuildHeight(y)) +
            ",outside=" + boolText(accessor.isOutsideBuildHeight(y));
        return json{{"unit", unit}, {"value", value}}.dump();
    }
    if (mode == "sectionIndex" && parts.size() == 4) {
        return json{{"unit", unit}, {"value", accessor.getSectionIndex(std::stoi(parts[3]))}}.dump();
    }
    if (mode == "sectionYToIndex" && parts.size() == 4) {
        return json{{"unit", unit}, {"value", accessor.getSectionIndexFromSectionY(std::stoi(parts[3]))}}.dump();
    }
    if (mode == "indexToSectionY" && parts.size() == 4) {
        return json{{"unit", unit}, {"value", accessor.getSectionYFromSectionIndex(std::stoi(parts[3]))}}.dump();
    }

    throw std::runtime_error("level_height: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(level_height, &writeList, &runLine)

} // namespace mc::diff
