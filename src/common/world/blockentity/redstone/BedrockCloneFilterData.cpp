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

#include "common/world/blockentity/redstone/BedrockCloneFilterData.hpp"

#include "common/core/Types.hpp"

#include <charconv>

namespace mc::blockentity {

std::string bedrockCloneFilterDataToJavaProps(const std::string& blockTok, const std::string& dataTok)
{
    if (dataTok.empty()) {
        return "";
    }
    // 仅处理 stairs 类方块
    if (blockTok.find("stairs") == std::string::npos) {
        return "";
    }

    i32 data = 0;
    // 用 from_chars 而非 stoi：stoi 靠抛异常报告非法输入，而本项目不允许用异常做控制流；
    // 且 stoi 会接受尾随垃圾（"3abc" → 3），这里要求整串都是数字，解析失败即退化为空串。
    const auto [parseEnd, parseError] = std::from_chars(dataTok.data(), dataTok.data() + dataTok.size(), data);
    if (parseError != std::errc{} || parseEnd != dataTok.data() + dataTok.size()) {
        return "";
    }
    if (data < 0 || data > 7) {
        return "";
    }

    const i32 weirdoDirection = data % 4;
    const bool upsideDown = (data / 4) != 0;
    const char* facing = nullptr;
    switch (weirdoDirection) {
        case 0:
            facing = "east";
            break;
        case 1:
            facing = "west";
            break;
        case 2:
            facing = "south";
            break;
        case 3:
            facing = "north";
            break;
        default:
            return "";
    }
    return std::string("[facing=") + facing + ",half=" + (upsideDown ? "top" : "bottom") + "]";
}

} // namespace mc::blockentity
