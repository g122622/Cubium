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
 * IMPLIED, INCLUDING ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "FishingBobberRenderer.hpp"
#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include <cmath>
#include <vector>

namespace mc::client::renderer::entity::renderer::projectile {

using model::ModelVertex;

FishingBobberRenderer::FishingBobberRenderer()
{
    m_shadowSize = 0.0;
}

void FishingBobberRenderer::render(Entity& entity, f64 partialTicks)
{
    // Vulkan 管线路径处理渲染，此方法为传统渲染路径保留
    (void)entity;
    (void)partialTicks;
}

bool FishingBobberRenderer::generateMesh(
    ClientEntity& entity, std::vector<ModelVertex>& vertices, std::vector<u32>& indices)
{
    // 钓鱼浮标 + 钓线全部使用 LINE_LIST 拓扑
    // 浮标渲染为十字线段，钓线为抛物线

    // 读取咬钩状态（通过元数据同步自服务端 DATA_BITING_PARAM）。
    // 对应 MC 1.21.11 FishingHook.onSyncedDataUpdated(DATA_BITING)：
    // 咬钩时浮标获得向下速度并下沉，此处以 Y 轴负偏移模拟下沉视觉效果。
    const bool biting = entity.fishingBiting();

    _generateBobberCross(vertices, indices, biting);
    _generateFishingLine(entity, vertices, indices, biting);

    return !vertices.empty() && !indices.empty();
}

bool FishingBobberRenderer::needsMeshUpdate(ClientEntity& entity) const
{
    // 钓线位置随实体移动而变化，每帧都需要更新；
    // 咬钩状态变化也需要重建网格（浮标下沉偏移）。
    (void)entity;
    return true;
}

void FishingBobberRenderer::_generateBobberCross(
    std::vector<ModelVertex>& vertices, std::vector<u32>& indices, bool biting)
{
    // 浮标渲染为十字线段（LINE_LIST）
    const f32 halfSize = 0.0625f; // 1/16 格，约 MC 原版浮标大小
    // 浮标浮在水面上的高度偏移；咬钩时下沉 0.1 格模拟 MC 中
    // DATA_BITING 触发的 -0.4*random[0.6,1.0] 向下速度造成的视觉下沉。
    const f32 yOffset = biting ? 0.15f : 0.25f;

    const u32 baseIndex = static_cast<u32>(vertices.size());

    // 水平线段 (X轴)
    vertices.emplace_back(
        ModelVertex{Vector3f(-halfSize, yOffset, 0.0f), Vector2f(0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f)});
    vertices.emplace_back(
        ModelVertex{Vector3f(halfSize, yOffset, 0.0f), Vector2f(0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f)});

    // 纵向线段 (Z轴)
    vertices.emplace_back(
        ModelVertex{Vector3f(0.0f, yOffset, -halfSize), Vector2f(0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f)});
    vertices.emplace_back(
        ModelVertex{Vector3f(0.0f, yOffset, halfSize), Vector2f(0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f)});

    // 垂直线段 (Y轴 - 浮标尖端)
    vertices.emplace_back(
        ModelVertex{Vector3f(0.0f, yOffset - halfSize, 0.0f), Vector2f(0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f)});
    vertices.emplace_back(
        ModelVertex{Vector3f(0.0f, yOffset + halfSize * 2.0f, 0.0f), Vector2f(0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f)});

    // X轴线索引
    indices.emplace_back(baseIndex + 0);
    indices.emplace_back(baseIndex + 1);

    // Z轴线索引
    indices.emplace_back(baseIndex + 2);
    indices.emplace_back(baseIndex + 3);

    // Y轴线索引
    indices.emplace_back(baseIndex + 4);
    indices.emplace_back(baseIndex + 5);
}

void FishingBobberRenderer::_generateFishingLine(
    ClientEntity& entity, std::vector<ModelVertex>& vertices, std::vector<u32>& indices, bool biting)
{
    // 钓线渲染为16段抛物线
    constexpr i32 SEGMENTS = 16;

    // 读取被钩住实体 ID 镜像（通过元数据同步自服务端 DATA_HOOKED_ENTITY_PARAM）。
    // >0 表示浮标钩住了某个实体，此时钓线应绷紧（下垂量减小），
    // 模拟钓线连接到附近被钩实体而非远端玩家的视觉。
    // 对应 MC 1.21.11 FishingHook.onSyncedDataUpdated(DATA_HOOKED_ENTITY)。
    const i32 hookedEntityId = entity.fishingHookedEntityId();
    const bool hasHookedEntity = (hookedEntityId > 0);

    // 浮标世界位置（咬钩时与浮标下沉偏移保持一致）
    const f32 bobberYOffset = biting ? 0.15f : 0.25f;
    Vector3f bobberPos(
        static_cast<f32>(entity.x()), static_cast<f32>(entity.y() + bobberYOffset), static_cast<f32>(entity.z()));

    // 玩家手持位置（简化为浮标上方偏移）
    // TODO: 当渲染层能访问持有者实体（及被钩住实体）时，计算实际的手持位置。
    //   钓线两端在 MC 1.21.11 中由 FishingHook.getPlayerOwner() 决定起点（玩家眼睛/手部），
    //   若存在被钩住实体（hasHookedEntity == true），
    //   钓线另一端应连接到该实体位置而非玩家手中。
    //   当前 PipelineMeshProvider::generateMesh 接口仅传入 ClientEntity&，
    //   无世界查找回调，因此暂用固定偏移占位，仅通过 hasHookedEntity 调整下垂量。
    Vector3f playerHandPos(bobberPos.x, bobberPos.y + 1.5f, bobberPos.z);

    Vector3f start = playerHandPos;
    Vector3f end = bobberPos;

    // 计算钓线的中点下垂量
    f32 dx = end.x - start.x;
    f32 dy = end.y - start.y;
    f32 dz = end.z - start.z;
    f32 horizontalDist = std::sqrt(dx * dx + dz * dz);

    // 下垂量取决于水平距离，最大约 0.25 格。
    // 钩住实体时钓线绷紧（下垂量减半），模拟连接到附近被钩实体。
    f32 sag = horizontalDist * 0.1f + 0.15f;
    if (hasHookedEntity) {
        sag *= 0.5f;
    }

    const u32 baseIndex = static_cast<u32>(vertices.size());

    for (i32 i = 0; i <= SEGMENTS; ++i) {
        f32 t = static_cast<f32>(i) / static_cast<f32>(SEGMENTS);

        // 线性插值 + 抛物线下垂
        f32 x = start.x + (end.x - start.x) * t;
        f32 z = start.z + (end.z - start.z) * t;
        // Y 轴：线性插值 + sin 曲线模拟重力下垂
        f32 y = start.y + (end.y - start.y) * t - sag * std::sin(t * static_cast<f32>(math::PI));

        vertices.emplace_back(ModelVertex{Vector3f(x, y, z), Vector2f(0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f)});

        if (i > 0) {
            indices.emplace_back(baseIndex + static_cast<u32>(i - 1));
            indices.emplace_back(baseIndex + static_cast<u32>(i));
        }
    }
}

} // namespace mc::client::renderer::entity::renderer::projectile
