/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
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
 * 池内元素存储方式：
 * - 普通 piece：克隆后由 m_owned 持有所有权，m_pieces 存原始指针
 * - EmptyJigsawPiece（单例）：m_pieces 存 &EmptyJigsawPiece::instance()，不入 m_owned
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
     * 返回一个打乱后的拼图块指针列表，用于 Jigsaw 组装时的候选块遍历。
     * 对应 MC 1.21 StructureTemplatePool.getShuffledJigsawBlocks。
     *
     * @param rng 随机数生成器
     * @return 打乱后的拼图块指针列表
     */
    [[nodiscard]] std::vector<const JigsawPiece*> getShuffledPieces(math::Random& rng) const;

    /**
     * @brief 获取总权重（池中所有元素权重之和，展开后的元素数量）
     *
     * 对应 MC 1.21 StructureTemplatePool.getTotalWeight。原 getNumberOfPieces 名称误导
     * （返回的是权重展开后的数量，非唯一元素数量），重命名为 getTotalWeight。
     */
    size_t getTotalWeight() const noexcept { return m_pieces.size(); }

    bool isEmpty() const noexcept { return m_pieces.empty(); }

    /**
     * @brief 添加拼图块到模板池
     *
     * 按 weight 克隆 piece 并加入池。若 piece 是 EmptyJigsawPiece（clone 返回 nullptr），
     * 直接存单例 instance 指针（非拥有），不克隆。
     *
     * @param piece 拼图块（所有权转入）
     * @param weight 权重（克隆份数）
     */
    void addPiece(std::unique_ptr<JigsawPiece> piece, i32 weight);

private:
    ResourceLocation m_name;
    ResourceLocation m_fallback;

    // 持有所有权的拼图块副本（按权重展开）。EmptyJigsawPiece 单例不入此容器。
    std::vector<std::unique_ptr<JigsawPiece>> m_owned;

    // 池元素指针列表（按权重展开）。普通 piece 指向 m_owned 中的副本，
    // EmptyJigsawPiece 指向单例 instance（非拥有）。
    std::vector<const JigsawPiece*> m_pieces;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
