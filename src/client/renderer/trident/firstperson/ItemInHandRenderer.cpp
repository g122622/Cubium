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
#include "../../../resource/ResourceManager.hpp"
#include "../core/TridentEngine.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::firstperson {

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

Result<void> ItemInHandRenderer::initialize(trident::TridentEngine* engine, ResourceManager* resourceManager)
{
    if (engine == nullptr) {
        return Error(ErrorCode::NullPointer, "TridentEngine is null");
    }

    m_engine = engine;
    m_resourceManager = resourceManager;

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
    m_engine = nullptr;
    m_resourceManager = nullptr;
    m_initialized = false;
}

// ============================================================================
// 渲染方法
// ============================================================================

void ItemInHandRenderer::renderItem(
    MatrixStack& matrixStack, const ItemStack& itemStack, TransformType transformType, bool leftHanded)
{
    if (!m_initialized || itemStack.isEmpty()) {
        return;
    }

    // TODO: 检查是否为方块物品
    // const Item* item = itemStack.getItem();
    // bool isBlock = item != nullptr && item->isBlock();

    // 应用变换
    applyTransform(matrixStack, itemStack, transformType, leftHanded);

    // 渲染物品
    // 目前先使用默认变换，后续集成 BlockModelCache 和 ItemRenderer
    // if (isBlock) {
    //     renderBlockItem(matrixStack, itemStack, transformType, leftHanded);
    // } else {
    //     renderRegularItem(matrixStack, itemStack, transformType, leftHanded);
    // }

    // 临时：应用默认变换后需要实际渲染
    // 这部分将在集成 ItemRenderer 和 BlockModelCache 后实现
}

void ItemInHandRenderer::renderBlockItem(
    MatrixStack& matrixStack, const ItemStack& itemStack, TransformType transformType, bool leftHanded)
{
    // TODO: 实现方块物品渲染
    // 需要 BlockModelCache 支持
    (void)matrixStack;
    (void)itemStack;
    (void)transformType;
    (void)leftHanded;
}

void ItemInHandRenderer::renderRegularItem(
    MatrixStack& matrixStack, const ItemStack& itemStack, TransformType transformType, bool leftHanded)
{
    // TODO: 实现普通物品渲染
    // 需要 ItemTextureAtlas 支持
    (void)matrixStack;
    (void)itemStack;
    (void)transformType;
    (void)leftHanded;
}

// ============================================================================
// 变换应用
// ============================================================================

bool ItemInHandRenderer::applyTransform(
    MatrixStack& matrixStack, const ItemStack& itemStack, TransformType transformType, bool leftHanded)
{
    // TODO: 从物品模型获取自定义变换
    // const Item* item = itemStack.getItem();
    // if (item != nullptr) {
    //     const BakedBlockModel* model = getBlockModel(item);
    //     if (model != nullptr && model->hasTransform(transformType)) {
    //         model->applyTransform(matrixStack, transformType);
    //         return true;
    //     }
    // }

    // 使用默认变换
    applyDefaultTransform(matrixStack, transformType, leftHanded);
    (void)itemStack;
    return false;
}

void ItemInHandRenderer::applyDefaultTransform(MatrixStack& matrixStack, TransformType transformType, bool leftHanded)
{
    // 获取对应的变换
    const ItemTransform& transform = m_transforms.getTransform(transformType);

    if (transform.isDefault()) {
        // 没有自定义变换，使用硬编码的默认值
        switch (transformType) {
            case TransformType::ThirdPersonRightHand:
                matrixStack.translate(0.0f, 3.0f, 1.0f);
                matrixStack.scale(0.55f, 0.55f, 0.55f);
                break;

            case TransformType::ThirdPersonLeftHand:
                matrixStack.translate(0.0f, 3.0f, 1.0f);
                matrixStack.scale(0.55f, 0.55f, 0.55f);
                break;

            case TransformType::FirstPersonRightHand:
                // 第一人称右手：物品稍微倾斜
                matrixStack.translate(1.13f, 3.2f, 1.13f);
                matrixStack.rotateY(45.0f);
                matrixStack.rotateX(0.0f);
                matrixStack.scale(0.68f, 0.68f, 0.68f);
                break;

            case TransformType::FirstPersonLeftHand:
                // 第一人称左手：镜像右手
                matrixStack.translate(1.13f, 3.2f, 1.13f);
                matrixStack.rotateY(-45.0f);
                matrixStack.rotateX(0.0f);
                matrixStack.scale(0.68f, 0.68f, 0.68f);
                break;

            case TransformType::Gui:
                // GUI 显示：俯视角度
                matrixStack.translate(0.0f, 0.0f, 0.0f);
                matrixStack.rotateX(30.0f);
                matrixStack.rotateY(225.0f);
                matrixStack.scale(0.625f, 0.625f, 0.625f);
                break;

            case TransformType::Ground:
                // 地面掉落物
                matrixStack.translate(0.0f, 0.0f, 0.0f);
                matrixStack.scale(0.25f, 0.25f, 0.25f);
                break;

            case TransformType::Fixed:
                // 固定位置（物品展示框）
                matrixStack.translate(0.0f, 0.0f, 0.0f);
                matrixStack.scale(0.5f, 0.5f, 0.5f);
                break;

            case TransformType::Head:
                // 头部位置
                matrixStack.translate(0.0f, 0.0f, 0.0f);
                matrixStack.rotateY(180.0f);
                matrixStack.scale(1.0f, 1.0f, 1.0f);
                break;

            default:
                break;
        }
    } else {
        // 使用自定义变换
        transform.apply(matrixStack);

        // 如果是左手，需要镜像
        if (leftHanded) {
            // 镜像 X 轴旋转
            // 注意：这需要根据具体模型来调整
        }
    }
}

} // namespace mc::client::renderer::firstperson
