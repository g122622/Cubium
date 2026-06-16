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

#include "common/resource/pack/InMemoryResourcePack.hpp"
#include <memory>

namespace mc::resource {

/**
 * @brief 原版内置资源
 *
 * 提供 Minecraft 原版的基础模型和 blockstates。
 * 这些资源作为内置资源包，优先级最低，始终加载。
 */
class VanillaResources {
public:
    /**
     * @brief 创建内置资源包
     * @return 包含原版基础模型的内存资源包
     */
    [[nodiscard]] static std::unique_ptr<InMemoryResourcePack> createResourcePack();

private:
    /**
     * @brief 注册基础模型
     */
    static void _registerBaseModels(InMemoryResourcePack& pack);

    /**
     * @brief 注册 blockstates
     */
    static void _registerBlockStates(InMemoryResourcePack& pack);

    // 模型模板
    static const char* MODEL_CUBE_ALL;     // 单面纹理方块
    static const char* MODEL_CUBE_COLUMN;  // 柱状方块（原木等）
    static const char* MODEL_CUBE;         // 六面不同纹理
    static const char* MODEL_LEAVES;       // 树叶
    static const char* MODEL_CROSS;        // 交叉纹理（花草等）
    static const char* MODEL_TINTED_CROSS; // 染色交叉纹理
    static const char* MODEL_AIR;          // 空气
};

} // namespace mc::resource

namespace mc {
using VanillaResources = resource::VanillaResources;
} // namespace mc
