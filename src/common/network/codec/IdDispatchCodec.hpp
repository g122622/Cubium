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
#include "common/network/codec/StreamCodec.hpp"

#include <functional>
#include <unordered_map>
#include <vector>

namespace mc::network::codec {

/**
 * @brief 按注册顺序分配整数 ID 的分发 codec
 *
 * 对应 Java 1.21.11 的 IdDispatchCodec：注册顺序即 packet ID。addPacket 顺序加入，
 * build() 前不分配；encode 时按类型查 ID 写 VarInt(id)+payload，decode 时读 VarInt(id)
 * 查 codec 解码成 variant。绝不硬编码 case 语句。
 *
 * @tparam B 缓冲类型
 * @tparam Variant 包变体类型（如 ir::PlayPacket = std::variant<...>）
 */
template <typename B, typename Variant>
class IdDispatchCodec {
public:
    /**
     * @brief 单个包的编码/解码项
     *
     * matches：判断 variant 当前是否持有本备选项（基于 variant::index() 或类型检查）。
     * encodePayload：variant 确定持有本类型后，写 payload（不含 id）。
     * decode：从 buf（已读过 id）读出 payload 构造 variant 备选项。
     *
     * 设计为"先匹配再写"：encode 先用 matches 定位，再写 id+payload，
     * 避免"写 id 后发现不匹配需回滚"的脆弱性（VarInt id 字节数可变）。
     */
    struct Entry {
        std::function<bool(const Variant& value)> matches;
        std::function<void(B& buf, const Variant& value)> encodePayload;
        std::function<Result<Variant>(B& buf)> decode;
    };

    IdDispatchCodec() = default;
    IdDispatchCodec(const IdDispatchCodec&) = delete;
    IdDispatchCodec& operator=(const IdDispatchCodec&) = delete;
    IdDispatchCodec(IdDispatchCodec&&) noexcept = default;
    IdDispatchCodec& operator=(IdDispatchCodec&&) noexcept = default;

    /**
     * @brief 登记一个包（顺序即 id，0 起递增）
     */
    void addPacket(std::function<bool(const Variant& value)> matches,
        std::function<void(B& buf, const Variant& value)> encodePayload,
        std::function<Result<Variant>(B& buf)> decode)
    {
        m_entries.push_back(Entry{std::move(matches), std::move(encodePayload), std::move(decode)});
    }

    /**
     * @brief 当前已登记包数（= build 后的 id 上界）
     */
    [[nodiscard]] usize size() const noexcept { return m_entries.size(); }

    /**
     * @brief 编码：先 matches 定位 variant 当前备选项，再写 VarInt(id) + payload
     *
     * @return 失败返回错误（无匹配类型 = ProtocolError）
     */
    [[nodiscard]] Result<void> encode(B& buf, const Variant& value) const
    {
        for (usize id = 0; id < m_entries.size(); ++id) {
            if (m_entries[id].matches(value)) {
                buf.writeVarInt(static_cast<i32>(id));
                m_entries[id].encodePayload(buf, value);
                return Result<void>::ok();
            }
        }
        return Error(ErrorCode::ProtocolError, "IdDispatchCodec 无匹配包类型", "IdDispatchCodec::encode");
    }

    /**
     * @brief 解码：读 VarInt(id)，查 entries 解码 payload 成 variant
     */
    [[nodiscard]] Result<Variant> decode(B& buf) const
    {
        i32 id = 0;
        MC_TRY_ASSIGN(id, buf.readVarInt());
        if (id < 0 || static_cast<usize>(id) >= m_entries.size()) {
            return Error(ErrorCode::ProtocolError, "未知 packet id", "IdDispatchCodec::decode");
        }
        return m_entries[static_cast<usize>(id)].decode(buf);
    }

    [[nodiscard]] const std::vector<Entry>& entries() const noexcept { return m_entries; }

private:
    std::vector<Entry> m_entries;
};

} // namespace mc::network::codec
