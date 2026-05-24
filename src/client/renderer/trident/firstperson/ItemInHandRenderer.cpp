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

#include "ItemInHandRenderer.hpp"
#include "../../../resource/ItemModelCache.hpp"
#include "../../../resource/ItemModelLoader.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::trident::firstperson {

using namespace mc::math;

// ============================================================================
// 构造函数和析构函数
// ============================================================================

ItemInHandRenderer::ItemInHandRenderer()
    : m_transforms()
{}

ItemInHandRenderer::~ItemInHandRenderer()
{
    destroy();
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> ItemInHandRenderer::initialize()
{
    // 设置默认变换
    // 第三人称右手
    m_transforms.thirdPersonRight = ItemTransform(0.0f,
        0.0f,
        0.0f, // 旋转
        0.0f,
        3.0f,
        1.0f, // 平移
        0.55f,
        0.55f,
        0.55f // 缩放
    );

    // 第三人称左手（镜像右手）
    m_transforms.thirdPersonLeft = ItemTransform(0.0f, 0.0f, 0.0f, 0.0f, 3.0f, 1.0f, 0.55f, 0.55f, 0.55f);

    // 第一人称右手
    m_transforms.firstPersonRight = ItemTransform(0.0f,
        45.0f,
        0.0f, // Y 轴旋转 45 度
        0.0f,
        2.5f,
        0.0f, // Y 平移
        0.4f,
        0.4f,
        0.4f // 缩放
    );

    // 第一人称左手（镜像右手）
    m_transforms.firstPersonLeft = ItemTransform(0.0f,
        -45.0f,
        0.0f, // Y 轴旋转 -45 度
        0.0f,
        2.5f,
        0.0f,
        0.4f,
        0.4f,
        0.4f);

    // GUI 显示
    m_transforms.gui = ItemTransform(30.0f,
        225.0f,
        0.0f, // 俯视角度
        0.0f,
        0.0f,
        0.0f,
        0.625f,
        0.625f,
        0.625f);

    // 地面掉落物
    m_transforms.ground = ItemTransform(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.25f, 0.25f, 0.25f);

    // 固定位置（物品展示框）
    m_transforms.fixed = ItemTransform(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f);

    // 头部位置（如南瓜）
    m_transforms.head = ItemTransform(0.0f, 180.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    m_initialized = true;

    spdlog::info("ItemInHandRenderer: Initialized");
    return {};
}

void ItemInHandRenderer::destroy()
{
    m_initialized = false;
}

// ============================================================================
// 辅助方法
// ============================================================================

bool ItemInHandRenderer::isBlockItem(const ItemStack& itemStack)
{
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }

    // 方式1: 检查是否为 BlockItem 类型
    if (dynamic_cast<const BlockItem*>(item) != nullptr) {
        return true;
    }

    // 方式2: 检查物品模型类型
    const resource::BakedItemModel* model = getItemModel(itemStack);
    if (model != nullptr) {
        return model->type == resource::ItemModelType::Block;
    }

    return false;
}

const resource::BakedItemModel* ItemInHandRenderer::getItemModel(const ItemStack& itemStack)
{
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return nullptr;
    }

    // 从 ItemModelCache 单例获取模型
    auto& cache = resource::ItemModelCache::instance();
    if (!cache.isInitialized()) {
        return nullptr;
    }

    return cache.getItemModel(*item);
}

// ============================================================================
// 渲染方法
// ============================================================================

void ItemInHandRenderer::renderItem(
    MatrixStack& stack, const ItemStack& itemStack, TransformType transformType, bool leftHanded)
{
    if (!m_initialized || itemStack.isEmpty()) {
        return;
    }

    // 应用变换
    applyTransform(stack, itemStack, transformType, leftHanded);

    // 检查是否为方块物品
    const bool isBlock = isBlockItem(itemStack);

    // 根据物品类型渲染
    if (isBlock) {
        renderBlockItem(stack, itemStack, transformType, leftHanded);
    } else {
        renderRegularItem(stack, itemStack, transformType, leftHanded);
    }
}

void ItemInHandRenderer::renderBlockItem(
    MatrixStack& stack, const ItemStack& itemStack, TransformType transformType, bool leftHanded)
{
    // 参考 MC 1.16.5 ItemRenderer.renderItem()
    // 方块物品使用 ItemMeshBuilder::buildBlockItemMesh() 构建3D网格
    //
    // 渲染流程：
    // 1. 从 ItemModelCache 获取 BakedItemModel
    // 2. 使用 ItemMeshBuilder 构建3D网格（包含 elements 数据）
    // 3. 模型变换已在 ItemMeshBuilder 中应用
    //
    // 注意：此方法目前只负责变换部分
    // 实际的网格渲染由 FirstPersonRenderer 或 HeldItemLayer 完成
    // 它们使用 ItemMeshBuilder::buildHeldItemMesh() 获取顶点数据

    (void)stack;
    (void)transformType;
    (void)leftHanded;

    // 获取物品模型用于验证
    const resource::BakedItemModel* model = getItemModel(itemStack);
    if (model == nullptr) {
        // spdlog::trace("ItemInHandRenderer: No model for block item {}",
        //     itemStack.isEmpty() ? "empty" : itemStack.getItem()->itemLocation().toString());
        return;
    }

    // 网格构建和渲染由调用者完成
    // 此处仅记录日志用于调试
    // spdlog::trace("ItemInHandRenderer: Rendering block item {} with {} elements",
    //     itemStack.getItem()->itemLocation().toString(),
    //     model->elements.size());
}

void ItemInHandRenderer::renderRegularItem(
    MatrixStack& stack, const ItemStack& itemStack, TransformType transformType, bool leftHanded)
{
    // 参考 MC 1.16.5 ItemRenderer.renderItem()
    // 普通物品使用 ItemMeshBuilder::buildGeneratedMesh() 或 buildHeldItemMesh() 构建网格
    //
    // 渲染流程：
    // 1. 从 ItemModelCache 获取 BakedItemModel
    // 2. 使用 ItemMeshBuilder 根据模型类型构建网格
    //    - Generated/Handheld: buildGeneratedMesh() - billboard 四边形
    //    - Custom: buildCustomMesh() - 3D elements
    // 3. 模型变换已在 ItemMeshBuilder 中应用
    //
    // 注意：此方法目前只负责变换部分
    // 实际的网格渲染由 FirstPersonRenderer 或 HeldItemLayer 完成

    (void)stack;
    (void)transformType;
    (void)leftHanded;

    // 获取物品模型用于验证
    const resource::BakedItemModel* model = getItemModel(itemStack);
    if (model == nullptr) {
        spdlog::trace("ItemInHandRenderer: No model for item {}",
            itemStack.isEmpty() ? "empty" : itemStack.getItem()->itemLocation().toString());
        return;
    }

    // 网格构建和渲染由调用者完成
    // 此处仅记录日志用于调试
    spdlog::trace("ItemInHandRenderer: Rendering item {} with type {} and {} texture layers",
        itemStack.getItem()->itemLocation().toString(),
        static_cast<int>(model->type),
        model->textureLayers.size());
}

// ============================================================================
// 变换应用
// ============================================================================

bool ItemInHandRenderer::applyTransform(
    MatrixStack& stack, const ItemStack& itemStack, TransformType transformType, bool leftHanded)
{
    // 参考 MC 1.16.5 ItemRenderer.renderItem()
    // 1. 首先尝试从物品模型获取自定义变换
    // 2. 如果模型没有定义该变换类型，使用默认变换

    const resource::BakedItemModel* model = getItemModel(itemStack);
    if (model != nullptr) {
        // 将 TransformType 转换为 ItemDisplayContext
        resource::ItemDisplayContext displayContext;
        switch (transformType) {
            case TransformType::ThirdPersonRightHand:
                displayContext = resource::ItemDisplayContext::ThirdPersonRightHand;
                break;
            case TransformType::ThirdPersonLeftHand:
                displayContext = resource::ItemDisplayContext::ThirdPersonLeftHand;
                break;
            case TransformType::FirstPersonRightHand:
                displayContext = resource::ItemDisplayContext::FirstPersonRightHand;
                break;
            case TransformType::FirstPersonLeftHand:
                displayContext = resource::ItemDisplayContext::FirstPersonLeftHand;
                break;
            case TransformType::Head:
                displayContext = resource::ItemDisplayContext::Head;
                break;
            case TransformType::Gui:
                displayContext = resource::ItemDisplayContext::Gui;
                break;
            case TransformType::Ground:
                displayContext = resource::ItemDisplayContext::Ground;
                break;
            case TransformType::Fixed:
                displayContext = resource::ItemDisplayContext::Fixed;
                break;
            default:
                displayContext = resource::ItemDisplayContext::Gui;
                break;
        }

        // 获取模型变换
        const resource::ItemTransform& modelTransform = model->getTransform(displayContext);

        // 检查是否有自定义变换
        // 如果变换不是默认值（identity），则应用模型变换
        glm::mat4 mat = modelTransform.toMatrix();

        // 检查矩阵是否为单位矩阵（近似）
        constexpr f32 epsilon = 0.0001f;
        bool isIdentity = true;
        for (int i = 0; i < 4 && isIdentity; ++i) {
            for (int j = 0; j < 4 && isIdentity; ++j) {
                f32 expected = (i == j) ? 1.0f : 0.0f;
                f32 actual = mat[i][j];
                if (std::abs(actual - expected) > epsilon) {
                    isIdentity = false;
                }
            }
        }

        if (!isIdentity) {
            // 应用模型变换
            // 注意：ItemTransform 的变换顺序是 scale -> rotate -> translate
            // 但矩阵栈是后乘语义，所以顺序相反
            const glm::vec3& scale = modelTransform.scale;
            const glm::vec3& rotation = modelTransform.rotation;
            const glm::vec3& translation = modelTransform.translation;

            // 左手需要镜像 Y 轴旋转和 Z 轴旋转
            f32 rotY = rotation.y;
            f32 rotZ = rotation.z;
            if (leftHanded) {
                rotY = -rotY;
                rotZ = -rotZ;
            }

            // 平移（像素单位转换为方块单位，除以 16）
            stack.translate(translation.x / 16.0f, translation.y / 16.0f, translation.z / 16.0f);

            // 旋转（角度）
            stack.rotateZ(rotZ);
            stack.rotateY(rotY);
            stack.rotateX(rotation.x);

            // 缩放
            stack.scale(scale.x, scale.y, scale.z);

            return true;
        }
    }

    // 使用默认变换
    applyDefaultTransform(stack, transformType, leftHanded);
    return false;
}

void ItemInHandRenderer::applyDefaultTransform(MatrixStack& stack, TransformType transformType, bool leftHanded)
{
    // 获取对应的变换
    const ItemTransform& transform = m_transforms.getTransform(transformType);

    if (transform.isDefault()) {
        // 没有自定义变换，使用硬编码的默认值
        // 参考 MC 1.16.5 ItemCameraTransforms 默认变换
        switch (transformType) {
            case TransformType::ThirdPersonRightHand:
                // 第三人称右手：物品在手侧下方
                stack.translate(0.0f, 3.0f, 1.0f);
                stack.scale(0.55f, 0.55f, 0.55f);
                break;

            case TransformType::ThirdPersonLeftHand:
                // 第三人称左手：镜像右手
                stack.translate(0.0f, 3.0f, 1.0f);
                stack.scale(0.55f, 0.55f, 0.55f);
                break;

            case TransformType::FirstPersonRightHand: {
                // 第一人称右手：物品稍微倾斜
                // 参考 MC 1.16.5 FirstPersonRenderer.transformSideFirstPerson
                stack.translate(1.13f, 3.2f, 1.13f);
                stack.rotateY(45.0f);
                stack.rotateX(0.0f);
                stack.scale(0.68f, 0.68f, 0.68f);
                break;
            }

            case TransformType::FirstPersonLeftHand: {
                // 第一人称左手：镜像右手
                stack.translate(-1.13f, 3.2f, 1.13f);
                stack.rotateY(-45.0f);
                stack.rotateX(0.0f);
                stack.scale(0.68f, 0.68f, 0.68f);
                break;
            }

            case TransformType::Gui:
                // GUI 显示：俯视角度
                stack.translate(0.0f, 0.0f, 0.0f);
                stack.rotateX(30.0f);
                stack.rotateY(225.0f);
                stack.scale(0.625f, 0.625f, 0.625f);
                break;

            case TransformType::Ground:
                // 地面掉落物
                stack.translate(0.0f, 0.0f, 0.0f);
                stack.scale(0.25f, 0.25f, 0.25f);
                break;

            case TransformType::Fixed:
                // 固定位置（物品展示框）
                stack.translate(0.0f, 0.0f, 0.0f);
                stack.scale(0.5f, 0.5f, 0.5f);
                break;

            case TransformType::Head:
                // 头部位置
                stack.translate(0.0f, 0.0f, 0.0f);
                stack.rotateY(180.0f);
                stack.scale(1.0f, 1.0f, 1.0f);
                break;

            default:
                break;
        }
    } else {
        // 使用自定义变换
        // 左手需要镜像 Y 轴旋转和 Z 轴旋转
        f32 rotY = transform.rotation.y;
        f32 rotZ = transform.rotation.z;
        if (leftHanded) {
            rotY = -rotY;
            rotZ = -rotZ;
        }

        // 平移（像素单位转换为方块单位，除以 16）
        stack.translate(
            transform.translation.x / 16.0f, transform.translation.y / 16.0f, transform.translation.z / 16.0f);

        // 旋转（角度）
        stack.rotateZ(rotZ);
        stack.rotateY(rotY);
        stack.rotateX(transform.rotation.x);

        // 缩放
        stack.scale(transform.scale.x, transform.scale.y, transform.scale.z);
    }
}

} // namespace mc::client::renderer::trident::firstperson
