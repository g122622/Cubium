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

#include "client/renderer/trident/chunk/ChunkRenderer.hpp"
#include <gtest/gtest.h>

namespace mc::client::test {

TEST(ChunkRendererStagingLayoutTest, IndexSegmentDoesNotOverlapVertexSegment)
{
    const VkDeviceSize vertexSize = 37;
    const VkDeviceSize indexSize = 1024;

    const auto layout = ChunkRenderer::buildStagingCopyLayout(vertexSize, indexSize);

    EXPECT_EQ(layout.vertexOffset, 0);
    EXPECT_GE(layout.indexOffset, vertexSize);
    EXPECT_EQ(layout.indexOffset % ChunkRenderer::stagingCopyAlignment(), 0);
    EXPECT_EQ(layout.totalSize, layout.indexOffset + indexSize);
}

TEST(ChunkRendererStagingLayoutTest, HandlesTypicalChunkBufferSizes)
{
    const VkDeviceSize vertexSize = 65536;
    const VkDeviceSize indexSize = 98304;

    const auto layout = ChunkRenderer::buildStagingCopyLayout(vertexSize, indexSize);

    EXPECT_EQ(layout.vertexOffset, 0);
    EXPECT_EQ(layout.indexOffset, vertexSize);
    EXPECT_EQ(layout.totalSize, vertexSize + indexSize);
}

TEST(ChunkRendererStagingLayoutTest, HandlesZeroSizedSegments)
{
    const auto layout = ChunkRenderer::buildStagingCopyLayout(0, 0);

    EXPECT_EQ(layout.vertexOffset, 0);
    EXPECT_EQ(layout.indexOffset, 0);
    EXPECT_EQ(layout.totalSize, 0);
}

} // namespace mc::client::test
