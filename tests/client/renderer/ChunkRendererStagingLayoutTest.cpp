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
