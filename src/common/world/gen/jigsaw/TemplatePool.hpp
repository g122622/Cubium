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

#include "JigsawPiece.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

class EmptyJigsawPiece;

/**
 * @brief 模板池
 *
 * 对应 MC 1.21 的 StructureTemplatePool。持有带权重的拼图块列表，
 * 提供 getRandomPiece/getShuffledPieces 用于 Jigsaw 组装时的候选块选择。
 *
 * 存储采用折叠条目 (piece, weight)：每个唯一 piece 只克隆一份，
 * 权重在 getShuffledPieces/getRandomPiece 时按需展开。对齐 MC 原版
 * StructureTemplatePool 的 List<Pair<PoolElement, Integer>> 存储 +
 * getShuffledJigsawBlocks 按权重展开的语义。
 *
 * 条目存储方式：
 * - 普通 piece：clone 一份由 m_owned 持有，m_entries 存指针 + weight
 * - EmptyJigsawPiece（单例）：m_entries 存 &instance() 指针 + weight，不入 m_owned
 */
class TemplatePool {
public:
    TemplatePool(const ResourceLocation& name, const ResourceLocation& fallback);

    const ResourceLocation& getName() const noexcept { return m_name; }

    /**
     * @brief 获取回退模板池名称
     *
     * 当本池无可用候选块时，回退到此池继续选择。
     * 对应 MC 1.21 StructureTemplatePool.fallback。
     */
    const ResourceLocation& getFallback() const noexcept { return m_fallback; }

    /**
     * @brief 获取随机拼图块
     * @param rng 随机数生成器
     * @return 随机选择的拼图块指针，如果模板池为空返回 nullptr
     */
    const JigsawPiece* getRandomPiece(math::Random& rng) const;

    /**
     * @brief 获取打乱后的拼图块列表
     *
     * 按权重展开条目（每个条目生成 weight 份指针），对展开后的列表进行
     * Fisher-Yates 洗牌。返回的指针指向池内拥有的 piece。
     *
     * 对应 MC 1.21 StructureTemplatePool.getShuffledJigsawBlocks。
     *
     * @param rng 随机数生成器
     * @return 打乱后的拼图块指针列表
     */
    [[nodiscard]] std::vector<const JigsawPiece*> getShuffledPieces(math::Random& rng) const;

    /**
     * @brief 获取总权重（Σ 所有条目 weight）
     *
     * 对应 MC 1.21 StructureTemplatePool.getTotalWeight。
     */
    size_t getTotalWeight() const noexcept { return static_cast<size_t>(m_totalWeight); }

    bool isEmpty() const noexcept { return m_entries.empty(); }

    /**
     * @brief 添加拼图块到模板池
     *
     * 克隆 piece 一份加入池（EmptyJigsawPiece 单例不克隆，直接存实例指针）。
     * weight 仅作为权重记录，不再展开克隆。
     *
     * @param piece 拼图块（所有权转入）
     * @param weight 权重
     */
    void addPiece(std::unique_ptr<JigsawPiece> piece, i32 weight);

private:
    /**
     * @brief 折叠条目：piece 指针 + 权重
     *
     * piece 指向 m_owned 中的副本（普通 piece）或单例 instance（EmptyJigsawPiece）。
     */
    struct PoolEntry {
        const JigsawPiece* piece;
        i32 weight;
    };

    ResourceLocation m_name;
    ResourceLocation m_fallback;

    // 持有所有权的拼图块副本（每个唯一 piece 一份）。EmptyJigsawPiece 单例不入此容器。
    std::vector<std::unique_ptr<JigsawPiece>> m_owned;

    // 折叠条目列表。普通 piece 指向 m_owned 中的副本，
    // EmptyJigsawPiece 指向单例 instance（非拥有）。
    std::vector<PoolEntry> m_entries;

    // Σ 所有条目 weight，getTotalWeight 直接返回。
    i32 m_totalWeight = 0;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
