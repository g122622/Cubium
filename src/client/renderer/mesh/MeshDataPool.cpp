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

#include "MeshDataPool.hpp"

namespace mc::client {

MeshDataPool::MeshDataPool() = default;

MeshDataPool::~MeshDataPool() = default;

MeshData MeshDataPool::acquire(bool transparent)
{
    Bucket& bucket = transparent ? m_transparentBucket : m_solidBucket;
    std::lock_guard<std::mutex> lock(bucket.mutex);
    if (bucket.slots.empty()) {
        return {};
    }
    MeshData out = std::move(bucket.slots.back());
    bucket.slots.pop_back();
    return out;
}

void MeshDataPool::recycle(bool transparent, MeshData&& data)
{
    _shrinkIfBloated(data);

    Bucket& bucket = transparent ? m_transparentBucket : m_solidBucket;
    std::lock_guard<std::mutex> lock(bucket.mutex);
    if (bucket.slots.size() >= kMaxSlots) {
        // 桶满：不进池，data 在函数返回时析构释放（Tracy 对其 vector 缓冲区发对应 free，正确）。
        return;
    }
    bucket.slots.push_back(std::move(data));
}

void MeshDataPool::_shrinkIfBloated(MeshData& data)
{
    // 两条件同时满足才 shrink：capacity 超阈值 且 远大于实际 size(4 倍以上)。
    // 归还时 data 已 clear()(size=0)，条件退化为「超阈值即 shrink」——空壳不该带超大 capacity 入池。
    // shrink_to_fit 会触发 Tracy 成对 free(旧大块)+alloc(新小块)，严格一对一，不变量保持。
    if (data.vertices.capacity() > kMaxReuseVertexCapacity && data.vertices.capacity() > data.vertices.size() * 4) {
        data.vertices.shrink_to_fit();
    }
    if (data.indices.capacity() > kMaxReuseIndexCapacity && data.indices.capacity() > data.indices.size() * 4) {
        data.indices.shrink_to_fit();
    }
}

} // namespace mc::client
