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

#include <concepts>
#include <type_traits>

namespace mc::network::codec {

/**
 * @brief 协议无关 codec 接口（"LLVM IR 指令集"的原语层）
 *
 * 对应 Java 1.21.11 的 StreamCodec<B,V>：一个值 V 在缓冲 B 上的编解码契约。
 * encode 把 V 写入 B；decode 从 B 读出 V。协议无关——B 通常是 buffer::RegistryByteBuf，
 * 但接口不绑定具体后端，基岩后端可用自己的缓冲类型。
 *
 * 用法：组合 StreamCodecs 里的原语（composite/optional/collection/...）拼出每个 IR 包的 codec，
 * 绝不在派生处手写 case 语句（包 ID 分配交给 IdDispatchCodec）。
 *
 * @tparam B 缓冲类型（如 buffer::RegistryByteBuf）
 * @tparam V 值类型（如 ir::play::MovePlayerPos）
 */
template <typename B, typename V>
class StreamCodec {
public:
    virtual ~StreamCodec() = default;

    /**
     * @brief 把 value 编码进 buf
     */
    virtual void encode(B& buf, const V& value) const = 0;

    /**
     * @brief 从 buf 解码出一个 V
     *
     * 返回 Result：解码失败（越界/非法数据）返回错误，不抛异常。
     */
    [[nodiscard]] virtual Result<V> decode(B& buf) const = 0;
};

/**
 * @brief Codec 概念：约束类型满足 StreamCodec<B,V> 接口
 */
template <typename T, typename B, typename V>
concept CodecFor = requires(const T& codec, B& buf, const V& value) {
    { codec.encode(buf, value) } -> std::same_as<void>;
    { codec.decode(buf) } -> std::convertible_to<Result<V>>;
};

/**
 * @brief 适配自由函数为 StreamCodec（lambda 版 codec 的载体）
 *
 * codec 层大量用 lambda 表达 encode/decode 逻辑；本模板把一对 lambda 包成
 * 满足 StreamCodec 接口的对象，供组合器按值持有。
 */
template <typename B, typename V, typename EncodeFn, typename DecodeFn>
    requires std::invocable<const EncodeFn&, B&, const V&> && std::invocable<const DecodeFn&, B&>
class LambdaCodec final : public StreamCodec<B, V> {
public:
    LambdaCodec(EncodeFn encodeFn, DecodeFn decodeFn)
        : m_encode(std::move(encodeFn))
        , m_decode(std::move(decodeFn))
    {}

    void encode(B& buf, const V& value) const override { m_encode(buf, value); }

    [[nodiscard]] Result<V> decode(B& buf) const override { return m_decode(buf); }

private:
    EncodeFn m_encode;
    DecodeFn m_decode;
};

/**
 * @brief 工厂：从一对 lambda 构造 LambdaCodec（类型推导友好）
 */
template <typename B, typename V, typename EncodeFn, typename DecodeFn>
[[nodiscard]] auto makeLambdaCodec(EncodeFn encodeFn, DecodeFn decodeFn)
{
    return LambdaCodec<B, V, EncodeFn, DecodeFn>(std::move(encodeFn), std::move(decodeFn));
}

} // namespace mc::network::codec
