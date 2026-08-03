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

#include "client/resource/atlas/SpriteContents.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <memory>
#include <utility>
#include <variant>
#include <vector>

namespace mc::client::resource::atlas {

/**
 * @brief 懒解码精灵句柄
 *
 * 对齐原版 SpriteSource.Output.add(id, SpriteSupplier)。
 * 持有解码所需的引用，resolve(pack) 时产出 SpriteContents。
 *
 * 三种来源：
 * - TextureResource：从资源包读 PNG + 同名 .mcmeta（single/directory source 用）
 * - Predecoded：已解码像素（unstitch 切片 / paletted 调色板映射产出，无 mcmeta）
 */
class SpriteLoader {
public:
    /// single/directory source 用：按纹理资源位置从资源包解码
    [[nodiscard]] static SpriteLoader fromTextureResource(ResourceLocation textureLocation);

    /// unstitch/paletted source 用：直接持有已解码像素
    [[nodiscard]] static SpriteLoader fromPredecoded(SpriteContents contents);

    /**
     * @brief 解码产出精灵内容
     *
     * 对 TextureResource 来源：从 pack 读取 PNG（按 pack 优先级，后添加优先），
     * stbi 解码为 RGBA8，读取同名 .mcmeta 动画元数据。
     * 对 Predecoded 来源：直接返回已持有的像素。
     *
     * @param packs 资源包列表（仅 TextureResource 来源使用，按 rbegin 优先级查找）
     * @return 解码结果；找不到纹理返回错误
     */
    [[nodiscard]] Result<SpriteContents> resolve(const std::vector<ResourcePackPtr>& packs) const;

    /// 是否为 Predecoded 来源（unstitch/paletted，无 mcmeta）
    [[nodiscard]] bool isPredecoded() const;

private:
    struct TextureResource {
        ResourceLocation textureLocation;
    };
    struct Predecoded {
        std::shared_ptr<SpriteContents> contents;
    };
    std::variant<TextureResource, Predecoded> m_state;

    explicit SpriteLoader(std::variant<TextureResource, Predecoded> state)
        : m_state(std::move(state))
    {}

    // 内部：从单个资源包解码 PNG + mcmeta
    [[nodiscard]] static Result<SpriteContents> _decodeFromPack(IResourcePack& pack, const ResourceLocation& loc);
};

} // namespace mc::client::resource::atlas
