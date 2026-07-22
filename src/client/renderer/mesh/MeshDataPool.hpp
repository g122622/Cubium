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

#include "client/renderer/MeshTypes.hpp"
#include <mutex>
#include <vector>

namespace mc::client {

/**
 * @brief MeshData 回收池（单桶 free-list）
 *
 * 迁移到 UniversalWorkerPool 后取代原 MeshWorkerPool 的 per-worker 回收桶。
 * 原实现按 workerId 分桶以保持回收 MeshData 与原分配 worker 的 L2/L3 缓存热度；
 * 单桶后失去这层热度，但毫秒级网格构建中可忽略，换来结构简化与 workerId 概念消除。
 *
 * solid/transparent 各一桶，单把 mutex 保护。回收在主线程每帧 ≤16 次（GPU 上传后），
 * 获取在 worker 线程（MeshBuildTask::execute 内），互不持锁阻塞 generateSplitMesh，
 * 无死锁、低竞争。
 *
 * 归还前对异常膨胀的 capacity 做 shrink_to_fit，防止某次峰值后超大 capacity 永久驻留池里。
 */
class MeshDataPool {
public:
    MeshDataPool();
    ~MeshDataPool();

    MeshDataPool(const MeshDataPool&) = delete;
    MeshDataPool& operator=(const MeshDataPool&) = delete;

    /**
     * @brief 取出一个带历史 capacity 的 MeshData；池空则默认构造（capacity=0）。
     *
     * @param transparent true 取透明层桶，false 取实心层桶。
     */
    [[nodiscard]] MeshData acquire(bool transparent);

    /**
     * @brief 归还单个 MeshData 入池前做膨胀防护，桶满则不入池（右值参析构释放）。
     *
     * @param transparent 归入透明层桶还是实心层桶。
     * @param data 已 clear 的 MeshData（capacity 可复用）。
     */
    void recycle(bool transparent, MeshData&& data);

private:
    /// capacity 异常膨胀时 shrink_to_fit，其余情况保留 capacity 供复用。
    static void _shrinkIfBloated(MeshData& data);

    struct Bucket {
        std::vector<MeshData> slots;
        std::mutex mutex;
    };

    /// 每桶缓存的 MeshData 上限。回收在主线程每帧 ≤16 次，单桶 16 个足够吸收稳态归还抖动，
    /// 超出直接析构释放。
    static constexpr size_t kMaxSlots = 16;

    /// 容量膨胀防护阈值（顶点数）。任务 5 将 Solid reserve 上限降为 8192 faces = 32768 顶点，
    /// 取其 1.5 倍作防护：超过此 capacity 且远大于实际 size 时触发 shrink_to_fit。
    static constexpr size_t kMaxReuseVertexCapacity = 49152;
    static constexpr size_t kMaxReuseIndexCapacity = kMaxReuseVertexCapacity * 6 / 4;

    Bucket m_solidBucket;
    Bucket m_transparentBucket;
};

} // namespace mc::client
