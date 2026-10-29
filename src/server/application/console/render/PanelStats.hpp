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

#pragma once

#include "common/core/Types.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc {
class BlockState;

namespace nbt::tags {
struct tag;
} // namespace nbt::tags
} // namespace mc

namespace mc::server::console {

/**
 * @brief 区块状态统计
 *
 * 对应原版 `/forceload query`、`/debug` 之外的区块状态视角：哪些区块已完全加载、
 * 哪些已经过主线程后处理、哪些处于"方块可 tick"范围。三者是包含关系
 * （ticking ⊆ loaded，post-processed 与 loaded 各自统计），此处分别给出。
 */
struct ChunkStatusStats {
    usize loadedChunks = 0;        ///< 已加载区块数
    usize tickingChunks = 0;       ///< 处于玩家模拟距离内（方块可 tick）的区块数
    usize postProcessedChunks = 0; ///< 已完成主线程后处理（实体生成/光照/发送）的区块数
    usize loadedEntities = 0;      ///< 加载区块中承载的实体数（含未进入 EntityManager 的）
};

/**
 * @brief 把区块状态统计格式化为右侧面板的行
 *
 * 每行以空格缩进（与面板其它段落一致），不含段落标题。
 *
 * @param stats 区块状态统计
 * @return 面板行（按显示顺序）
 */
[[nodiscard]] std::vector<std::string> formatChunkStatusLines(const ChunkStatusStats& stats);

/**
 * @brief 把整数值格式化为带千位分隔符的文本（12345 → "12,345"）
 *
 * 面板中区块/实体数动辄五位数，千位分隔让读数更快。负数按符号处理。
 *
 * @param value 整数值
 * @return 格式化文本
 */
[[nodiscard]] std::string formatThousands(i64 value);

/**
 * @brief 把一组「名称 → 数量」格式化为紧凑的排行行
 *
 * 用于方块/群系 Top-N：输出形如 `" stone 1200 (31%)"` 的行，数量按降序排列并截断到
 * 最多 `maxEntries` 项。百分比按总数归一化，总数为 0 时不输出百分比。
 *
 * @param counts 名称到数量的映射（输入会被排序，调用方可传值）
 * @param maxEntries 最多输出几项
 * @param total 归一化总数（通常等于全部项之和，允许大于计数和）
 * @return 面板行（按数量降序）
 */
[[nodiscard]] std::vector<std::string> formatRankedCounts(
    std::vector<std::pair<std::string, i64>> counts, usize maxEntries, i64 total);

/**
 * @brief 去掉资源位置的命名空间前缀（"minecraft:stone" → "stone"）
 *
 * @param id 资源位置
 * @return 去掉前缀的文本；无前缀时原样返回
 */
[[nodiscard]] std::string stripNamespace(std::string_view id);

/**
 * @brief 把方块状态格式化为命令风格的文本（"stone" 或 "oak_log[axis=y]"）
 *
 * @param state 方块状态（可为空，返回 "unknown"）
 * @return 供面板展示的文本
 */
[[nodiscard]] std::string describeBlockState(const BlockState* state);

/**
 * @brief 把 NBT 复合标签格式化为紧凑的单行 SNBT
 *
 * 面板每行只放得下一行文本，故把标签树压成一行并限制长度。
 *
 * @param tag NBT 标签
 * @param maxLength 最大字符数（超出时截断并追加 "..."）
 * @return SNBT 文本
 */
[[nodiscard]] std::string describeNbt(const nbt::tags::tag& tag, usize maxLength);

} // namespace mc::server::console
