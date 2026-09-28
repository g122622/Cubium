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

#include "common/network/buffer/RegistryByteBuf.hpp"
#include "common/network/pipeline/ProtocolTableSet.hpp"

#include <memory>

namespace mc::fuzz {

/// Java 后端 fuzz 使用的缓冲类型别名（对应 Java 的 RegistryFriendlyByteBuf）。
using FuzzBuf = network::buffer::RegistryByteBuf;

/// Java 1.21.11 五阶段 × 两流向共 10 张包表的集合。
using FuzzTables = network::pipeline::ProtocolTableSet<FuzzBuf>;

/**
 * @brief 进程级一次性初始化（幂等）
 *
 * 依次完成三件事：
 *  1. `VanillaBlocks::initialize()` —— 方块注册表，`readBlockStateHolder` 查表的前置。
 *  2. `Items::initialize()`         —— 物品注册表，`readItemHolder` / ItemStack 解码的前置。
 *  3. `JavaProtocolTables::build()` —— 五阶段 × 两流向共 10 张包表。
 *
 * 三项均为**纯内存**操作（两个注册表都带 `s_initialized` 守卫，且不读资源包、不读数据包目录），
 * 因此 fuzz 目标可以在无资源包、无数据包、无世界、无存档的环境下运行。
 *
 * 幂等：重复调用无副作用，可在每次 `LLVMFuzzerTestOneInput` 入口无条件调用。
 */
void initializeOnce();

/**
 * @brief 取已构建的包表集合
 *
 * 内部保证 `initializeOnce()` 已执行，调用方无需自行初始化。
 */
[[nodiscard]] const std::shared_ptr<FuzzTables>& tables();

#ifdef _WIN32
/**
 * @brief 安装"异常现场报告器"（Windows 专用，fuzz 专用）
 *
 * 背景：libFuzzer 的崩溃检测依赖 Sanitizer 的死回调；而 `int3`/断点类异常
 * （STATUS_BREAKPOINT，0x80000003）不经 SEH 分发，进程会**静默死亡**——既不打印
 * 栈回溯、也不落盘复现用例，在 fuzz 输出里表现为"无任何提示地退出"，极难定位。
 *
 * 本函数用 AddVectoredExceptionHandler 挂在异常分发最前端，借用项目既有的
 * `mc::assert::CrashHandler::captureStackTraceFromSeh` 输出符号化栈，随后返回
 * EXCEPTION_CONTINUE_SEARCH 交回默认处理（不改变原有终止语义）。
 *
 * 幂等；由 `initializeOnce()` 自动调用。
 */
void installExceptionReporter();
#endif

} // namespace mc::fuzz
