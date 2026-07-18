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
 */

#include "client/resource/atlas/MissingNo.hpp"

namespace mc::client::resource::atlas {

const ResourceLocation& MissingNo::spriteLocation()
{
    static const ResourceLocation loc("minecraft", "missingno");
    return loc;
}

std::vector<u8> MissingNo::generatePixels()
{
    constexpr u32 SIZE = 16;
    constexpr u32 TILE = 8; // 每个棋盘格 8×8 像素
    constexpr u8 PURPLE_R = 128, PURPLE_G = 0, PURPLE_B = 128;
    constexpr u8 BLACK_R = 0, BLACK_G = 0, BLACK_B = 0;

    std::vector<u8> pixels(SIZE * SIZE * 4);
    for (u32 y = 0; y < SIZE; ++y) {
        for (u32 x = 0; x < SIZE; ++x) {
            const u32 idx = (y * SIZE + x) * 4;
            const bool isPurple = ((x / TILE) + (y / TILE)) % 2 == 0;
            pixels[idx + 0] = isPurple ? PURPLE_R : BLACK_R;
            pixels[idx + 1] = isPurple ? PURPLE_G : BLACK_G;
            pixels[idx + 2] = isPurple ? PURPLE_B : BLACK_B;
            pixels[idx + 3] = 255; // 完全不透明
        }
    }
    return pixels;
}

} // namespace mc::client::resource::atlas
