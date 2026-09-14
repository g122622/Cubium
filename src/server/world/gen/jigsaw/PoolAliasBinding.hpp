/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permitted persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

/**
 * @brief 池别名绑定基类
 *
 * 在 Jigsaw 结构组装过程中，将一个虚拟池名映射到实际的模板池。
 * 这允许同一个结构在不同生成实例中使用不同的池，实现随机化。
 *
 * 典型用途：试炼密室中的刷怪笼类型随机化。
 * 模板池中引用 "spawner/contents/melee" 等虚拟名，
 * 组装时通过别名绑定随机替换为 "spawner/melee/zombie" 等实际池。
 *
 * 对应 MC 1.21 net.minecraft.world.level.levelgen.structure.pools.alias.PoolAliasBinding。
 * 子类实现 forEachResolved(rng, callback)：一次性解析所有 alias→target 映射，
 * 由 PoolAliasLookup::create 在组装开始时调用，生成不可变映射表。
 */
class PoolAliasBinding {
public:
    /// 别名→目标映射回调（BiConsumer<alias, target>）
    using Resolver = std::function<void(const ResourceLocation&, const ResourceLocation&)>;

    virtual ~PoolAliasBinding() = default;

    /**
     * @brief 一次性解析本绑定，通过 callback 输出所有 alias→target 映射
     *
     * DirectPoolAliasBinding 输出 1 个映射；RandomPoolAliasBinding 输出 1 个（按权重随机选）；
     * RandomGroupPoolAliasBinding 选定一个组后输出组内所有绑定的映射。
     * callback 可能被调用 0 次或多次。
     *
     * @param rng 随机数生成器（用于随机/随机组绑定的解析）
     * @param callback 接收 (alias, target) 对
     */
    virtual void forEachResolved(math::Random& rng, const Resolver& callback) const = 0;

    /**
     * @brief 获取别名的虚拟池名
     *
     * RandomGroupPoolAliasBinding 无单一别名（组内各绑定各有别名），返回其组标识名（仅用于调试）。
     * @return 虚拟池名
     */
    [[nodiscard]] virtual const ResourceLocation& alias() const = 0;

    /**
     * @brief 克隆此别名绑定
     */
    [[nodiscard]] virtual std::unique_ptr<PoolAliasBinding> clone() const = 0;
};

/**
 * @brief 直接池别名绑定（一对一映射）
 *
 * 对应 MC 1.21 DirectPoolAlias。alias 固定映射到 target，无随机性。
 */
class DirectPoolAliasBinding final : public PoolAliasBinding {
public:
    DirectPoolAliasBinding(ResourceLocation aliasName, ResourceLocation targetName)
        : m_alias(std::move(aliasName))
        , m_target(std::move(targetName))
    {}

    void forEachResolved(math::Random& /*rng*/, const Resolver& callback) const override
    {
        callback(m_alias, m_target);
    }

    [[nodiscard]] const ResourceLocation& alias() const override { return m_alias; }
    [[nodiscard]] const ResourceLocation& target() const noexcept { return m_target; }

    [[nodiscard]] std::unique_ptr<PoolAliasBinding> clone() const override
    {
        return std::make_unique<DirectPoolAliasBinding>(m_alias, m_target);
    }

private:
    ResourceLocation m_alias;
    ResourceLocation m_target;
};

/**
 * @brief 随机池别名绑定
 *
 * 从一组候选池中按权重随机选择一个作为实际池。
 *
 * 示例（试炼密室近战型刷怪笼）：
 *   alias = "minecraft:trial_chambers/spawner/contents/melee"
 *   targets = {("minecraft:trial_chambers/spawner/melee/zombie", 1),
 *              ("minecraft:trial_chambers/spawner/melee/husk", 1),
 *              ("minecraft:trial_chambers/spawner/melee/spider", 1)}
 */
class RandomPoolAliasBinding final : public PoolAliasBinding {
public:
    /**
     * @brief 带权重的候选池
     */
    struct WeightedTarget {
        ResourceLocation pool; ///< 目标模板池
        i32 weight;            ///< 权重
    };

    /**
     * @brief 构造随机池别名绑定
     * @param aliasName 虚拟池名
     * @param targets 带权重的候选池列表
     */
    RandomPoolAliasBinding(ResourceLocation aliasName, std::vector<WeightedTarget> targets)
        : m_alias(std::move(aliasName))
        , m_targets(std::move(targets))
    {}

    void forEachResolved(math::Random& rng, const Resolver& callback) const override;

    [[nodiscard]] const ResourceLocation& alias() const override { return m_alias; }
    [[nodiscard]] std::unique_ptr<PoolAliasBinding> clone() const override
    {
        return std::make_unique<RandomPoolAliasBinding>(m_alias, m_targets);
    }

    /**
     * @brief 获取所有候选池
     */
    [[nodiscard]] const std::vector<WeightedTarget>& targets() const noexcept { return m_targets; }

    /**
     * @brief 按权重随机解析为单个目标池（仅供 forEachResolved 内部及旧调用点使用）
     * @param rng 随机数生成器
     * @return 实际目标池；候选为空时返回 alias 自身
     */
    [[nodiscard]] ResourceLocation resolve(math::Random& rng) const;

private:
    ResourceLocation m_alias;
    std::vector<WeightedTarget> m_targets;
};

/**
 * @brief 随机组池别名绑定
 *
 * 从多个组中随机选择一组，然后解析该组内所有别名绑定。
 * 同一组内的所有别名绑定共享同一个随机选择结果（一次性解析）。
 *
 * 示例（试炼密室刷怪笼组）：
 *   组1：melee=zombie, small_melee=slime, ranged=skeleton
 *   组2：melee=husk, small_melee=cave_spider, ranged=stray
 *   组3：melee=spider, small_melee=silverfish, ranged=poison_skeleton
 *
 * 选择一个组后，该组内所有别名同时生效。
 */
class RandomGroupPoolAliasBinding final : public PoolAliasBinding {
public:
    /**
     * @brief 一个别名组
     */
    struct AliasGroup {
        std::vector<std::unique_ptr<PoolAliasBinding>> bindings; ///< 组内的别名绑定
        i32 weight;                                              ///< 组权重
    };

    /**
     * @brief 构造随机组池别名绑定
     * @param aliasName 组标识名（仅用于调试，组内各绑定各有自己的 alias）
     * @param groups 候选组列表
     */
    RandomGroupPoolAliasBinding(ResourceLocation aliasName, std::vector<AliasGroup> groups)
        : m_alias(std::move(aliasName))
        , m_groups(std::move(groups))
    {}

    void forEachResolved(math::Random& rng, const Resolver& callback) const override;

    [[nodiscard]] const ResourceLocation& alias() const override { return m_alias; }
    [[nodiscard]] std::unique_ptr<PoolAliasBinding> clone() const override;

    /**
     * @brief 获取所有候选组
     */
    [[nodiscard]] const std::vector<AliasGroup>& groups() const noexcept { return m_groups; }

private:
    ResourceLocation m_alias;
    std::vector<AliasGroup> m_groups;
};

/**
 * @brief 池别名绑定集合
 *
 * 管理一组池别名绑定，在 Jigsaw 组装开始时通过 PoolAliasLookup::create 解析为
 * 不可变映射表，组装过程中通过 lookup(alias) 查询实际池名。
 */
class PoolAliasBindings {
public:
    /**
     * @brief 添加别名绑定
     * @param binding 别名绑定
     */
    void addBinding(std::unique_ptr<PoolAliasBinding> binding);

    /**
     * @brief 一次性解析所有绑定为 alias→target 映射
     * @param rng 随机数生成器
     * @param callback 接收每个 (alias, target) 对
     */
    void forEachResolved(math::Random& rng, const PoolAliasBinding::Resolver& callback) const;

    /**
     * @brief 是否为空
     */
    [[nodiscard]] bool empty() const noexcept { return m_bindings.empty(); }

    /**
     * @brief 获取所有绑定（只读）
     */
    [[nodiscard]] const std::vector<std::unique_ptr<PoolAliasBinding>>& bindings() const noexcept { return m_bindings; }

private:
    std::vector<std::unique_ptr<PoolAliasBinding>> m_bindings;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
