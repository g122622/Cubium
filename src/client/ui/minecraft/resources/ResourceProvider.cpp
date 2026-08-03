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

#include "ResourceProvider.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/ui/Font.hpp"
#include <string>

namespace mc::client::ui::minecraft {

ResourceProvider::ResourceProvider(Font& font, renderer::trident::gui::GuiRenderer& renderer)
    : m_font(font)
    , m_renderer(renderer)
{}

void ResourceProvider::loadGuiTextureAtlas(const std::string& path)
{
    // TODO: 从资源路径加载纹理图集
    // 需要通过GuiTextureLoader加载
    (void)path;
}

} // namespace mc::client::ui::minecraft
