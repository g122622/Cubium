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
#include "common/resource/ResourceLocation.hpp"
#include <string>
#include <unordered_map>

namespace mc {

/**
 * @brief 盔甲纹饰材料（`trim_material`）的数据定义
 *
 * 对应原版
 * {@code record TrimMaterial(MaterialAssetGroup assets, Component description)}。
 *
 * 数据包 `data/<ns>/trim_material/` 下的每个 JSON 定义一种材料（原版 11 种）。
 *
 * 结构对应关系：
 * - `asset_name` + `override_armor_assets` 合并为原版的 `assets`（`MaterialAssetGroup`）：
 *   前者是 `assets.base.suffix`，后者是 `assets.overrides`（装备材质键 → 后缀）。
 * - `description` 只保留翻译键与颜色，理由同 `TrimPattern`。
 */
class TrimMaterial {
public:
    /**
     * @param assetName 基础贴图后缀（数据文件字段 `asset_name`）
     * @param descriptionKey 描述翻译键（数据文件字段 `description.translate`）
     * @param descriptionColor 描述颜色（数据文件字段 `description.color`，可能为空）
     * @param overrides 覆盖的装备材质后缀表（数据文件字段 `override_armor_assets`）
     */
    TrimMaterial(std::string assetName,
        std::string descriptionKey,
        std::string descriptionColor,
        std::unordered_map<ResourceLocation, std::string> overrides);

    /**
     * @brief 基础贴图后缀
     *
     * 例如 `gold`，对应贴图路径中的材质段。
     */
    [[nodiscard]] const std::string& getAssetName() const noexcept
    {
        return m_assetName;
    }

    [[nodiscard]] const std::string& getDescriptionKey() const noexcept
    {
        return m_descriptionKey;
    }

    /**
     * @brief 描述颜色（形如 `#DEB12D`），数据文件未给出时为空串
     */
    [[nodiscard]] const std::string& getDescriptionColor() const noexcept
    {
        return m_descriptionColor;
    }

    /**
     * @brief 覆盖的装备材质后缀表
     *
     * 空表表示所有装备都用基础后缀。
     */
    [[nodiscard]] const std::unordered_map<ResourceLocation, std::string>& getOverrides() const noexcept
    {
        return m_overrides;
    }

private:
    std::string m_assetName;
    std::string m_descriptionKey;
    std::string m_descriptionColor;
    std::unordered_map<ResourceLocation, std::string> m_overrides;
};

} // namespace mc
