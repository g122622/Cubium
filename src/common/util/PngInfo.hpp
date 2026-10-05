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

#include <array>
#include <span>

namespace mc::util {

/**
 * @brief PNG 文件头（IHDR）解析结果
 *
 * 仅解析 PNG 签名 + IHDR 块的前两个字段（宽、高），不解码像素数据。用于服务端图标
 * 尺寸校验（必须 64×64），对齐 MC Java `net.minecraft.util.PngInfo`。
 */
struct PngInfo {
    i32 width = 0;
    i32 height = 0;
};

/**
 * @brief 从 PNG 字节流解析宽高
 *
 * PNG 结构（大端序）：8 字节签名 `89 50 4E 47 0D 0A 1A 0A`，随后 IHDR 块：
 * 4 字节长度（须为 13）、4 字节类型 `IHDR`、4 字节宽、4 字节高。
 *
 * @param data 完整 PNG 字节流（至少 24 字节）
 * @return 解析成功返回 PngInfo；签名/块类型/长度非法或数据过短时返回错误
 */
[[nodiscard]] inline Result<PngInfo> parsePngInfo(std::span<const u8> data)
{
    // 签名(8) + IHDR 长度(4) + IHDR 类型(4) + 宽(4) + 高(4) = 24 字节。
    constexpr usize MIN_SIZE = 24;
    if (data.size() < MIN_SIZE) {
        return Error(ErrorCode::InvalidData, "PNG data too short for header", "parsePngInfo");
    }

    static constexpr std::array<u8, 8> PNG_SIGNATURE = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    for (usize i = 0; i < PNG_SIGNATURE.size(); ++i) {
        if (data[i] != PNG_SIGNATURE[i]) {
            return Error(ErrorCode::InvalidData, "Bad PNG signature", "parsePngInfo");
        }
    }

    // 大端序读取 32 位整数。
    const auto readU32Be = [&data](usize offset) -> u32 {
        return (static_cast<u32>(data[offset]) << 24) | (static_cast<u32>(data[offset + 1]) << 16) |
            (static_cast<u32>(data[offset + 2]) << 8) | static_cast<u32>(data[offset + 3]);
    };

    const u32 ihdrLength = readU32Be(8);
    if (ihdrLength != 13) {
        return Error(ErrorCode::InvalidData, "Bad length for IHDR chunk", "parsePngInfo");
    }
    const u32 ihdrType = readU32Be(12);
    // "IHDR" 的 ASCII 大端序字节拼成的 32 位值 = 0x49484452。
    constexpr u32 IHDR_TYPE = 0x49484452;
    if (ihdrType != IHDR_TYPE) {
        return Error(ErrorCode::InvalidData, "Bad type for IHDR chunk", "parsePngInfo");
    }

    PngInfo info;
    info.width = static_cast<i32>(readU32Be(16));
    info.height = static_cast<i32>(readU32Be(20));
    return info;
}

} // namespace mc::util
