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

// Fuzz 目标：加密层（pipeline/CipherHandlers → crypto/AesCfb8）。
//
// 输入语义：前 kSharedSecretBytes 字节为共享密钥，其余为待处理字节。
// 覆盖要点：
//   - AES-CFB8 是**流式**密码，跨包状态必须连续：同一对 handler 连续处理多段，
//     覆盖"密文按到达顺序逐字节喂入"这一约束（pipeline/README.md 第 4 条坑：
//     加密字节与解密后明文残留不能混存）；
//   - 分段粒度几何递增，覆盖"单次全量"与"多次小块"两种到达模式；
//   - 加解密双向（encoder/decoder 各持有独立 cipher 状态）。

#include "common/network/crypto/Crypt.hpp"
#include "common/network/pipeline/CipherHandlers.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace mc;
using namespace mc::network;

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    // 至少要有密钥 + 1 字节负载才有意义。
    if (size < crypto::kSharedSecretBytes + 1) {
        return 0;
    }

    std::array<u8, crypto::kSharedSecretBytes> secret{};
    for (usize i = 0; i < crypto::kSharedSecretBytes; ++i) {
        secret[i] = data[i];
    }

    const u8* payload = data + crypto::kSharedSecretBytes;
    const usize payloadSize = size - crypto::kSharedSecretBytes;

    pipeline::CipherEncoder encoder;
    pipeline::CipherDecoder decoder;
    if (!encoder.init(secret).success() || !decoder.init(secret).success()) {
        return 0;
    }

    // 分段粒度从 1 开始几何递增：既覆盖"逐字节喂入"，也覆盖"一次全量"。
    usize offset = 0;
    usize chunk = 1;
    while (offset < payloadSize) {
        const usize remaining = payloadSize - offset;
        const usize take = (remaining < chunk) ? remaining : chunk;

        std::vector<u8> segment(payload + offset, payload + offset + take);
        offset += take;

        // 解密方向：密文 → 明文（同一 decoder 的流式状态跨段保持）。
        std::vector<u8> plaintext;
        (void)decoder.decode(segment, plaintext);

        // 加密方向：明文 → 密文（独立 encoder 的流式状态跨段保持）。
        std::vector<u8> ciphertext;
        (void)encoder.encode(segment, ciphertext);

        if (chunk < 65536) {
            chunk *= 2;
        }
    }

    return 0;
}
