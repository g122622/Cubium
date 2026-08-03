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

#include "EnvironmentScanPlacement.hpp"
#include "Placement.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <vector>

namespace mc {

/**
 * @brief 放置器工具函数
 *
 * 提供常用的放置器构建辅助函数。
 */
namespace PlacementUtils {

/**
 * @brief 在放置链末尾添加生物群系过滤（白名单模式）
 *
 * @param root 放置链根节点
 * @param allowedBiomes 允许的生物群系ID列表
 * @return 带有生物群系过滤的放置链
 */
[[nodiscard]] std::unique_ptr<ConfiguredPlacement> appendBiomePlacement(
    std::unique_ptr<ConfiguredPlacement> root, std::vector<u32> allowedBiomes);

/**
 * @brief 在放置链末尾添加环境扫描（向上扫描寻找实心面）
 *
 * 参考 MC 1.21.11: EnvironmentScanPlacement.upward()
 * 扫描方向 UP，目标条件为 hasSturdyFace(DOWN)，允许搜索条件为 onlyInAir。
 *
 * @param root 放置链根节点
 * @param maxSteps 最大扫描步数（1~32，默认12）
 * @return 带有向上环境扫描的放置链
 */
[[nodiscard]] std::unique_ptr<ConfiguredPlacement> appendEnvironmentScanUp(
    std::unique_ptr<ConfiguredPlacement> root, i32 maxSteps = 12);

/**
 * @brief 在放置链末尾添加环境扫描（向下扫描寻找实心面）
 *
 * 参考 MC 1.21.11: EnvironmentScanPlacement.downward()
 * 扫描方向 DOWN，目标条件为 hasSturdyFace(UP)，允许搜索条件为 onlyInAir。
 *
 * @param root 放置链根节点
 * @param maxSteps 最大扫描步数（1~32，默认12）
 * @return 带有向下环境扫描的放置链
 */
[[nodiscard]] std::unique_ptr<ConfiguredPlacement> appendEnvironmentScanDown(
    std::unique_ptr<ConfiguredPlacement> root, i32 maxSteps = 12);

/**
 * @brief 在放置链末尾添加垂直随机偏移
 *
 * 参考 MC 1.21.11: RandomOffsetPlacement.vertical(ConstantInt(offset))
 *
 * @param root 放置链根节点
 * @param offset Y轴偏移量（正值向上，负值向下）
 * @return 带有垂直偏移的放置链
 */
[[nodiscard]] std::unique_ptr<ConfiguredPlacement> appendVerticalOffset(
    std::unique_ptr<ConfiguredPlacement> root, i32 offset);

/**
 * @brief 创建地表放置链（带数量）
 *
 * 创建 Count -> Square -> Surface 链。
 *
 * @param count 每区块尝试次数
 * @param maxWaterDepth 最大水深（树木不能种在太深的水中）
 * @return 放置链
 */
[[nodiscard]] std::unique_ptr<ConfiguredPlacement> createCountedSurfacePlacement(i32 count, i32 maxWaterDepth = 0);

/**
 * @brief 创建地表放置链（带概率）
 *
 * 创建 Chance -> Square -> Surface 链。
 *
 * @param chance 成功概率（0.0 - 1.0）
 * @param maxWaterDepth 最大水深
 * @return 放置链
 */
[[nodiscard]] std::unique_ptr<ConfiguredPlacement> createChanceSurfacePlacement(f32 chance, i32 maxWaterDepth = 0);

/**
 * @brief 创建高度范围放置链（带数量）
 *
 * 创建 Count -> Square -> HeightRange 链。
 *
 * @param count 每区块尝试次数
 * @param minY 最小Y坐标
 * @param maxY 最大Y坐标
 * @return 放置链
 */
[[nodiscard]] std::unique_ptr<ConfiguredPlacement> createCountedHeightPlacement(i32 count, i32 minY, i32 maxY);

} // namespace PlacementUtils

} // namespace mc
