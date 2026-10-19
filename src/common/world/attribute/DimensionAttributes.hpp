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

#include "common/world/attribute/EnvironmentAttributeMap.hpp"
#include "common/world/attribute/EnvironmentAttributeSystem.hpp"

#include <memory>
#include <vector>

namespace mc {

class DimensionType;

namespace world {
namespace attribute {

/**
 * @brief 维度级环境属性装配
 *
 * 负责把维度类型声明的常量属性（`dimension_type.attributes`）与时间线
 * （`dimension_type.timelines`）组装成 EnvironmentAttributeSystem 的默认层。
 *
 * Cubium 只覆盖服务端玩法所需，群系位置层（`biome.attributes` 的空间插值）与天气层
 * 尚未接入；装饰性视觉效果（天空色、雾色等）的时间线层照常构建，
 * 供诊断与后续渲染/天气实现查询。
 */
class DimensionAttributes {
public:
    /**
     * @brief 为维度构建环境属性系统
     *
     * 层序：维度常量层 → 时间线层（按维度声明的时间线顺序）。
     * 时间线按环境属性注册顺序遍历，每个属性从各自所属时间线创建轨道采样器。
     *
     * @param dimensionType 维度类型
     * @param dayTimeGetter 获取当前 dayTime 的函数（通常为世界的 dayTimeOfDay）
     * @param gameTimeGetter 获取当前累积游戏时间的函数（无周期时间线使用）
     * @return 构建完成的环境属性系统
     */
    [[nodiscard]] static std::unique_ptr<EnvironmentAttributeSystem> createSystem(const DimensionType& dimensionType,
        std::function<i64()> dayTimeGetter,
        std::function<i64()> gameTimeGetter);
};

} // namespace attribute
} // namespace world
} // namespace mc
