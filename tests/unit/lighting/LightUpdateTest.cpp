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

// macOS系统头文件中，BYTE_SIZE被定义为宏，会与NibbleArray的静态常数冲突
// 使用pragma push_macro/pop_macro来暂时屏蔽系统宏
#pragma push_macro("BYTE_SIZE")
#undef BYTE_SIZE

#include <gtest/gtest.h>

#include "common/network/sync/ChunkSerializer.hpp"
#include "common/util/Direction.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/light/SWMRNibbleArray.hpp"

#undef BYTE_SIZE // Re-undef after includes which may re-define BYTE_SIZE

using namespace mc::network;
using namespace mc;

// ============================================================================
// ChunkData 光照存储测试
// ============================================================================
//
// 光照的权威副本在 ChunkData 的 SWMRNibbleArray（对齐原版 1.21.11：光照归 chunk 层，
// LevelChunkSection 只含 states + biomes）。ChunkSection 不再持有光照，故这里全部
// 经 chunk->skyNibbleAt/blockNibbleAt(sectionIndex) 读写。

class ChunkDataLightTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        chunk = std::make_unique<ChunkData>(0, 0);

        // 段索引 4 对应世界 Y=0..15（MIN_BUILD_HEIGHT=-64，故 (0-(-64))/16 = 4）。
        // 光照 nibble 的索引由 skyNibbleAt/blockNibbleAt 统一换算（内部偏移 1 段）。
        constexpr i32 kSectionIndex = 4;

        // 设置天空光: localY=0 -> 15（底部最亮），localY=15 -> 0（顶部最暗）。
        // 首次逐点写入前先 materialize 全亮，否则未写入坐标会从默认 15 骤变为 0。
        SWMRNibbleArray& skyLight = chunk->skyNibbleAt(kSectionIndex);
        skyLight.setFull();
        for (i32 y = 0; y < 16; ++y) {
            for (i32 z = 0; z < 16; ++z) {
                for (i32 x = 0; x < 16; ++x) {
                    skyLight.set(x, y, z, static_cast<u8>(15 - y));
                }
            }
        }
        skyLight.updateVisible();

        // 方块光默认即 0，无需 materialize。
        SWMRNibbleArray& blockLight = chunk->blockNibbleAt(kSectionIndex);
        blockLight.setZero();
        blockLight.updateVisible();
    }

    std::unique_ptr<ChunkData> chunk;
};

TEST_F(ChunkDataLightTest, GetSkyLight)
{
    // worldY=0 -> localY=0 -> 光照=15（底部）
    // worldY=1 -> localY=1 -> 光照=14
    // worldY=15 -> localY=15 -> 光照=0（顶部）
    EXPECT_EQ(chunk->getSkyLight(0, 0, 0), 15);
    EXPECT_EQ(chunk->getSkyLight(0, 1, 0), 14);
    EXPECT_EQ(chunk->getSkyLight(0, 15, 0), 0);
}

TEST_F(ChunkDataLightTest, GetBlockLight)
{
    EXPECT_EQ(chunk->getBlockLight(0, 15, 0), 0);
    EXPECT_EQ(chunk->getBlockLight(8, 8, 8), 0);
}

TEST_F(ChunkDataLightTest, SetBlockLight)
{
    // 设置发光方块
    chunk->setBlockLight(5, 5, 5, 15);

    EXPECT_EQ(chunk->getBlockLight(5, 5, 5), 15);
}

TEST_F(ChunkDataLightTest, SetSkyLight)
{
    // SetUp 已把整段写成 15-y，故此处只验证逐点覆盖生效、且不影响同层其它坐标。
    chunk->setSkyLight(10, 10, 10, 8);

    EXPECT_EQ(chunk->getSkyLight(10, 10, 10), 8);
    EXPECT_EQ(chunk->getSkyLight(11, 10, 10), 5); // SetUp 在 y=10 写入的 15-10
}

TEST_F(ChunkDataLightTest, SkyLightNibbleArrayDirectAccess)
{
    // 经 skyNibbleAt 直接操作 nibble（光照引擎走的路径）
    SWMRNibbleArray& skyLight = chunk->skyNibbleAt(4);

    skyLight.set(7, 7, 7, 12);
    skyLight.updateVisible();

    EXPECT_EQ(chunk->getSkyLight(7, 7, 7), 12);
}

TEST_F(ChunkDataLightTest, BlockLightNibbleArrayDirectAccess)
{
    SWMRNibbleArray& blockLight = chunk->blockNibbleAt(4);

    blockLight.set(3, 5, 7, 8);
    blockLight.updateVisible();

    EXPECT_EQ(chunk->getBlockLight(3, 5, 7), 8);
}

TEST_F(ChunkDataLightTest, SectionSerializationCarriesNoLightBytes)
{
    // 段序列化只含方块数据；光照归 ChunkData，不进段字节流。
    ChunkSection section;
    const auto serialized = ChunkSerializer::serializeSection(section);

    // 2(计数) + 4096*4(方块状态ID)
    EXPECT_EQ(serialized.size(), 2 + ChunkSection::VOLUME * sizeof(u32));
}

// ============================================================================
// NibbleArray 光照存储测试
// ============================================================================

class LightNibbleArrayTest : public ::testing::Test {
protected:
    void SetUp() override { array = std::make_unique<NibbleArray>(); }

    std::unique_ptr<NibbleArray> array;
};

TEST_F(LightNibbleArrayTest, InitiallyEmpty)
{
    EXPECT_TRUE(array->isEmpty());
    EXPECT_FALSE(array->isValid());
}

TEST_F(LightNibbleArrayTest, SetAndGet)
{
    array->set(5, 10, 3, 12);

    EXPECT_EQ(array->get(5, 10, 3), 12);
    EXPECT_TRUE(array->isValid());
}

TEST_F(LightNibbleArrayTest, ValueTruncation)
{
    // 值大于15应该被截断
    array->set(0, 0, 0, 20);

    EXPECT_EQ(array->get(0, 0, 0), 4); // 20 & 0xF = 4
}

TEST_F(LightNibbleArrayTest, IndexWrapping)
{
    // 坐标自动取模
    array->set(0, 0, 0, 7);
    array->set(16, 0, 0, 8); // 16 % 16 = 0, 应该覆盖前一个值

    EXPECT_EQ(array->get(0, 0, 0), 8);
    EXPECT_EQ(array->get(16, 0, 0), 8); // 读取也会取模
}

TEST_F(LightNibbleArrayTest, Fill)
{
    array->fill(10);

    // 验证所有位置都是10
    for (i32 y = 0; y < 16; ++y) {
        for (i32 z = 0; z < 16; ++z) {
            for (i32 x = 0; x < 16; ++x) {
                EXPECT_EQ(array->get(x, y, z), 10);
            }
        }
    }
}

TEST_F(LightNibbleArrayTest, FilledStaticMethod)
{
    auto filledArray = NibbleArray::filled(15);

    EXPECT_TRUE(filledArray.isValid());
    EXPECT_EQ(filledArray.get(7, 7, 7), 15);
}

TEST_F(LightNibbleArrayTest, LinearIndexAccess)
{
    // 使用线性索引设置
    array->set(0, 15); // 设置索引0为15

    EXPECT_EQ(array->get(0), 15);

    // 验证3D坐标访问一致
    // 索引0 = y*256 + z*16 + x = 0, 所以 x=0, y=0, z=0
    EXPECT_EQ(array->get(0, 0, 0), 15);
}

TEST_F(LightNibbleArrayTest, DataSize)
{
    array->set(0, 0, 0, 1); // 触发分配

    EXPECT_EQ(array->data().size(), NibbleArray::BYTE_SIZE);
    EXPECT_EQ(NibbleArray::BYTE_SIZE, 2048);
    EXPECT_EQ(NibbleArray::VALUE_COUNT, 4096);
}

TEST_F(LightNibbleArrayTest, Copy)
{
    array->set(5, 5, 5, 7);
    array->set(10, 10, 10, 13);

    NibbleArray copy = array->copy();

    EXPECT_EQ(copy.get(5, 5, 5), 7);
    EXPECT_EQ(copy.get(10, 10, 10), 13);

    // 修改原数组不影响副本
    array->set(5, 5, 5, 14);
    EXPECT_EQ(copy.get(5, 5, 5), 7);
}

TEST_F(LightNibbleArrayTest, ConstructFromData)
{
    // 创建测试数据
    std::vector<u8> data(NibbleArray::BYTE_SIZE, 0xAB);

    NibbleArray arrayFromData(std::move(data));

    EXPECT_TRUE(arrayFromData.isValid());

    // 0xAB = 10101011
    // 偶数索引存储低4位 (B = 1011 = 11)
    // 奇数索引存储高4位 (A = 1010 = 10)
    EXPECT_EQ(arrayFromData.get(0), 0xB); // 索引0，低4位
    EXPECT_EQ(arrayFromData.get(1), 0xA); // 索引1，高4位
}

// ============================================================================
// LightEngineUtils 方向测试
// ============================================================================

TEST(LightEngineUtilsDirectionTest, AllDirections)
{
    // 测试所有方向
    EXPECT_EQ(mc::Directions::fromDelta(1, 0, 0), mc::Direction::East);
    EXPECT_EQ(mc::Directions::fromDelta(-1, 0, 0), mc::Direction::West);
    EXPECT_EQ(mc::Directions::fromDelta(0, 1, 0), mc::Direction::Up);
    EXPECT_EQ(mc::Directions::fromDelta(0, -1, 0), mc::Direction::Down);
    EXPECT_EQ(mc::Directions::fromDelta(0, 0, 1), mc::Direction::South);
    EXPECT_EQ(mc::Directions::fromDelta(0, 0, -1), mc::Direction::North);
}

TEST(LightEngineUtilsDirectionTest, OffsetsConsistency)
{
    // 验证方向偏移的一致性
    for (int i = 0; i < 6; ++i) {
        mc::Direction dir = static_cast<mc::Direction>(i);

        // 根据偏移获取方向
        mc::Direction fromOffset = mc::Directions::fromDelta(
            mc::Directions::xOffset(dir), mc::Directions::yOffset(dir), mc::Directions::zOffset(dir));

        EXPECT_EQ(fromOffset, dir) << "Direction offset consistency failed for direction " << i;
    }
}

TEST(LightEngineUtilsDirectionTest, OppositeDirections)
{
    EXPECT_EQ(mc::Directions::opposite(mc::Direction::Down), mc::Direction::Up);
    EXPECT_EQ(mc::Directions::opposite(mc::Direction::Up), mc::Direction::Down);
    EXPECT_EQ(mc::Directions::opposite(mc::Direction::North), mc::Direction::South);
    EXPECT_EQ(mc::Directions::opposite(mc::Direction::South), mc::Direction::North);
    EXPECT_EQ(mc::Directions::opposite(mc::Direction::West), mc::Direction::East);
    EXPECT_EQ(mc::Directions::opposite(mc::Direction::East), mc::Direction::West);
}

TEST(LightEngineUtilsDirectionTest, AxisDirection)
{
    // 垂直方向
    EXPECT_TRUE(mc::Directions::isVertical(mc::Direction::Up));
    EXPECT_TRUE(mc::Directions::isVertical(mc::Direction::Down));
    EXPECT_FALSE(mc::Directions::isVertical(mc::Direction::North));

    // 水平方向
    EXPECT_TRUE(mc::Directions::isHorizontal(mc::Direction::North));
    EXPECT_TRUE(mc::Directions::isHorizontal(mc::Direction::South));
    EXPECT_TRUE(mc::Directions::isHorizontal(mc::Direction::East));
    EXPECT_TRUE(mc::Directions::isHorizontal(mc::Direction::West));
    EXPECT_FALSE(mc::Directions::isHorizontal(mc::Direction::Up));

    // 轴
    EXPECT_EQ(mc::Directions::getAxis(mc::Direction::Up), mc::Axis::Y);
    EXPECT_EQ(mc::Directions::getAxis(mc::Direction::Down), mc::Axis::Y);
    EXPECT_EQ(mc::Directions::getAxis(mc::Direction::North), mc::Axis::Z);
    EXPECT_EQ(mc::Directions::getAxis(mc::Direction::South), mc::Axis::Z);
    EXPECT_EQ(mc::Directions::getAxis(mc::Direction::East), mc::Axis::X);
    EXPECT_EQ(mc::Directions::getAxis(mc::Direction::West), mc::Axis::X);
}

#pragma pop_macro("BYTE_SIZE")
