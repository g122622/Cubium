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
#include "common/util/TriState.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/attribute/EnvironmentAttribute.hpp"
#include "common/world/attribute/EnvironmentAttributeLayer.hpp"
#include "common/world/attribute/EnvironmentAttributeMap.hpp"
#include "common/world/level/MoonPhase.hpp"
#include "common/world/timeline/AttributeTrackSampler.hpp"

#include <any>
#include <concepts>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace attribute {

class EnvironmentAttributeSystem;

/**
 * @brief 环境属性系统构建器
 *
 * MC 1.21.11 class EnvironmentAttributeSystem.Builder。
 * 按属性收集层：常量层（维度/群系属性表）、时间线层（轨道采样器)、位置层。
 * 层序即添加顺序，与 vanilla 的"维度 → 群系 → 时间线 → 天气"一致。
 *
 * C++ 的类型擦除：每个属性对应一个"求值函数"（`AttributeEntry::eval`），
 * 层与求值结果都以 `std::any` 承载（保存具体属性值类型），按属性对象地址索引。
 * 这样无需为每种属性值类型维护独立的采样器类型。
 *
 * 运行时状态：求值上下文（位置、tick 序号）由下层调用传入（`eval` 的参数），
 * 而不是由层闭包捕获——系统因此是值语义、可自由拷贝的对象。
 */
class EnvironmentAttributeSystemBuilder {
public:
    /**
     * @brief 追加常量层：按属性表声明的修改器与参数修改值
     *
     * @tparam Value 属性值类型
     * @param attribute 环境属性
     * @param map 属性表（维度或群系声明的常量属性）
     * @return 构建器自身
     */
    template <typename Value>
    EnvironmentAttributeSystemBuilder& addConstantLayer(
        const EnvironmentAttribute<Value>& attribute, const EnvironmentAttributeMap& map);

    /**
     * @brief 追加位置层
     *
     * @tparam Value 属性值类型
     * @param attribute 环境属性
     * @param apply 应用函数：从当前值计算新值
     * @return 构建器自身
     */
    template <typename Value>
    EnvironmentAttributeSystemBuilder& addPositionalLayer(
        const EnvironmentAttribute<Value>& attribute, std::function<Value(const Value&, const Vector3&)> apply);

    /**
     * @brief 追加时间线层（属性轨道采样器）
     *
     * 采样器的缓存序号与系统一致：同一 tick 内多次求值只采样一次。
     *
     * @tparam Value 属性值类型
     * @tparam Argument 修改器参数类型
     * @param attribute 环境属性
     * @param sampler 采样器（由 Timeline::createTrackSampler 创建）
     * @return 构建器自身
     */
    template <typename Value, typename Argument>
    EnvironmentAttributeSystemBuilder& addTimeBasedLayer(const EnvironmentAttribute<Value>& attribute,
        std::unique_ptr<timeline::AttributeTrackSampler<Value, Argument>> sampler);

    /**
     * @brief 追加任意层（实现 EnvironmentAttributeLayer<Value> 的 TimeBased / Constant / Positional）
     *
     * @tparam Value 属性值类型
     * @param attribute 环境属性
     * @param layer 层
     * @return 构建器自身
     */
    template <typename Value, typename Layer>
        requires requires(Layer& layer) {
            { layer.applyConstant(std::declval<Value>()) } -> std::same_as<Value>;
        }
    EnvironmentAttributeSystemBuilder& addLayer(
        const EnvironmentAttribute<Value>& attribute, std::shared_ptr<const Layer> layer);

    template <typename Value, typename Layer>
        requires requires(Layer& layer) {
            { layer.applyTimeBased(std::declval<Value>(), i64{}) } -> std::same_as<Value>;
        }
    EnvironmentAttributeSystemBuilder& addLayer(
        const EnvironmentAttribute<Value>& attribute, std::shared_ptr<const Layer> layer);

    template <typename Value, typename Layer>
        requires requires(Layer& layer) {
            { layer.applyPositional(std::declval<Value>(), std::declval<const Vector3&>()) } -> std::same_as<Value>;
        }
    EnvironmentAttributeSystemBuilder& addLayer(
        const EnvironmentAttribute<Value>& attribute, std::shared_ptr<const Layer> layer);

    /// 单次求值的上下文
    struct _EvalContext {
        /// 是否按位置求值（false 时跳过位置层）
        bool positional;
        /// 当 tick 缓存序号（时间线层使用）；-1 表示不缓存（纯函数求值，可从 const 对象复制后使用）
        i64 tickId;
        /// 查询位置（positional 为 false 时无意义）
        Vector3 pos;
    };

    /// 单个属性待烘焙的层序列
    struct AttributeLayers {
        /// 每层签名为"输入值槽 → 输出值槽"
        std::vector<std::function<std::any(const std::any&, const _EvalContext&)>> layers;
        /// 各层自带的 tick 缓存刷新回调（系统失效缓存时一并调用）
        std::vector<std::function<void()>> resetTickCaches;
        /// 是否整体依赖位置（任一位置层）
        bool affectedByPosition = false;
    };

    /**
     * @brief 烘焙全部属性：把层序列固化为求值函数
     *
     * @return 构建完成的环境属性系统
     */
    [[nodiscard]] EnvironmentAttributeSystem build() const;

private:
    friend class EnvironmentAttributeSystem;

    std::map<const void*, AttributeLayers> m_layers;
};

/**
 * @brief 环境属性系统
 *
 * MC 1.21.11 class EnvironmentAttributeSystem。
 * 保存烘焙后的属性求值函数，对外按属性类型查询值；非位置属性带 per-tick 缓存。
 *
 * 系统创建后不可变（层在构建时固定），因此可以安全地跨线程只读共享；
 * per-tick 缓存以可变状态持有，仅在所属线程的 tick 流程内访问。
 */
class EnvironmentAttributeSystem {
public:
    EnvironmentAttributeSystem() = default;

    /**
     * @brief 拷贝：复制层与当 tick 缓存
     *
     * 层对象共享（只读），求值函数与缓存按属性复制。
     */
    EnvironmentAttributeSystem(const EnvironmentAttributeSystem& other);

    /**
     * @brief 拷贝赋值：同拷贝构造语义
     */
    EnvironmentAttributeSystem& operator=(const EnvironmentAttributeSystem& other);

    EnvironmentAttributeSystem(EnvironmentAttributeSystem&&) noexcept = default;
    EnvironmentAttributeSystem& operator=(EnvironmentAttributeSystem&&) noexcept = default;
    ~EnvironmentAttributeSystem() = default;

    /**
     * @brief 按属性查询值（与位置无关）
     *
     * @param attribute 环境属性
     * @return 当 tick 的值；该属性没有任何层时返回其默认值
     */
    template <typename Value>
    [[nodiscard]] Value getDimensionValue(const EnvironmentAttribute<Value>& attribute) const;

    /**
     * @brief 按属性与位置查询值
     *
     * @param attribute 环境属性
     * @param pos 世界坐标
     * @return 属性值；该属性没有任何层时返回其默认值
     */
    template <typename Value>
    [[nodiscard]] Value getValue(const EnvironmentAttribute<Value>& attribute, const Vector3& pos) const;

    /// 判断属性是否受位置影响
    template <typename Value>
    [[nodiscard]] bool isAffectedByPosition(const EnvironmentAttribute<Value>& attribute) const;

    /// 失效所有属性的当 tick 缓存（每个 tick 开始时调用一次）
    void invalidateTickCache() const;

    /// 已登记属性数量
    [[nodiscard]] size_t attributeCount() const noexcept
    {
        return m_attributes.size();
    }

private:
    friend class EnvironmentAttributeSystemBuilder;

    /**
     * @brief 属性求值记录
     *
     * 缓存序号由系统持有：每次 invalidateTickCache 递增，层（尤其是时间线采样器）
     * 以同一序号作为自身的缓存判据，保证"每 tick 只采样一次"。
     */
    struct AttributeEntry {
        /// 求值函数：输入为本属性默认值（值槽），输出为应用全部层后的值槽
        std::function<std::any(const std::any& base, const EnvironmentAttributeSystemBuilder::_EvalContext& context)>
            eval;
        bool affectedByPosition;
        mutable std::any cachedTickValue;
        /// 附着状态（层自带的 tick 缓存），每次失效缓存时一并刷新
        std::shared_ptr<std::vector<std::function<void()>>> resetTickCaches;
    };

    std::map<const void*, AttributeEntry> m_attributes;
    mutable i64 m_tickId = 0;
};

} // namespace attribute
} // namespace world
} // namespace mc

#include "common/world/attribute/EnvironmentAttributeSystem.inl"
