/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

namespace mc {

// Forward declarations
class IInventory;

/**
 * @brief 容器变更监听器接口
 *
 * 当容器（IInventory）内容发生变更时通知监听者。
 * 参考: net.minecraft.world.ContainerListener
 *
 * 典型使用场景：
 * - 方块实体标记自身为脏（需要保存）
 * - 容器菜单广播变更到客户端
 * - 成就/进度系统检测物品变化
 *
 * 用法：
 * @code
 * class MyListener : public ContainerListener {
 * public:
 *     void containerChanged(IInventory& inventory) override {
 *         // 处理容器变更
 *     }
 * };
 *
 * myInventory.addListener(myListener);
 * // ... 物品变更时自动通知 ...
 * myInventory.removeListener(myListener);
 * @endcode
 */
class ContainerListener {
public:
    virtual ~ContainerListener() = default;

    /**
     * @brief 容器内容发生变更时调用
     *
     * 在 setItem、removeItem、clear 等修改操作后调用。
     * 注意：removeItemNoUpdate 不会触发此回调。
     *
     * @param inventory 发生变更的容器引用
     */
    virtual void containerChanged(IInventory& inventory) = 0;
};

} // namespace mc
