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
#include "common/util/color/DyeColor.hpp"

#include <optional>
#include <vector>

namespace mc {

/**
 * @brief 可染色物品的颜色混色（vanilla DyedItemColor.applyDyes 的算法部分）
 *
 * vanilla 1.21.11 的 `DyedItemColor.applyDyes(ItemStack, List<DyeItem>)` 做两件事：
 *   1. 判定物品是否属于 `ItemTags.DYEABLE`（含 4 件皮革盔甲、皮革马铠、狼铠）；
 *   2. 把「当前颜色（若有）」与「全部染料颜色」**一次性**累加求混合色。
 *
 * 本文件只承载第 2 步的纯算法（与物品/NBT 无关，便于单元测试与跨子系统复用）；
 * 第 1 步的标签判定由调用方（合成配方 / 炼药锅 / 实体交互）各自用 `ItemTags::DYEABLE()` 完成。
 *
 * 算法（对齐 vanilla DyedItemColor.applyDyes）：
 * ```text
 * 对每个参与混色的颜色（当前颜色若存在 + 每个染料颜色）：
 *     sumR += r; sumG += g; sumB += b;
 *     sumMax += max(r, g, b);
 *     n++;
 * avgR = sumR / n; avgG = sumG / n; avgB = sumB / n;   // 整数除法，向零截断
 * f     = sumMax / n;                                  // float
 * maxAvg = max(avgR, avgG, avgB);
 * avgR = (int)(avgR * f / maxAvg);                     // 亮度归一化：把结果拉回 f 量级
 * avgG = (int)(avgG * f / maxAvg);
 * avgB = (int)(avgB * f / maxAvg);
 * ```
 *
 * 关键点（这也是本算法与"逐次两两平均"的差异所在）：
 * - **一次性**处理全部染料，而非逐个与当前颜色二平均——逐次平均结果顺序敏感且丢失亮度缩放；
 * - 每个参与色额外累加其 `max(r,g,b)`，最后用 `avgMax / max(avgChannel)` 做亮度归一化，
 *   使混色结果保持与输入相当的明度（等量的白+黑不会退化成中灰而是被拉亮）。
 */
class DyedItemColor {
public:
    /// 皮革的默认颜色（vanilla DyedItemColor.LEATHER_COLOR，即 0xFFA06540 的不透明形式）
    static constexpr u32 LEATHER_COLOR = 0xA06540;

    /**
     * @brief 混色：把全部染料颜色一次性叠加到当前颜色上
     *
     * @param currentColor 物品当前颜色（0xRRGGBB 或 0xAARRGGBB）。若物品尚无颜色（未染色），
     *        传 std::nullopt——此时只有染料参与平均，对齐 vanilla 中 `DYED_COLOR` 组件为空的
     *        分支（不把默认色计入累加）。
     * @param dyeColors 全部染料的纹理漫反射色（用 [[dyeTextureDiffuseColor]] 取得），0xRRGGBB
     * @return 混合后的 0xRRGGBB 颜色；dyeColors 为空时返回 currentColor（无颜色时返回 0）
     */
    [[nodiscard]] static u32 applyDyes(std::optional<u32> currentColor, const std::vector<u32>& dyeColors) noexcept;
};

} // namespace mc
