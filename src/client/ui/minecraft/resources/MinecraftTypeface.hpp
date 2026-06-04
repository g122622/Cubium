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

#include "client/ui/kagero/paint/contracts/ITypeface.hpp"
#include <memory>

namespace mc::client::ui::minecraft {

/**
 * @brief Minecraft 风格字体封装
 *
 * 对 kagero 渲染引擎的 ITypeface 接口进行封装，
 * 提供与 Minecraft 资源系统集成的字体对象。
 */
class MinecraftTypeface {
public:
    /**
     * @brief 构造 MinecraftTypeface
     *
     * @param typeface kagero 字体接口实例
     */
    explicit MinecraftTypeface(std::unique_ptr<kagero::paint::ITypeface> typeface);

    /**
     * @brief 获取底层 kagero 字体接口指针
     *
     * @return 字体接口的裸指针，不转移所有权
     */
    [[nodiscard]] const kagero::paint::ITypeface* get() const;

private:
    std::unique_ptr<kagero::paint::ITypeface> m_typeface;
};

} // namespace mc::client::ui::minecraft
