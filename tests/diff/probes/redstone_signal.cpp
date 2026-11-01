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
 * @file redstone_signal.cpp
 * @brief 差分能力：红石信号——谁向哪个方向输出多少强度，方块被充能后又能转出多少。
 *
 * `list`：全部场景名。
 * `run` ：{ "unit": "lever_switch_direct" }
 *         → { "unit": "...", "ok": true,
 *             "signal":  { "down": 15, ... },      // 原版 SignalGetter#getSignal
 *             "weak":    { "down": 15, ... },      // 原版 BlockBehaviour#getSignal
 *             "strong":  { "down": 15, ... },      // 原版 BlockBehaviour#getDirectSignal
 *             "bestNeighbor": 15,                  // 原版 getBestNeighborSignal
 *             "quietSignal": { ... },              // 同上，但红石线自身的输出已关闭
 *             "quietBestNeighbor": 0,              // 同上
 *             "wire": {                            // 仅红石线场景
 *               "input": 14,                       // 红石线自身的输入功率（已衰减）
 *               "links": { "north": "side", ... } } }
 *
 * ## 为什么值得比
 *
 * 红石是一套「方块问邻居要信号」的协议：强度、方向、强/弱之分，任何一处与原版不一致，
 * 玩家搭出来的电路就会整体失灵——中继器不亮、门不开、活塞不推。它不会崩溃、不会报错，
 * 只是「做得和原版不一样」，只能靠给定地形逐点比对发现。
 *
 * ## 两套口径：属性本身 / 关掉自我屏蔽
 *
 * `getSignal` 的语义里藏着一个「自我屏蔽」开关：查询红石线时，原版先把自己（红石线
 * 类）的 `shouldSignal` 置为 false，否则红石线会拿自己的旧功率回灌，算出永远不小于自身
 * 的循环值。Cubium 用 `m_canProvidePower` 表达同一件事。
 *
 * 这个开关是**瞬时状态**而非场景的一部分：同一地形放在不同的查询序列里，红石线的功率
 * 可以不同（「先问邻居再问自己」与「只问自己」结果不一样）。所以本能力分成两段、各自独立：
 *
 * 1. 第一段**不动开关**，逐点问答「这块方块朝这个方向输出多少」——比的是各方块自己的
 *    信号属性（`Block#getSignal` / `getDirectSignal`）。
 * 2. 第二段先把开关关掉再问同一批问题。这对应原版 `getBlockSignal`：关掉自己的输出，
 *    再问「邻居能给我多少」。红石线的功率由这一段决定。
 *
 * 两段分开是必须的：第 1 段必须先把答案全部取完，否则第 2 段的开关会反过来污染第 1 段。
 *
 * ## 字段与原版的对应
 *
 * 「强 / 弱」是容易混淆的一对词，这里按原版的三条入口逐字对齐，不做自创口径：
 *
 * - `signal`  → `SignalGetter#getSignal`：先问方块自己，若它是红石导体再取
 *   `getDirectSignalTo`（六个邻居朝它输出的强信号）。**这是电路真正"看到"的那个值**。
 * - `weak`    → `BlockBehaviour#getSignal`（即常说的「弱信号」）：红石线、红石块、
 *   拉杆等从自己的面输出的功率，导体恒为 0。
 * - `strong`  → `BlockBehaviour#getDirectSignal`（即常说的「强信号」）：导体被邻居充能后
 *   朝外转出的功率；红石火把除此之外只朝下方输出。
 *
 * 三者不是同一件事的不同名字：红石块 `weak` 处处 15 而 `strong` 处处 0；被火把点亮的
 * 石头 `weak` 处处 0 而 `strong` 处处 15。电路失效恰恰常发生在这两个混淆上。
 *
 * ## 地形编码
 *
 * 每个场景是一片 5×5×3 的地形，分三层描述：
 * - `rows`  中心层（y=0），第 2 行第 2 列由调用方放主语，地形表里写空气
 * - `above` 上层（y=+1）的 5×5 薄片
 * - `below` 下层（y=-1）的 5×5 薄片
 *
 * 坐标约定：行号增大对应 +Z（南），列号增大对应 +X（东）；第 2 行第 2 列是主语。
 * 5×5 足够表达「紧贴主语的六个邻居」与「它们外侧一格」——红石的信号强度上限是 15，
 * 而本能力只问「主语朝邻居输出多少」，不需要更远的地形。
 *
 * 字符表（三层通用，每格一个字符）：
 * - `.` 空气
 * - `S` 石头（红石导体）
 * - `G` 玻璃（完整碰撞箱但不是红石导体——`isRedstoneConductor` 的核心反例）
 * - `s` 台阶（有碰撞形状但不完整，也不是导体）
 * - `R` 红石块（向所有方向输出弱信号 15、强 0）
 * - `T` 点亮的红石火把（除朝上外各方向弱 15，向下强 15）
 * - `t` 熄灭的红石火把
 * - `L` 点亮的拉杆（附着在脚下，向所有方向弱 15，只朝下方强 15）
 * - `w` 红石线，功率 0，四方向连接一律「侧面连接」
 * - `x` 红石线，功率 0，四方向连接一律「无连接」（点状）
 */
#include <algorithm>
#include <array>
#include <cstdio>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/redstone/RedstoneWireBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/redstone/RedstonePower.hpp"

#include "probe.hpp"
#include "vanilla_datapack.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

// ============================================================================
// 场景表
// ============================================================================

/// 5×5 薄片里的一格空气。
constexpr const char* kE = ".....";

/// 一层 5×5 薄片，按「北 → 南」逐行扁平存放（行优先，共 25 格）。
using Layer = std::array<const char*, 25>;

struct Scenario {
    const char* unit;
    /// 主语方块：`方块ID` 或 `方块ID[属性=值,...]`。
    const char* subject;
    /// 中心层（y=0）：第 2 行第 2 列是主语，由调用方单独放置。
    Layer plane;
    /// 上层（y=+1）。
    Layer above;
    /// 下层（y=-1）。
    Layer below;
};

const Scenario
    kScenarios[] =
        {
            // 地形是 5×5×3 的薄片，主语固定在中心格。紧贴主语的六个邻居分别是：
            // 中心层第 2 行的列 1（西）/ 列 3（东）、第 1 行与第 3 行的中心列（北 / 南），
            // 以及上下两层的中心格。其余格子留空——本能力只问「主语朝邻居输出多少」。
            {
                .unit = "lever_off",
                .subject = "minecraft:lever[face=floor,facing=north,powered=false]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "lever_floor_north",
                .subject = "minecraft:lever[face=floor,facing=north,powered=true]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "lever_floor_south",
                .subject = "minecraft:lever[face=floor,facing=south,powered=true]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "lever_wall_north",
                .subject = "minecraft:lever[face=wall,facing=north,powered=true]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "lever_wall_south",
                .subject = "minecraft:lever[face=wall,facing=south,powered=true]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "lever_ceiling",
                .subject = "minecraft:lever[face=ceiling,facing=south,powered=true]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "lever_on_stone_side",
                .subject = "minecraft:lever[face=wall,facing=north,powered=true]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "lever_island_in_stone",
                .subject = "minecraft:lever[face=wall,facing=north,powered=true]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        "S",
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "redstone_block_alone",
                .subject = "minecraft:redstone_block",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "redstone_block_beside_stone",
                .subject = "minecraft:redstone_block",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "redstone_block_under_stone",
                .subject = "minecraft:redstone_block",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        "S",
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "torch_lit_alone",
                .subject = "minecraft:redstone_torch[lit=true]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "torch_unlit_alone",
                .subject = "minecraft:redstone_torch[lit=false]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "torch_lit_under_stone",
                .subject = "minecraft:redstone_torch[lit=true]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        "S",
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "torch_lit_above_stone",
                .subject = "minecraft:redstone_torch[lit=true]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        "S",
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "wall_torch_north",
                .subject = "minecraft:redstone_wall_torch[lit=true,facing=north]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "wall_torch_south",
                .subject = "minecraft:redstone_wall_torch[lit=true,facing=south]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "wall_torch_unlit",
                .subject = "minecraft:redstone_wall_torch[lit=false,facing=north]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "observer_powered_north",
                .subject = "minecraft:observer[facing=north,powered=true]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "observer_unpowered",
                .subject = "minecraft:observer[facing=north,powered=false]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "observer_facing_up",
                .subject = "minecraft:observer[facing=up,powered=true]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "observer_beside_stone",
                .subject = "minecraft:observer[facing=north,powered=true]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "conductor_alone",
                .subject = "minecraft:stone",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "conductor_torch_side",
                .subject = "minecraft:stone",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "T",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "conductor_two_torches",
                .subject = "minecraft:stone",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "T",
                        kE,
                        "T",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "conductor_torch_below",
                .subject = "minecraft:stone",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "T",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "conductor_torch_above",
                .subject = "minecraft:stone",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "T",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "conductor_lever_side",
                .subject = "minecraft:stone",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "L",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "conductor_redstone_block_side",
                .subject = "minecraft:stone",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "R",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "glass_torch_below",
                .subject = "minecraft:glass",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "T",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "glass_lever_side",
                .subject = "minecraft:glass",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "L",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "slab_torch_below",
                .subject = "minecraft:smooth_stone_slab[type=bottom]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "T",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "slab_lever_side",
                .subject = "minecraft:smooth_stone_slab[type=bottom]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "L",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "wire_isolated",
                .subject = "minecraft:redstone_wire[power=0,north=side,south=side,east=side,west=side]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "wire_beside_wire",
                .subject = "minecraft:redstone_wire[power=0,north=side,south=side,east=side,west=side]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "w",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        "S",
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "wire_beside_torch",
                .subject = "minecraft:redstone_wire[power=0,north=side,south=side,east=side,west=side]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "T",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        "S",
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "wire_beside_redstone_block",
                .subject = "minecraft:redstone_wire[power=0,north=side,south=side,east=side,west=side]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "R",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        "S",
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "wire_beside_lever",
                .subject = "minecraft:redstone_wire[power=0,north=side,south=side,east=side,west=side]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "L",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        "S",
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "wire_over_glass",
                .subject = "minecraft:redstone_wire[power=0,north=side,south=side,east=side,west=side]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "G",
                        "G",
                        "G",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "wire_above_wire_on_stone",
                .subject = "minecraft:redstone_wire[power=0,north=side,south=side,east=side,west=side]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "w",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        "S",
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
            {
                .unit = "wire_dot_beside_wire",
                .subject = "minecraft:redstone_wire[power=0,north=none,south=none,east=none,west=none]",
                .plane =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "w",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .above =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
                .below =
                    {
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        "S",
                        "S",
                        "S",
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                        kE,
                    },
            },
};

// ============================================================================
// 测试世界
// ============================================================================

/**
 * @brief 一片 5×5×3 的临时世界，只用于回答红石查询。
 *
 * 纵向三层：中心层（y=0）放地形，y=+1 与 y=-1 各是一张 5×5 薄片，用来放「上方邻居 /
 * 下方邻居」。红石导体能否被充能取决于六个方向，纵向邻居必须能写，因此三层都要能摆方块。
 */
class SignalWorld final : public IWorld {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(Key{x, y, z});
        return it == m_blocks.end() ? nullptr : it->second;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(Key{x, y, z});
        } else {
            m_blocks[Key{x, y, z}] = state;
        }
        return true;
    }

    void put(i32 x, i32 y, i32 z, const BlockState& state)
    {
        m_blocks[Key{x, y, z}] = &state;
    }

    // ========== 与红石无关的纯虚：给最小实现 ==========

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return nullptr;
    }

    [[nodiscard]] i32 getHeight(i32, i32) const override
    {
        return 256;
    }

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override
    {
        return 0;
    }

    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override
    {
        return 15;
    }

    [[nodiscard]] bool isRaining() const override
    {
        return false;
    }

    [[nodiscard]] bool isThundering() const override
    {
        return false;
    }

    [[nodiscard]] bool isHardcore() const override
    {
        return false;
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override
    {
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override
    {
        return {};
    }

    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override
    {
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] PhysicsEngine* physicsEngine() override
    {
        return nullptr;
    }

    [[nodiscard]] const PhysicsEngine* physicsEngine() const override
    {
        return nullptr;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::logic_error("SignalWorld::tickManager 不可用：红石查询不应依赖世界 tick");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("SignalWorld::tickManager 不可用：红石查询不应依赖世界 tick");
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId) override
    {
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId) const override
    {
        return nullptr;
    }

    [[nodiscard]] DimensionId dimension() const override
    {
        return 0;
    }

    [[nodiscard]] u64 seed() const override
    {
        return 0;
    }

    [[nodiscard]] u64 currentTick() const override
    {
        return 0;
    }

    [[nodiscard]] i64 dayTime() const override
    {
        return 0;
    }

    [[nodiscard]] bool isClientSide() const override
    {
        return false;
    }

    [[nodiscard]] Difficulty difficulty() const override
    {
        return Difficulty::Normal;
    }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override
    {
        return m_worldBorder;
    }

    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override
    {
        return m_worldBorder;
    }

    [[nodiscard]] ecs::EntityRegistry* entityRegistry() override
    {
        return nullptr;
    }

    [[nodiscard]] const ecs::EntityRegistry* entityRegistry() const override
    {
        return nullptr;
    }

    [[nodiscard]] math::IRandom& getRandom() override
    {
        return m_random;
    }

    [[nodiscard]] const math::IRandom& getRandom() const override
    {
        return m_random;
    }

private:
    struct Key {
        i32 x;
        i32 y;
        i32 z;

        bool operator==(const Key& other) const noexcept
        {
            return x == other.x && y == other.y && z == other.z;
        }
    };

    struct KeyHash {
        size_t operator()(const Key& key) const noexcept
        {
            size_t h = static_cast<size_t>(static_cast<u32>(key.x)) * 0x9E3779B1u;
            h ^= static_cast<size_t>(static_cast<u32>(key.y)) + 0x7F4A7C15u + (h << 6) + (h >> 2);
            h ^= static_cast<size_t>(static_cast<u32>(key.z)) + 0x9E3779B9u + (h << 6) + (h >> 2);
            return h;
        }
    };

    std::unordered_map<Key, const BlockState*, KeyHash> m_blocks;
    world::border::WorldBorder m_worldBorder;
    math::Random m_random{0};
};

// ============================================================================
// 状态记法
// ============================================================================

/**
 * @brief 解析 `方块ID[属性=值,...]`，返回方块默认状态叠加属性后的状态。
 *
 * 属性解析走类型擦除的 `IProperty::parseValue`，与原版 `BlockStateParser` 同一路径，
 * 因此 `facing=north`、`power=12`、`lit=true` 都按注册好的属性定义解释。
 */
const BlockState* parseSubject(std::string_view text)
{
    const size_t bracket = text.find('[');
    const std::string blockId(text.substr(0, bracket));
    const Block* block = BlockRegistry::instance().getBlock(ResourceLocation(blockId));
    if (block == nullptr) {
        return nullptr;
    }

    const BlockState* state = &block->defaultState();
    if (bracket == std::string_view::npos) {
        return state;
    }

    const auto& container = block->stateContainer();
    const std::string_view body = text.substr(bracket + 1, text.size() - bracket - 2);
    size_t start = 0;
    while (start < body.size()) {
        size_t comma = body.find(',', start);
        if (comma == std::string_view::npos) {
            comma = body.size();
        }
        const std::string_view pair = body.substr(start, comma - start);
        const size_t equals = pair.find('=');
        if (equals == std::string_view::npos) {
            return nullptr;
        }
        const IProperty* prop = container.getProperty(pair.substr(0, equals));
        if (prop == nullptr) {
            return nullptr;
        }
        const auto index = prop->parseValue(pair.substr(equals + 1));
        if (!index) {
            return nullptr;
        }
        state = &state->withValueIndex(*prop, *index);
        start = comma + 1;
    }
    return state;
}

/// 地形单元 → 方块记法。未知字符返回空串（调用方跳过）。
std::string cellToBlockId(char cell)
{
    switch (cell) {
        case '.':
            return "minecraft:air";
        case 'S':
            return "minecraft:stone";
        case 'G':
            return "minecraft:glass";
        case 's':
            return "minecraft:smooth_stone_slab[type=bottom]";
        case 'R':
            return "minecraft:redstone_block";
        case 'T':
            return "minecraft:redstone_torch[lit=true]";
        case 't':
            return "minecraft:redstone_torch[lit=false]";
        case 'L':
            return "minecraft:lever[face=floor,facing=north,powered=true]";
        case 'w':
            return "minecraft:redstone_wire[power=0,north=side,south=side,east=side,west=side]";
        case 'x':
            return "minecraft:redstone_wire[power=0,north=none,south=none,east=none,west=none]";
        default:
            return {};
    }
}

/**
 * @brief 把一张 5×5 薄片摆进世界。
 *
 * 坐标约定：行号增大对应 +Z（南），列号增大对应 +X（东）；第 2 行第 2 列是原点。
 * `skipCenter` 为真时跳过中心格（中心层由调用方单独放主语）。
 */
void buildLayer(SignalWorld& world, const Layer& layer, i32 y, bool skipCenter)
{
    for (i32 row = 0; row < 5; ++row) {
        for (i32 col = 0; col < 5; ++col) {
            if (skipCenter && row == 2 && col == 2) {
                continue;
            }
            const char* cell = layer[static_cast<size_t>(row * 5 + col)];
            const std::string blockId = cellToBlockId(cell[0]);
            if (blockId.empty() || blockId == "minecraft:air") {
                continue;
            }
            const BlockState* state = parseSubject(blockId);
            if (state == nullptr) {
                std::fprintf(stderr, "redstone_signal: 无法解析方块 '%s'\n", blockId.c_str());
                continue;
            }
            world.put(col - 2, y, row - 2, *state);
        }
    }
}

// ============================================================================
// 查询
// ============================================================================

json directionMap(const std::function<i32(Direction)>& query)
{
    json result = json::object();
    for (Direction dir : Directions::all()) {
        result[Directions::toString(dir)] = query(dir);
    }
    return result;
}

/// 六个邻居能提供的最强信号（原版 `SignalGetter#getBestNeighborSignal` 的等价物）。
i32 bestNeighbor(IWorld& world, const BlockPos& pos)
{
    i32 best = 0;
    for (Direction side : Directions::all()) {
        best = std::max(best, world::redstone::RedstonePower::getSignal(world, pos.offset(side), side));
        if (best >= 15) {
            return best;
        }
    }
    return best;
}

/// 水平方向 → 红石线的连接属性。
const mc::EnumProperty<mc::BlockStateProperties::RedstoneSide>& redstoneSideProperty(mc::Direction side)
{
    using mc::BlockStateProperties;
    switch (side) {
        case mc::Direction::North:
            return BlockStateProperties::REDSTONE_NORTH();
        case mc::Direction::East:
            return BlockStateProperties::REDSTONE_EAST();
        case mc::Direction::South:
            return BlockStateProperties::REDSTONE_SOUTH();
        default:
            return BlockStateProperties::REDSTONE_WEST();
    }
}

/// 红石线自身的「掩码」开关（原版 `RedStoneWireBlock#shouldSignal`）。
blocks::RedstoneWireBlock* wireBlock()
{
    return VanillaBlocks::REDSTONE_WIRE == nullptr
        ? nullptr
        : static_cast<blocks::RedstoneWireBlock*>(VanillaBlocks::REDSTONE_WIRE);
}

void writeList(FILE* out)
{
    for (const Scenario& scenario : kScenarios) {
        std::fprintf(out, "%s\n", scenario.unit);
    }
}

std::string runLine(std::string_view inputLine)
{
    static bool s_initialised = false;
    if (!s_initialised) {
        s_initialised = true;
        datapack::initializeRegistries();
    }

    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const Scenario* scenario = nullptr;
    for (const Scenario& candidate : kScenarios) {
        if (unit == candidate.unit) {
            scenario = &candidate;
            break;
        }
    }
    if (scenario == nullptr) {
        return json{{"unit", unit}, {"ok", false}, {"error", "unknown scenario"}}.dump();
    }

    // 红石线的掩码开关是全局状态：每个场景都从「开」开始，避免上一场景的遗留影响。
    if (auto* wire = wireBlock()) {
        wire->setSelfSignalling(true);
    }

    const BlockState* subject = parseSubject(scenario->subject);
    if (subject == nullptr) {
        return json{{"unit", unit}, {"ok", false}, {"error", "unknown block"}}.dump();
    }

    SignalWorld world;
    buildLayer(world, scenario->plane, 0, true);
    buildLayer(world, scenario->above, 1, false);
    buildLayer(world, scenario->below, -1, false);
    world.put(0, 0, 0, *subject);

    const BlockPos origin(0, 0, 0);

    using world::redstone::RedstonePower;

    json result = json::object();
    result["unit"] = unit;
    result["ok"] = true;

    // 第 1 段：不改掩码，逐点问答（比的是各方块自己的信号属性）。
    // 三个字段与原版三条入口一一对应，口径见文件头的「字段与原版的对应」。
    result["signal"] = directionMap([&](Direction side) {
        return RedstonePower::getSignal(world, origin, side);
    });
    result["weak"] = directionMap([&](Direction side) {
        return RedstonePower::getWeakPower(world, origin, side);
    });
    result["strong"] = directionMap([&](Direction side) {
        return RedstonePower::getStrongPower(world, origin, side);
    });
    result["bestNeighbor"] = bestNeighbor(world, origin);

    // 第 2 段：关掉红石线的自我输出后再问一遍，对应原版 `getBlockSignal`。
    if (auto* wire = wireBlock()) {
        wire->setSelfSignalling(false);
    }
    result["quietSignal"] = directionMap([&](Direction side) {
        return RedstonePower::getSignal(world, origin, side);
    });
    result["quietBestNeighbor"] = bestNeighbor(world, origin);

    if (VanillaBlocks::REDSTONE_WIRE != nullptr && subject->is(VanillaBlocks::REDSTONE_WIRE)) {
        const auto* wire = static_cast<const blocks::RedstoneWireBlock*>(&subject->getBlock());
        json wireInfo = json::object();
        wireInfo["neighborPower"] = static_cast<i32>(RedstonePower::getRedstonePowerFromNeighbors(world, origin));
        wireInfo["input"] = wire->calculateInputPowerForQuery(world, origin, *subject);
        // 连接状态读的是主语「当前存着」的属性值，而不是现场重新推演：地形是逐格写进世界的，
        // 没有跑过一次 updateShape，两侧存的属性才是同一份输入。现场推演会把「地形怎么摆」
        // 混进答案里，与另一侧直接读属性的口径对不上。
        json links = json::object();
        for (Direction side : Directions::horizontal()) {
            const bool connected = subject->get(redstoneSideProperty(side)) != BlockStateProperties::RedstoneSide::None;
            links[Directions::toString(side)] = connected ? "side" : "none";
        }
        wireInfo["links"] = links;
        result["wire"] = wireInfo;
    }

    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("redstone_signal", &writeList, &runLine)

} // namespace mc::diff
