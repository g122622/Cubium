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

#pragma once

#include "client/ui/kagero/paint/TextureImage.hpp"
#include <string>

namespace mc::client::renderer::trident::gui {

// 前向声明，避免循环包含
struct GuiSprite;

/**
 * @brief 精灵图集抽象接口
 *
 * 为 UI Widget 提供与具体图集实现（如 `GuiSpriteAtlas`）解耦的精灵访问能力。
 *
 * 设计动机：
 * `GuiSpriteAtlas` 持有 Vulkan 资源，其实现无法在单元测试环境中实例化。
 * 引入此接口后，依赖图集的 Widget（如 `ImageWidget`）可仅依赖抽象接口，
 * 便于在测试中注入轻量级实现，同时保持生产环境的零成本抽象。
 *
 * 实现方（如 `GuiSpriteAtlas`）通过 `override` 提供具体语义。
 */
class ISpriteAtlas {
public:
    virtual ~ISpriteAtlas() = default;

    /**
     * @brief 获取精灵定义
     * @param id 精灵ID
     * @return 精灵指针，不存在时返回 nullptr
     */
    [[nodiscard]] virtual const GuiSprite* getSprite(const std::string& id) const = 0;

    /**
     * @brief 检查精灵是否存在
     */
    [[nodiscard]] virtual bool hasSprite(const std::string& id) const = 0;

    /**
     * @brief 创建 TextureImage 用于 PaintContext 绘制
     *
     * 返回的 `TextureImage` 不拥有纹理资源，纹理生命周期由图集管理。
     *
     * @param spriteId 精灵ID
     * @return TextureImage 对象，精灵不存在时返回无效对象
     */
    [[nodiscard]] virtual ui::kagero::paint::TextureImage createTextureImage(const std::string& spriteId) const = 0;
};

} // namespace mc::client::renderer::trident::gui
