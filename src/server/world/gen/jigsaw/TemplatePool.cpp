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
#include "common/world/block/BlockPos.hpp"
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
    //   templates.get(random.nextInt(templates.size()))
    //
    // 这里的 `templates` 是**折叠**的 `List<Pair<StructurePoolElement, Integer>>`——
    // 每个 element 只占一项，weight 是配对里的整数、**不参与索引**。因此抽取是
    // `nextInt(条目数)`，不是 `nextInt(权重和)`：两者取值域不同，随后的映射也不同，
    // 权重再按比例随机分流。
    //
    // 此前这里用的是 `nextInt(totalWeight)` + 累加权重定位，等于把权重当成了重复份数，
    // 与原版完全不是同一套抽样——起始 piece 直接就选错（trial_chambers 的 end_1/end_2
    // 互换），后续整条拼图链跟着错位。
    const size_t index = static_cast<size_t>(rng.nextInt(static_cast<i32>(m_entries.size())));
    return m_entries[index].piece;
}

std::vector<const JigsawPiece*> TemplatePool::getShuffledPieces(math::IRandom& rng) const
{
    // 原版 `StructureTemplatePool.getShuffledTemplates` 是
    // `Util.shuffledCopy(this.templates, random)`——`templates` 是**折叠**的
    // `List<Pair<StructurePoolElement, Integer>>`，每个 element 只占一项、weight
    // 只作为配对里的整数带着走。洗牌因此只做 `size - 1` 次交换，消耗的随机数与
    // 「池里有几个条目」成正比，而不是与权重和成正比。
    //
    // 此前这里先按权重展开再洗牌，消耗量变成总权重次——trial_chambers 这类大权重池
    // 会因此让整条随机流错位，首个 piece 就选错，后续所有拼图跟着偏移（差分能力
    // structure_starts 的「双方都有但内容不同」）。
    std::vector<size_t> order(m_entries.size());
    for (size_t i = 0; i < order.size(); ++i) {
        order[i] = i;
    }
    rng.shuffle(order);

    // 洗牌之后再按权重展开：条目顺序由洗好的顺序决定，与折叠列表的语义一致。
    std::vector<const JigsawPiece*> result;
    result.reserve(static_cast<size_t>(m_totalWeight));
    for (const size_t index : order) {
        const PoolEntry& entry = m_entries[index];
        for (i32 i = 0; i < entry.weight; ++i) {
            result.push_back(entry.piece);
        }
    }

    return result;
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
