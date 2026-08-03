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
 * The above notice and this permission notice shall be included in all
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

#include "common/core/Types.hpp"
#include "entity/inventory/ContainerTypes.hpp"
#include <memory>
#include <string>

namespace mc {

// Forward declarations
class AbstractContainerMenu;
class Player;

/**
 * @brief 容器提供者接口
 *
 * 定义创建容器菜单的方法。
 * 当实体或方块需要提供容器时，实现此接口。
 * 例如：箱子、村民、矿车等。
 */
class IContainerProvider {
public:
    virtual ~IContainerProvider() = default;

    /**
     * @brief 创建容器菜单
     *
     * 当玩家打开容器时调用此方法创建对应的菜单实例。
     *
     * @param containerId 容器ID（由服务端分配）
     * @param player 打开容器的玩家
     * @return 创建的容器菜单，如果无法创建返回 nullptr
     */
    [[nodiscard]] virtual std::unique_ptr<AbstractContainerMenu> createMenu(i32 containerId, Player& player) = 0;
};

/**
 * @brief 命名容器提供者接口
 *
 * 继承 IContainerProvider，增加获取显示名称的能力。
 * 旁观者模式玩家只能与实现此接口的实体交互（打开容器）。
 * 用于箱子矿车、村民交易界面等场景。
 */
class INamedContainerProvider : public IContainerProvider {
public:
    ~INamedContainerProvider() override = default;

    /**
     * @brief 获取容器显示名称
     *
     * 返回容器在GUI标题栏显示的名称。
     * 可以是自定义名称或默认翻译键。
     *
     * @return 显示名称
     */
    [[nodiscard]] virtual std::string getDisplayName() const = 0;
};

} // namespace mc
