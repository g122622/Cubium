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

#include "common/core/Result.hpp"
#include "common/entity/ai/brain/schedule/Activity.hpp"
#include "common/util/TriState.hpp"
#include "common/world/attribute/AttributeType.hpp"
#include "common/world/level/MoonPhase.hpp"

namespace mc {
namespace world {
namespace attribute {

/**
 * @brief 预定义属性类型
 *
 * MC 1.21.11 interface AttributeTypes。
 * 每个属性类型对应 vanilla 注册表中的一项（注册名为 `minecraft:<type>`）。
 *
 * 除 BED_RULE 外的 vanilla 类型均已实现；BED_RULE 依赖床行为规则子系统
 * （BedRule 类型与序列化）落地后补充，届时需同步登记到环境属性注册表。
 */
class AttributeTypes {
public:
    /**
     * @brief 布尔类型
     *
     * 不可插值；允许 AND/NAND/OR/NOR/XOR/XNOR 六种布尔修改器。
     */
    [[nodiscard]] static const AttributeType<bool>& BOOLEAN();

    /**
     * @brief 三态类型
     *
     * 不可插值；无修饰符库。
     */
    [[nodiscard]] static const AttributeType<util::TriState>& TRI_STATE();

    /**
     * @brief 浮点类型
     *
     * 线性插值；允许 ADD/SUBTRACT/MULTIPLY/MINIMUM/MAXIMUM 五种浮点修改器。
     */
    [[nodiscard]] static const AttributeType<f32>& FLOAT();

    /**
     * @brief 角度（度）类型
     *
     * 关键帧插值按角度环绕（差值绝对值 >= 90° 时直接取目标值）；
     * 允许 ADD/SUBTRACT/MULTIPLY/MINIMUM/MAXIMUM 五种浮点修改器。
     */
    [[nodiscard]] static const AttributeType<f32>& ANGLE_DEGREES();

    /**
     * @brief RGB 颜色类型（低 24 位）
     *
     * sRGB 线性插值；允许 ALPHA_BLEND/ADD/SUBTRACT/MULTIPLY/BLEND_TO_GRAY。
     */
    [[nodiscard]] static const AttributeType<i32>& RGB_COLOR();

    /**
     * @brief ARGB 颜色类型
     *
     * sRGB 线性插值；允许 ALPHA_BLEND/ADD/SUBTRACT/MULTIPLY/BLEND_TO_GRAY。
     */
    [[nodiscard]] static const AttributeType<i32>& ARGB_COLOR();

    /**
     * @brief 月相类型
     *
     * 不可插值；无修饰符库。
     */
    [[nodiscard]] static const AttributeType<MoonPhase>& MOON_PHASE();

    /**
     * @brief 活动类型
     *
     * 不可插值：keyframeLerp = ofStep(1.0)，采样进度到达或越过 toTicks 时
     * 返回下一关键帧的值，否则保持当前关键帧的值。
     */
    [[nodiscard]] static const AttributeType<entity::ai::brain::schedule::Activity>& ACTIVITY();

    /**
     * @brief 把全部属性类型登记进 AttributeTypeRegistry
     *
     * 可在属性被引用前多次调用：已存在的同名条目会被跳过。
     *
     * @return 本次新登记的类型数量
     */
    static Result<size_t> registerAll();
};

} // namespace attribute
} // namespace world
} // namespace mc