/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software are
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

#include "common/world/storage/reader/bedrock/BedrockChunkReader.hpp"

#include "common/world/storage/reader/bedrock/BedrockBiomeMapper.hpp"
#include "common/world/storage/reader/bedrock/BedrockLevelDb.hpp"
#include <gtest/gtest.h>

namespace mc::world::storage::reader::bedrock {
namespace {

class BedrockChunkReaderTest : public ::testing::Test {
protected:
    BedrockBiomeMapper biomeMapper;
    BedrockChunkReader reader{biomeMapper};
};

TEST_F(BedrockChunkReaderTest, ReadVarUintDecodesMultiByteValue)
{
    const std::vector<u8> bytes{0xAC, 0x02};
    size_t pos = 0;
    auto result = reader.readVarUint(bytes, pos);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), 300u);
    EXPECT_EQ(pos, 2u);
}

TEST_F(BedrockChunkReaderTest, ReadPackedIndicesDecodesBedrockWordOrder)
{
    std::vector<u8> bytes;
    bytes.resize(4);
    const u32 packedWord = 0x76543210u;
    bytes[0] = static_cast<u8>(packedWord & 0xFF);
    bytes[1] = static_cast<u8>((packedWord >> 8) & 0xFF);
    bytes[2] = static_cast<u8>((packedWord >> 16) & 0xFF);
    bytes[3] = static_cast<u8>((packedWord >> 24) & 0xFF);

    size_t pos = 0;
    auto result = reader.readPackedIndices(bytes, pos, 4, 8, 32);
    ASSERT_TRUE(result.success());
    const auto& indices = result.value();
    ASSERT_EQ(indices.size(), 8u);
    for (u32 i = 0; i < 8; ++i) {
        EXPECT_EQ(indices[static_cast<size_t>(i)], i);
    }
}

TEST(BedrockLevelDbKeyTest, ActorAndLocalPlayerKeysRemainStable)
{
    const auto actorPrefix = BedrockLevelDb::buildActorPrefix();
    EXPECT_EQ(std::string(actorPrefix.begin(), actorPrefix.end()), "actorprefix");

    const auto localPlayer = BedrockLevelDb::buildLocalPlayerKey();
    EXPECT_EQ(std::string(localPlayer.begin(), localPlayer.end()), "~local_player");
}

} // namespace
} // namespace mc::world::storage::reader::bedrock
