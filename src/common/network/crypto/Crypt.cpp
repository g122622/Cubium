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

#include "common/network/crypto/Crypt.hpp"

#include <openssl/rand.h>

namespace mc::network::crypto {

Result<std::array<u8, kSharedSecretBytes>> generateSharedSecret()
{
    std::array<u8, kSharedSecretBytes> secret{};
    if (RAND_bytes(secret.data(), static_cast<int>(secret.size())) != 1) {
        return Error(ErrorCode::Unknown, "RAND_bytes 生成共享密钥失败", "crypto::generateSharedSecret");
    }
    return secret;
}

Result<std::vector<u8>> generateRandomBytes(usize n)
{
    std::vector<u8> bytes(n);
    if (n > 0 && RAND_bytes(bytes.data(), static_cast<int>(n)) != 1) {
        return Error(ErrorCode::Unknown, "RAND_bytes 生成随机字节失败", "crypto::generateRandomBytes");
    }
    return bytes;
}

} // namespace mc::network::crypto
