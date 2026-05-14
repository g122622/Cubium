#pragma once

#include "../../../core/Types.hpp"
#include "../../../resource/ResourceLocation.hpp"
#include "../../../util/math/random/Random.hpp"
#include "JigsawPiece.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

class JigsawPattern {
public:
    JigsawPattern(const ResourceLocation& name, const ResourceLocation& fallback);

    const ResourceLocation& getName() const { return m_name; }
    const ResourceLocation& getFallback() const { return m_fallback; }

    /**
     * @brief 获取随机拼图块
     * @param rng 随机数生成器
     * @return 随机选择的拼图块指针，如果模板池为空返回 nullptr
     */
    const JigsawPiece* getRandomPiece(math::Random& rng) const;

    /**
     * @brief 获取打乱后的拼图块列表
     *
     * MC 1.16.5: getShuffledPieces(Random)
     * 返回一个打乱后的拼图块列表副本，用于Jigsaw组装时的候选块遍历。
     *
     * @param rng 随机数生成器
     * @return 打乱后的拼图块指针列表
     */
    [[nodiscard]] std::vector<const JigsawPiece*> getShuffledPieces(math::Random& rng) const;

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
