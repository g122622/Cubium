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

namespace mc::client::ui::kagero::tpl {

/**
 * @brief 初始化模板系统
 *
 * 注册所有内置Widget工厂、属性设置器和事件绑定器。
 * 在程序启动时调用一次。
 *
 * @note 此函数是幂等的，多次调用不会有副作用
 */
void initializeTemplateSystem();

/**
 * @brief 关闭模板系统
 *
 * 清理模板系统资源。
 * 在程序退出时调用。
 */
void shutdownTemplateSystem();

/**
 * @brief 检查模板系统是否已初始化
 *
 * @return 是否已初始化
 */
[[nodiscard]] bool isTemplateSystemInitialized();

} // namespace mc::client::ui::kagero::tpl
