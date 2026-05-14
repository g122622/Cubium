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

#include "MatrixStack.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc::client::renderer {

/**
 * @brief 物品变换类型
 *
 * 定义物品在不同上下文中的渲染变换方式。
 *
 * 参考 MC 1.16.5 ItemCameraTransforms.TransformType
 */
enum class TransformType : u8 {
    /// 无变换
    None = 0,

    /// 第三人称左手
    ThirdPersonLeftHand = 1,

    /// 第三人称右手
    ThirdPersonRightHand = 2,

    /// 第一人称左手
    FirstPersonLeftHand = 3,

    /// 第一人称右手
    FirstPersonRightHand = 4,

    /// 头部位置（如佩戴的南瓜、头盔等）
    Head = 5,

    /// GUI 显示
    Gui = 6,

    /// 地面掉落物
    Ground = 7,

    /// 固定位置（如物品展示框）
    Fixed = 8
};

/**
 * @brief 判断是否为第一人称视角
 */
[[nodiscard]] inline bool isFirstPerson(TransformType type)
{
    return type == TransformType::FirstPersonLeftHand || type == TransformType::FirstPersonRightHand;
}

/**
 * @brief 判断是否为第三人称视角
 */
[[nodiscard]] inline bool isThirdPerson(TransformType type)
{
    return type == TransformType::ThirdPersonLeftHand || type == TransformType::ThirdPersonRightHand;
}

/**
 * @brief 判断是否为左手
 */
[[nodiscard]] inline bool isLeftHand(TransformType type)
{
    return type == TransformType::FirstPersonLeftHand || type == TransformType::ThirdPersonLeftHand;
}

/**
 * @brief 物品变换参数
 *
 * 定义物品在特定 TransformType 下的变换参数。
 * 包括旋转、平移、缩放。
 *
 * 参考 MC 1.16.5 ItemTransformVec3f
 */
struct ItemTransform {
    /// 旋转角度（度），绕 X/Y/Z 轴
    Vector3f rotation{0.0f, 0.0f, 0.0f};

    /// 平移（像素单位，通常 -1 到 1 范围）
    Vector3f translation{0.0f, 0.0f, 0.0f};

    /// 缩放因子
    Vector3f scale{1.0f, 1.0f, 1.0f};

    /**
     * @brief 默认构造（无变换）
     */
    ItemTransform() = default;

    /**
     * @brief 构造变换参数
     */
    ItemTransform(f32 rotX,
        f32 rotY,
        f32 rotZ,
        f32 transX,
        f32 transY,
        f32 transZ,
        f32 scaleX = 1.0f,
        f32 scaleY = 1.0f,
        f32 scaleZ = 1.0f)
        : rotation(rotX, rotY, rotZ)
        , translation(transX, transY, transZ)
        , scale(scaleX, scaleY, scaleZ)
    {}

    /**
     * @brief 判断是否为默认变换（无变换）
     */
    [[nodiscard]] bool isDefault() const
    {
        return rotation.x == 0.0f && rotation.y == 0.0f && rotation.z == 0.0f && translation.x == 0.0f &&
            translation.y == 0.0f && translation.z == 0.0f && scale.x == 1.0f && scale.y == 1.0f && scale.z == 1.0f;
    }

    /**
     * @brief 应用变换到矩阵栈
     */
    void apply(MatrixStack& stack) const
    {
        if (isDefault()) {
            return;
        }

        // 应用顺序：缩放 -> 旋转 -> 平移
        // 但在矩阵栈中是反序应用的（右乘）
        stack.translate(translation.x, translation.y, translation.z);
        stack.rotateZ(rotation.z);
        stack.rotateY(rotation.y);
        stack.rotateX(rotation.x);
        stack.scale(scale.x, scale.y, scale.z);
    }

    /**
     * @brief 获取默认变换（无变换）
     */
    static ItemTransform defaultTransform() { return ItemTransform(); }
};

/**
 * @brief 物品相机变换
 *
 * 存储物品在各种渲染场景下的变换参数。
 * 这些变换来自物品模型的 JSON 文件定义。
 *
 * 参考 MC 1.16.5 ItemCameraTransforms
 */
class ItemCameraTransforms {
public:
    /**
     * @brief 默认构造（所有变换为默认值）
     */
    ItemCameraTransforms() = default;

    /**
     * @brief 获取指定类型的变换
     */
    [[nodiscard]] const ItemTransform& getTransform(TransformType type) const
    {
        switch (type) {
            case TransformType::ThirdPersonLeftHand:
                return thirdPersonLeft;
            case TransformType::ThirdPersonRightHand:
                return thirdPersonRight;
            case TransformType::FirstPersonLeftHand:
                return firstPersonLeft;
            case TransformType::FirstPersonRightHand:
                return firstPersonRight;
            case TransformType::Head:
                return head;
            case TransformType::Gui:
                return gui;
            case TransformType::Ground:
                return ground;
            case TransformType::Fixed:
                return fixed;
            default:
                return s_defaultTransform;
        }
    }

    /**
     * @brief 检查指定类型是否有自定义变换
     */
    [[nodiscard]] bool hasCustomTransform(TransformType type) const { return !getTransform(type).isDefault(); }

    /**
     * @brief 应用指定类型的变换到矩阵栈
     */
    void applyTransform(MatrixStack& stack, TransformType type) const { getTransform(type).apply(stack); }

    // ========== 静态默认变换 ==========

    /**
     * @brief 获取第三人称右手的默认变换
     *
     * 默认值来自 MC 1.16.5 方块/物品模型的默认变换。
     */
    static ItemTransform getDefaultThirdPersonRight()
    {
        // 第三人称手持物品的标准变换
        return ItemTransform(0.0f,
            0.0f,
            0.0f, // 旋转
            0.0f,
            2.5f,
            0.0f, // 平移（向上偏移）
            0.375f,
            0.375f,
            0.375f // 缩放
        );
    }

    /**
     * @brief 获取第一人称右手的默认变换
     */
    static ItemTransform getDefaultFirstPersonRight()
    {
        // 第一人称手持物品的标准变换
        return ItemTransform(0.0f,
            45.0f,
            0.0f, // 旋转（Y轴45度）
            0.0f,
            0.0f,
            0.0f, // 平移
            0.4f,
            0.4f,
            0.4f // 缩放
        );
    }

    /**
     * @brief 获取GUI显示的默认变换
     */
    static ItemTransform getDefaultGui()
    {
        // GUI 显示的标准变换
        return ItemTransform(30.0f,
            225.0f,
            0.0f, // 旋转（俯视30度，Y轴旋转225度）
            0.0f,
            0.0f,
            0.0f, // 平移
            0.625f,
            0.625f,
            0.625f // 缩放
        );
    }

    /**
     * @brief 获取地面掉落物的默认变换
     */
    static ItemTransform getDefaultGround()
    {
        return ItemTransform(0.0f,
            0.0f,
            0.0f, // 旋转
            0.0f,
            0.0f,
            0.0f, // 平移
            0.25f,
            0.25f,
            0.25f // 缩放
        );
    }

    /**
     * @brief 获取固定位置的默认变换
     */
    static ItemTransform getDefaultFixed()
    {
        return ItemTransform(0.0f,
            0.0f,
            0.0f, // 旋转
            0.0f,
            0.0f,
            0.0f, // 平移
            0.5f,
            0.5f,
            0.5f // 缩放
        );
    }

    // ========== 变换数据成员 ==========

    /// 第三人称左手变换
    ItemTransform thirdPersonLeft;

    /// 第三人称右手变换
    ItemTransform thirdPersonRight;

    /// 第一人称左手变换
    ItemTransform firstPersonLeft;

    /// 第一人称右手变换
    ItemTransform firstPersonRight;

    /// 头部变换（如佩戴南瓜）
    ItemTransform head;

    /// GUI 显示变换
    ItemTransform gui;

    /// 地面掉落物变换
    ItemTransform ground;

    /// 固定位置变换（如物品展示框）
    ItemTransform fixed;

private:
    /// 默认变换（无变换）
    static const ItemTransform s_defaultTransform;
};

} // namespace mc::client::renderer
