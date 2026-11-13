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
#include "server/world/chunk/base/pipeline/ChunkStatus.hpp"
#include <functional>
#include <future>
#include <memory>

namespace mc::world::chunk {
class ChunkData;
}

namespace mc::server {
/** @brief 一个阶段请求的交付端；目标指向进程生命周期的阶段表。 */
struct ChunkRequestWaiter {
    std::function<void(bool, mc::world::chunk::ChunkData*)> callback;
    std::shared_ptr<std::promise<mc::world::chunk::ChunkData*>> promise;
    const mc::world::chunk::ChunkStatus* targetStatus;
};
} // namespace mc::server
