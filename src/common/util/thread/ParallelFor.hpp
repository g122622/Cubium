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

#include "common/core/Types.hpp"

#include <functional>

namespace mc::util {

/**
 * @brief 把 [0, count) 分给若干线程并行处理，全部结束后返回
 *
 * 调用线程自己也参与执行，另外临时创建 `min(count, 硬件并发数) - 1` 条线程；
 * 元素按下标动态领取（不是静态切块），因此各元素耗时差异较大时也能均摊。
 *
 * 【为什么用独立线程而不是提交到 UniversalWorkerPool】
 * 典型调用点在**任务池的工作线程里**（区块生成任务内部的批量预计算）。把子任务提交回
 * 同一个池再阻塞等待会自锁：池里所有工作线程都可能正等着自己的子任务，而子任务还排在
 * 队列里没人执行。也不能在等待时"顺手执行池里的其它任务"——`ChunkGenerationScheduler`
 * 依赖线程局部的调度上下文（`thread_local SyncSchedulingContext`），让工作线程跨任务
 * 执行会破坏该前提。独立线程只跑本次传入的 body，不触碰池内任何状态。
 *
 * 【适用场景】
 * 一次性、粒度较大的 CPU 并行（世界初始化、批量预计算）。每次调用都要创建线程
 * （毫秒级开销），不要在区块级热路径上反复调用。
 *
 * @param count 待处理元素个数，必须非负
 * @param body 处理单个元素；同一时刻最多一个线程处理同一个下标，但不同下标会并发，
 *             故 body 必须线程安全。`src/` 关闭了异常，body 抛出会导致进程终止
 */
void parallelFor(i32 count, const std::function<void(i32)>& body);

} // namespace mc::util
