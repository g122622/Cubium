/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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

/**
 * @file Crc32c.hpp
 * @brief CRC-32C（Castagnoli）哈希算法实现
 *
 * 用于 HashedStack 组件哈希：把单个数据组件的 wire 编码字节喂入 CRC-32C，
 * 取 32 位结果作为 HashedPatchMap 的组件哈希值。
 *
 * 注意：vanilla 1.21.11 的 HashGenerator 走 RegistryOps<HashCode> + HashOps.CRC32C_INSTANCE
 * （DataFixers 体系，按 DynamicOps 把组件值结构化喂入 CRC-32C 上下文）。本实现是轻量直接
 * CRC-32C（输入=组件 wire 字节），与 vanilla 的 HashCode 不保证字节相等——仅用于我方 IR
 * 自洽承载结构化哈希字段，真 Java 服务端 RemoteSlot 视图匹配与否不阻塞互通（空/不等哈希
 * 最坏导致 Java 端多发全量 ContainerSetContent，不崩不丢物）。
 */
#pragma once

#include "common/core/Types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace mc {
namespace util {
namespace crypto {

/**
 * @brief CRC-32C（Castagnoli，多项式 0x1EDC6F41）哈希计算器
 *
 * 表驱动实现，反射输入/输出（与 zlib/inflate 一致的 little-endian CRC 约定）。
 * 初始值 0xFFFFFFFF，输出异或 0xFFFFFFFF，与软件 CRC-32C 通用约定一致。
 */
class Crc32c {
public:
    /**
     * @brief 计算字节数据的 CRC-32C
     *
     * @param data 输入字节视图
     * @return 32 位 CRC-32C 值
     */
    [[nodiscard]] static u32 hash(std::span<const u8> data);

    /**
     * @brief 计算字符串的 CRC-32C（按字节，不含结尾 NUL）
     *
     * @param str 输入字符串
     * @return 32 位 CRC-32C 值
     */
    [[nodiscard]] static u32 hash(std::string_view str);

private:
    /// CRC-32C 反射查表（256 项，按 Castagnoli 多项式 0x1EDC6F41 预生成）
    static const std::array<u32, 256> TABLE;
};

} // namespace crypto
} // namespace util
} // namespace mc
