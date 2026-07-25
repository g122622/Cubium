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

#include "client/world/ClientMapDataCache.hpp"

#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"

#include <nlohmann/json.hpp>

namespace mc::client {

namespace {

/// 把 MapDecorationWire 还原成 MapDecoration（registryId+1 → DecorationType，name JSON → ITextComponent）
[[nodiscard]] mc::world::map::MapDecoration wireToDecoration(const mc::network::ir::play::MapDecorationWire& w)
{
    // holderRegistry 编码为 VarInt(registryId+1)；registryId 即 DecorationType 枚举值
    const u32 encoded = w.typeRegistryIdPlusOne;
    u8 registryId = 0;
    if (encoded > 0) {
        registryId = static_cast<u8>(encoded - 1);
    }
    const auto type = mc::world::map::decorationTypeByIcon(registryId);

    std::unique_ptr<mc::text::ITextComponent> name;
    if (w.name.has_value() && !w.name->empty()) {
        try {
            const auto json = nlohmann::json::parse(std::string(w.name->begin(), w.name->end()));
            name = mc::text::ITextComponent::fromJson(json);
        }
        catch (const nlohmann::json::exception&) {
            // JSON 解析失败，回退为纯文本组件
            name = std::make_unique<mc::text::StringTextComponent>(std::string(w.name->begin(), w.name->end()));
        }
    }

    return mc::world::map::MapDecoration(type, w.x, w.y, w.rotation, std::move(name));
}

} // namespace

bool ClientMapDataCache::apply(const mc::network::ir::play::MapItemData& pkt)
{
    auto it = m_mapData.find(pkt.mapId);
    if (it == m_mapData.end()) {
        // 新地图：用 mapId 构造并插入
        it = m_mapData.try_emplace(pkt.mapId, mc::world::map::MapData{pkt.mapId}).first;
    }

    auto& mapData = it->second;
    bool changed = false;

    // locked 直接覆盖（scale/xCenter/zCenter 等仅影响坐标映射，客户端渲染纹理不需要）
    if (mapData.locked() != pkt.locked) {
        mapData.setLocked(pkt.locked);
        changed = true;
    }

    // 装饰：present 则清空重加（对齐 Java addClientSideDecorations）
    if (pkt.decorations.has_value()) {
        std::vector<mc::world::map::MapDecoration> decos;
        decos.reserve(pkt.decorations->size());
        for (const auto& w : *pkt.decorations) {
            decos.push_back(wireToDecoration(w));
        }
        mapData.applyClientDecorations(std::move(decos));
        changed = true;
    }

    // 色块补丁：present 则写入子区域（对齐 Java MapPatch.applyToMap）
    if (pkt.colorPatch.has_value() && pkt.colorPatch->width > 0) {
        mapData.applyColorPatch(pkt.colorPatch->startX,
            pkt.colorPatch->startY,
            pkt.colorPatch->width,
            pkt.colorPatch->height,
            pkt.colorPatch->colors);
        changed = true;
    }

    return changed;
}

const mc::world::map::MapData* ClientMapDataCache::getMapData(i32 mapId) const
{
    auto it = m_mapData.find(mapId);
    return it == m_mapData.end() ? nullptr : &it->second;
}

mc::world::map::MapData* ClientMapDataCache::getMapDataMutable(i32 mapId)
{
    auto it = m_mapData.find(mapId);
    return it == m_mapData.end() ? nullptr : &it->second;
}

} // namespace mc::client
