#pragma once

#include "JigsawPiece.hpp"
#include "../../../core/Types.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../resource/ResourceLocation.hpp"
#include <vector>
#include <memory>
#include <unordered_map>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

class JigsawPattern {
public:
    JigsawPattern(const ResourceLocation& name, const ResourceLocation& fallback);

    const ResourceLocation& getName() const { return m_name; }
    const ResourceLocation& getFallback() const { return m_fallback; }
    const JigsawPiece* getRandomPiece(math::Random& rng) const;
    size_t getNumberOfPieces() const { return m_pieces.size(); }
    bool isEmpty() const { return m_pieces.empty(); }

    void addPiece(std::unique_ptr<JigsawPiece> piece, i32 weight = 1);

private:
    ResourceLocation m_name;
    ResourceLocation m_fallback;
    std::vector<std::unique_ptr<JigsawPiece>> m_pieces;
};

class JigsawPatternRegistry {
public:
    static JigsawPatternRegistry& instance();

    void registerPattern(std::unique_ptr<JigsawPattern> pattern);
    const JigsawPattern* getPattern(const ResourceLocation& name) const;
    void clear();

private:
    JigsawPatternRegistry();
    std::unordered_map<ResourceLocation, std::unique_ptr<JigsawPattern>> m_patterns;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
