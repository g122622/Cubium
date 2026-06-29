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

#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/resource/BlockModelLoader.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include <array>
#include <utility>
#include <vector>
#include <glm/glm.hpp>

namespace mc {
class ItemStack;
class Item;

namespace client {
class ItemTextureAtlas;
}
} // namespace mc

namespace mc::client::renderer::api {
struct TextureRegion;
}

namespace mc::client::resource {
struct BakedItemModel;
}

namespace mc::client::renderer::entity::item {

/**
 * @brief 物品变换类型
 *
 * 定义物品在不同渲染场景下的变换方式。
 */
enum class ItemTransformType : u8 {
    None,                 // 无变换
    FirstPersonLeftHand,  // 第一人称左手
    FirstPersonRightHand, // 第一人称右手
    ThirdPersonLeftHand,  // 第三人称左手
    ThirdPersonRightHand, // 第三人称右手
    Head,                 // 头部（戴在头上）
    Gui,                  // GUI 界面
    Ground,               // 地面（掉落物）
    Fixed                 // 固定（框架等）
};

/**
 * @brief 物品网格构建器
 *
 * 从物品模型生成 3D 网格，用于层渲染器和世界空间渲染。
 * 支持：
 * - 手持物品渲染（第三人称）
 * - 头部物品渲染（头盔、南瓜等）
 * - 盔甲渲染
 * - 掉落物渲染
 */
class ItemMeshBuilder {
public:
    /**
     * @brief 设置物品纹理图集
     *
     * 必须在首次使用网格构建方法之前调用，用于解析物品纹理坐标。
     *
     * @param atlas 物品纹理图集指针
     */
    static void setItemTextureAtlas(const ::mc::client::ItemTextureAtlas* atlas);

    /**
     * @brief 构建手持物品网格
     *
     * @param itemStack 物品堆
     * @param transformType 变换类型
     * @return 顶点和索引对
     */
    static std::pair<std::vector<model::ModelVertex>, std::vector<u32>> buildHeldItemMesh(
        const ::mc::ItemStack& itemStack, ItemTransformType transformType);

    /**
     * @brief 构建盔甲网格
     *
     * @param itemStack 盔甲物品堆
     * @param slot 装备槽位
     * @param bodyPartTransform 身体部件变换矩阵
     * @return 顶点和索引对
     */
    static std::pair<std::vector<model::ModelVertex>, std::vector<u32>> buildArmorMesh(
        const ::mc::ItemStack& itemStack, u32 slot, const std::array<f64, 16>& bodyPartTransform);

    /**
     * @brief 构建头部物品网格
     *
     * 用于头盔、南瓜等戴在头上的物品。
     *
     * @param itemStack 物品堆
     * @return 顶点和索引对
     */
    static std::pair<std::vector<model::ModelVertex>, std::vector<u32>> buildHeadMesh(const ::mc::ItemStack& itemStack);

    /**
     * @brief 构建地面物品网格
     *
     * 用于掉落物渲染。
     *
     * @param itemStack 物品堆
     * @param rotation 旋转角度（度）
     * @return 顶点和索引对
     */
    static std::pair<std::vector<model::ModelVertex>, std::vector<u32>> buildGroundItemMesh(
        const ::mc::ItemStack& itemStack, f64 rotation);

    /**
     * @brief 构建物品图标的简单四边形网格
     *
     * 从纹理区域创建一个面向相机的四边形。
     *
     * @param region 纹理区域
     * @param size 网格大小（世界单位）
     * @return 顶点和索引对
     */
    static std::pair<std::vector<model::ModelVertex>, std::vector<u32>> buildIconMesh(
        const ::mc::client::renderer::api::TextureRegion& region, f64 size);

    /**
     * @brief 获取物品变换矩阵
     *
     * 根据变换类型返回物品的模型矩阵。
     *
     * @param transformType 变换类型
     * @param limbSwing 步态动画周期
     * @param swingProgress 挥动进度 (0.0 - 1.0)
     * @param isRightHand 是否为右手
     * @return 变换矩阵
     */
    static std::array<f64, 16> getItemTransform(
        ItemTransformType transformType, f32 limbSwing, f32 swingProgress, bool isRightHand);

private:
    /**
     * @brief 构建简单的物品图标四边形
     */
    static void _buildItemQuad(const ::mc::client::renderer::api::TextureRegion& region,
        f64 size,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices);

    /**
     * @brief 构建 3D 物品模型网格
     */
    static void _build3DItemMesh(const ::mc::Item& item,
        ItemTransformType transformType,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices);

    /**
     * @brief 构建平面图标网格（Generated/Handheld 类型）
     */
    static void _buildGeneratedMesh(const ::mc::client::resource::BakedItemModel& model,
        const ::mc::Item& item,
        ItemTransformType transformType,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices);

    /**
     * @brief 构建方块物品网格
     */
    static void _buildBlockItemMesh(const ::mc::client::resource::BakedItemModel& model,
        const ::mc::Item& item,
        ItemTransformType transformType,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices);

    /**
     * @brief 构建自定义 3D 模型网格
     */
    static void _buildCustomMesh(const ::mc::client::resource::BakedItemModel& model,
        const ::mc::Item& item,
        ItemTransformType transformType,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices);

    /**
     * @brief 构建回退网格（简单立方体）
     */
    static void _buildFallbackMesh(
        const ::mc::Item& item, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 应用矩阵到顶点
     */
    static void _applyMatrixToVertices(std::vector<model::ModelVertex>& vertices, const glm::mat4& matrix);

    /**
     * @brief 构建元素旋转矩阵（含rescale缩放）
     *
     * 参考 MC FaceBakery.bakeQuad() 的元素旋转逻辑：
     * 1. 构建绕指定轴的旋转矩阵
     * 2. 若 rescale 为 true，计算各轴缩放因子补偿旋转投影收缩
     * 3. 组合为 origin^-1 * R(可选S) * origin 的变换矩阵
     *
     * @param rotation 模型元素旋转参数
     * @param scale 顶点缩放因子（用于将像素坐标转为世界坐标）
     * @return 旋转+平移组合矩阵，若无旋转则返回单位矩阵
     */
    static glm::mat4 _buildElementRotationMatrix(const ::mc::ModelRotation& rotation, f64 scale);

    /**
     * @brief 计算旋转后各轴的rescale缩放因子
     *
     * 对于每个坐标轴，取该轴单位向量经过旋转矩阵变换后的最大绝对分量，
     * 其倒数即为缩放因子。这补偿了旋转导致的轴向投影收缩。
     *
     * @param rotMatrix 旋转矩阵（3x3部分）
     * @return 各轴缩放因子 (sx, sy, sz)
     */
    static glm::vec3 _computeRescaleFactors(const glm::mat3& rotMatrix);

    /**
     * @brief 获取面UV旋转后的顶点UV坐标
     *
     * UV旋转通过顶点索引排列实现，参考 MC Quadrant.rotateVertexIndex()。
     * rotation 为 0/90/180/270 度，对应 shift 为 0/1/2/3。
     * 每个顶点 i 使用无旋转时顶点 (i + shift) % 4 的 UV 坐标。
     *
     * @param vertexIndex 顶点索引 (0-3)
     * @param uvRotation UV旋转角度 (0/90/180/270)
     * @param u0, v0, u1, v1 UV坐标范围
     * @return 该顶点的 (u, v) 坐标
     */
    static std::pair<f32, f32> _getRotatedUV(int vertexIndex, i32 uvRotation, f32 u0, f32 v0, f32 u1, f32 v1);

    /**
     * @brief 应用手持物品变换
     */
    static void _applyHeldItemTransform(std::vector<model::ModelVertex>& vertices,
        ItemTransformType transformType,
        f32 limbSwing,
        f32 swingProgress,
        bool isRightHand);

    /**
     * @brief 变换顶点
     */
    static void _transformVertices(std::vector<model::ModelVertex>& vertices, const std::array<f64, 16>& matrix);

    // 物品渲染常量
    static constexpr f64 ITEM_SCALE = 1.0 / 16.0;     // 物品缩放因子
    static constexpr f64 ITEM_GUI_SCALE = 1.0 / 32.0; // GUI 物品缩放
    static constexpr f64 ARM_SWING_ANGLE = 45.0;      // 手臂挥动角度
    static constexpr f64 ITEM_ROTATION_SPEED = 2.0;   // 物品旋转速度（度/tick）

    // 物品纹理图集（由 setItemTextureAtlas 设置）
    static const ::mc::client::ItemTextureAtlas* s_itemTextureAtlas;
};

} // namespace mc::client::renderer::entity::item
