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
 * The above copyright notice shall be included in all
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
#include <string_view>

namespace mc::resource {

/**
 * @brief 资源包类型枚举
 *
 * 区分客户端资源包和服务端数据包：
 * - ClientResources 映射到 assets/ 目录（纹理、模型、音效、语言等）
 * - ServerData 映射到 data/ 目录（战利品表、配方、标签、函数等）
 *
 * 同一个物理资源包可以同时包含 assets/ 和 data/ 目录，
 * 通过 PackType 决定读取哪个子目录。
 */
enum class PackType : u8 {
    ClientResources, ///< 客户端资源包，映射到 assets/ 目录
    ServerData       ///< 服务端数据包，映射到 data/ 目录
};

/**
 * @brief 获取 PackType 对应的目录名
 * @param type 资源包类型
 * @return "assets" 或 "data"
 */
[[nodiscard]] constexpr std::string_view packTypeDirectoryName(PackType type) noexcept
{
    switch (type) {
        case PackType::ClientResources:
            return "assets";
        case PackType::ServerData:
            return "data";
    }
    return "assets"; // 不可达，但编译器可能需要
}

} // namespace mc::resource

namespace mc {
using resource::PackType;
} // namespace mc
