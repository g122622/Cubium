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
#include "client/resource/ItemModelCache.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
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
        1.13f,
        3.2f,
        1.13f, // 平移
        0.68f,
        0.68f,
        0.68f // 缩放
    );

    // 第一人称左手（镜像右手）
    m_transforms.firstPersonLeft = ItemTransform(0.0f,
        -45.0f,
        0.0f, // Y 轴旋转 -45 度
        -1.13f,
        3.2f,
        1.13f,
        0.68f,
        0.68f,
        0.68f);

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
// 变换应用
// ============================================================================

bool ItemInHandRenderer::applyTransform(
    MatrixStack& stack, const ItemStack& itemStack, TransformType transformType, bool leftHanded)
{
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
        bool isIdentity = true;
        for (i32 i = 0; i < 4 && isIdentity; ++i) {
            for (i32 j = 0; j < 4 && isIdentity; ++j) {
                f32 expected = (i == j) ? 1.0f : 0.0f;
                f32 actual = mat[i][j];
                if (!approxEqual(actual, expected, LARGE_EPSILON)) {
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
    // 物品模型未提供自定义 display 变换时的回退：使用 initialize() 中设置的默认变换。
    const ItemTransform& transform = m_transforms.getTransform(transformType);

    // 左手需要镜像 Y 轴旋转和 Z 轴旋转
    f32 rotY = transform.rotation.y;
    f32 rotZ = transform.rotation.z;
    if (leftHanded) {
        rotY = -rotY;
        rotZ = -rotZ;
    }

    // 平移（像素单位转换为方块单位，除以 16）
    stack.translate(transform.translation.x / 16.0f, transform.translation.y / 16.0f, transform.translation.z / 16.0f);

    // 旋转（角度）
    stack.rotateZ(rotZ);
    stack.rotateY(rotY);
    stack.rotateX(transform.rotation.x);

    // 缩放
    stack.scale(transform.scale.x, transform.scale.y, transform.scale.z);
}

} // namespace mc::client::renderer::trident::firstperson
