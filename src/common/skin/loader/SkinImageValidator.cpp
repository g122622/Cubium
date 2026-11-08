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

#include "SkinImageValidator.hpp"

#include <cstring>
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <stb_image_write.h>

namespace mc::skin {

Result<std::vector<u8>> SkinImageValidator::validateAndNormalize(const std::vector<u8>& pngData)
{
    if (pngData.empty()) {
        return Error(ErrorCode::InvalidData, "Skin data is empty");
    }

    i32 width = 0;
    i32 height = 0;
    i32 channels = 0;

    // 强制解码为 RGBA：后续按 4 通道处理，避免 3 通道输入造成的步长错位。
    u8* pixels = stbi_load_from_memory(pngData.data(), static_cast<i32>(pngData.size()), &width, &height, &channels, 4);
    if (pixels == nullptr) {
        return Error(ErrorCode::InvalidData, "Failed to decode skin image");
    }

    if (width != SKIN_WIDTH) {
        stbi_image_free(pixels);
        return Error(ErrorCode::InvalidData, "Invalid skin width: expected 64, got " + std::to_string(width));
    }

    if (height != SKIN_HEIGHT && height != LEGACY_SKIN_HEIGHT) {
        stbi_image_free(pixels);
        return Error(ErrorCode::InvalidData, "Invalid skin height: expected 32 or 64, got " + std::to_string(height));
    }

    // 统一到 64x64 RGBA。旧版 64x32 皮肤的未覆盖区域保持透明（初始化为 0）。
    std::vector<u8> rgba(static_cast<size_t>(SKIN_WIDTH) * SKIN_HEIGHT * 4, 0);
    const size_t rowBytes = static_cast<size_t>(SKIN_WIDTH) * 4;
    const size_t copyRows = static_cast<size_t>(height);
    for (size_t y = 0; y < copyRows; ++y) {
        std::memcpy(rgba.data() + y * rowBytes, pixels + y * rowBytes, rowBytes);
    }
    stbi_image_free(pixels);

    if (height == LEGACY_SKIN_HEIGHT) {
        spdlog::info("SkinImageValidator: converted legacy 64x32 skin to 64x64");
    }

    // 用内存写入模式重新编码，避免临时文件。
    struct PngWriteContext {
        std::vector<u8> buffer;
    };

    PngWriteContext ctx;
    stbi_write_png_to_func(
        [](void* userdata, void* data, i32 size) {
            auto* context = static_cast<PngWriteContext*>(userdata);
            auto* bytes = static_cast<u8*>(data);
            context->buffer.insert(context->buffer.end(), bytes, bytes + size);
        },
        &ctx,
        SKIN_WIDTH,
        SKIN_HEIGHT,
        4, // components (RGBA)
        rgba.data(),
        SKIN_WIDTH * 4 // stride
    );

    if (ctx.buffer.empty()) {
        return Error(ErrorCode::InvalidData, "Failed to encode skin data to PNG");
    }

    return ctx.buffer;
}

} // namespace mc::skin
