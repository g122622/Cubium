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

// Fuzz 目标：Java VarInt21 长度前缀帧层（pipeline/VarintFraming）。
//
// 输入语义：整段 fuzz 字节被视作一条 TCP 流，不含任何控制字节。
// 覆盖要点：
//   - VarInt 长度前缀被截断（1..4 字节到达即暂停）；
//   - 声明的帧长大于已到达字节数（半包）；
//   - 单次到达含多个完整帧 + 残留（粘包）；
//   - 非法 VarInt（> 5 字节，续位未清）；
//   - 声明超大帧长（该路径在生产侧无上限校验，见 docs/test/FUZZING.md 的已知缺陷记录）。

#include "common/network/pipeline/VarintFraming.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace mc;
using namespace mc::network;

namespace {

/// 单次喂入的最大分块数上限：把逐字节模式限制在短输入上，避免 O(n²) 拖慢 fuzz。
constexpr usize kByteWiseSizeLimit = 512;

/// 累计触发"帧结构非法"路径的次数。仅用于让该分支产生可观测副作用，
/// 便于调试时确认畸形长度前缀确实走到了错误路径。
usize _framingErrors = 0;

/**
 * @brief 把一段字节按指定粒度分块喂入切帧器，并取出全部完整帧
 *
 * 每喂一块就持续切帧直到不足一帧，切出的 payload 再做一次 encodeFrame 往返
 * （覆盖出站方向）。scratch 跨块保留，正是 Connection::_handleWireBytes 里
 * m_plainIn 的语义——残留字节必须留到下次。
 *
 * @param data      待喂入字节
 * @param size      字节数
 * @param chunkSize 每次喂入的字节数（>= 1）
 * @param scratch   输出参数：跨块保留的未切帧字节，调用前须为空
 */
void _feedInChunks(const u8* data, usize size, usize chunkSize, std::vector<u8>& scratch)
{
    std::vector<u8> frame;
    usize offset = 0;
    while (offset < size) {
        const usize remaining = size - offset;
        const usize take = (remaining < chunkSize) ? remaining : chunkSize;
        scratch.insert(scratch.end(), data + offset, data + offset + take);
        offset += take;

        // 一次喂入可能含 0..N 个完整帧；持续切直到不足一帧。
        while (true) {
            auto frameResult = pipeline::VarintFraming::tryDecodeFrame(scratch, frame);
            if (!frameResult.success()) {
                // 帧结构非法（长度前缀超 5 字节，或声明帧长超上限）。生产侧
                // Connection::_handleWireBytes 会据此清缓冲并断开连接；harness 只记录
                // 该路径已覆盖并结束本轮。
                ++_framingErrors;
                return;
            }
            if (!frameResult.value()) {
                break; // 数据不足，等下一块
            }
            // 出站方向往返：对切出的 payload 重新加长度前缀。
            // 结果不参与断言，仅用于覆盖 encodeFrame 的写入路径。
            std::vector<u8> reframed;
            pipeline::VarintFraming::encodeFrame(frame.data(), frame.size(), reframed);
        }
    }
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    if (size == 0) {
        return 0;
    }
    const auto* bytes = reinterpret_cast<const u8*>(data);

    // 三种分块粒度各跑一遍，分别对应三种真实到达模式：
    //   chunk = 1    —— 逐字节到达（长度前缀被切断、半包续传）；仅短输入启用
    //   chunk = 7    —— 任意不齐整到达（模拟 TCP 任意切分）
    //   chunk = size —— 一次到达（典型粘包）
    if (size <= kByteWiseSizeLimit) {
        std::vector<u8> scratch;
        _feedInChunks(bytes, size, 1, scratch);
    }

    {
        std::vector<u8> scratch;
        _feedInChunks(bytes, size, 7, scratch);
    }

    {
        std::vector<u8> scratch;
        _feedInChunks(bytes, size, size, scratch);
    }

    return 0;
}
