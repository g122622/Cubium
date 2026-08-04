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

#include "VehicleRenderers.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/entity/entities/vehicle/MinecartEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace mc::client::renderer::entity::renderer::vehicle {

namespace {
using mc::math::PI;
using mc::math::PI_DOUBLE;

/// TNT 矿车类型 ID 字符串（用于 typeId 比较）
constexpr const char* TNT_MINECART_TYPE = ::mc::entity::EntityTypeKeys::TNT_MINECART;

/// 从 ClientEntity 读取类型 ID（去掉 minecraft: 前缀的简短形式也兼容）
[[nodiscard]] bool isTntMinecart(const ::mc::client::ClientEntity& entity) noexcept
{
    const std::string& tid = entity.getTypeId();
    return tid == TNT_MINECART_TYPE || tid == "tnt_minecart";
}

/// 将度数转换为弧度
[[nodiscard]] inline f64 degToRad(f64 degrees) noexcept
{
    return degrees * static_cast<f64>(mc::math::DEG_TO_RAD);
}

} // namespace

// ==================== 船模型 ====================

BoatModel::BoatModel()
    : m_textureWidth(128)
    , m_textureHeight(64)
{
    _setupParts();
}

void BoatModel::_setupParts()
{
    // 纹理尺寸：128x64

    // 底部面 (amodelrenderer[0])
    m_bottom = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatBottom");
    m_bottom->setTextureSize(128, 64);
    m_bottom->setTextureOffset(0, 0);
    m_bottom->addBox(-14.0f, -9.0f, -3.0f, 28.0f, 16.0f, 3.0f, 0.0f);
    m_bottom->setRotationPoint(0.0f, 3.0f, 1.0f);
    m_bottom->setRotateAngleX(static_cast<f32>(PI_DOUBLE / 2.0));

    // 右侧面 (amodelrenderer[1]) - 旋转 PI_DOUBLE*1.5
    m_right = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatRight");
    m_right->setTextureSize(128, 64);
    m_right->setTextureOffset(0, 19);
    m_right->addBox(-13.0f, -7.0f, -1.0f, 18.0f, 6.0f, 2.0f, 0.0f);
    m_right->setRotationPoint(-15.0f, 4.0f, 4.0f);
    m_right->setRotateAngleY(static_cast<f32>(PI_DOUBLE * 1.5));

    // 左侧面 (amodelrenderer[2]) - 旋转 PI_DOUBLE/2
    m_left = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatLeft");
    m_left->setTextureSize(128, 64);
    m_left->setTextureOffset(0, 19);
    m_left->addBox(-8.0f, -7.0f, -1.0f, 16.0f, 6.0f, 2.0f, 0.0f);
    m_left->setRotationPoint(15.0f, 4.0f, 0.0f);
    m_left->setRotateAngleY(static_cast<f32>(PI_DOUBLE / 2.0));

    // 前面 (amodelrenderer[3]) - 旋转 PI_DOUBLE
    m_front = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatFront");
    m_front->setTextureSize(128, 64);
    m_front->setTextureOffset(0, 19);
    m_front->addBox(-14.0f, -7.0f, -1.0f, 28.0f, 6.0f, 2.0f, 0.0f);
    m_front->setRotationPoint(0.0f, 4.0f, -9.0f);
    m_front->setRotateAngleY(static_cast<f32>(PI_DOUBLE));

    // 后面 (amodelrenderer[4])
    m_back = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatBack");
    m_back->setTextureSize(128, 64);
    m_back->setTextureOffset(0, 19);
    m_back->addBox(-14.0f, -7.0f, -1.0f, 28.0f, 6.0f, 2.0f, 0.0f);
    m_back->setRotationPoint(0.0f, 4.0f, 9.0f);

    // 桨
    m_paddleLeft = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("paddleLeft");
    m_paddleLeft->setTextureSize(128, 64);
    m_paddleLeft->setTextureOffset(62, 0);
    m_paddleLeft->addBox(-1.0f, 0.0f, -5.0f, 2.0f, 2.0f, 18.0f, 0.0f);
    m_paddleLeft->addBox(-1.001f, -3.0f, 8.0f, 1.0f, 6.0f, 7.0f, 0.0f);
    m_paddleLeft->setRotationPoint(3.0f, -5.0f, 9.0f);
    m_paddleLeft->setRotateAngleZ(PI / 16.0f);

    m_paddleRight = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("paddleRight");
    m_paddleRight->setTextureSize(128, 64);
    m_paddleRight->setTextureOffset(62, 20);
    m_paddleRight->addBox(-1.0f, 0.0f, -5.0f, 2.0f, 2.0f, 18.0f, 0.0f);
    m_paddleRight->addBox(0.001f, -3.0f, 8.0f, 1.0f, 6.0f, 7.0f, 0.0f);
    m_paddleRight->setRotationPoint(3.0f, -5.0f, -9.0f);
    m_paddleRight->setRotateAngleY(static_cast<f32>(PI_DOUBLE));
    m_paddleRight->setRotateAngleZ(PI / 16.0f);

    // 水面以下不可见的底部
    m_noWater = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatNoWater");
    m_noWater->setTextureSize(128, 64);
    m_noWater->setTextureOffset(0, 0);
    m_noWater->addBox(-14.0f, -9.0f, -3.0f, 28.0f, 16.0f, 3.0f, 0.0f);
    m_noWater->setRotationPoint(0.0f, -3.0f, 1.0f);
    m_noWater->setRotateAngleX(static_cast<f32>(PI_DOUBLE / 2.0));
}

void BoatModel::generateMesh(std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices,
    const std::array<f64, 16>& parentMatrix,
    f64 scale) const
{
    m_bottom->generateMesh(vertices, indices, parentMatrix, scale);
    m_right->generateMesh(vertices, indices, parentMatrix, scale);
    m_left->generateMesh(vertices, indices, parentMatrix, scale);
    m_front->generateMesh(vertices, indices, parentMatrix, scale);
    m_back->generateMesh(vertices, indices, parentMatrix, scale);
    m_paddleLeft->generateMesh(vertices, indices, parentMatrix, scale);
    m_paddleRight->generateMesh(vertices, indices, parentMatrix, scale);
}

void BoatModel::setPaddleAngle(i32 paddleIndex, f32 angle)
{
    if (paddleIndex == 0) {
        m_paddleLeft->setRotateAngleX(angle);
    } else if (paddleIndex == 1) {
        m_paddleRight->setRotateAngleX(angle);
    }
}

// ==================== 船渲染器 ====================

BoatRenderer::BoatRenderer(BoatType type)
    : m_type(type)
    , m_model()
{
    m_shadowSize = 0.8f;
    m_shadowAlpha = 0.8f;
}

void BoatRenderer::render(Entity& entity, f64 partialTicks)
{
    // GPU 管线路径处理渲染（参见 EntityRendererManager::renderWithPipeline），
    // 此方法为遗留 CPU 路径保留，无操作。
    (void)entity;
    (void)partialTicks;
}

ResourceLocation BoatRenderer::getTexture() const
{
    static const ResourceLocation textures[] = {ResourceLocation("minecraft", "textures/entity/boat/oak.png"),
        ResourceLocation("minecraft", "textures/entity/boat/spruce.png"),
        ResourceLocation("minecraft", "textures/entity/boat/birch.png"),
        ResourceLocation("minecraft", "textures/entity/boat/jungle.png"),
        ResourceLocation("minecraft", "textures/entity/boat/acacia.png"),
        ResourceLocation("minecraft", "textures/entity/boat/dark_oak.png"),
        ResourceLocation("minecraft", "textures/entity/boat/mangrove.png"),
        ResourceLocation("minecraft", "textures/entity/boat/cherry.png"),
        ResourceLocation("minecraft", "textures/entity/boat/pale_oak.png"),
        ResourceLocation("minecraft", "textures/entity/boat/bamboo.png")};
    return textures[static_cast<size_t>(m_type)];
}

bool BoatRenderer::generateMesh(
    ::mc::client::ClientEntity& entity, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    // 划桨动画：根据同步状态更新桨角度
    // 注意：needsMeshUpdate 返回 true，每次更新都会重新调用 generateMesh
    // 使用实体 ticksExisted + partialTicks 模拟划桨相位
    const f64 partialTicks = 0.0; // generateMesh 不传 partialTicks，使用 0 占位
    _setupPaddleAnimation(entity, partialTicks);

    // 单位矩阵作为 parentMatrix，模型矩阵由 computeCustomModelMatrix 在管线路径注入
    const auto identity = matrix::identity();
    m_model.generateMesh(vertices, indices, identity, 1.0);

    return !vertices.empty() && !indices.empty();
}

bool BoatRenderer::needsMeshUpdate(::mc::client::ClientEntity& entity) const
{
    // 桨角度随时间变化，每帧更新
    (void)entity;
    return true;
}

bool BoatRenderer::computeCustomModelMatrix(::mc::client::ClientEntity& entity,
    f64 partialTicks,
    std::array<f64, 16>& outMatrix,
    f32& outHurtTime,
    f32& outDeathTime)
{
    outMatrix = _buildBoatModelMatrix(entity, partialTicks);
    // 船不使用着色器红色闪烁（不是 LivingEntity），hurtTime / deathTime 置 0
    outHurtTime = 0.0f;
    outDeathTime = 0.0f;
    return true;
}

std::array<f64, 16> BoatRenderer::_buildBoatModelMatrix(::mc::client::ClientEntity& entity, f64 partialTicks) const
{
    using namespace mc::entity;

    // 读取同步状态
    const auto& dm = entity.dataManager();
    const i32 timeSinceHit =
        dm.hasParam(BoatEntity::getTimeSinceHitParam().id()) ? dm.get<i32>(BoatEntity::getTimeSinceHitParam()) : 0;
    const i32 forwardDir = dm.hasParam(BoatEntity::getForwardDirectionParam().id())
        ? dm.get<i32>(BoatEntity::getForwardDirectionParam())
        : 1;
    const f32 damageTaken =
        dm.hasParam(BoatEntity::getDamageTakenParam().id()) ? dm.get<f32>(BoatEntity::getDamageTakenParam()) : 0.0f;

    // 插值：hurtTime = timeSinceHit - partialTicks, damageTime = max(damage - partialTicks, 0)
    const f64 hurtTime = static_cast<f64>(timeSinceHit) - partialTicks;
    const f64 damageTime = std::max(static_cast<f64>(damageTaken) - partialTicks, 0.0);

    // 朝向
    const f32 yawF32 = entity.getInterpolatedYaw(static_cast<f32>(partialTicks));
    const f64 yaw = static_cast<f64>(yawF32);

    // 变换链（对齐 MC Java AbstractBoatRenderer.submit）：
    //   translate(0, 0.375, 0)
    //   rotateY(180 - yaw)
    //   [hurt shake rotateX]  (if hurtTime > 0)
    //   [bubble tilt around (1, 0, 1)]  (if !isUnderWater && bubbleAngle != 0)
    //   scale(-1, -1, 1)
    //   rotateY(90)

    auto m = matrix::translation(0.0, 0.375, 0.0);
    m = matrix::multiply(m, matrix::rotationY(degToRad(180.0 - yaw)));

    // 受损抖动
    if (hurtTime > 0.0) {
        const f64 shakeDeg = std::sin(hurtTime) * hurtTime * damageTime / 10.0 * static_cast<f64>(forwardDir);
        m = matrix::multiply(m, matrix::rotationX(degToRad(shakeDeg)));
    }

    // 气泡柱倾斜
    // TODO: 同步 m_rockingAngle / m_prevRockingAngle 到 ClientEntity，当前未同步故使用 0
    const f64 bubbleAngleDeg = 0.0;
    const bool isUnderWater = false; // TODO: 同步 isUnderWater 状态
    if (!isUnderWater && std::abs(bubbleAngleDeg) > 1e-6) {
        m = matrix::multiply(m, matrix::rotationAxis(degToRad(bubbleAngleDeg), 1.0, 0.0, 1.0));
    }

    m = matrix::multiply(m, matrix::scale(-1.0, -1.0, 1.0));
    m = matrix::multiply(m, matrix::rotationY(degToRad(90.0)));

    return m;
}

void BoatRenderer::_setupPaddleAnimation(::mc::client::ClientEntity& entity, f64 partialTicks)
{
    using namespace mc::entity;

    const auto& dm = entity.dataManager();
    const bool leftActive =
        dm.hasParam(BoatEntity::getLeftPaddleParam().id()) ? dm.get<bool>(BoatEntity::getLeftPaddleParam()) : false;
    const bool rightActive =
        dm.hasParam(BoatEntity::getRightPaddleParam().id()) ? dm.get<bool>(BoatEntity::getRightPaddleParam()) : false;

    // 对齐 MC Java BoatModel.setupAnim：
    //   paddle.rotateAngleX = (isPaddleActive(side) ? rowingTime : 0) - PI/2
    // 其中 rowingTime 由 AbstractBoat.getRowingTime(side, partialTicks) 计算：
    //   clampedLerp(partialTicks, paddlePositions[side] - PI/8, paddlePositions[side])
    //
    // 当前实体的 paddlePositions 不在 DataParameter 同步范围内，
    // 使用基于 ticksExisted 的恒定速度动画近似：
    //   paddlePositions[side] = (ticksExisted + partialTicks) * 0.79  （MC 中每 tick 约 0.79 弧度）
    // TODO: 后续同步 paddlePositions 数组到 ClientEntity，使用真实插值
    const f64 time = static_cast<f64>(entity.ticksExisted()) + partialTicks;
    const f64 paddlePhase = time * 0.79;

    const f32 leftAngle = leftActive
        ? static_cast<f32>(mc::math::clampedLerp(partialTicks, paddlePhase - PI / 8.0, paddlePhase) - PI / 2.0)
        : static_cast<f32>(-PI / 2.0);
    const f32 rightAngle = rightActive
        ? static_cast<f32>(mc::math::clampedLerp(partialTicks, paddlePhase - PI / 8.0, paddlePhase) - PI / 2.0)
        : static_cast<f32>(-PI / 2.0);

    m_model.setPaddleAngle(0, leftAngle);
    m_model.setPaddleAngle(1, rightAngle);
}

// ==================== 矿车模型 ====================

MinecartModel::MinecartModel()
{
    _setupParts();
}

void MinecartModel::_setupParts()
{
    // 纹理尺寸：64x32
    // 6个面：底部、左侧、右侧、前面、后面、内部底

    // sideModels[0] - 底部，旋转 PI_DOUBLE/2
    m_sides[0] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartBottom");
    m_sides[0]->setTextureSize(64, 32);
    m_sides[0]->setTextureOffset(0, 10);
    m_sides[0]->addBox(-10.0f, -8.0f, -1.0f, 20.0f, 16.0f, 2.0f, 0.0f);
    m_sides[0]->setRotationPoint(0.0f, 4.0f, 0.0f);
    m_sides[0]->setRotateAngleX(static_cast<f32>(PI_DOUBLE / 2.0));

    // sideModels[1] - 左侧，旋转 PI_DOUBLE*1.5
    m_sides[1] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartLeft");
    m_sides[1]->setTextureSize(64, 32);
    m_sides[1]->setTextureOffset(0, 0);
    m_sides[1]->addBox(-8.0f, -9.0f, -1.0f, 16.0f, 8.0f, 2.0f, 0.0f);
    m_sides[1]->setRotationPoint(-9.0f, 4.0f, 0.0f);
    m_sides[1]->setRotateAngleY(static_cast<f32>(PI_DOUBLE * 1.5));

    // sideModels[2] - 右侧，旋转 PI_DOUBLE/2
    m_sides[2] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartRight");
    m_sides[2]->setTextureSize(64, 32);
    m_sides[2]->setTextureOffset(0, 0);
    m_sides[2]->addBox(-8.0f, -9.0f, -1.0f, 16.0f, 8.0f, 2.0f, 0.0f);
    m_sides[2]->setRotationPoint(9.0f, 4.0f, 0.0f);
    m_sides[2]->setRotateAngleY(static_cast<f32>(PI_DOUBLE / 2.0));

    // sideModels[3] - 后面，旋转 PI_DOUBLE
    m_sides[3] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartBack");
    m_sides[3]->setTextureSize(64, 32);
    m_sides[3]->setTextureOffset(0, 0);
    m_sides[3]->addBox(-8.0f, -9.0f, -1.0f, 16.0f, 8.0f, 2.0f, 0.0f);
    m_sides[3]->setRotationPoint(0.0f, 4.0f, -7.0f);
    m_sides[3]->setRotateAngleY(static_cast<f32>(PI_DOUBLE));

    // sideModels[4] - 前面
    m_sides[4] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartFront");
    m_sides[4]->setTextureSize(64, 32);
    m_sides[4]->setTextureOffset(0, 0);
    m_sides[4]->addBox(-8.0f, -9.0f, -1.0f, 16.0f, 8.0f, 2.0f, 0.0f);
    m_sides[4]->setRotationPoint(0.0f, 4.0f, 7.0f);

    // sideModels[5] - 内部底，旋转 -PI_DOUBLE/2
    m_sides[5] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartInside");
    m_sides[5]->setTextureSize(64, 32);
    m_sides[5]->setTextureOffset(44, 10);
    m_sides[5]->addBox(-9.0f, -7.0f, -1.0f, 18.0f, 14.0f, 1.0f, 0.0f);
    m_sides[5]->setRotationPoint(0.0f, 4.0f, 0.0f);
    m_sides[5]->setRotateAngleX(static_cast<f32>(-PI_DOUBLE / 2.0));
}

void MinecartModel::generateMesh(std::vector<model::ModelVertex>& vertices,
    std::vector<u32>& indices,
    const std::array<f64, 16>& parentMatrix,
    f64 scale) const
{
    for (const auto& side : m_sides) {
        if (side) {
            side->generateMesh(vertices, indices, parentMatrix, scale);
        }
    }
}

void MinecartModel::setInsideOffset(f32 yOffset)
{
    if (m_sides[5]) {
        m_sides[5]->setRotationPointY(4.0f - yOffset);
    }
}

// ==================== 矿车渲染器 ====================

MinecartRenderer::MinecartRenderer()
    : m_model()
{
    m_shadowSize = 0.5f;
    m_shadowAlpha = 0.8f;
}

void MinecartRenderer::render(Entity& entity, f64 partialTicks)
{
    // GPU 管线路径处理渲染，此方法为遗留 CPU 路径保留，无操作。
    (void)entity;
    (void)partialTicks;
}

ResourceLocation MinecartRenderer::getMinecartTexture()
{
    return ResourceLocation("minecraft", "textures/entity/minecart.png");
}

bool MinecartRenderer::generateMesh(
    ::mc::client::ClientEntity& entity, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    // 内部底板偏移：TODO 当乘客乘坐时调整（MC 中通过 setInsideOffset 实现）
    m_model.setInsideOffset(0.0f);

    const auto identity = matrix::identity();
    m_model.generateMesh(vertices, indices, identity, 1.0);

    return !vertices.empty() && !indices.empty();
}

bool MinecartRenderer::needsMeshUpdate(::mc::client::ClientEntity& entity) const
{
    // TNT 矿车闪烁需要每帧更新（fuse 递减）
    // 普通矿车静态网格，但为简化实现统一返回 true（开销可接受）
    (void)entity;
    return true;
}

bool MinecartRenderer::computeCustomModelMatrix(::mc::client::ClientEntity& entity,
    f64 partialTicks,
    std::array<f64, 16>& outMatrix,
    f32& outHurtTime,
    f32& outDeathTime)
{
    outMatrix = _buildMinecartModelMatrix(entity, partialTicks, outHurtTime);
    outDeathTime = 0.0f; // 矿车无死亡淡出
    return true;
}

std::array<f64, 16> MinecartRenderer::_buildMinecartModelMatrix(
    ::mc::client::ClientEntity& entity, f64 partialTicks, f32& outHurtTime) const
{
    using namespace mc::entity;

    // 读取同步状态
    // TODO: 矿车摇晃动画（rollingAmplitude/rollingDirection）在 vanilla 1.21.11 走 EntityEvent
    //       广播而非 SynchedEntityData;项目旧实现用 wire 字段同步摇晃,对齐 vanilla 时已删该 wire 字段。
    //       此处暂用本地默认值 0/1,客户端暂时看不到矿车受损摇晃动画(功能回退),待 EntityEvent
    //       体系接入矿车摇晃状态码后恢复。damage 仍走 wire DATA_DAMAGE_PARAM 同步。
    const auto& dm = entity.dataManager();
    const i32 rollingAmplitude = 0;
    const i32 rollingDir = 1;
    const f32 damage = dm.hasParam(AbstractMinecartEntity::getDamageParam().id())
        ? dm.get<f32>(AbstractMinecartEntity::getDamageParam())
        : 0.0f;

    // 插值：hurtTime = rollingAmplitude - partialTicks, damageTime = max(damage - partialTicks, 0)
    const f64 hurtTime = static_cast<f64>(rollingAmplitude) - partialTicks;
    const f64 damageTime = std::max(static_cast<f64>(damage) - partialTicks, 0.0);

    // 朝向
    const f32 yawF32 = entity.getInterpolatedYaw(static_cast<f32>(partialTicks));
    const f32 pitchF32 = entity.getInterpolatedPitch(static_cast<f32>(partialTicks));
    const f64 yaw = static_cast<f64>(yawF32);
    const f64 pitch = static_cast<f64>(pitchF32);

    // 变换链（对齐 MC Java AbstractMinecartRenderer.oldRender + submit）：
    //   translate(0, 0.375, 0)
    //   rotateY(180 - yaw)
    //   rotateZ(-pitch)
    //   [hurt shake rotateX]  (if hurtTime > 0)
    //   scale(-1, -1, 1)
    //   [TNT flash scale]  (if TNT minecart && fuse in (-1, 10))

    auto m = matrix::translation(0.0, 0.375, 0.0);
    m = matrix::multiply(m, matrix::rotationY(degToRad(180.0 - yaw)));
    m = matrix::multiply(m, matrix::rotationZ(degToRad(-pitch)));

    // 受损抖动
    if (hurtTime > 0.0) {
        const f64 shakeDeg = std::sin(hurtTime) * hurtTime * damageTime / 10.0 * static_cast<f64>(rollingDir);
        m = matrix::multiply(m, matrix::rotationX(degToRad(shakeDeg)));
    }

    m = matrix::multiply(m, matrix::scale(-1.0, -1.0, 1.0));

    // TNT 矿车闪烁缩放
    if (isTntMinecart(entity)) {
        const i32 fuse = entity.fuseTimer();
        const f64 flashScale = calculateTntFlashScale(fuse);
        if (flashScale != 1.0) {
            m = matrix::multiply(m, matrix::scale(flashScale, flashScale, flashScale));
        }
        // 白色闪烁通过 overlayColor 传递（对齐 MC TntMinecartRenderer 的 OverlayTexture.pack(10)）
        // outHurtTime 在管线路径会传给着色器作为红色闪烁因子，这里复用实现白色叠加
        if (isTntFlashFrame(fuse)) {
            // 使用 overlayColor 而非 hurtTime，但当前 drawMesh 不接受自定义 overlayColor
            // TODO: 后续扩展 EntityRenderer 接口允许渲染器自定义 overlayColor
            // 当前先通过 hurtTime 通道近似（红色闪烁），视觉上可识别 TNT 即将爆炸
            outHurtTime = 1.0f;
        } else {
            outHurtTime = 0.0f;
        }
    } else {
        outHurtTime = 0.0f;
    }

    return m;
}

f64 MinecartRenderer::calculateTntFlashScale(i32 fuse) noexcept
{
    // 对齐 MC TntMinecartRenderer.submitMinecartContents：
    //   if (fuse > -1 && fuse < 10) {
    //       f = 1 - fuse/10;
    //       f = clamp(f, 0, 1);
    //       f *= f; f *= f;  // 4 次方
    //       return 1 + f * 0.3;
    //   }
    //   return 1;
    if (fuse <= -1 || fuse >= 10) {
        return 1.0;
    }
    f64 f = 1.0 - static_cast<f64>(fuse) / 10.0;
    f = mc::math::clamp(f, 0.0, 1.0);
    f *= f;
    f *= f;
    return 1.0 + f * 0.3;
}

bool MinecartRenderer::isTntFlashFrame(i32 fuse) noexcept
{
    // 对齐 MC TntMinecartRenderer：
    //   fuse > -1 && (int)fuse / 5 % 2 == 0
    if (fuse <= -1) {
        return false;
    }
    return (fuse / 5) % 2 == 0;
}

} // namespace mc::client::renderer::entity::renderer::vehicle
