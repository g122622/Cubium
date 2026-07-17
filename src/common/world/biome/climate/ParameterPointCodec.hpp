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
#include "common/world/biome/climate/ParameterTypes.hpp"

#include <nlohmann/json_fwd.hpp>

namespace mc::world::biome::climate {

/**
 * @brief 气候参数点 JSON 反序列化器（MC 1.21.11）
 *
 * 对应原版 Climate.ParameterPoint.CODEC，解析 noise_settings 的 spawn_target 数组元素。
 *
 * 每个气候参数（temperature/humidity/continentalness/erosion/depth/weirdness）的 JSON
 * 形态由 Climate.Parameter.CODEC（= ExtraCodecs.intervalCodec）决定，三种合法形态：
 * - 裸数字 float    → point(value)        // min == max
 * - [min, max] 数组 → span(min, max)
 * - {min,max} 对象  → span(min, max)
 *
 * offset 是裸 float（0.0~1.0），量化为 i64。
 *
 * 全部 6 气候参数 + offset 必填（原版 RecordCodecBuilder，无默认）。
 */
class ParameterPointCodec {
public:
    /** 解析单个 spawn_target 元素 JSON 为 ParameterPoint。 */
    [[nodiscard]] static Result<ParameterPoint> fromJson(const nlohmann::json& element);

    /** 解析 spawn_target 数组为 ParameterPoint 列表（空数组合法）。 */
    [[nodiscard]] static Result<std::vector<ParameterPoint>> fromJsonArray(const nlohmann::json& array);
};

} // namespace mc::world::biome::climate
