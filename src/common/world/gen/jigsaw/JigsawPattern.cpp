#include "JigsawPattern.hpp"
#include <algorithm>
#include <utility>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

JigsawPattern::JigsawPattern(const ResourceLocation& name, const ResourceLocation& fallback)
    : m_name(name)
    , m_fallback(fallback)
{
}

void JigsawPattern::addPiece(std::unique_ptr<JigsawPiece> piece, i32 weight) {
    if (piece && weight > 0) {
        for (i32 i = 0; i < weight; ++i) {
            m_pieces.push_back(piece->clone());
        }
    }
}

const JigsawPiece* JigsawPattern::getRandomPiece(math::Random& rng) const {
    if (m_pieces.empty()) {
        return nullptr;
    }
    i32 index = rng.nextInt(static_cast<i32>(m_pieces.size()));
    return m_pieces[index].get();
}

std::vector<const JigsawPiece*> JigsawPattern::getShuffledPieces(math::Random& rng) const {
    std::vector<const JigsawPiece*> result;
    result.reserve(m_pieces.size());

    // 收集所有拼图块指针
    for (const auto& piece : m_pieces) {
        result.push_back(piece.get());
    }

    // 使用 IRandom::shuffle 进行 Fisher-Yates 洗牌
    rng.shuffle(result);

    return result;
}

JigsawPatternRegistry& JigsawPatternRegistry::instance() {
    static JigsawPatternRegistry registry;
    return registry;
}

JigsawPatternRegistry::JigsawPatternRegistry() = default;

void JigsawPatternRegistry::registerPattern(std::unique_ptr<JigsawPattern> pattern) {
    if (pattern) {
        m_patterns[pattern->getName()] = std::move(pattern);
    }
}

const JigsawPattern* JigsawPatternRegistry::getPattern(const ResourceLocation& name) const {
    auto it = m_patterns.find(name);
    return it != m_patterns.end() ? it->second.get() : nullptr;
}

void JigsawPatternRegistry::clear() {
    m_patterns.clear();
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
