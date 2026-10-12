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

#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/light/SWMRNibbleArray.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <utility>

namespace mc {
namespace {

/**
 * ChunkData 移动语义与 nibble 指针缓存的一致性回归测试。
 *
 * 【背景】ChunkData 用 m_skyNibblePtrs / m_blockNibblePtrs 两个指针数组实现
 * IChunk::getSkyNibbles()/getBlockNibbles()（返回 SWMRNibbleArray* const*），缓存的是
 * **本对象内部** &m_skyNibbles[i] / &m_blockNibbles[i] 的地址，由 _ensureNibblePtrs()
 * 在首次查询时惰性填充，并以 m_nibblePtrsInitialized 标记已填充。
 *
 * 【缺陷】移动构造/移动赋值若把这两个指针数组按位搬走、并继承"已初始化"标志，缓存里
 * 记录的仍是**源对象**的内部地址。而 nibble 数组本身是被逐元素移动的，目标对象的数组
 * 落在全新地址上，因此搬过来的指针全部悬垂；源对象析构后，对目标对象调用
 * getSkyNibbles() 就交出一组指向已释放内存的指针，读 nibbles[i]（8 字节指针）即
 * heap-use-after-free。存档加载路径
 * （`std::optional<ChunkData>(std::move(chunk))` → `make_unique<ChunkData>(std::move(...))`）
 * 正是这样连续移动 ChunkData 的。
 *
 * 【修复后不变式】移动后，目标对象的 nibble 指针缓存必须指向**目标对象自身**的数组，
 * 且已写入的光照数据随移动保留；源对象不得再交出指向已搬空数组的旧缓存。
 *
 * ASAN 构建下这些用例是有效的释放后使用探测器：若缓存仍指向源对象，析构源对象后经
 * 返回的指针数组解引用会被立即捕获。
 */
class ChunkDataMoveTest : public ::testing::Test {
protected:
    /// 天空光写入所用的段与坐标。
    ///
    /// PROBE_Y 取 MIN_BUILD_HEIGHT + 2 * 段高 + 3，即段索引 2、段内局部 y = 3，
    /// 便于用可读的常量验证"段索引 → 光照数组下标 +1"的换算。
    static constexpr i32 PROBE_X = 2;
    static constexpr i32 PROBE_Z = 4;
    static constexpr BlockCoord PROBE_Y = mc::world::MIN_BUILD_HEIGHT + 2 * mc::world::CHUNK_SECTION_HEIGHT + 3;
    static constexpr i32 PROBE_SECTION_INDEX = 2;
    static constexpr i32 PROBE_LOCAL_Y = 3;
    static constexpr u8 PROBE_SKY_LIGHT = 7;
    static constexpr u8 PROBE_BLOCK_LIGHT = 11;

    /**
     * 写入可辨识的光照数据，并触发 nibble 指针缓存的惰性填充。
     *
     * 缓存只在 getSkyNibbles()/getBlockNibbles() 首次被调用时填充，故必须显式查询一次，
     * 才能构造出"移动前缓存已初始化"这一触发缺陷的前置状态。
     */
    static void _prepareWithLight(ChunkData& chunk)
    {
        chunk.setSkyLight(PROBE_X, PROBE_Y, PROBE_Z, PROBE_SKY_LIGHT);
        chunk.setBlockLight(PROBE_X, PROBE_Y, PROBE_Z, PROBE_BLOCK_LIGHT);

        ASSERT_NE(chunk.getSkyNibbles(), nullptr);
        ASSERT_NE(chunk.getBlockNibbles(), nullptr);
    }

    /// 断言 nibble 指针数组的每一项都指向该 chunk 自身的数组。
    static void _expectSkyPointersBelongTo(const ChunkData& chunk, SWMRNibbleArray* const* nibbles)
    {
        const auto& own = chunk.skyNibbles();
        for (size_t i = 0; i < ChunkData::LIGHT_SECTIONS; ++i) {
            EXPECT_EQ(static_cast<const void*>(nibbles[i]), static_cast<const void*>(&own[i]))
                << "天空光 nibble 指针缓存第 " << i << " 项未指向本对象的数组";
        }
    }

    /// 同上，方块光照侧。
    static void _expectBlockPointersBelongTo(const ChunkData& chunk, SWMRNibbleArray* const* nibbles)
    {
        const auto& own = chunk.blockNibbles();
        for (size_t i = 0; i < ChunkData::LIGHT_SECTIONS; ++i) {
            EXPECT_EQ(static_cast<const void*>(nibbles[i]), static_cast<const void*>(&own[i]))
                << "方块光 nibble 指针缓存第 " << i << " 项未指向本对象的数组";
        }
    }

    /// 经**指针数组**（而非成员访问器）读取光照，确保走的是被测的缓存路径。
    static void _expectLightPreserved(const ChunkData& chunk)
    {
        SWMRNibbleArray* const* sky = chunk.getSkyNibbles();
        ASSERT_NE(sky, nullptr);
        SWMRNibbleArray* const* block = chunk.getBlockNibbles();
        ASSERT_NE(block, nullptr);

        // 段索引 → 光照数组下标需 +1（见 skyNibbleAt 注释：下界外多留一段缓冲）。
        ASSERT_EQ(mc::world::toSectionIndex(PROBE_Y), PROBE_SECTION_INDEX);
        ASSERT_EQ(mc::world::toSectionLocalY(PROBE_Y), PROBE_LOCAL_Y);
        const i32 lightIndex = PROBE_SECTION_INDEX + 1;
        ASSERT_LT(lightIndex, ChunkData::LIGHT_SECTIONS);

        ASSERT_NE(sky[lightIndex], nullptr);
        ASSERT_NE(block[lightIndex], nullptr);
        EXPECT_EQ(sky[lightIndex]->getVisible(PROBE_X, PROBE_LOCAL_Y, PROBE_Z), PROBE_SKY_LIGHT)
            << "天空光数据未随移动保留";
        EXPECT_EQ(block[lightIndex]->getVisible(PROBE_X, PROBE_LOCAL_Y, PROBE_Z), PROBE_BLOCK_LIGHT)
            << "方块光数据未随移动保留";

        _expectSkyPointersBelongTo(chunk, sky);
        _expectBlockPointersBelongTo(chunk, block);
    }
};

/**
 * 移动构造后，目标对象的 nibble 指针缓存必须指向目标自身的数组，且光照数据保留。
 *
 * 修复前：缓存被按位搬运，指向源对象内部地址；源对象离开作用域后即为悬垂指针。
 */
TEST_F(ChunkDataMoveTest, MoveConstructorRebuildsNibblePointerCache)
{
    ChunkData source(3, -7);
    _prepareWithLight(source);

    ChunkData moved(std::move(source));

    _expectLightPreserved(moved);
}

/**
 * 源对象析构后，目标对象经缓存取出的 nibble 仍必须可读（释放后使用的直接回归）。
 *
 * 用堆上的源对象并在移动后立即 reset，精确复现"源对象先于目标对象析构"的时序，
 * 也就是存档加载路径中 `std::optional<ChunkData>` 被消费后的真实情形。
 */
TEST_F(ChunkDataMoveTest, NibblesUsableAfterSourceDestroyed)
{
    auto source = std::make_unique<ChunkData>(11, 12);
    _prepareWithLight(*source);

    // 模拟 `std::optional<ChunkData>(std::move(chunk))` 后由 make_unique 再次移动：
    // 两次移动都不允许把悬垂指针带到最终对象。
    std::optional<ChunkData> staged(std::move(*source));
    auto finalChunk = std::make_unique<ChunkData>(std::move(staged.value()));
    staged.reset();
    source.reset(); // 源对象析构，其 nibble 数组的内存不再有效

    _expectLightPreserved(*finalChunk);

    // 再走一次真实读取路径：toByteArray() 读 visible 侧，悬垂时即 ASAN 命中。
    SWMRNibbleArray* const* sky = finalChunk->getSkyNibbles();
    ASSERT_NE(sky[PROBE_SECTION_INDEX + 1], nullptr);
    EXPECT_EQ(sky[PROBE_SECTION_INDEX + 1]->toByteArray().size(), SWMRNibbleArray::ARRAY_SIZE);
}

/**
 * 移动赋值同样不得把源对象的内部地址带入目标对象，且光照数据须保留。
 */
TEST_F(ChunkDataMoveTest, MoveAssignmentRebuildsNibblePointerCache)
{
    ChunkData source(1, 2);
    _prepareWithLight(source);

    ChunkData target(5, 6);
    _prepareWithLight(target);

    target = std::move(source);

    _expectLightPreserved(target);
}

/**
 * 移动后的源对象若被再次查询，也必须交出自身有效的数组，而不是悬垂的旧缓存。
 *
 * 源对象处于"有效但未指定"状态；本用例把不变式收紧为：查询它不构成释放后使用。
 */
TEST_F(ChunkDataMoveTest, MovedFromChunkReturnsItsOwnNibbles)
{
    ChunkData source(9, 9);
    _prepareWithLight(source);

    ChunkData moved(std::move(source));

    SWMRNibbleArray* const* fromSource = source.getSkyNibbles();
    ASSERT_NE(fromSource, nullptr);
    _expectSkyPointersBelongTo(source, fromSource);
    SWMRNibbleArray* const* blockFromSource = source.getBlockNibbles();
    ASSERT_NE(blockFromSource, nullptr);
    _expectBlockPointersBelongTo(source, blockFromSource);
}

} // namespace
} // namespace mc
