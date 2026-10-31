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
#include <optional>
#include <string>

namespace mc {

/**
 * @brief 画（`painting_variant`）变体的数据定义
 *
 * 对应原版
 * {@code record PaintingVariant(int width, int height, Identifier assetId,
 *  Optional<Component> title, Optional<Component> author)}
 *
 * `title` / `author` 在数据文件里是**带样式的文本组件**，但本类只保留其翻译键：
 * 画作标题与作者本身不参与任何玩法判定，保留样式信息没有用处，而翻译键是稳定且可
 * 逐字节比较的。若后续渲染需要样式，应改为保存完整组件。
 */
class PaintingVariant {
public:
    /**
     * @param width 宽度（方块数）
     * @param height 高度（方块数）
     * @param assetId 贴图资源 ID
     * @param titleKey 标题翻译键（可能为空）
     * @param authorKey 作者翻译键（可能为空）
     */
    PaintingVariant(i32 width,
        i32 height,
        ResourceLocation assetId,
        std::optional<std::string> titleKey,
        std::optional<std::string> authorKey);

    [[nodiscard]] i32 getWidth() const noexcept
    {
        return m_width;
    }

    [[nodiscard]] i32 getHeight() const noexcept
    {
        return m_height;
    }

    [[nodiscard]] const ResourceLocation& getAssetId() const noexcept
    {
        return m_assetId;
    }

    [[nodiscard]] const std::optional<std::string>& getTitleKey() const noexcept
    {
        return m_titleKey;
    }

    [[nodiscard]] const std::optional<std::string>& getAuthorKey() const noexcept
    {
        return m_authorKey;
    }

    /** @brief 展示区域面积（宽 × 高），画作摆放与选择时的常用量。 */
    [[nodiscard]] i32 area() const noexcept
    {
        return m_width * m_height;
    }

private:
    i32 m_width;
    i32 m_height;
    ResourceLocation m_assetId;
    std::optional<std::string> m_titleKey;
    std::optional<std::string> m_authorKey;
};

} // namespace mc
