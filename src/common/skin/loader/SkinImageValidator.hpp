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
#include "common/core/Types.hpp"

#include <string>
#include <vector>

namespace mc::skin {

/**
 * @brief 皮肤 PNG 的校验与规范化
 *
 * 皮肤有两个来源（本地文件 `FileSkinLoader`、远程下载 `HttpSkinLoader`），但**校验规则必须
 * 完全一致**，否则同一张皮肤经由不同来源会得到不同的字节，缓存哈希也会因此分裂。这里把
 * 规则收在一处，两个 loader 都调用它，避免规则在两份实现里各自漂移。
 *
 * 规则（与原版一致）：
 * - 必须能被解码为 PNG；
 * - 宽度必须为 64；
 * - 高度必须为 64（现代皮肤）或 32（旧版皮肤）；
 * - 旧版 64x32 皮肤补全为 64x64（下半部分保持透明）；
 * - 输出统一重新编码为 64x64 RGBA PNG。
 */
class SkinImageValidator {
public:
    /// 皮肤的标准宽度（原版固定 64）。
    static constexpr i32 SKIN_WIDTH = 64;

    /// 现代皮肤的标准高度。
    static constexpr i32 SKIN_HEIGHT = 64;

    /// 旧版皮肤的高度（仅含头部与身体上半部分）。
    static constexpr i32 LEGACY_SKIN_HEIGHT = 32;

    /**
     * @brief 校验皮肤 PNG 并规范化为 64x64 PNG
     *
     * @param pngData 原始 PNG 字节
     * @return 规范化后的 64x64 PNG 字节；失败时返回错误（编码无法解码、尺寸非法、重新编码失败）
     */
    [[nodiscard]] static Result<std::vector<u8>> validateAndNormalize(const std::vector<u8>& pngData);
};

} // namespace mc::skin
