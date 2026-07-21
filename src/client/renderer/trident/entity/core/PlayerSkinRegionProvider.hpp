/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without including limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
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

#pragma once

#include "client/renderer/MeshTypes.hpp" // for TextureRegion
#include "common/core/Types.hpp"

namespace mc::client::renderer::entity::core {

/**
 * @brief 玩家皮肤区域提供者（抽象接口）
 *
 * 渲染层（EntityRendererManager / PlayerRenderer）通过此接口按 entityId 查询
 * 玩家皮肤在实体纹理图集中的动态区域，避免渲染层直接依赖皮肤层具体类
 *（ClientSkinManager），实现控制反转。
 *
 * 实现方（ClientSkinManager facade 或 SkinTextureUploader）内部按
 * PlayerIdentityRegistry.uuidOf(entityId) → EntityTextureAtlas.findRegion("player_skin:<uuid>")
 * 解析。未上传的皮肤会触发懒上传。
 *
 * 返回 nullptr 表示该 entityId 非玩家或皮肤未就绪，调用方回退到默认玩家纹理。
 */
class PlayerSkinRegionProvider {
public:
    virtual ~PlayerSkinRegionProvider() = default;

    /**
     * @brief 查询玩家皮肤区域
     * @param entityId 玩家实体 ID
     * @return 皮肤区域指针；非玩家或未就绪返回 nullptr
     */
    [[nodiscard]] virtual const TextureRegion* getSkinRegionForEntity(EntityInstanceId entityId) const = 0;
};

} // namespace mc::client::renderer::entity::core
