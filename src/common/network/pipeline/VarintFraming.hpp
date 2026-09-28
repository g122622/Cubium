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

#include <vector>

namespace mc::network::pipeline {

/**
 * @brief Java VarInt21 长度前缀帧编解码
 *
 * 对应 MC Java Varint21FrameDecoder/Prepender：每条消息 = VarInt(payload 长度) + payload。
 * 属于 Java wire 格式，由 Connection 流水线持有（不在通用 ITransport 里——
 * LocalTransport 零拷贝、RakNetTransport 自有分帧都不需要它）。
 *
 * 与压缩/加密层的相对位置（对齐 Java 出站 compress→frame→encrypt）：
 * - 出站：先压缩得 (VarInt(数据长度)+data)，再 frame 得 (VarInt(帧长)+前述)，
 *   最后 encrypt 整个帧。
 * - 入站：先 decrypt 整个帧，再 deframe 取出 (VarInt(数据长度)+data)，最后 decompress。
 */
class VarintFraming {
public:
    /**
     * @brief 单帧 payload（不含长度前缀）的最大合法长度
     *
     * 帧 payload 即压缩层输出：启用压缩时为 VarInt(数据长度)+zlib 流，其上限由
     * `crypto::ZlibCodec::kMaxCompressed`（2MB）约束；未启用压缩（threshold<0）时
     * 为原始 packetID+payload，长度取决于发送方编码（本项目最大为 Configuration
     * 阶段的全量注册表 NBT，实测远小于 8MB）。故此处取 8MB 作为宽松上界——目的是
     * 拦住"声明 4GB 帧长让对端无上限累积入站缓冲"的畸形报文，不追求贴合协议上限。
     *
     * TODO: 本值与 core/Constants.hpp 的 MAX_PACKET_SIZE / MAX_UNCOMPRESSED_SIZE
     *       （两者均为 2MB）不一致，而那两个常量目前全项目未被任何网络路径使用，
     *       且与 crypto/ZlibCodec 内部的 8MB/2MB 限制也对不上。待协议尺寸常量统一后，
     *       本值应改为引用统一常量，而不是各自定义。
     */
    static constexpr u32 kMaxFramePayloadSize = 8u * 1024u * 1024u;

    /**
     * @brief 给 payload 加 VarInt 长度前缀，输出完整帧
     */
    static void encodeFrame(const u8* payload, usize size, std::vector<u8>& output);

    /**
     * @brief 从流缓冲尝试切出完整帧
     *
     * @param buffer 输入输出：含已读入但未切帧的残留字节；成功切帧时移除已消费部分
     * @param frameOut 输出：切出的完整 payload（不含长度前缀）；无完整帧时不修改
     * @return
     *   - `ok(true)`：切出一帧；
     *   - `ok(false)`：数据不足，需继续累积；
     *   - 错误：帧结构非法——长度前缀超过 5 字节（续位未清），或声明的帧长超过
     *     `kMaxFramePayloadSize`。**调用方必须据此终止连接**：这两种情况下流已无法
     *     重新同步，继续累积只会让入站缓冲无上限增长（畸形长度前缀永远等不到
     *     "足够"的数据），构成内存耗尽。
     */
    [[nodiscard]] static Result<bool> tryDecodeFrame(std::vector<u8>& buffer, std::vector<u8>& frameOut);
};

} // namespace mc::network::pipeline
