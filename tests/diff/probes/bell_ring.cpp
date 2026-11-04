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
 * @brief 差分能力：敲钟的**有效敲击判定**（`BellBlock#isProperHit`）。
 *
 * `list`：4 种附着方式 × 4 种钟朝向 × 6 种点击面 × 5 个点击高度 = 480 个单元
 * `run` ：一行 -> { "proper": bool }
 *
 * ## 为什么「打得响」还要判定
 *
 * 钟不是碰一下就叫。原版 `isProperHit` 按钟的**挂法和朝向**决定从哪个面敲才响：
 *
 * - 点击面在 **Y 轴**（上下）：永远不算有效 —— 从顶上或底下拍钟不响
 * - 点击高度 **> 0.8124**：永远不算有效 —— 敲钟的「帽子」部分不响，只有钟体下半响
 * - **floor**：点击面与钟朝向在**同一轴**才响（钟立着，正面/背面拍）
 * - **single_wall / double_wall**：点击面与钟朝向**不同轴**才响（钟挂着，从左/右拍）
 * - **ceiling**：任何水平面都响
 *
 * 0.8124 这个阈值对应钟体顶部与横梁之间的缝隙，是原版写死的经验值。
 *
 * ## 抄错的后果
 *
 * 判定错不会崩溃，只会让**某些角度敲不响**或**不该响的地方能响**：
 *
 * - 阈值写成 0.5：钟体上半部分全拍不响，玩家觉得钟「坏了」；
 * - floor 的轴判反：只能从侧面拍地上的钟，正面拍没反应；
 * - 墙钟的轴判反：只能贴着墙那面拍，实际玩家总是从外侧拍，等于完全敲不响。
 *
 * 这些都不报错、不掉日志，只能靠逐组合比对发现。
 */

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/functional/BellBlock.hpp"
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

constexpr const char* ATTACHMENTS[] = {"floor", "ceiling", "single_wall", "double_wall"};
constexpr const char* FACINGS[] = {"north", "south", "west", "east"};
constexpr const char* FACES[] = {"down", "up", "north", "south", "west", "east"};
/// 采样点击高度：钟体下半（0.2 / 0.5）、阈值边界两侧（0.81 / 0.82）、钟帽（0.95）
constexpr f64 SAMPLE_HEIGHTS[] = {0.2, 0.5, 0.81, 0.82, 0.95};

const char* directionName(Direction direction)
{
    switch (direction) {
        case Direction::Down:
            return "down";
        case Direction::Up:
            return "up";
        case Direction::North:
            return "north";
        case Direction::South:
            return "south";
        case Direction::West:
            return "west";
        case Direction::East:
            return "east";
        case Direction::None:
            break;
    }
    return "none";
}

void writeList(FILE* out)
{
    ensureInitialized();
    for (const char* attachment : ATTACHMENTS) {
        for (const char* facing : FACINGS) {
            for (const char* face : FACES) {
                for (const f64 height : SAMPLE_HEIGHTS) {
                    std::fprintf(out, "%s:%s:%s:%.2f\n", attachment, facing, face, height);
                }
            }
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    // 单元形如 "<attachment>:<facing>:<face>:<height>"
    const std::size_t p1 = unit.find(':');
    const std::size_t p2 = unit.find(':', p1 + 1);
    const std::size_t p3 = unit.find(':', p2 + 1);
    if (p1 == std::string::npos || p2 == std::string::npos || p3 == std::string::npos) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    const std::string attachmentName = unit.substr(0, p1);
    const std::string facingName = unit.substr(p1 + 1, p2 - p1 - 1);
    const std::string faceName = unit.substr(p2 + 1, p3 - p2 - 1);
    const f64 height = std::stod(unit.substr(p3 + 1));

    BlockStateProperties::BellAttachment attachment = BlockStateProperties::BellAttachment::Floor;
    if (attachmentName == "ceiling") {
        attachment = BlockStateProperties::BellAttachment::Ceiling;
    } else if (attachmentName == "single_wall") {
        attachment = BlockStateProperties::BellAttachment::SingleWall;
    } else if (attachmentName == "double_wall") {
        attachment = BlockStateProperties::BellAttachment::DoubleWall;
    }

    Direction facing = Direction::North;
    if (facingName == "south") {
        facing = Direction::South;
    } else if (facingName == "west") {
        facing = Direction::West;
    } else if (facingName == "east") {
        facing = Direction::East;
    }

    Direction face = Direction::None;
    for (const char* candidate : FACES) {
        if (faceName == candidate) {
            const auto parsed = Directions::fromName(candidate);
            if (parsed.has_value()) {
                face = *parsed;
            }
            break;
        }
    }
    if (face == Direction::None) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    Block* bell = VanillaBlocks::BELL;
    if (bell == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    const BlockState state = bell->defaultState()
                                 .with(BlockStateProperties::BELL_ATTACHMENT(), attachment)
                                 .with(BlockStateProperties::HORIZONTAL_FACING(), facing);

    const bool proper = blocks::BellBlock::isProperHit(state, face, height);
    return json{{"unit", unit}, {"proper", proper}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(bell_ring, &writeList, &runLine)

} // namespace mc::diff
