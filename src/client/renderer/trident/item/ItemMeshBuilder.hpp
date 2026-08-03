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

#include "ElementRotation.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/resource/BlockModelLoader.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include <array>
#include <utility>
#include <vector>
#include <glm/ext/matrix_float4x4.hpp>
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
     * @param bakeTransforms 是否将 display 变换与 getItemTransform 摄像机矩阵烘焙进顶点。
     *        true：与第三人称 HeldItemLayer 一致，变换烘焙进顶点，调用方仅传手臂相对矩阵。
     *        false：返回原始模型几何，由调用方在矩阵栈上单独施加 display 变换
     *        （第一人称路径由 ItemInHandRenderer::applyTransform 拥有 display 变换，避免双重施加）。
     * @return 顶点和索引对
     */
    static std::pair<std::vector<model::ModelVertex>, std::vector<u32>> buildHeldItemMesh(
        const ::mc::ItemStack& itemStack, ItemTransformType transformType, bool bakeTransforms);

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
     *
     * @param bakeDisplayTransform 是否将模型 display 变换烘焙进顶点。false 时返回原始几何，
     *        由调用方在矩阵栈上单独施加 display 变换。
     */
    static void _build3DItemMesh(const ::mc::Item& item,
        ItemTransformType transformType,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices,
        bool bakeDisplayTransform);

    /**
     * @brief 构建平面图标网格（Generated/Handheld 类型）
     */
    static void _buildGeneratedMesh(const ::mc::client::resource::BakedItemModel& model,
        const ::mc::Item& item,
        ItemTransformType transformType,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices,
        bool bakeDisplayTransform);

    /**
     * @brief 构建方块物品网格
     */
    static void _buildBlockItemMesh(const ::mc::client::resource::BakedItemModel& model,
        const ::mc::Item& item,
        ItemTransformType transformType,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices,
        bool bakeDisplayTransform);

    /**
     * @brief 构建自定义 3D 模型网格
     */
    static void _buildCustomMesh(const ::mc::client::resource::BakedItemModel& model,
        const ::mc::Item& item,
        ItemTransformType transformType,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices,
        bool bakeDisplayTransform);

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
