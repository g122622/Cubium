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

#include "TemplatePool.hpp"
#include "EmptyJigsawPiece.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "server/world/gen/jigsaw/JigsawPiece.hpp"
#include "server/world/gen/jigsaw/JigsawTransform.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

TemplatePool::TemplatePool(const ResourceLocation& name, const ResourceLocation& fallback)
    : m_name(name)
    , m_fallback(fallback)
{}

i32 TemplatePool::getMaxYSpan() const
{
    if (m_maxYSpan.has_value()) {
        return *m_maxYSpan;
    }

    // 对应 StructureTemplatePool.getMaxSize：池内元素在 Rotation.NONE 下包围盒 Y 跨度的最大值。
    // 空池（含只含 EmptyJigsawPiece 的池）为 0——与原版 orElse(0) 同义。
    i32 maxSpan = 0;
    for (const auto& entry : m_entries) {
        if (entry.piece == nullptr || entry.piece->isEmpty()) {
            continue;
        }
        const auto box = JigsawTransform::calculateBoundingBox(*entry.piece, BlockPos(0, 0, 0), Rotation::None);
        maxSpan = std::max(maxSpan, box.ySpan());
    }

    m_maxYSpan = maxSpan;
    return maxSpan;
}

void TemplatePool::addPiece(std::unique_ptr<JigsawPiece> piece, i32 weight)
{
    if (!piece || weight <= 0) {
        return;
    }

    // EmptyJigsawPiece 是单例，clone 返回 nullptr。直接存单例 instance 指针（非拥有），
    // 不克隆、不入 m_owned。对应 MC 1.21 EmptyPoolElement 在池中作为终止元素共享单例。
    const JigsawPiece* piecePtr = nullptr;
    if (piece->isEmpty()) {
        piecePtr = &EmptyJigsawPiece::instance();
    } else {
        // 普通 piece：克隆一份由 m_owned 持有。weight 仅作权重记录，不再展开克隆。
        auto clone = piece->clone();
        if (!clone) {
            return;
        }
        piecePtr = clone.get();
        m_owned.push_back(std::move(clone));
    }

    m_entries.push_back({piecePtr, weight});
    m_totalWeight += weight;
}

const JigsawPiece* TemplatePool::getRandomPiece(math::IRandom& rng) const
{
    if (m_entries.empty() || m_totalWeight <= 0) {
        return nullptr;
    }
    // 对应 MC 1.21 `StructureTemplatePool.getRandomTemplate`：
    //     templates.get(random.nextInt(templates.size()))
    // 其中 `templates` 是**按权重展开**的 `ObjectArrayList`（构造期把每个元素重复 weight 次），
    // 而 `rawTemplates` 才是折叠的 `List<Pair<Element, Integer>>`。因此抽样域是权重和，
    // 不是条目数——用条目数会把加权池抽成均匀分布，起始块与后续所有拼图整体错位。
    const i32 roll = rng.nextInt(m_totalWeight);
    i32 accumulated = 0;
    for (const auto& entry : m_entries) {
        accumulated += entry.weight;
        if (roll < accumulated) {
            return entry.piece;
        }
    }
    // 权重记账一致时不可能走到这里；返回最后一项避免空指针。
    return m_entries.back().piece;
}

std::vector<const JigsawPiece*> TemplatePool::getShuffledPieces(math::IRandom& rng) const
{
    std::vector<const JigsawPiece*> expanded;
    getShuffledPieces(rng, expanded);
    return expanded;
}

void TemplatePool::getShuffledPieces(math::IRandom& rng, std::vector<const JigsawPiece*>& expanded) const
{
    // 原版 `StructureTemplatePool.getShuffledTemplates` 是 `Util.shuffledCopy(this.templates, random)`，
    // 其中 `templates` 是**按权重展开**的列表：洗牌的规模是权重和，同一元素按权重出现多次。
    // 折叠后再洗牌会让随机数消耗与候选列表长度同时偏离原版（大权重池上尤其明显）。
    expanded.clear();
    if (m_totalWeight > 0) {
        expanded.reserve(static_cast<size_t>(m_totalWeight));
    }
    for (const auto& entry : m_entries) {
        for (i32 i = 0; i < entry.weight; ++i) {
            expanded.push_back(entry.piece);
        }
    }
    rng.shuffle(expanded);
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
