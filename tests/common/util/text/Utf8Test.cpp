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

#include "util/text/Utf8.hpp"
#include <string>
#include <vector>
#include <gtest/gtest.h>

using namespace mc::util::text;
using mc::u32;

// ============================================================================
// utf8EncodeLength
// ============================================================================

TEST(Utf8EncodeLength, Ascii)
{
    EXPECT_EQ(utf8EncodeLength(0x00), 1);
    EXPECT_EQ(utf8EncodeLength(0x41), 1); // 'A'
    EXPECT_EQ(utf8EncodeLength(0x7F), 1);
}

TEST(Utf8EncodeLength, TwoByte)
{
    EXPECT_EQ(utf8EncodeLength(0x80), 2);
    EXPECT_EQ(utf8EncodeLength(0xC0), 2); // À
    EXPECT_EQ(utf8EncodeLength(0x7FF), 2);
}

TEST(Utf8EncodeLength, ThreeByte)
{
    EXPECT_EQ(utf8EncodeLength(0x800), 3);
    EXPECT_EQ(utf8EncodeLength(0x4F60), 3); // 你
    EXPECT_EQ(utf8EncodeLength(0xFFFF), 3);
}

TEST(Utf8EncodeLength, FourByte)
{
    EXPECT_EQ(utf8EncodeLength(0x10000), 4);
    EXPECT_EQ(utf8EncodeLength(0x1F600), 4); // 😀
    EXPECT_EQ(utf8EncodeLength(0x10FFFF), 4);
}

TEST(Utf8EncodeLength, InvalidCodePoint)
{
    EXPECT_EQ(utf8EncodeLength(0x110000), 0);
    EXPECT_EQ(utf8EncodeLength(0xFFFFFFFF), 0);
}

// ============================================================================
// utf8Encode
// ============================================================================

TEST(Utf8Encode, Ascii)
{
    EXPECT_EQ(utf8Encode(0x41), "A");
    EXPECT_EQ(utf8Encode(0x00), std::string(1, '\0'));
    EXPECT_EQ(utf8Encode(0x7F), std::string(1, '\x7F'));
}

TEST(Utf8Encode, TwoByte)
{
    // U+00C0 = À → C3 80
    EXPECT_EQ(utf8Encode(0xC0), "\xC3\x80");
    // U+00E9 = é → C3 A9
    EXPECT_EQ(utf8Encode(0xE9), "\xC3\xA9");
}

TEST(Utf8Encode, ThreeByte_CJK)
{
    // U+4F60 = 你 → E4 BD A0
    EXPECT_EQ(utf8Encode(0x4F60), "\xE4\xBD\xA0");
    // U+3053 = こ → E3 81 93
    EXPECT_EQ(utf8Encode(0x3053), "\xE3\x81\x93");
}

TEST(Utf8Encode, FourByte_Emoji)
{
    // U+1F600 = 😀 → F0 9F 98 80
    EXPECT_EQ(utf8Encode(0x1F600), "\xF0\x9F\x98\x80");
    // U+1F4A9 = 💩 → F0 9F 92 A9
    EXPECT_EQ(utf8Encode(0x1F4A9), "\xF0\x9F\x92\xA9");
}

TEST(Utf8Encode, InvalidCodePoint_ReturnsEmpty)
{
    EXPECT_EQ(utf8Encode(0x110000), "");
    EXPECT_EQ(utf8Encode(0xFFFFFFFF), "");
}

// ============================================================================
// utf8Append
// ============================================================================

TEST(Utf8Append, BasicUsage)
{
    std::string result;
    utf8Append(result, 0x48);    // 'H'
    utf8Append(result, 0x4F60);  // 你
    utf8Append(result, 0x1F600); // 😀
    EXPECT_EQ(result,
        "H"
        "\xE4\xBD\xA0"
        "\xF0\x9F\x98\x80");
}

// ============================================================================
// utf8IsContinuationByte
// ============================================================================

TEST(Utf8IsContinuationByte, Ascii)
{
    EXPECT_FALSE(utf8IsContinuationByte(0x00));
    EXPECT_FALSE(utf8IsContinuationByte(0x41));
    EXPECT_FALSE(utf8IsContinuationByte(0x7F));
}

TEST(Utf8IsContinuationByte, LeadBytes)
{
    EXPECT_FALSE(utf8IsContinuationByte(0xC0));
    EXPECT_FALSE(utf8IsContinuationByte(0xDF));
    EXPECT_FALSE(utf8IsContinuationByte(0xE0));
    EXPECT_FALSE(utf8IsContinuationByte(0xEF));
    EXPECT_FALSE(utf8IsContinuationByte(0xF0));
    EXPECT_FALSE(utf8IsContinuationByte(0xF7));
}

TEST(Utf8IsContinuationByte, ContinuationBytes)
{
    EXPECT_TRUE(utf8IsContinuationByte(0x80));
    EXPECT_TRUE(utf8IsContinuationByte(0x9F));
    EXPECT_TRUE(utf8IsContinuationByte(0xBF));
}

// ============================================================================
// utf8LeadByteLength
// ============================================================================

TEST(Utf8LeadByteLength, Ascii)
{
    EXPECT_EQ(utf8LeadByteLength(0x00), 1);
    EXPECT_EQ(utf8LeadByteLength(0x41), 1);
    EXPECT_EQ(utf8LeadByteLength(0x7F), 1);
}

TEST(Utf8LeadByteLength, MultiByte)
{
    EXPECT_EQ(utf8LeadByteLength(0xC0), 2);
    EXPECT_EQ(utf8LeadByteLength(0xDF), 2);
    EXPECT_EQ(utf8LeadByteLength(0xE0), 3);
    EXPECT_EQ(utf8LeadByteLength(0xEF), 3);
    EXPECT_EQ(utf8LeadByteLength(0xF0), 4);
    EXPECT_EQ(utf8LeadByteLength(0xF7), 4);
}

TEST(Utf8LeadByteLength, InvalidLeadByte)
{
    // 0xF8 ~ 0xFF 不是合法的 UTF-8 首字节
    EXPECT_EQ(utf8LeadByteLength(0xF8), 1);
    EXPECT_EQ(utf8LeadByteLength(0xFF), 1);
    // 续接字节作为首字节也是无效的，按单字节处理
    EXPECT_EQ(utf8LeadByteLength(0x80), 1);
    EXPECT_EQ(utf8LeadByteLength(0xBF), 1);
}

// ============================================================================
// utf8Decode
// ============================================================================

TEST(Utf8Decode, Ascii)
{
    std::string text = "Hello";
    size_t index = 0;
    EXPECT_EQ(utf8Decode(text, index), 0x48);
    EXPECT_EQ(index, 1u);
    EXPECT_EQ(utf8Decode(text, index), 0x65);
    EXPECT_EQ(index, 2u);
}

TEST(Utf8Decode, TwoByteSequence)
{
    std::string text = "\xC3\xA9"; // é
    size_t index = 0;
    EXPECT_EQ(utf8Decode(text, index), 0xE9u);
    EXPECT_EQ(index, 2u);
}

TEST(Utf8Decode, ThreeByteSequence_CJK)
{
    std::string text = "\xE4\xBD\xA0"; // 你
    size_t index = 0;
    EXPECT_EQ(utf8Decode(text, index), 0x4F60u);
    EXPECT_EQ(index, 3u);
}

TEST(Utf8Decode, FourByteSequence_Emoji)
{
    std::string text = "\xF0\x9F\x98\x80"; // 😀
    size_t index = 0;
    EXPECT_EQ(utf8Decode(text, index), 0x1F600u);
    EXPECT_EQ(index, 4u);
}

TEST(Utf8Decode, MixedContent)
{
    // "Aé你😀B"
    std::string text = "A"
                       "\xC3\xA9"
                       "\xE4\xBD\xA0"
                       "\xF0\x9F\x98\x80"
                       "B";
    size_t index = 0;
    EXPECT_EQ(utf8Decode(text, index), 0x41u);
    EXPECT_EQ(index, 1u);
    EXPECT_EQ(utf8Decode(text, index), 0xE9u);
    EXPECT_EQ(index, 3u);
    EXPECT_EQ(utf8Decode(text, index), 0x4F60u);
    EXPECT_EQ(index, 6u);
    EXPECT_EQ(utf8Decode(text, index), 0x1F600u);
    EXPECT_EQ(index, 10u);
    EXPECT_EQ(utf8Decode(text, index), 0x42u);
    EXPECT_EQ(index, 11u);
}

TEST(Utf8Decode, EmptyString)
{
    std::string text;
    size_t index = 0;
    EXPECT_EQ(utf8Decode(text, index), U'\0');
    EXPECT_EQ(index, 0u);
}

TEST(Utf8Decode, IndexAtEnd)
{
    std::string text = "A";
    size_t index = 1;
    EXPECT_EQ(utf8Decode(text, index), U'\0');
    EXPECT_EQ(index, 1u);
}

TEST(Utf8Decode, TruncatedTwoByteSequence)
{
    std::string text = "\xC3"; // 缺少续接字节
    size_t index = 0;
    EXPECT_EQ(utf8Decode(text, index), U'?');
    EXPECT_EQ(index, 1u);
}

TEST(Utf8Decode, TruncatedThreeByteSequence)
{
    std::string text = "\xE4\xBD"; // 缺少第三个字节
    size_t index = 0;
    EXPECT_EQ(utf8Decode(text, index), U'?');
    EXPECT_EQ(index, 1u);
}

TEST(Utf8Decode, TruncatedFourByteSequence)
{
    std::string text = "\xF0\x9F\x98"; // 缺少第四个字节
    size_t index = 0;
    EXPECT_EQ(utf8Decode(text, index), U'?');
    EXPECT_EQ(index, 1u);
}

TEST(Utf8Decode, OverlongEncoding)
{
    // U+0041 ('A') 的 overlong 两字节编码：C1 81（非法）
    std::string text = "\xC1\x81";
    size_t index = 0;
    EXPECT_EQ(utf8Decode(text, index), U'?');
}

TEST(Utf8Decode, SurrogateRange)
{
    // U+D800 是代理范围，应返回 '?'
    // U+D800 的 UTF-8 编码：ED A0 80
    std::string text = "\xED\xA0\x80";
    size_t index = 0;
    EXPECT_EQ(utf8Decode(text, index), U'?');
}

TEST(Utf8Decode, InvalidContinuationByte)
{
    // 首字节为 0xE0 但续接字节不是 10xxxxxx
    std::string text = "\xE0\x41\x41";
    size_t index = 0;
    EXPECT_EQ(utf8Decode(text, index), U'?');
    EXPECT_EQ(index, 1u);
}

// ============================================================================
// utf8CodepointByteLength
// ============================================================================

TEST(Utf8CodepointByteLength, Ascii)
{
    std::string text = "A";
    EXPECT_EQ(utf8CodepointByteLength(text, 0), 1u);
}

TEST(Utf8CodepointByteLength, ThreeByte)
{
    std::string text = "\xE4\xBD\xA0"; // 你
    EXPECT_EQ(utf8CodepointByteLength(text, 0), 3u);
}

TEST(Utf8CodepointByteLength, FourByte)
{
    std::string text = "\xF0\x9F\x98\x80"; // 😀
    EXPECT_EQ(utf8CodepointByteLength(text, 0), 4u);
}

TEST(Utf8CodepointByteLength, OutOfBounds)
{
    std::string text = "A";
    EXPECT_EQ(utf8CodepointByteLength(text, 1), 0u);
    EXPECT_EQ(utf8CodepointByteLength(text, 5), 0u);
}

TEST(Utf8CodepointByteLength, TruncatedSequence)
{
    // 只有两个字节但首字节指示三字节序列
    std::string text = "\xE4\xBD";
    EXPECT_EQ(utf8CodepointByteLength(text, 0), 2u);
}

// ============================================================================
// utf8NextCodepointIndex / utf8PrevCodepointIndex
// ============================================================================

TEST(Utf8NextCodepointIndex, Basic)
{
    // "A你B"
    std::string text = "A"
                       "\xE4\xBD\xA0"
                       "B";
    EXPECT_EQ(utf8NextCodepointIndex(text, 0), 1u);
    EXPECT_EQ(utf8NextCodepointIndex(text, 1), 4u);
    EXPECT_EQ(utf8NextCodepointIndex(text, 4), 5u);
    EXPECT_EQ(utf8NextCodepointIndex(text, 5), 5u);
}

TEST(Utf8NextCodepointIndex, OutOfBounds)
{
    std::string text = "Hi";
    EXPECT_EQ(utf8NextCodepointIndex(text, 10), 2u);
}

TEST(Utf8PrevCodepointIndex, Basic)
{
    // "A你B"
    std::string text = "A"
                       "\xE4\xBD\xA0"
                       "B";
    EXPECT_EQ(utf8PrevCodepointIndex(text, 5), 4u); // B -> 你
    EXPECT_EQ(utf8PrevCodepointIndex(text, 4), 1u); // 你 -> A
    EXPECT_EQ(utf8PrevCodepointIndex(text, 1), 0u); // A -> 开头
    EXPECT_EQ(utf8PrevCodepointIndex(text, 0), 0u); // 开头
}

TEST(Utf8PrevCodepointIndex, FromMiddleOfMultibyte)
{
    // "你" = E4 BD A0，从中间位置（2）回退应跳到序列开头（1）
    std::string text = "\xE4\xBD\xA0";
    EXPECT_EQ(utf8PrevCodepointIndex(text, 2), 0u);
    EXPECT_EQ(utf8PrevCodepointIndex(text, 1), 0u);
}

// ============================================================================
// utf8CodepointCount
// ============================================================================

TEST(Utf8CodepointCount, EmptyString)
{
    EXPECT_EQ(utf8CodepointCount(""), 0u);
}

TEST(Utf8CodepointCount, PureAscii)
{
    EXPECT_EQ(utf8CodepointCount("Hello"), 5u);
}

TEST(Utf8CodepointCount, MixedContent)
{
    // "A你😀B" = 1 + 1 + 1 + 1 = 4 codepoints
    std::string text = "A"
                       "\xE4\xBD\xA0"
                       "\xF0\x9F\x98\x80"
                       "B";
    EXPECT_EQ(utf8CodepointCount(text), 4u);
}

TEST(Utf8CodepointCount, PureCJK)
{
    // "你好" = 2 codepoints, 6 bytes
    std::string text = "\xE4\xBD\xA0"
                       "\xE5\xA5\xBD";
    EXPECT_EQ(utf8CodepointCount(text), 2u);
}

TEST(Utf8CodepointCount, PureEmoji)
{
    // "😀🎉" = 2 codepoints, 8 bytes
    std::string text = "\xF0\x9F\x98\x80"
                       "\xF0\x9F\x8E\x89";
    EXPECT_EQ(utf8CodepointCount(text), 2u);
}

// ============================================================================
// utf8CodepointToByteOffset / utf8ByteOffsetToCodepointIndex
// ============================================================================

TEST(Utf8CodepointToByteOffset, Basic)
{
    // "A你B" = bytes: [41] [E4 BD A0] [42]
    std::string text = "A"
                       "\xE4\xBD\xA0"
                       "B";
    EXPECT_EQ(utf8CodepointToByteOffset(text, 0), 0u);
    EXPECT_EQ(utf8CodepointToByteOffset(text, 1), 1u);
    EXPECT_EQ(utf8CodepointToByteOffset(text, 2), 4u);
    EXPECT_EQ(utf8CodepointToByteOffset(text, 3), 5u);
    // 越界返回 text.size()
    EXPECT_EQ(utf8CodepointToByteOffset(text, 10), 5u);
}

TEST(Utf8ByteOffsetToCodepointIndex, Basic)
{
    // "A你B" = bytes: [41] [E4 BD A0] [42]
    std::string text = "A"
                       "\xE4\xBD\xA0"
                       "B";
    EXPECT_EQ(utf8ByteOffsetToCodepointIndex(text, 0), 0u);
    EXPECT_EQ(utf8ByteOffsetToCodepointIndex(text, 1), 1u);
    EXPECT_EQ(utf8ByteOffsetToCodepointIndex(text, 4), 2u);
    EXPECT_EQ(utf8ByteOffsetToCodepointIndex(text, 5), 3u);
}

TEST(Utf8ByteOffsetToCodepointIndex, RoundTripConversion)
{
    // 码点索引 → 字节偏移 → 码点索引，双向转换应一致
    std::string text = "A"
                       "\xE4\xBD\xA0"
                       "\xF0\x9F\x98\x80"
                       "B";
    for (size_t i = 0; i <= utf8CodepointCount(text); ++i) {
        size_t byteOffset = utf8CodepointToByteOffset(text, i);
        size_t cpIndex = utf8ByteOffsetToCodepointIndex(text, byteOffset);
        EXPECT_EQ(cpIndex, i) << "Round-trip failed at codepoint index " << i;
    }
}

// ============================================================================
// utf8AlignToCodepointBoundary
// ============================================================================

TEST(Utf8AlignToCodepointBoundary, AlreadyAligned)
{
    // "A你B"
    std::string text = "A"
                       "\xE4\xBD\xA0"
                       "B";
    EXPECT_EQ(utf8AlignToCodepointBoundary(text, 0), 0u);
    EXPECT_EQ(utf8AlignToCodepointBoundary(text, 1), 1u);
    EXPECT_EQ(utf8AlignToCodepointBoundary(text, 4), 4u);
    EXPECT_EQ(utf8AlignToCodepointBoundary(text, 5), 5u);
}

TEST(Utf8AlignToCodepointBoundary, MiddleOfSequence)
{
    // "你" = E4(0) BD(1) A0(2)，共3字节
    // BD(1) 和 A0(2) 都是续接字节，对齐到下一个码点边界
    std::string text = "\xE4\xBD\xA0";
    // 从字节1(BD)对齐：跳过BD、A0，到达字节3（字符串末尾）
    EXPECT_EQ(utf8AlignToCodepointBoundary(text, 1), 3u);
    // 从字节2(A0)对齐：跳过A0，到达字节3（字符串末尾）
    EXPECT_EQ(utf8AlignToCodepointBoundary(text, 2), 3u);

    // 更复杂的例子："A你B" = [41] [E4 BD A0] [42]
    std::string text2 = "A"
                        "\xE4\xBD\xA0"
                        "B";
    // 从字节1(E4)对齐：E4是首字节，已在边界上
    EXPECT_EQ(utf8AlignToCodepointBoundary(text2, 1), 1u);
    // 从字节2(BD)对齐：跳过BD、A0，到达字节4(42='B')
    EXPECT_EQ(utf8AlignToCodepointBoundary(text2, 2), 4u);
    // 从字节3(A0)对齐：跳过A0，到达字节4(42='B')
    EXPECT_EQ(utf8AlignToCodepointBoundary(text2, 3), 4u);
}

TEST(Utf8AlignToCodepointBoundary, EndOfString)
{
    std::string text = "Hi";
    EXPECT_EQ(utf8AlignToCodepointBoundary(text, 2), 2u);
}

// ============================================================================
// utf8ForEachCodepoint
// ============================================================================

TEST(Utf8ForEachCodepoint, EmptyString)
{
    std::string text;
    int count = 0;
    utf8ForEachCodepoint(text, [&](u32, size_t, size_t) { ++count; });
    EXPECT_EQ(count, 0);
}

TEST(Utf8ForEachCodepoint, Ascii)
{
    std::string text = "ABC";
    std::vector<u32> codepoints;
    utf8ForEachCodepoint(text, [&](u32 cp, size_t offset, size_t len) {
        codepoints.push_back(cp);
        EXPECT_EQ(len, 1u);
    });
    EXPECT_EQ(codepoints.size(), 3u);
    EXPECT_EQ(codepoints[0], 0x41u);
    EXPECT_EQ(codepoints[1], 0x42u);
    EXPECT_EQ(codepoints[2], 0x43u);
}

TEST(Utf8ForEachCodepoint, MixedContent)
{
    // "A你😀B"
    std::string text = "A"
                       "\xE4\xBD\xA0"
                       "\xF0\x9F\x98\x80"
                       "B";
    std::vector<u32> codepoints;
    std::vector<size_t> offsets;
    std::vector<size_t> lengths;
    utf8ForEachCodepoint(text, [&](u32 cp, size_t offset, size_t len) {
        codepoints.push_back(cp);
        offsets.push_back(offset);
        lengths.push_back(len);
    });
    EXPECT_EQ(codepoints.size(), 4u);
    EXPECT_EQ(codepoints[0], 0x41u);
    EXPECT_EQ(codepoints[1], 0x4F60u);
    EXPECT_EQ(codepoints[2], 0x1F600u);
    EXPECT_EQ(codepoints[3], 0x42u);

    EXPECT_EQ(offsets[0], 0u);
    EXPECT_EQ(offsets[1], 1u);
    EXPECT_EQ(offsets[2], 4u);
    EXPECT_EQ(offsets[3], 8u);

    EXPECT_EQ(lengths[0], 1u);
    EXPECT_EQ(lengths[1], 3u);
    EXPECT_EQ(lengths[2], 4u);
    EXPECT_EQ(lengths[3], 1u);
}

TEST(Utf8ForEachCodepoint, EarlyExitWithBoolCallback)
{
    // 测试回调返回 false 时提前退出
    std::string text = "ABCDE";
    int count = 0;
    utf8ForEachCodepoint(text, [&](u32 cp, size_t, size_t) -> bool {
        ++count;
        if (cp == 'C') {
            return false; // 在 'C' 处退出
        }
        return true;
    });
    EXPECT_EQ(count, 3); // A, B, C -> 3 次调用后退出
}

TEST(Utf8ForEachCodepoint, VoidCallbackIteratesAll)
{
    // 测试 void 回调遍历全部码点
    std::string text = "ABCDE";
    int count = 0;
    utf8ForEachCodepoint(text, [&](u32, size_t, size_t) { ++count; });
    EXPECT_EQ(count, 5);
}

TEST(Utf8ForEachCodepoint, EarlyExitWithMultibyteContent)
{
    // "A你B😀C" — 在 '你' 处提前退出
    std::string text = "A"
                       "\xE4\xBD\xA0"
                       "B"
                       "\xF0\x9F\x98\x80"
                       "C";
    std::vector<u32> collected;
    utf8ForEachCodepoint(text, [&](u32 cp, size_t, size_t) -> bool {
        collected.push_back(cp);
        if (cp == 0x4F60u) { // 你
            return false;
        }
        return true;
    });
    EXPECT_EQ(collected.size(), 2u);
    EXPECT_EQ(collected[0], 0x41u);
    EXPECT_EQ(collected[1], 0x4F60u);
}

// ============================================================================
// 综合测试：编码 → 解码 往返
// ============================================================================

TEST(Utf8RoundTrip, AllCodePointRanges)
{
    const u32 testPoints[] = {
        0x00,     // NUL
        0x41,     // 'A'
        0x7F,     // DEL
        0x80,     // 第一个两字节码点
        0xC0,     // À
        0xE9,     // é
        0x7FF,    // 最后一个两字节码点
        0x800,    // 第一个三字节码点
        0x4F60,   // 你
        0xFFFF,   // 最后一个三字节码点
        0x10000,  // 第一个四字节码点
        0x1F600,  // 😀
        0x10FFFF, // 最后一个合法码点
    };

    for (u32 cp : testPoints) {
        std::string encoded = utf8Encode(cp);
        EXPECT_FALSE(encoded.empty()) << "utf8Encode(0x" << std::hex << cp << ") returned empty";
        EXPECT_EQ(encoded.size(), static_cast<size_t>(utf8EncodeLength(cp)));

        size_t index = 0;
        u32 decoded = utf8Decode(encoded, index);
        EXPECT_EQ(decoded, cp) << "Round-trip failed for codepoint 0x" << std::hex << cp;
        EXPECT_EQ(index, encoded.size());
    }
}

TEST(Utf8RoundTrip, EncodeDecodeAppend)
{
    // 模拟文本输入场景：逐码点追加，再逐码点解码
    std::string built;
    const u32 codepoints[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x4F60, 0x1F600}; // Hello你😀
    for (u32 cp : codepoints) {
        utf8Append(built, cp);
    }

    std::vector<u32> decoded;
    utf8ForEachCodepoint(built, [&](u32 cp, size_t, size_t) { decoded.push_back(cp); });

    EXPECT_EQ(decoded.size(), 7u);
    for (size_t i = 0; i < decoded.size(); ++i) {
        EXPECT_EQ(decoded[i], codepoints[i]);
    }
}
