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

#include "Font.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <array>

namespace mc::client {

/**
 * @brief 默认ASCII位图字体生成器
 *
 * 生成基本的ASCII字符位图，用于调试和基础显示。
 * 字体数据内置在代码中，不依赖外部资源。
 *
 * 参考：
 * - MC的默认字体是8x8像素
 * - 包含ASCII可打印字符（32-126）
 */
class DefaultAsciiFont {
public:
    /**
     * @brief 创建默认ASCII字体
     * @param font 目标字体对象
     * @return 成功或错误
     */
    [[nodiscard]] static Result<void> create(Font& font);

    /**
     * @brief 获取字体高度
     */
    [[nodiscard]] static constexpr u32 fontHeight() { return 8; }

    /**
     * @brief 获取字体宽度
     */
    [[nodiscard]] static constexpr u32 fontWidth() { return 8; }

private:
    /**
     * @brief 生成单个字符的位图数据
     * @param c 字符
     * @param outPixels 输出像素数据（8x8）
     * @return 字符宽度
     *
     * TODO: 目前未被调用，待UI系统完善后启用
     */
    [[nodiscard]] static u32 _generateCharBitmap(char c, std::array<u8, 64>& outPixels);

    /**
     * @brief 获取5x7点阵字体数据
     *
     * TODO: 目前仅被_generateCharBitmap调用，待UI系统完善后启用
     */
    [[nodiscard]] static const u8* _getFontData(char c);
};

} // namespace mc::client
