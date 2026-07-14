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

namespace mc::client::resource {
struct BakedItemModel;
}

namespace mc::client::renderer::trident::firstperson {

/**
 * @brief 手持物品渲染器
 *
 * 负责渲染玩家手中的物品，包括：
 * - 第一人称手持物品
 * - 第三人称手持物品
 * - GUI 物品显示
 * - 地面掉落物
 * - 物品展示框
 *
 * 物品渲染流程：
 * 1. 获取物品模型 (BakedItemModel)
 * 2. 根据模型类型选择渲染方式 (Generated/Handheld/Block/Custom)
 * 3. 应用物品变换 (从模型 JSON 或默认)
 * 4. 构建网格并渲染
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
     * 设置默认变换参数。此方法不依赖外部资源，
     * 因为物品模型从 ItemModelCache 单例获取。
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize();

    /**
     * @brief 销毁资源
     */
    void destroy();

    // ========== 变换应用 ==========

    /**
     * @brief 应用物品变换
     *
     * 根据 TransformType 应用相应的变换到矩阵栈。
     * 首先尝试从物品模型获取自定义变换，如果模型没有定义则使用默认变换。
     *
     * @param stack 矩阵栈
     * @param itemStack 物品堆
     * @param transformType 变换类型
     * @param leftHanded 是否为左手
     * @return 是否应用了变换
     */
    bool applyTransform(MatrixStack& stack, const ItemStack& itemStack, TransformType transformType, bool leftHanded);

    /**
     * @brief 应用默认手持变换
     *
     * 当物品模型没有自定义变换时使用。
     *
     * @param stack 矩阵栈
     * @param transformType 变换类型
     * @param leftHanded 是否为左手
     */
    void applyDefaultTransform(MatrixStack& stack, TransformType transformType, bool leftHanded);

    // ========== 辅助方法 ==========

    /**
     * @brief 检查物品是否为方块物品
     *
     * 方块物品使用 Block 类型模型，需要特殊的渲染逻辑。
     *
     * @param itemStack 物品堆
     * @return 是否为方块物品
     */
    [[nodiscard]] static bool isBlockItem(const ItemStack& itemStack);

    /**
     * @brief 获取物品模型
     *
     * 从 ItemModelCache 获取物品的烘焙模型。
     *
     * @param itemStack 物品堆
     * @return 物品模型指针，如果不存在返回 nullptr
     */
    [[nodiscard]] static const resource::BakedItemModel* getItemModel(const ItemStack& itemStack);

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
    // 物品相机变换
    ItemCameraTransforms m_transforms;

    // 初始化标志
    bool m_initialized = false;
};

} // namespace mc::client::renderer::trident::firstperson
