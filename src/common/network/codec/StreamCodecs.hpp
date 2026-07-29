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

#include "common/network/codec/StreamCodec.hpp"

#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace mc::network::codec {

/**
 * @brief 协议无关 codec 原语与组合器
 *
 * 每个组合器是值类型结构体，提供 encode(B&, const V&) / decode(B&) -> Result<V>，
 * 鸭子类型满足 CodecFor 概念，可层层组合。对应 Java 1.21.11 StreamCodecs 的
 * composite/optional/collection 等组合器。
 *
 * 设计要点：
 * - 无虚函数开销：组合器按值持有子 codec，编译期单态化（除非顶层经 StreamCodec& 分发）。
 * - Result 传播：子 decode 失败用 MC_TRY_ASSIGN 向上冒泡，不抛异常。
 */
namespace stream_codecs {

// ============================================================================
// 定长原语（直接转发到 ByteBuf 方法，B 须有对应读写方法）
// ============================================================================

template <typename B>
struct U8Codec {
    void encode(B& buf, u8 value) const { buf.writeU8(value); }
    [[nodiscard]] Result<u8> decode(B& buf) const { return buf.readU8(); }
};

template <typename B>
struct U16Codec {
    void encode(B& buf, u16 value) const { buf.writeU16(value); }
    [[nodiscard]] Result<u16> decode(B& buf) const { return buf.readU16(); }
};

template <typename B>
struct I16Codec {
    void encode(B& buf, i16 value) const { buf.writeI16(value); }
    [[nodiscard]] Result<i16> decode(B& buf) const { return buf.readI16(); }
};

template <typename B>
struct U32Codec {
    void encode(B& buf, u32 value) const { buf.writeU32(value); }
    [[nodiscard]] Result<u32> decode(B& buf) const { return buf.readU32(); }
};

template <typename B>
struct I32Codec {
    void encode(B& buf, i32 value) const { buf.writeI32(value); }
    [[nodiscard]] Result<i32> decode(B& buf) const { return buf.readI32(); }
};

template <typename B>
struct I64Codec {
    void encode(B& buf, i64 value) const { buf.writeI64(value); }
    [[nodiscard]] Result<i64> decode(B& buf) const { return buf.readI64(); }
};

template <typename B>
struct F32Codec {
    void encode(B& buf, f32 value) const { buf.writeF32(value); }
    [[nodiscard]] Result<f32> decode(B& buf) const { return buf.readF32(); }
};

template <typename B>
struct F64Codec {
    void encode(B& buf, f64 value) const { buf.writeF64(value); }
    [[nodiscard]] Result<f64> decode(B& buf) const { return buf.readF64(); }
};

template <typename B>
struct BoolCodec {
    void encode(B& buf, bool value) const { buf.writeBool(value); }
    [[nodiscard]] Result<bool> decode(B& buf) const { return buf.readBool(); }
};

template <typename B>
struct VarIntCodec {
    void encode(B& buf, i32 value) const { buf.writeVarInt(value); }
    [[nodiscard]] Result<i32> decode(B& buf) const { return buf.readVarInt(); }
};

template <typename B>
struct VarLongCodec {
    void encode(B& buf, i64 value) const { buf.writeVarLong(value); }
    [[nodiscard]] Result<i64> decode(B& buf) const { return buf.readVarLong(); }
};

template <typename B>
struct StringCodec {
    void encode(B& buf, std::string_view value) const { buf.writeString(value); }
    [[nodiscard]] Result<std::string> decode(B& buf) const { return buf.readString(); }
};

// ============================================================================
// optional<T>：前置 bool 标志，true 时编码 T
// ============================================================================

template <typename B, typename Inner>
struct OptionalCodec {
    Inner inner;

    explicit OptionalCodec(Inner c)
        : inner(std::move(c))
    {}

    using T = std::decay_t<decltype(std::declval<Inner>().decode(std::declval<B&>()).value())>;

    void encode(B& buf, const std::optional<T>& value) const
    {
        buf.writeBool(value.has_value());
        if (value.has_value()) {
            inner.encode(buf, *value);
        }
    }

    [[nodiscard]] Result<std::optional<T>> decode(B& buf) const
    {
        bool present = false;
        MC_TRY_ASSIGN(present, buf.readBool());
        if (!present) {
            return std::optional<T>{};
        }
        T val;
        MC_TRY_ASSIGN(val, inner.decode(buf));
        return std::optional<T>{std::move(val)};
    }
};

template <typename B, typename Inner>
[[nodiscard]] auto optional(Inner inner)
{
    return OptionalCodec<B, Inner>(std::move(inner));
}

// ============================================================================
// collection<T>：VarInt 长度前缀 + 逐元素编码（对应 Java StreamCodecs.collection）
// ============================================================================

template <typename B, typename Inner>
struct CollectionCodec {
    Inner inner;

    explicit CollectionCodec(Inner c)
        : inner(std::move(c))
    {}

    using T = std::decay_t<decltype(std::declval<Inner>().decode(std::declval<B&>()).value())>;

    void encode(B& buf, const std::vector<T>& value) const
    {
        buf.writeVarInt(static_cast<i32>(value.size()));
        for (const auto& item : value) {
            inner.encode(buf, item);
        }
    }

    [[nodiscard]] Result<std::vector<T>> decode(B& buf) const
    {
        i32 count = 0;
        MC_TRY_ASSIGN(count, buf.readVarInt());
        if (count < 0) {
            return Error(ErrorCode::InvalidData, "Collection length is negative", "CollectionCodec::decode");
        }
        std::vector<T> out;
        out.reserve(static_cast<usize>(count));
        for (i32 i = 0; i < count; ++i) {
            T item;
            MC_TRY_ASSIGN(item, inner.decode(buf));
            out.push_back(std::move(item));
        }
        return out;
    }
};

template <typename B, typename Inner>
[[nodiscard]] auto collection(Inner inner)
{
    return CollectionCodec<B, Inner>(std::move(inner));
}

// ============================================================================
// composite：组合多个成员 codec 拼出一个 struct V
//
// 为避免过重的模板元编程，本骨架提供 memberCodec 辅助（逐字段拼装工具），
// 具体 IR 包 codec 在 backend/java/codecs/ 里用 lambda + ByteBuf 直接读写实现
// （见 StreamCodec.hpp 的 makeLambdaCodec）。
// ============================================================================

/**
 * @brief 成员 codec：绑定（V 的某成员指针）到（该成员类型的 codec）
 *
 * 提供按成员指针读写字段的工具，供 IR 包 codec 复用。encode/decode 操作的是 V 实例的成员。
 */
template <typename V, typename MemberPtr, typename Inner>
struct MemberCodec {
    MemberPtr memberPtr;
    Inner inner;

    MemberCodec(MemberPtr ptr, Inner c)
        : memberPtr(ptr)
        , inner(std::move(c))
    {}

    // MemberCodec 作为工具：调用方显式传 buf 与 V 实例。
    template <typename Buf>
    void encode(Buf& buf, const V& value) const
    {
        inner.encode(buf, value.*memberPtr);
    }

    template <typename Buf>
    [[nodiscard]] Result<void> decode(Buf& buf, V& value) const
    {
        using InnerT = std::decay_t<decltype(inner.decode(std::declval<Buf&>()).value())>;
        InnerT val;
        MC_TRY_ASSIGN(val, inner.decode(buf));
        value.*memberPtr = std::move(val);
        return Result<void>::ok();
    }
};

template <typename V, typename MemberPtr, typename Inner>
[[nodiscard]] auto memberCodec(MemberPtr ptr, Inner inner)
{
    return MemberCodec<V, MemberPtr, Inner>(ptr, std::move(inner));
}

// ============================================================================
// unit：无字段的包/标记（如 StatusRequest），编码空，解码返回默认值
// ============================================================================

template <typename V>
struct UnitCodec {
    template <typename B>
    void encode(B& /*buf*/, const V& /*value*/) const
    {}

    template <typename B>
    [[nodiscard]] Result<V> decode(B& /*buf*/) const
    {
        return V{};
    }
};

template <typename V>
[[nodiscard]] auto unit()
{
    return UnitCodec<V>{};
}

} // namespace stream_codecs

} // namespace mc::network::codec
