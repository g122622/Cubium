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

#pragma once

#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

// Forward declarations
namespace mc::entity {
class BoatEntity;
class AbstractMinecartEntity;
} // namespace mc::entity

namespace mc::client::renderer::entity::renderer::vehicle {

// ============================================================================
// 行主序 4x4 矩阵工具
// ============================================================================
//
// 项目中 ModelRenderer / EntityPipeline 均使用行主序 std::array<f64, 16>：
//   索引布局：[row*4 + col]
//   矩阵-向量乘法：result.x = m[0]*v.x + m[1]*v.y + m[2]*v.z + m[3]
// 本工具提供构造与组合原语，供载具渲染器构建自定义模型矩阵使用。
//

namespace matrix {

/// 单位矩阵
[[nodiscard]] inline std::array<f64, 16> identity() noexcept
{
    return {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
}

/// 平移矩阵
[[nodiscard]] inline std::array<f64, 16> translation(f64 x, f64 y, f64 z) noexcept
{
    return {1.0, 0.0, 0.0, x, 0.0, 1.0, 0.0, y, 0.0, 0.0, 1.0, z, 0.0, 0.0, 0.0, 1.0};
}

/// 缩放矩阵
[[nodiscard]] inline std::array<f64, 16> scale(f64 x, f64 y, f64 z) noexcept
{
    return {x, 0.0, 0.0, 0.0, 0.0, y, 0.0, 0.0, 0.0, 0.0, z, 0.0, 0.0, 0.0, 0.0, 1.0};
}

/// 绕 X 轴旋转矩阵（弧度）
[[nodiscard]] inline std::array<f64, 16> rotationX(f64 radians) noexcept
{
    const f64 c = std::cos(radians);
    const f64 s = std::sin(radians);
    return {1.0, 0.0, 0.0, 0.0, 0.0, c, -s, 0.0, 0.0, s, c, 0.0, 0.0, 0.0, 0.0, 1.0};
}

/// 绕 Y 轴旋转矩阵（弧度）
[[nodiscard]] inline std::array<f64, 16> rotationY(f64 radians) noexcept
{
    const f64 c = std::cos(radians);
    const f64 s = std::sin(radians);
    return {c, 0.0, s, 0.0, 0.0, 1.0, 0.0, 0.0, -s, 0.0, c, 0.0, 0.0, 0.0, 0.0, 1.0};
}

/// 绕 Z 轴旋转矩阵（弧度）
[[nodiscard]] inline std::array<f64, 16> rotationZ(f64 radians) noexcept
{
    const f64 c = std::cos(radians);
    const f64 s = std::sin(radians);
    return {c, -s, 0.0, 0.0, s, c, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
}

/// 绕任意轴旋转矩阵（弧度），轴不需要预先归一化
[[nodiscard]] inline std::array<f64, 16> rotationAxis(f64 radians, f64 ax, f64 ay, f64 az) noexcept
{
    // 归一化轴向量
    const f64 len2 = ax * ax + ay * ay + az * az;
    if (len2 < 1e-12) {
        return identity();
    }
    const f64 invLen = 1.0 / std::sqrt(len2);
    const f64 x = ax * invLen;
    const f64 y = ay * invLen;
    const f64 z = az * invLen;

    const f64 c = std::cos(radians);
    const f64 s = std::sin(radians);
    const f64 t = 1.0 - c;

    // Rodrigues 公式（行主序）
    return {(t * x * x + c),
        (t * x * y - s * z),
        (t * x * z + s * y),
        0.0,
        (t * x * y + s * z),
        (t * y * y + c),
        (t * y * z - s * x),
        0.0,
        (t * x * z - s * y),
        (t * y * z + s * x),
        (t * z * z + c),
        0.0,
        0.0,
        0.0,
        0.0,
        1.0};
}

/// 矩阵乘法 a * b（行主序）
[[nodiscard]] inline std::array<f64, 16> multiply(const std::array<f64, 16>& a, const std::array<f64, 16>& b) noexcept
{
    std::array<f64, 16> result{};
    for (i32 row = 0; row < 4; ++row) {
        for (i32 col = 0; col < 4; ++col) {
            const auto idx = static_cast<std::size_t>(row * 4 + col);
            result[idx] = 0.0;
            for (i32 k = 0; k < 4; ++k) {
                result[idx] += a[static_cast<std::size_t>(row * 4 + k)] * b[static_cast<std::size_t>(k * 4 + col)];
            }
        }
    }
    return result;
}

/// 便利：连续左乘，返回 result = translation * rotationY * rotationX（常用变换链）
[[nodiscard]] inline std::array<f64, 16> chain(const std::array<f64, 16>& a, const std::array<f64, 16>& b) noexcept
{
    return multiply(a, b);
}

} // namespace matrix

// ============================================================================
// 船类型枚举
// ============================================================================

/**
 * @brief 船类型枚举
 */
enum class BoatType : u8 {
    Oak = 0,
    Spruce = 1,
    Birch = 2,
    Jungle = 3,
    Acacia = 4,
    DarkOak = 5,
    Mangrove = 6,
    Cherry = 7,
    PaleOak = 8,
    Bamboo = 9
};

// ============================================================================
// 船模型
// ============================================================================

/**
 * @brief 船模型
 */
class BoatModel {
public:
    BoatModel();
    ~BoatModel() = default;

    /**
     * @brief 生成渲染网格
     * @param vertices 顶点输出缓冲区
     * @param indices 索引输出缓冲区
     * @param parentMatrix 父变换矩阵（4x4，行主序）
     * @param scale 缩放因子（载具渲染器使用 1.0，由 drawMesh 的 MODEL_SCALE 统一缩放）
     */
    void generateMesh(std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices,
        const std::array<f64, 16>& parentMatrix,
        f64 scale) const;

    /**
     * @brief 设置桨的角度
     * @param paddleIndex 0=左桨, 1=右桨
     * @param angle X轴旋转角度（弧度）
     */
    void setPaddleAngle(i32 paddleIndex, f32 angle);

private:
    void _setupParts();

    i32 m_textureWidth = 128;
    i32 m_textureHeight = 64;

    // 船体部件
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_bottom;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_back;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_front;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_left;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_right;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_paddleLeft;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_paddleRight;
    std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer> m_noWater;
};

// ============================================================================
// 船渲染器
// ============================================================================

/**
 * @brief 船渲染器
 *
 * 同时实现 core::EntityRenderer 与 core::PipelineMeshProvider，
 * 通过 GPU 管线路径（EntityRendererManager::renderWithPipeline）渲染：
 * - generateMesh 输出"像素空间"几何体（scale=1.0）
 * - computeCustomModelMatrix 提供按 MC AbstractBoatRenderer 的完整变换链：
 *     translate(0, 0.375, 0)
 *     rotateY(180 - yaw)
 *     [hurt shake rotateX]
 *     [bubble tilt around (1, 0, 1)]
 *     scale(-1, -1, 1)
 *     rotateY(90)
 *   划桨动画通过设置 m_paddleLeft/Right 的 setRotateAngleX 实现。
 */
class BoatRenderer : public core::EntityRenderer, public core::PipelineMeshProvider {
public:
    explicit BoatRenderer(BoatType type);
    ~BoatRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    [[nodiscard]] ResourceLocation getTexture() const;

    // ========== PipelineMeshProvider 接口 ==========

    [[nodiscard]] core::PipelineMeshProvider* getPipelineMeshProvider() override { return this; }

    [[nodiscard]] bool generateMesh(::mc::client::ClientEntity& entity,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices) override;

    [[nodiscard]] bool needsMeshUpdate(::mc::client::ClientEntity& entity) const override;

    // ========== 自定义模型矩阵 ==========

    [[nodiscard]] bool computeCustomModelMatrix(::mc::client::ClientEntity& entity,
        f64 partialTicks,
        std::array<f64, 16>& outMatrix,
        f32& outHurtTime,
        f32& outDeathTime) override;

private:
    BoatType m_type;
    BoatModel m_model;

    /**
     * @brief 从客户端实体读取船的同步状态并构建模型矩阵
     * @param entity 客户端实体
     * @param partialTicks 部分 tick
     * @return 4x4 行主序模型矩阵
     *
     * 状态来源（通过 ClientEntity::dataManager() 读取 BoatEntity 的 DataParameter）：
     * - DATA_TIME_SINCE_HIT_PARAM：受击时间（插值 hurtTime = getTimeSinceHit - partialTicks）
     * - DATA_FORWARD_DIRECTION_PARAM：受击方向（±1）
     * - DATA_DAMAGE_TAKEN_PARAM：累积伤害（插值 damageTime = max(damage - partialTicks, 0)）
     * - 气泡角度：实体未直接同步，使用 0（TODO: 后续同步 m_rockingAngle）
     * - isUnderWater：通过 typeId + 状态判断（暂用 false）
     *
     * 受损抖动公式（对齐 MC Java AbstractBoatRenderer）：
     *   shake = sin(hurtTime) * hurtTime * damageTime / 10 * hurtDir （度数）
     *   rotateX(shake * DEG_TO_RAD)
     */
    [[nodiscard]] std::array<f64, 16> _buildBoatModelMatrix(::mc::client::ClientEntity& entity, f64 partialTicks) const;

    /**
     * @brief 根据划桨状态设置桨角度
     *
     * 对齐 MC Java BoatModel.setupAnim(swing)：
     *   paddle.rotateAngleX = (isPaddleActive(side) ? rowingTime : 0) - PI/2
     * 其中 rowingTime = clampedLerp(partialTicks, paddlePositions[side] - PI/8, paddlePositions[side])
     *
     * 当前实体的 paddlePositions 不在 DataParameter 同步范围内，
     * 使用简化的恒定速度动画（TODO: 后续同步 paddlePositions 数组）。
     */
    void _setupPaddleAnimation(::mc::client::ClientEntity& entity, f64 partialTicks);
};

// ============================================================================
// 矿车模型
// ============================================================================

/**
 * @brief 矿车模型
 */
class MinecartModel {
public:
    MinecartModel();
    ~MinecartModel() = default;

    /**
     * @brief 生成渲染网格
     */
    void generateMesh(std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices,
        const std::array<f64, 16>& parentMatrix,
        f64 scale) const;

    /**
     * @brief 设置内部底板Y偏移
     * 用于乘客乘坐时的动画
     */
    void setInsideOffset(f32 yOffset);

private:
    void _setupParts();

    // 6个面：底部、左、右、后、前、内部底
    std::array<std::shared_ptr<::mc::client::renderer::entity::model::ModelRenderer>, 6> m_sides;
};

// ============================================================================
// 矿车渲染器
// ============================================================================

/**
 * @brief 矿车渲染器
 *
 * 同时实现 core::EntityRenderer 与 core::PipelineMeshProvider，
 * 通过 GPU 管线路径渲染。变换链对齐 MC AbstractMinecartRenderer.oldRender：
 *   translate(0, 0.375, 0)
 *   rotateY(180 - yaw)
 *   rotateZ(-pitch)
 *   [hurt shake rotateX]
 *   scale(-1, -1, 1)
 *
 * TNT 矿车闪烁效果：当 fuse > 0 且 (fuse/5)%2 == 0 时，通过 overlayColor
 * 传给着色器实现白色闪烁（对齐 MC TntMinecartRenderer 的 OverlayTexture.pack(10)）。
 * 闪烁缩放因子 = 1 + (1 - fuse/10)^4 * 0.3 也通过模型矩阵的 scale 应用。
 */
class MinecartRenderer : public core::EntityRenderer, public core::PipelineMeshProvider {
public:
    MinecartRenderer();
    ~MinecartRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    [[nodiscard]] static ResourceLocation getMinecartTexture();

    // ========== PipelineMeshProvider 接口 ==========

    [[nodiscard]] core::PipelineMeshProvider* getPipelineMeshProvider() override { return this; }

    [[nodiscard]] bool generateMesh(::mc::client::ClientEntity& entity,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices) override;

    [[nodiscard]] bool needsMeshUpdate(::mc::client::ClientEntity& entity) const override;

    // ========== 自定义模型矩阵 ==========

    [[nodiscard]] bool computeCustomModelMatrix(::mc::client::ClientEntity& entity,
        f64 partialTicks,
        std::array<f64, 16>& outMatrix,
        f32& outHurtTime,
        f32& outDeathTime) override;

    // ========== TNT 闪烁计算（公开供单元测试） ==========

    /**
     * @brief 计算 TNT 闪烁缩放因子
     * @param fuse 引信剩余 tick（-1 表示未点燃）
     * @return 缩放因子（1.0 表示不缩放）
     *
     * 对齐 MC TntMinecartRenderer：
     *   if (fuse > -1 && fuse < 10) {
     *       f = 1 - fuse/10;
     *       f = clamp(f, 0, 1);
     *       f *= f; f *= f;  // 4 次方
     *       return 1 + f * 0.3;
     *   }
     *   return 1;
     */
    [[nodiscard]] static f64 calculateTntFlashScale(i32 fuse) noexcept;

    /**
     * @brief 判断 TNT 是否处于白色闪烁帧
     * @param fuse 引信剩余 tick
     * @return true 表示本帧应渲染白色闪烁叠加
     *
     * 对齐 MC TntMinecartRenderer：
     *   fuse > -1 && (int)fuse / 5 % 2 == 0
     */
    [[nodiscard]] static bool isTntFlashFrame(i32 fuse) noexcept;

private:
    MinecartModel m_model;

    /**
     * @brief 从客户端实体读取矿车的同步状态并构建模型矩阵
     *
     * 状态来源（通过 ClientEntity::dataManager() 读取 AbstractMinecartEntity 的 DataParameter）：
     * - DATA_ROLLING_AMPLITUDE_PARAM：摇晃幅度（插值 hurtTime = rollingAmplitude - partialTicks）
     * - DATA_ROLLING_DIRECTION_PARAM：摇晃方向（±1）
     * - DATA_DAMAGE_PARAM：累积伤害（插值 damageTime = max(damage - partialTicks, 0)）
     * - 实体 yaw/pitch：通过 ClientEntity::getInterpolatedYaw / pitch
     * - TNT 矿车 fuse：通过 ClientEntity::fuseTimer()
     *
     * 受损抖动公式（对齐 MC Java AbstractMinecartRenderer）：
     *   shake = sin(hurtTime) * hurtTime * damageTime / 10 * hurtDir （度数）
     *   rotateX(shake * DEG_TO_RAD)
     */
    [[nodiscard]] std::array<f64, 16> _buildMinecartModelMatrix(
        ::mc::client::ClientEntity& entity, f64 partialTicks, f32& outHurtTime) const;
};

} // namespace mc::client::renderer::entity::renderer::vehicle
