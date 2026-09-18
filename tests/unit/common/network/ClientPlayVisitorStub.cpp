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

// ClientPlayVisitor 测试桩：ClientNetwork.cpp 在 m_visitor!=nullptr 时引用
// ClientPlayVisitor::handle / handleConfiguration 的符号。真实现 ClientPlayVisitor.cpp
// 耦合 ClientApplication（friend 私有成员），无法编入 mc_tests。本桩提供 no-op 定义
// 仅满足链接——capstone 测试传 nullptr 永不调用，符号仅在链接期需要。
//
// 不含任何 TEST：纯链接桩。ClientPlayVisitor.cpp 不在 mc_tests 源列表，故无重复定义。

#include "client/network/ClientPlayVisitor.hpp"

#include "common/core/Result.hpp"

namespace mc::client::net {

Result<void> ClientPlayVisitor::handle(const mc::network::ir::IrPacket& /*packet*/)
{
    return Result<void>::ok();
}

Result<void> ClientPlayVisitor::handleConfiguration(const mc::network::ir::IrPacket& /*packet*/)
{
    return Result<void>::ok();
}

} // namespace mc::client::net
