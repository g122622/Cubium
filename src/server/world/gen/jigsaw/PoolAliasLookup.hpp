/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to the permitted persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "PoolAliasBinding.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include <unordered_map>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

/**
 * @brief 池别名查找表
 *
 * 对应 MC 1.21 net.minecraft.world.level.levelgen.structure.pools.alias.PoolAliasLookup。
 *
 * 在 Jigsaw 组装开始时，将 PoolAliasBindings 中的所有绑定一次性解析为不可变的
 * alias→target 映射表。组装过程中通过 lookup(alias) 查询实际池名，
 * 若无匹配则返回 alias 自身（PoolAliasLookup.EMPTY 的恒等行为）。
 *
 * 一次性解析保证同一结构内同一别名多次出现时解析结果一致（例如 RandomPoolAliasBinding
 * 只随机一次，后续 lookup 命中缓存），对应 MC 用 forkPositional().at(pos) 派生确定种子
 * 再 forEachResolved 构建 ImmutableMap 的语义。
 */
class PoolAliasLookup {
public:
    /**
     * @brief 空查找表（恒等映射：lookup 总是返回输入）
     *
     * 对应 MC PoolAliasLookup.EMPTY。用于无别名绑定的结构。
     */
    PoolAliasLookup() = default;

    /**
     * @brief 从别名绑定集合构建查找表
     *
     * 使用 rng 一次性解析所有绑定（RandomPoolAliasBinding/RandomGroupPoolAliasBinding 的
     * 随机选择在此发生），构建不可变映射表。
     *
     * @param bindings 别名绑定集合
     * @param rng 随机数生成器
     */
    explicit PoolAliasLookup(const PoolAliasBindings& bindings, math::Random& rng)
    {
        bindings.forEachResolved(
            rng, [this](const ResourceLocation& alias, const ResourceLocation& target) { m_map[alias] = target; });
    }

    /**
     * @brief 查询别名对应的实际池名
     * @param alias 虚拟池名
     * @return 实际池名；若无匹配别名则返回 alias 自身（恒等映射）
     */
    [[nodiscard]] const ResourceLocation& lookup(const ResourceLocation& alias) const
    {
        auto it = m_map.find(alias);
        if (it != m_map.end()) {
            return it->second;
        }
        return alias;
    }

    /**
     * @brief 是否为空（无任何别名映射）
     */
    [[nodiscard]] bool isEmpty() const noexcept { return m_map.empty(); }

private:
    std::unordered_map<ResourceLocation, ResourceLocation> m_map;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
