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

#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <cstddef>
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {

namespace resource {
class DataPackRepository;
class IResourcePack;
} // namespace resource

namespace world::gen::density {

class DensityFunction;

/**
 * @brief 密度函数 JSON 加载器（MC 1.21.11 worldgen/density_function）
 *
 * 从数据包加载 35 个 density_function JSON，经两阶段 Holder 引用解析后注册到
 * DensityFunctionRegistry（name→shared_ptr<DensityFunction>）。
 *
 * JSON 元素三态（MC 1.21.11 DensityFunction.HOLDER_CODEC）：
 * - 裸数字   → Constant(数字)
 * - 裸字符串 → Holder 引用另一具名 density_function（RL，递归解析，共享子图）
 * - 对象     → {"type":"minecraft:xxx", ...}，按 type 调 DensityFunctionTypeRegistry 工厂
 *
 * 两阶段 Holder 解析（处理前向引用 + 共享子图 + 循环引用）：
 * - 阶段 A：遍历全部 JSON，建 name→json 表（不解析）
 * - 阶段 B：对每个 name 递归 resolveHolder：
 *   1. memo 命中 → 返回 SharedHolder(已解析 shared_ptr)
 *   2. visiting 命中 → 循环引用报错
 *   3. 解析 json（三态分发），子字段是字符串→递归 resolveHolder，是对象→resolveInline 内联
 *   4. 存入 memo（shared_ptr），返回 SharedHolder
 *
 * 噪声叶子（noise/shifted_noise/shift_a/.../old_blended_noise）解析期存 UnboundNoiseLeaf
 * 占位（构造真实噪声需 RandomState，在解析期外做），由 NoiseBindingVisitor 在
 * RandomState 组装 NoiseRouter 时替换为真实叶子。
 *
 * 加载路径: data/<namespace>/worldgen/density_function/<path>.json
 */
class DensityFunctionLoader {
public:
    /**
     * @brief 从数据包列表加载所有密度函数
     *
     * 先 clear() DensityFunctionRegistry，再两阶段解析注入，最后 markLoadedFromDatapack(true)。
     *
     * @param dataPackList 数据包列表
     * @return 加载的密度函数数量
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPackList);

    /**
     * @brief 从单个资源包加载所有密度函数
     *
     * @param pack 资源包
     * @return 加载的密度函数数量
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);

private:
    /**
     * @brief 从 JSON 对象解析单个密度函数（内联，不入 memo，不共享）
     *
     * 三态分发：裸数字→Constant，裸字符串→递归 resolveHolder，对象→TypeRegistry::create。
     * 供 DensityFunctionTypeRegistry 工厂解析子字段时回调。
     *
     * @param element JSON 元素
     * @return 密度函数，或错误
     */
    [[nodiscard]] static Result<std::unique_ptr<DensityFunction>> resolveInline(const nlohmann::json& element);

public:
    /**
     * @brief 加载后解析 DF Holder 元素（registry-backed，供 noise_settings noise_router 用）
     *
     * 在 DensityFunctionLoader::loadFromDataPackRepository 完成后调用（35 个命名 DF 已在
     * DensityFunctionRegistry）。三态分发：
     * - 裸数字 → Constant
     * - 裸字符串 → DensityFunctionRegistry::get(rl) → SharedHolder(共享 shared_ptr)；
     *   未注册则报错
     * - 对象 → DensityFunctionTypeRegistry::create（子字段经同一 registry-backed resolveInline 递归）
     *
     * 与加载期 resolveInline 的区别：字符串引用查注册表（已解析共享子图）而非 rawMap。
     *
     * @param element JSON 元素（noise_router 某字段）
     * @return 密度函数（unique_ptr，噪声叶子为 UnboundNoiseLeaf 占位），或错误
     */
    [[nodiscard]] static Result<std::unique_ptr<DensityFunction>> resolveHolderElement(const nlohmann::json& element);
};

} // namespace world::gen::density
} // namespace mc
