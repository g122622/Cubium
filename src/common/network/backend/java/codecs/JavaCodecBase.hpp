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

#include "common/network/buffer/RegistryByteBuf.hpp"
#include "common/network/codec/StreamCodec.hpp"
#include "common/network/codec/StreamCodecs.hpp"
#include "common/network/ir/IrPacket.hpp"

#include <utility>

namespace mc::network::backend::java::codecs {

/// Java 后端 codec 使用的缓冲类型别名（RegistryFriendlyByteBuf 的 Java 对应）。
using B = buffer::RegistryByteBuf;

/**
 * @brief 用 lambda 构造 StreamCodec<B, V>（按值持有，可被 ProtocolInfoBuilder addPacket 接收）
 *
 * 各阶段 codec 头文件经此工具组装字段级 encode/decode lambda。抽出本文件，使
 * JavaConfigurationCodecs.hpp / JavaPlayCodecs.hpp 可独立包含，无需依赖 JavaCodecs.hpp
 * 的定义顺序。
 */
template <typename V, typename EncodeFn, typename DecodeFn>
[[nodiscard]] auto makeCodec(EncodeFn encodeFn, DecodeFn decodeFn)
{
    return codec::makeLambdaCodec<B, V>(std::move(encodeFn), std::move(decodeFn));
}

} // namespace mc::network::backend::java::codecs
