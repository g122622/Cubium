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

#include "common/network/pipeline/CipherHandlers.hpp"

namespace mc::network::pipeline {

Result<void> CipherEncoder::init(const std::array<u8, crypto::kSharedSecretBytes>& key)
{
    auto r = crypto::AesCfb8::forEncryption(key);
    if (!r.success()) {
        return r.error();
    }
    m_cipher = std::move(r).value();
    return Result<void>::ok();
}

Result<void> CipherEncoder::encode(const std::vector<u8>& input, std::vector<u8>& output)
{
    if (!m_cipher.isValid()) {
        // 未激活：明文直通（离线模式）。
        output.insert(output.end(), input.begin(), input.end());
        return Result<void>::ok();
    }
    auto r = m_cipher.process(input.data(), input.size());
    if (!r.success()) {
        return r.error();
    }
    output = std::move(r).value();
    return Result<void>::ok();
}

Result<void> CipherDecoder::init(const std::array<u8, crypto::kSharedSecretBytes>& key)
{
    auto r = crypto::AesCfb8::forDecryption(key);
    if (!r.success()) {
        return r.error();
    }
    m_cipher = std::move(r).value();
    return Result<void>::ok();
}

Result<void> CipherDecoder::decode(const std::vector<u8>& input, std::vector<u8>& output)
{
    if (!m_cipher.isValid()) {
        // 未激活：明文直通（离线模式）。
        output.insert(output.end(), input.begin(), input.end());
        return Result<void>::ok();
    }
    auto r = m_cipher.process(input.data(), input.size());
    if (!r.success()) {
        return r.error();
    }
    output = std::move(r).value();
    return Result<void>::ok();
}

} // namespace mc::network::pipeline
