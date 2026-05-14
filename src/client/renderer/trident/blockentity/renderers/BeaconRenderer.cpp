#include "BeaconRenderer.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/blockentity/processing/BeaconEntity.hpp"
#include <cmath>

namespace mc::client::renderer::trident::blockentity {

BeaconRenderer::BeaconRenderer()
    : BlockEntityRenderer<mc::blockentity::BeaconEntity>()
    , m_beamModel()
    , m_helper()
{}

void BeaconRenderer::render(const mc::blockentity::BeaconEntity& entity, f32 partialTick, u32 light)
{
    const BlockPos& pos = entity.getPos();

    // 渲染信标基座（普通方块渲染）
    renderBeaconBase(pos, light);

    // 如果未激活，不渲染光束
    if (!entity.isActive()) {
        return;
    }

    // 获取光束段数据
    const auto& segments = entity.getBeamSegments();
    if (segments.empty()) {
        return;
    }

    // 获取游戏时间
    // MC 1.16.5: long i = tileEntityIn.getWorld().getGameTime();
    // 注意: 需要 BlockEntity 持有世界引用或通过 render() 参数传入
    // 当前使用默认值，完整集成需要在渲染调度器中传入
    i64 gameTime = 0;

    // 渲染光束
    renderBeam(pos, segments, gameTime, partialTick, light);
}

void BeaconRenderer::renderBeaconBase(const BlockPos& pos, u32 light)
{
    // 信标基座使用普通方块模型渲染
    // 由 BlockModelCache 和区块渲染器处理
    // 此处无需额外处理
    (void)pos;
    (void)light;
}

void BeaconRenderer::renderBeam(const BlockPos& pos,
    const std::vector<mc::blockentity::BeaconBeamSegment>& segments,
    i64 gameTime,
    f32 partialTick,
    u32 light)
{
    // 参考 MC 1.16.5 BeaconTileEntityRenderer.render()
    // 1. 设置模型变换（平移到方块中心）
    // 2. 计算旋转角度
    // 3. 渲染每个光束段

    // 清除并设置光束段（BeaconBeamSegment 与 BeamSegment 是同一类型）
    m_beamModel.clearSegments();
    for (const auto& segment : segments) {
        m_beamModel.addSegment(segment);
    }

    // 生成网格数据
    std::vector<entity::model::ModelVertex> vertices;
    std::vector<u32> indices;
    m_beamModel.generateMesh(vertices, indices, gameTime, partialTick);

    // 网格数据已生成，后续集成步骤：
    // 1. 获取 RenderType.beaconBeam(texture, true/false)
    // 2. 创建变换矩阵（平移到 pos + 0.5, 0.0, 0.5）
    // 3. 应用旋转（绕 Y 轴）
    // 4. 提交顶点数据到渲染管线
    // 注意：完整渲染管线集成需要在 EntityPipeline 或专用 BlockEntityPipeline 中实现

    (void)pos;
    (void)light;
}

} // namespace mc::client::renderer::trident::blockentity
