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

/**
 * @file BannerPatternLayers.hpp
 * @brief `minecraft:banner_patterns` 组件的 JSON ↔ 内部表示
 *
 * 组件在数据包 JSON 里是 `[{"pattern": "<图案 ID>", "color": "<染料颜色>"}, ...]`，
 * 内部沿用旗帜方块实体那套 (图案枚举, 颜色索引) 表示——渲染与 NBT 往返都只认后者。
 */

#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc::item::component {

/// 一层旗帜图案：图案（枚举）与染料颜色索引
struct BannerPatternLayer {
    /// 图案枚举（`BannerPatternType` 的数值）
    int pattern = 0;
    /// 染料颜色索引（`_colorNames` 的下标：0 = white … 15 = black）
    int color = 0;
};

/**
 * @brief 解析 `banner_patterns` 组件 JSON
 *
 * @param json 组件 JSON 值
 * @param out 解析结果（成功时覆盖）
 * @return 结构合法时为 true；图案或颜色无法识别时同样返回 true 并跳过该层（与原版解码失败
 *         时丢弃该层的处理一致），整体结构错误（不是数组、元素不是对象）返回 false
 */
[[nodiscard]] bool readBannerPatternLayers(const nlohmann::json& json, std::vector<BannerPatternLayer>& out);

/**
 * @brief 写出 `banner_patterns` 组件 JSON
 */
[[nodiscard]] nlohmann::json writeBannerPatternLayers(const std::vector<BannerPatternLayer>& layers);

} // namespace mc::item::component