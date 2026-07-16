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

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include <nlohmann/json_fwd.hpp>

namespace mc::world::gen::density {

class DensityFunction;

/**
 * @brief Holder 引用解析上下文
 *
 * DensityFunctionTypeRegistry 的工厂解析子字段（argument/argument1/argument2/input/
 * shift_x/...）时，子字段可能是裸数字（→Constant）、字符串 RL（→Holder 引用，递归解析
 * 同名 density_function）、或内联对象（→递归内联解析，不入 memo 不共享）。本上下文持有
 * 两个回调，由 DensityFunctionLoader 注入：
 * - resolveHolder：按 RL 递归解析（带 memo + 循环检测），返回 SharedHolder 包装的共享子树
 * - resolveInline：内联解析一个 JSON 元素（数字/字符串/对象），返回独立 unique_ptr（不共享）
 *
 * 噪声名验证：noise/shift_a 等的 noise 字段是噪声 RL，工厂只存名字到 UnboundNoiseLeaf
 * 占位（构造真实 NormalNoise 需 RandomState，在解析期外做）。
 */
struct ResolveContext {
    /** 解析一个 DF JSON 元素为独立 unique_ptr（内联，不入 memo）。元素可为 number/string/object */
    std::function<Result<std::unique_ptr<DensityFunction>>(const nlohmann::json& element)> resolveInline;
};

/**
 * @brief 密度函数类型注册表（MC 1.21.11 worldgen/density_function type）
 *
 * type 字符串 → 工厂映射。DensityFunctionLoader 解析 JSON 时读 `type` 字段（去 minecraft:
 * 前缀），调 create(type, json, ctx) 得 unique_ptr<DensityFunction>。
 *
 * 严格报错策略：未注册 type 返回 Error（中断该文件加载），便于定位未实现 type。
 *
 * 工厂职责：解析本 type 自身字段 + 用 ctx.resolveInline 解析子 DF 字段。噪声叶子 type
 * （noise/shifted_noise/shift_a/...）只存 noise 名到 UnboundNoiseLeaf 占位。
 */
class DensityFunctionTypeRegistry {
public:
    using Factory =
        std::function<Result<std::unique_ptr<DensityFunction>>(const nlohmann::json& json, const ResolveContext& ctx)>;

    static DensityFunctionTypeRegistry& instance();

    /**
     * @brief 注册 type 工厂
     * @param type type 名（不含 minecraft: 前缀，如 "add"）
     * @param factory 工厂函数
     */
    void registerType(const std::string& type, Factory factory);

    /**
     * @brief 按 type 构造密度函数
     *
     * 严格报错：type 未注册时返回 Error。type 可带或不带 minecraft: 前缀。
     *
     * @param type type 名
     * @param json 该 density_function JSON 对象（含 type 字段本身）
     * @param ctx Holder 解析上下文
     * @return 构造的密度函数，或错误
     */
    [[nodiscard]] Result<std::unique_ptr<DensityFunction>> create(
        const std::string& type, const nlohmann::json& json, const ResolveContext& ctx) const;

    /** 是否已注册指定 type */
    [[nodiscard]] bool has(const std::string& type) const noexcept;

    /** 清除所有已注册工厂 */
    void clear() noexcept;

private:
    DensityFunctionTypeRegistry() = default;
    ~DensityFunctionTypeRegistry() = default;
    DensityFunctionTypeRegistry(const DensityFunctionTypeRegistry&) = delete;
    DensityFunctionTypeRegistry& operator=(const DensityFunctionTypeRegistry&) = delete;

    std::unordered_map<std::string, Factory> m_factories;
};

/**
 * @brief 注册全部内置 density_function type 工厂
 *
 * 在 DensityFunctionLoader 加载数据包前调用。覆盖原版 1.21.11 全部 type：
 * constant/y_clamped_gradient/clamp/mapped(7)/twoarg(4)/lerp/noise/shifted_noise/
 * shift_a/shift_b/shift/weird_scaled_sampler/old_blended_noise/range_choice/spline/
 * end_islands/beardifier/interpolated/cache_once/cache_all_in_cell/flat_cache/cache_2d/
 * blend_alpha/blend_offset/blend_density。
 */
void initializeBuiltinDensityFunctionTypes();

} // namespace mc::world::gen::density
