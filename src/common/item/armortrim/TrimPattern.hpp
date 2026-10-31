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

namespace mc {

/**
 * @brief 盔甲纹饰图案（`trim_pattern`）的数据定义
 *
 * 对应原版 {@code record TrimPattern(Identifier assetId, Component description, boolean decal)}。
 *
 * 数据包 `data/<ns>/trim_pattern/` 下的每个 JSON 定义一个图案（原版 18 种）。
 *
 * `description` 在数据文件里是**带样式的文本组件**，本类只保留翻译键：图案的描述文本
 * 不参与玩法判定，翻译键稳定且可逐字节比较。若后续 UI 需要样式，应改为保存完整组件。
 */
class TrimPattern {
public:
    /**
     * @param assetId 贴图资源 ID（数据文件字段 `asset_id`）
     * @param descriptionKey 描述翻译键（数据文件字段 `description.translate`）
     * @param decal 是否为贴花（数据文件字段 `decal`）
     */
    TrimPattern(ResourceLocation assetId, std::string descriptionKey, bool decal);

    [[nodiscard]] const ResourceLocation& getAssetId() const noexcept
    {
        return m_assetId;
    }

    [[nodiscard]] const std::string& getDescriptionKey() const noexcept
    {
        return m_descriptionKey;
    }

    /**
     * @brief 是否为贴花
     *
     * 贴花图案只渲染纹饰纹理本身，不额外渲染底层盔甲材质的明暗变化。
     */
    [[nodiscard]] bool isDecal() const noexcept
    {
        return m_decal;
    }

private:
    ResourceLocation m_assetId;
    std::string m_descriptionKey;
    bool m_decal;
};

} // namespace mc
