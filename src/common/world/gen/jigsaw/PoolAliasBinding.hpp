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

#include "../../../core/Types.hpp"
#include "../../../resource/ResourceLocation.hpp"
#include "../../../util/math/random/Random.hpp"
#include <memory>
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
 */
class PoolAliasBinding {
public:
    virtual ~PoolAliasBinding() = default;

    /**
     * @brief 根据随机数解析别名，返回实际模板池
     * @param rng 随机数生成器
     * @return 实际模板池的 ResourceLocation，如果没有匹配返回空
     */
    [[nodiscard]] virtual ResourceLocation resolve(math::Random& rng) const = 0;

    /**
     * @brief 获取别名的虚拟池名
     * @return 虚拟池名
     */
    [[nodiscard]] virtual const ResourceLocation& alias() const = 0;

    /**
     * @brief 克隆此别名绑定
     */
    [[nodiscard]] virtual std::unique_ptr<PoolAliasBinding> clone() const = 0;
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

    [[nodiscard]] ResourceLocation resolve(math::Random& rng) const override;
    [[nodiscard]] const ResourceLocation& alias() const override { return m_alias; }
    [[nodiscard]] std::unique_ptr<PoolAliasBinding> clone() const override
    {
        return std::make_unique<RandomPoolAliasBinding>(m_alias, m_targets);
    }

    /**
     * @brief 获取所有候选池
     */
    [[nodiscard]] const std::vector<WeightedTarget>& targets() const noexcept { return m_targets; }

private:
    ResourceLocation m_alias;
    std::vector<WeightedTarget> m_targets;
};

/**
 * @brief 随机组池别名绑定
 *
 * 从多个组中随机选择一组，然后使用该组内的所有别名绑定。
 * 同一组内的所有别名绑定共享同一个随机种子。
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
     * @param aliasName 虚拟池名（用于标识此组绑定）
     * @param groups 候选组列表
     */
    RandomGroupPoolAliasBinding(ResourceLocation aliasName, std::vector<AliasGroup> groups)
        : m_alias(std::move(aliasName))
        , m_groups(std::move(groups))
    {}

    [[nodiscard]] ResourceLocation resolve(math::Random& rng) const override;
    [[nodiscard]] const ResourceLocation& alias() const override { return m_alias; }
    [[nodiscard]] std::unique_ptr<PoolAliasBinding> clone() const override;

    /**
     * @brief 获取所有候选组
     */
    [[nodiscard]] const std::vector<AliasGroup>& groups() const noexcept { return m_groups; }

    /**
     * @brief 解析整个组，返回组内所有绑定的映射
     * @param rng 随机数生成器
     * @return 从虚拟池名到实际池名的映射
     */
    [[nodiscard]] std::vector<std::pair<ResourceLocation, ResourceLocation>> resolveGroup(math::Random& rng) const;

private:
    ResourceLocation m_alias;
    std::vector<AliasGroup> m_groups;
};

/**
 * @brief 池别名绑定集合
 *
 * 管理一组池别名绑定，在 Jigsaw 组装时应用。
 * 当遇到虚拟池名时，查询此集合获取实际池名。
 */
class PoolAliasBindings {
public:
    /**
     * @brief 添加别名绑定
     * @param binding 别名绑定
     */
    void addBinding(std::unique_ptr<PoolAliasBinding> binding);

    /**
     * @brief 解析虚拟池名
     * @param alias 虚拟池名
     * @param rng 随机数生成器
     * @return 实际池名，如果未找到别名则返回原始名称
     */
    [[nodiscard]] ResourceLocation resolve(const ResourceLocation& alias, math::Random& rng) const;

    /**
     * @brief 解析所有随机组绑定，展开为平铺的别名映射
     * @param rng 随机数生成器
     * @return 从虚拟池名到实际池名的映射
     */
    [[nodiscard]] std::vector<std::pair<ResourceLocation, ResourceLocation>> resolveAllGroups(math::Random& rng) const;

    /**
     * @brief 是否为空
     */
    [[nodiscard]] bool empty() const noexcept { return m_bindings.empty(); }

private:
    std::vector<std::unique_ptr<PoolAliasBinding>> m_bindings;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
