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
#include "common/world/gen/surface/VerticalAnchor.hpp"
#include "common/world/gen/valueprovider/HeightProvider.hpp"

#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace world {
namespace gen {
namespace valueprovider {

/**
 * @brief VerticalAnchor / HeightProvider JSON 反序列化工具
 *
 * 从数据包 JSON 解析 VerticalAnchor 与 HeightProvider，支持 MC 1.21.11 数据包格式。
 *
 * VerticalAnchor 简写格式（单键对象）：
 * - { "absolute": 0 }      -> absolute(0)
 * - { "above_bottom": 8 }  -> aboveBottom(8)
 * - { "below_top": 0 }     -> belowTop(0)
 *
 * HeightProvider 分派格式：
 * - { "type": "minecraft:uniform",
 *     "min_inclusive": { "absolute": 0 },
 *     "max_inclusive": { "below_top": 0 } }
 * - { "type": "minecraft:constant", "value": { "absolute": 64 } }
 * - { "type": "minecraft:biased_to_bottom", "min_inclusive": {...}, "max_inclusive": {...}, "inner": 1 }
 * - { "type": "minecraft:very_biased_to_bottom", ... }
 * - { "type": "minecraft:trapezoid", "min_inclusive": {...}, "max_inclusive": {...}, "plateau": 0 }
 *
 * 命名空间前缀 "minecraft:" 可选。
 */
namespace HeightProviderParser {

/**
 * @brief 从 JSON 解析 VerticalAnchor
 *
 * 接受单键对象 {"absolute":N} / {"above_bottom":N} / {"below_top":N}。
 *
 * @param json JSON 对象
 * @return VerticalAnchor，或错误
 */
[[nodiscard]] Result<surface::VerticalAnchor> parseAnchor(const nlohmann::json& json);

/**
 * @brief 从 JSON 解析 HeightProvider
 *
 * 接受 type 分派对象（uniform/constant/biased_to_bottom/very_biased_to_bottom/trapezoid）。
 *
 * @param json JSON 对象
 * @return HeightProvider，或错误
 */
[[nodiscard]] Result<std::unique_ptr<HeightProvider>> parse(const nlohmann::json& json);

} // namespace HeightProviderParser

} // namespace valueprovider
} // namespace gen
} // namespace world
} // namespace mc
