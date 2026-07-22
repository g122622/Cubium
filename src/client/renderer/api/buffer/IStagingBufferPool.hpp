/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"

namespace mc::client::renderer::api {

// 前置声明：避免在接口头中引入 Vulkan 头依赖，保持 api 层后端无关
class IBuffer;

/**
 * @brief 暂存分配句柄
 *
 * 池返回的临时句柄，调用方据此将数据 memcpy 到 mappedPtr（已包含 offset），
 * 然后调用池的 copyToBuffer 录制复制命令，或自行用 backingBuffer() + offset
 * 录制 vkCmdCopyBufferToImage。
 *
 * 同步模式：copyToBuffer / release 由调用方显式配对，release 后句柄立即失效。
 * 异步模式：句柄登记到某帧回收桶，由池在下一帧 recycleFrame 回收，调用方不 release。
 */
struct StagingHandle {
    void* mappedPtr = nullptr; ///< 映射指针（已加 offset，可直接 memcpy）
    u64 offset = 0;            ///< 在 backing buffer 内的偏移
    u32 metadata = 0;          ///< 分配器内部记账（OffsetAllocator 的 node index），release 时需原样回传
    u32 segmentIndex = 0;      ///< 多段 backing buffer 索引（当前实现恒为 0，预留多段扩展）
    u64 size = 0;              ///< 实际数据大小（未含对齐 padding）
    bool valid = false;        ///< 是否有效（分配失败时 false，调用方需走 fallback）
};

/**
 * @brief 统一暂存缓冲池接口
 *
 * 用 OffsetAllocator 在一个持久映射的大 HOST_VISIBLE|HOST_COHERENT buffer 内
 * 子分配暂存区间，消除散落在各处的"每次上传都 vkCreateBuffer+vkAllocateMemory+
 * vkDestroyBuffer+vkFreeMemory"反模式。
 *
 * 提供同步与异步两种回收语义：
 * - 同步模式（stage / copyToBuffer / release）：copyToBuffer 内部 submit 单次
 *   命令缓冲并等待 fence，release 立即归还 offset。用于资源加载/初始化/图集子区域上传。
 * - 异步模式（stageAsync / backingBuffer）：调用方自行将复制命令录进当前帧命令缓冲，
 *   句柄登记到 frameIndex 回收桶，池在下一帧 beginFrame 的 recycleFrame 归还。
 *   用于每帧动画上传，避免每帧阻塞等待 fence。
 *
 * 为什么不封装 copyToImage：图集上传需要 VkBufferImageCopy（srcOffset/dstImageOffset/
 * extent）+ image layout transition，后者必须由持有 VkImage 的图集对象自己录制，
 * 池强行封装会破坏分层。故池只暴露 backingBuffer() + handle.offset，由调用方自行录制。
 */
class IStagingBufferPool {
public:
    virtual ~IStagingBufferPool() = default;

    /**
     * @brief 初始化池：创建 backing buffer + OffsetAllocator
     * @param context 渲染上下文（后端特定，Trident 实现接收 TridentContext*）
     * @param initialCapacity backing buffer 初始容量（字节）
     * @param maxFramesInFlight 异步回收桶数（= 帧在飞数）
     * @return 成功或错误
     */
    [[nodiscard]] virtual Result<void> initialize(void* context, u64 initialCapacity, u32 maxFramesInFlight) = 0;

    /**
     * @brief 销毁池，释放 backing buffer 与分配器
     */
    virtual void destroy() = 0;

    // ------------------------------------------------------------------------
    // 同步模式
    // ------------------------------------------------------------------------

    /**
     * @brief 同步分配一段暂存空间
     * @param size 需要的数据大小（字节）
     * @return 句柄；valid=false 表示池容量不足，调用方应走 fallback 一次性上传
     */
    [[nodiscard]] virtual StagingHandle stage(u64 size) = 0;

    /**
     * @brief 同步将句柄数据复制到目标 buffer（内部 submit 单次命令并等待 fence）
     * @param handle stage 返回的句柄
     * @param dstBuffer 目标 buffer 的原生句柄（Vulkan 为 VkBuffer，经 void* 传递）
     * @param dstOffset 目标 buffer 内偏移
     * @return 成功或错误
     */
    [[nodiscard]] virtual Result<void> copyToBuffer(const StagingHandle& handle, void* dstBuffer, u64 dstOffset) = 0;

    /**
     * @brief 同步释放句柄，归还 offset 给分配器
     *
     * 必须在 copyToBuffer 返回后（命令已 GPU 完成）调用。
     */
    virtual void release(const StagingHandle& handle) = 0;

    // ------------------------------------------------------------------------
    // 异步模式
    // ------------------------------------------------------------------------

    /**
     * @brief 异步分配一段暂存空间，登记到 frameIndex 回收桶
     *
     * 调用方需自行用 backingBuffer() + handle.offset 录制复制命令到当前帧命令缓冲。
     * 不调用 release，由 recycleFrame 在该帧 slot 被复用时统一回收。
     */
    [[nodiscard]] virtual StagingHandle stageAsync(u64 size, u32 frameIndex) = 0;

    /**
     * @brief 返回 backing buffer 的原生句柄（Vulkan 为 VkBuffer）
     * @param segmentIndex 多段索引（当前实现恒 0）
     */
    [[nodiscard]] virtual void* backingBuffer(u32 segmentIndex) const = 0;

    /**
     * @brief 每帧 beginFrame 调用：回收 frameIndex 对应轮次的异步分配
     *
     * 此刻 acquireNextImage 已等待该 slot fence，上一轮同 slot 的 GPU 命令已完成，
     * 回收其异步 staging 分配是安全的。
     */
    virtual void recycleFrame(u32 frameIndex) = 0;

    // ------------------------------------------------------------------------
    // 异步 copy 提交（fence 回收，独立于帧命令缓冲）
    // ------------------------------------------------------------------------
    //
    // 用于"调用点拿不到当前帧命令缓冲、又不想同步等待 fence"的上传场景
    // （如 ChunkRenderer::updateChunk 跑在 update 阶段，先于 render 的 beginFrame）。
    //
    // 语义：submitAsyncCopy 内部分配一次性命令缓冲，录制 vkCmdCopyBuffer(staging→dst)，
    // 创建 fence 后 submit 到 graphics queue 但不等待；handle + fence + cmd 入待回收队列。
    // 调用方随后每帧调 pollAsyncCopies 推进：对每条 fence 调 vkGetFenceStatus，signaled
    // 则 release(handle)、销毁 fence、归还命令缓冲。源数据在 submitAsyncCopy 返回后即可释放
    // （memcpy 在 submit 前完成，是 CPU→CPU）。

    /**
     * @brief 异步把句柄数据复制到目标 buffer（submit 带 fence，不等待）
     *
     * @param handle stage 返回的有效句柄（调用方已 memcpy 完源数据）
     * @param dstBuffer 目标 buffer 原生句柄
     * @param dstOffset 目标 buffer 内偏移
     * @return 成功或错误。成功后 handle 不再由调用方 release，由 pollAsyncCopies 回收。
     */
    [[nodiscard]] virtual Result<void> submitAsyncCopy(const StagingHandle& handle, void* dstBuffer, u64 dstOffset) = 0;

    /**
     * @brief 轮询异步 copy 的 fence，回收已完成的暂存区间与同步对象
     *
     * 每帧调用一次。signaled 的条目：release(handle) 归还 offset、销毁 fence、归还命令缓冲。
     * 未 signaled 的保留到下一帧。设备 idle 时（如 destroy 前）调用会一次性回收全部。
     * @return 本轮回收的条目数
     */
    virtual u32 pollAsyncCopies() = 0;

    // ------------------------------------------------------------------------
    // 调试
    // ------------------------------------------------------------------------

    /**
     * @brief 当前总空闲空间（字节），用于守恒断言与容量监控
     */
    [[nodiscard]] virtual u64 totalFreeSpace() const = 0;
};

} // namespace mc::client::renderer::api
