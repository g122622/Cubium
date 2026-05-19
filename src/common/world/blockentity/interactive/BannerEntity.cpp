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

#include "world/blockentity/interactive/BannerEntity.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/IWorld.hpp"

namespace mc {
namespace blockentity {

// ========== BannerEntity 实现 ==========

BannerEntity::BannerEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Banner, pos)
{}

BannerEntity::~BannerEntity() = default;

bool BannerEntity::addPattern(const BannerPattern& pattern)
{
    if (static_cast<i32>(m_patterns.size()) >= MAX_PATTERNS) {
        return false;
    }

    m_patterns.push_back(pattern);
    setChanged();
    return true;
}

void BannerEntity::setPatterns(const std::vector<BannerPattern>& patterns)
{
    m_patterns.clear();
    for (const auto& pattern : patterns) {
        if (static_cast<i32>(m_patterns.size()) < MAX_PATTERNS) {
            m_patterns.push_back(pattern);
        }
    }
    setChanged();
}

bool BannerEntity::removeTopPattern()
{
    if (m_patterns.empty()) {
        return false;
    }

    m_patterns.pop_back();
    setChanged();
    return true;
}

void BannerEntity::clearPatterns()
{
    m_patterns.clear();
    setChanged();
}

void BannerEntity::setBaseColor(DyeColor color)
{
    if (m_baseColor != color) {
        m_baseColor = color;
        setChanged();
    }
}

std::string BannerEntity::getTextureName() const
{
    // 生成纹理名称，用于渲染
    // 格式: banner_<base_color>[_<pattern_hash>_<color>]*
    // 每个图案追加 "_<hash>_<color>"
    std::string name = "banner_" + std::to_string(static_cast<i32>(m_baseColor));

    // 添加图案信息
    for (const auto& pattern : m_patterns) {
        name +=
            "_" + BannerPatterns::getHashName(pattern.pattern) + "_" + std::to_string(static_cast<i32>(pattern.color));
    }

    return name;
}

void BannerEntity::tick(IWorld& world)
{
    MC_UNUSED(world);
    // 旗帜不需要tick更新
}

bool BannerEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    // 加载底色
    if (data.contains("base_color")) {
        m_baseColor = static_cast<DyeColor>(data["base_color"].get<i32>());
    }

    // 加载图案
    m_patterns.clear();
    if (data.contains("patterns")) {
        const auto& patternsJson = data["patterns"];
        if (patternsJson.is_array()) {
            for (const auto& patternJson : patternsJson) {
                BannerPattern pattern;
                if (patternJson.contains("pattern")) {
                    // 从哈希名解析图案类型
                    std::string hashName = patternJson["pattern"].get<std::string>();
                    pattern.pattern = BannerPatterns::byHash(hashName);
                }
                if (patternJson.contains("color")) {
                    pattern.color = static_cast<DyeColor>(patternJson["color"].get<i32>());
                }
                m_patterns.push_back(pattern);
            }
        }
    }

    return true;
}

void BannerEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    // 保存底色
    data["base_color"] = static_cast<i32>(m_baseColor);

    // 保存图案
    nlohmann::json patternsJson = nlohmann::json::array();
    for (const auto& pattern : m_patterns) {
        nlohmann::json patternJson;
        patternJson["pattern"] = BannerPatterns::getHashName(pattern.pattern);
        patternJson["color"] = static_cast<i32>(pattern.color);
        patternsJson.push_back(patternJson);
    }
    data["patterns"] = patternsJson;
}

std::unique_ptr<BlockEntity> BannerEntity::clone() const
{
    auto clone = std::make_unique<BannerEntity>(m_pos);
    clone->m_patterns = m_patterns;
    clone->m_baseColor = m_baseColor;
    return clone;
}

} // namespace blockentity
} // namespace mc
