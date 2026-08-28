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
#include "common/world/gen/jigsaw/JigsawPiece.hpp"
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

const JigsawPiece* TemplatePool::getRandomPiece(math::Random& rng) const
{
    if (m_entries.empty() || m_totalWeight <= 0) {
        return nullptr;
    }
    // 加权随机选择：生成 [0, m_totalWeight) 随机数，遍历累加权重定位条目。
    i32 target = rng.nextInt(m_totalWeight);
    for (const auto& entry : m_entries) {
        target -= entry.weight;
        if (target < 0) {
            return entry.piece;
        }
    }
    return m_entries.back().piece;
}

std::vector<const JigsawPiece*> TemplatePool::getShuffledPieces(math::Random& rng) const
{
    std::vector<const JigsawPiece*> result;
    result.reserve(static_cast<size_t>(m_totalWeight));

    // 按权重展开指针：每个条目生成 weight 份指针。
    for (const auto& entry : m_entries) {
        for (i32 i = 0; i < entry.weight; ++i) {
            result.push_back(entry.piece);
        }
    }

    // Fisher-Yates 洗牌
    rng.shuffle(result);

    return result;
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
