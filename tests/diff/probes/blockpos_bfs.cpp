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
 * @brief 差分能力：`BlockPos.breadthFirstTraversal` 的遍历顺序与计数（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "count": N, "order": "x,y,z;x,y,z;..." }
 *
 * 复刻原版：
 *
 * ```java
 * public static int breadthFirstTraversal(
 *     BlockPos origin, int maxDepth, int maxCount,
 *     BiConsumer<BlockPos, Consumer<BlockPos>> consumer,
 *     Function<BlockPos, TraversalNodeStatus> nodeStatus) {
 *     Queue<Pair<BlockPos, Integer>> queue = new ArrayDeque<>();
 *     LongSet visited = new LongOpenHashSet();
 *     queue.add(Pair.of(origin, 0));
 *     int count = 0;
 *
 *     while (!queue.isEmpty()) {
 *         Pair<BlockPos, Integer> pair = queue.poll();
 *         BlockPos pos = pair.getLeft();
 *         int depth = pair.getRight();
 *         long key = pos.asLong();
 *         if (visited.add(key)) {                       // 去重在状态判断之前
 *             TraversalNodeStatus status = nodeStatus.apply(pos);
 *             if (status != SKIP) {
 *                 if (status == STOP) { break; }        // STOP 立即跳出
 *                 if (++count >= maxCount) { return count; }   // 先自增再比较
 *                 if (depth < maxDepth) { consumer.accept(pos, child -> queue.add(child, depth + 1)); }
 *             }
 *         }
 *     }
 *     return count;
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **去重发生在状态函数之前。** `visited.add(key)` 为假时直接跳过，**连 `nodeStatus` 都不
 *    调用**。所以同一个位置被多个父节点指向时，只有第一次能触发状态判断与扩散。这也意味着
 *    **`SKIP` 的节点已经被标记为访问过**——后续再遇到它不会重试。
 *
 * 2. **`if (++count >= maxCount) return count;` 是先自增再比较。** `maxCount = 1` 时访问第一个
 *    节点后立即返回，计数为 1。这与 `if (count++ >= ...)` 不同，后者会多访问一个。
 *
 * 3. **`depth < maxDepth` 是严格小于。** `maxDepth = 1` 时只从深度 0 的起点扩散一次，产生深度
 *    1 的子节点；这些子节点**被访问**但不再扩散。所以能触达的最大深度恰好等于 `maxDepth`。
 *
 * 4. **`SKIP` 不计数也不扩散，但 `STOP` 会中断整个遍历。** `SKIP` 只是「这个节点跳过」，遍历
 *    继续；`STOP` 是「到此为止」，`break` 出循环后返回当前计数。两者都**不会**把当前节点计入。
 *
 * 单元格式：`<maxDepth>|<maxCount>|<规则>`
 *
 * `规则` 是一串按位置生效的字符，用于构造不确定的地形。当前支持的规则：
 * - `free` —— 全部可通行（4 个水平方向 + 上下，共 6 个邻居）
 * - `skipX0` —— `x == 0` 的节点返回 `SKIP`
 * - `stopX1` —— `x == 1` 的节点返回 `STOP`
 * - `skipOdd` —— `x` 为奇数时返回 `SKIP`
 *
 * 邻居集合固定为 6 个方向（`Direction.values()` 顺序：`DOWN, UP, NORTH, SOUTH, WEST, EAST`）。
 *
 * TODO: 未覆盖 `asLong` 的位打包细节（见 blockpos_codec）、真实方块的邻居函数、
 * `TraversalNodeStatus` 在其他调用方（红石线、流体）里的具体判定。
 */
#include <cstdio>
#include <deque>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 原版 `TraversalNodeStatus`。
enum class NodeStatus { ACCEPT, SKIP, STOP };

/// 原版 `Direction.values()` 的顺序：DOWN, UP, NORTH, SOUTH, WEST, EAST。
constexpr i32 NEIGHBOR_OFFSETS[6][3] = {
    {0, -1, 0}, // DOWN
    {0, 1, 0},  // UP
    {0, 0, -1}, // NORTH
    {0, 0, 1},  // SOUTH
    {-1, 0, 0}, // WEST
    {1, 0, 0},  // EAST
};

using Pos = std::tuple<i32, i32, i32>;

NodeStatus evaluate(const std::string& rule, const Pos& pos)
{
    const i32 x = std::get<0>(pos);
    if (rule == "free") {
        return NodeStatus::ACCEPT;
    }
    if (rule == "skipX0") {
        return x == 0 ? NodeStatus::SKIP : NodeStatus::ACCEPT;
    }
    if (rule == "stopX1") {
        return x == 1 ? NodeStatus::STOP : NodeStatus::ACCEPT;
    }
    if (rule == "skipOdd") {
        return (x % 2 != 0) ? NodeStatus::SKIP : NodeStatus::ACCEPT;
    }
    throw std::runtime_error("blockpos_bfs: 未知规则：" + rule);
}

struct TraversalResult {
    i32 count = 0;
    std::vector<Pos> order;
};

TraversalResult traverse(i32 maxDepth, i32 maxCount, const std::string& rule)
{
    std::deque<std::pair<Pos, i32>> queue;
    std::set<Pos> visited;
    queue.emplace_back(Pos{0, 0, 0}, 0);

    TraversalResult result;
    while (!queue.empty()) {
        auto [pos, depth] = queue.front();
        queue.pop_front();

        // 去重在状态判断之前：重复访问的节点不触发状态函数。
        if (!visited.insert(pos).second) {
            continue;
        }

        const NodeStatus status = evaluate(rule, pos);
        if (status == NodeStatus::SKIP) {
            continue;
        }
        if (status == NodeStatus::STOP) {
            break;
        }

        // 先自增再比较。
        ++result.count;
        result.order.push_back(pos);
        if (result.count >= maxCount) {
            return result;
        }

        if (depth < maxDepth) {
            for (const auto& offset : NEIGHBOR_OFFSETS) {
                const Pos child{
                    std::get<0>(pos) + offset[0], std::get<1>(pos) + offset[1], std::get<2>(pos) + offset[2]};
                queue.emplace_back(child, depth + 1);
            }
        }
    }
    return result;
}

/// 单元表。
constexpr const char* UNITS[] = {
    // 深度 0：只有起点
    "0|100|free",
    // 深度 1：起点 + 6 个邻居
    "1|100|free",
    // 深度 2：起点 + 6 + 次层（去重后）
    "2|100|free",
    // 计数上限
    "2|1|free",
    "2|2|free",
    "2|3|free",
    "2|7|free",
    "2|8|free",
    // 计数上限恰好等于总数
    "1|7|free",
    // SKIP：起点被跳过 -> 整个遍历立即结束
    "2|100|skipX0",
    // SKIP：x==0 的节点跳过
    "2|100|skipOdd",
    // STOP
    "2|100|stopX1",
    // 深 + 大计数（验证去重后的总数稳定）
    "3|1000|free",
    // 深度上限 0 且计数上限 1
    "0|1|free",
};

void writeList(FILE* out)
{
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

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() != 3) {
        throw std::runtime_error("blockpos_bfs: 单元应为 '<maxDepth>|<maxCount>|<规则>'：" + unit);
    }

    const i32 maxDepth = std::stoi(parts[0]);
    const i32 maxCount = std::stoi(parts[1]);
    const std::string rule = parts[2];

    const TraversalResult result = traverse(maxDepth, maxCount, rule);

    std::string order;
    for (const auto& pos : result.order) {
        if (!order.empty()) {
            order += ";";
        }
        order += std::to_string(std::get<0>(pos)) + "," + std::to_string(std::get<1>(pos)) + "," +
            std::to_string(std::get<2>(pos));
    }
    if (order.empty()) {
        order = "-";
    }

    return json{{"unit", unit}, {"count", result.count}, {"order", order}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(blockpos_bfs, &writeList, &runLine)

} // namespace mc::diff
