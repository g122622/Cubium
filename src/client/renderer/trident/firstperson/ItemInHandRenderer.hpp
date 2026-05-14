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

#include "ItemCameraTransforms.hpp"
#include "MatrixStack.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include <memory>

// 前向声明
namespace mc::client::renderer::trident {
class TridentEngine;
}

namespace mc::client {
class ResourceManager;
class ItemTextureAtlas;
} // namespace mc::client

namespace mc::client::renderer::firstperson {

/**
 * @brief 手持物品渲染器
 *
 * 负责渲染玩家手中的物品，包括：
 * - 第一人称手持物品
 * - 第三人称手持物品
 * - GUI 牺牲显示
 * - 地面掉落物
 * - 物品展示框
 *
 * 参考 MC 1.16.5 ItemInHandRenderer
 */
class ItemInHandRenderer {
public:
    ItemInHandRenderer();
    ~ItemInHandRenderer();

    // 禁止拷贝
    ItemInHandRenderer(const ItemInHandRenderer&) = delete;
    ItemInHandRenderer& operator=(const ItemInHandRenderer&) = delete;

    // ========== 初始化 ==========

    /**
     * @brief 初始化渲染器
     *
     * @param engine 渲染引擎
     * @param resourceManager 资源管理器
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(trident::TridentEngine* engine, ResourceManager* resourceManager);

    /**
     * @brief 销毁资源
     */
    void destroy();

    // ========== 渲染方法 ==========

    /**
     * @brief 渲染手持物品
     *
     * 根据变换类型渲染物品。
     *
     * @param stack 矩阵栈
     * @param itemStack 物品堆
     * @param transformType 变换类型
     * @param leftHanded 是否为左手
     */
    void renderItem(
        MatrixStack& matrixStack, const ItemStack& itemStack, TransformType transformType, bool leftHanded = false);

    /**
     * @brief 渲染方块物品
     *
     * 方块物品需要特殊的变换和渲染逻辑。
     *
     * @param stack 矩阵栈
     * @param itemStack 物品堆（必须是方块物品）
     * @param transformType 变换类型
     * @param leftHanded 是否为左手
     */
    void renderBlockItem(
        MatrixStack& matrixStack, const ItemStack& itemStack, TransformType transformType, bool leftHanded = false);

    /**
     * @brief 渲染普通物品
     *
     * 非方块物品的渲染逻辑。
     *
     * @param stack 矩阵栈
     * @param itemStack 物品堆
     * @param transformType 变换类型
     * @param leftHanded 是否为左手
     */
    void renderRegularItem(
        MatrixStack& matrixStack, const ItemStack& itemStack, TransformType transformType, bool leftHanded = false);

    // ========== 变换应用 ==========

    /**
     * @brief 应用物品变换
     *
     * 根据 TransformType 应用相应的变换到矩阵栈。
     *
     * @param stack 矩阵栈
     * @param itemStack 物品堆
     * @param transformType 变换类型
     * @param leftHanded 是否为左手
     * @return 是否应用了变换
     */
    bool applyTransform(
        MatrixStack& matrixStack, const ItemStack& itemStack, TransformType transformType, bool leftHanded = false);

    /**
     * @brief 应用默认手持变换
     *
     * 当物品模型没有自定义变换时使用。
     *
     * @param stack 矩阵栈
     * @param transformType 变换类型
     * @param leftHanded 是否为左手
     */
    void applyDefaultTransform(MatrixStack& matrixStack, TransformType transformType, bool leftHanded = false);

    // ========== 访问器 ==========

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief 获取物品相机变换
     */
    [[nodiscard]] const ItemCameraTransforms& transforms() const { return m_transforms; }

private:
    // 渲染引擎
    trident::TridentEngine* m_engine = nullptr;
    ResourceManager* m_resourceManager = nullptr;

    // 物品相机变换
    ItemCameraTransforms m_transforms;

    // 初始化标志
    bool m_initialized = false;
};

} // namespace mc::client::renderer::firstperson
