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
 */

#pragma once

#include "client/resource/atlas/AtlasSource.hpp"
#include "client/resource/atlas/IdentifierPattern.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace mc::client::resource::atlas {

// ============================================================================
// minecraft:single —— 加载单个纹理，sprite 名可独立于资源名
// ============================================================================
class SingleFileSource final : public AtlasSource {
public:
    SingleFileSource(ResourceLocation resource, ResourceLocation sprite)
        : m_resource(std::move(resource))
        , m_sprite(std::move(sprite))
    {}

    [[nodiscard]] Result<void> run(IResourcePack& pack, SpriteSourceOutput& output) const override;
    [[nodiscard]] std::string describe() const override;

private:
    ResourceLocation m_resource;
    ResourceLocation m_sprite;
};

// ============================================================================
// minecraft:directory —— 枚举 textures/<source>/**/*.png
// ============================================================================
class DirectoryListerSource final : public AtlasSource {
public:
    DirectoryListerSource(std::string source, std::string prefix)
        : m_source(std::move(source))
        , m_prefix(std::move(prefix))
    {}

    [[nodiscard]] Result<void> run(IResourcePack& pack, SpriteSourceOutput& output) const override;
    [[nodiscard]] std::string describe() const override;

    const std::string& source() const { return m_source; }
    const std::string& prefix() const { return m_prefix; }

private:
    std::string m_source;
    std::string m_prefix;
};

// ============================================================================
// minecraft:filter —— 从已累积 sprite 集合 removeAll 匹配项
// ============================================================================
class FilterSource final : public AtlasSource {
public:
    explicit FilterSource(IdentifierPattern pattern)
        : m_pattern(std::move(pattern))
    {}

    [[nodiscard]] Result<void> run(IResourcePack& pack, SpriteSourceOutput& output) const override;
    [[nodiscard]] std::string describe() const override;

private:
    IdentifierPattern m_pattern;
};

// ============================================================================
// minecraft:unstitch —— 从大图切出多个子纹理
// ============================================================================
struct UnstitchRegion {
    ResourceLocation sprite;
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
};

class UnstitcherSource final : public AtlasSource {
public:
    UnstitcherSource(ResourceLocation resource, std::vector<UnstitchRegion> regions, double divisorX, double divisorY)
        : m_resource(std::move(resource))
        , m_regions(std::move(regions))
        , m_divisorX(divisorX)
        , m_divisorY(divisorY)
    {}

    [[nodiscard]] Result<void> run(IResourcePack& pack, SpriteSourceOutput& output) const override;
    [[nodiscard]] std::string describe() const override;

private:
    ResourceLocation m_resource;
    std::vector<UnstitchRegion> m_regions;
    double m_divisorX = 1.0;
    double m_divisorY = 1.0;
};

// ============================================================================
// minecraft:paletted_permutations —— 调色板映射批量生成衍生纹理
// ============================================================================
class PalettedPermutationsSource final : public AtlasSource {
public:
    PalettedPermutationsSource(std::vector<ResourceLocation> textures,
        ResourceLocation paletteKey,
        std::map<std::string, ResourceLocation> permutations,
        std::string separator)
        : m_textures(std::move(textures))
        , m_paletteKey(std::move(paletteKey))
        , m_permutations(std::move(permutations))
        , m_separator(std::move(separator))
    {}

    [[nodiscard]] Result<void> run(IResourcePack& pack, SpriteSourceOutput& output) const override;
    [[nodiscard]] std::string describe() const override;

private:
    std::vector<ResourceLocation> m_textures;
    ResourceLocation m_paletteKey;
    std::map<std::string, ResourceLocation> m_permutations;
    std::string m_separator;
};

} // namespace mc::client::resource::atlas
