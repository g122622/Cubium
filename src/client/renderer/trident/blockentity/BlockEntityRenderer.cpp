#include "BlockEntityRenderer.hpp"
#include "client/resource/BlockModelCache.hpp"
#include "client/renderer/trident/core/texture/TridentTexture.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"

namespace mc::client::renderer::trident::blockentity {

BlockEntityRendererHelper::BlockEntityRendererHelper() = default;
BlockEntityRendererHelper::~BlockEntityRendererHelper() = default;

bool BlockEntityRendererHelper::renderBlock(
    const BlockState& state,
    const BlockPos& pos,
    u32 light)
{
    return renderBlockWithOffset(state, pos, 0.0f, 0.0f, 0.0f, light);
}

bool BlockEntityRendererHelper::renderBlockWithOffset(
    const BlockState& state,
    const BlockPos& pos,
    f32 offsetX,
    f32 offsetY,
    f32 offsetZ,
    u32 light)
{
    if (!m_modelCache) {
        return false;
    }

    // 获取方块外观
    const auto* appearance = m_modelCache->getBlockAppearance(&state);
    if (!appearance) {
        return false;
    }

    // TODO: 实现方块模型渲染
    // 这需要：
    // 1. 创建变换矩阵（位置 + 偏移）
    // 2. 遍历模型元素
    // 3. 生成顶点数据
    // 4. 提交到渲染管线

    return true;
}

u32 BlockEntityRendererHelper::getLightAt(IWorld& world, const BlockPos& pos) {
    // 获取天空光和方块光
    const u32 skyLight = world.getSkyLight(pos);
    const u32 blockLight = world.getBlockLight(pos);

    // 组合光照值：天空光在低4位，方块光在高4位
    // MC格式：skyLight << 20 | blockLight << 4
    return (skyLight << 20) | (blockLight << 4);
}

} // namespace mc::client::renderer::trident::blockentity
