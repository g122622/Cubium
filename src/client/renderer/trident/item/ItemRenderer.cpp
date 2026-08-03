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

#include "ItemRenderer.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/resource/ItemTextureAtlas.hpp"
#include "client/resource/ResourceManager.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/Block.hpp"
#include <spdlog/spdlog.h>

namespace mc::client::renderer::trident::item {

ItemRenderer::ItemRenderer()
    : m_resourceManager(nullptr)
    , m_itemTextureAtlas(nullptr)
    , m_initialized(false)
{}

Result<void> ItemRenderer::initialize(ResourceManager* resourceManager, ItemTextureAtlas* itemTextureAtlas)
{
    MC_ASSERT_RELEASE(resourceManager != nullptr);
    MC_ASSERT_RELEASE(itemTextureAtlas != nullptr);

    m_resourceManager = resourceManager;
    m_itemTextureAtlas = itemTextureAtlas;
    m_initialized = true;

    spdlog::info("ItemRenderer: Initialized");
    return {};
}

void ItemRenderer::renderItem(gui::GuiRenderer& gui, const ItemStack& stack, f64 x, f64 y, f64 size)
{
    if (stack.isEmpty()) {
        return;
    }

    const Item* item = stack.getItem();
    MC_ASSERT_RELEASE(item != nullptr);

    renderItem(gui, item, x, y, size);
}

void ItemRenderer::renderItem(gui::GuiRenderer& gui, const Item* item, f64 x, f64 y, f64 size)
{
    MC_ASSERT_RELEASE(item != nullptr);
    MC_ASSERT_RELEASE(m_initialized);

    const TextureRegion* region = getItemTextureRegion(item);
    if (region == nullptr) {
        // 没有找到纹理，绘制占位符
        // 使用半透明纯色矩形表示缺失纹理
        gui.fillRect(x, y, size, size, 0x80FF0000);
        return;
    }

    renderItem(gui, *region, x, y, size);
}

void ItemRenderer::renderItem(gui::GuiRenderer& gui, const TextureRegion& region, f64 x, f64 y, f64 size)
{
    // 使用drawTexturedRect绘制物品纹理
    // 使用 alpha=254 的颜色，确保走物品纹理采样分支且保持可见
    gui.drawTexturedRect(
        x, y, size, size, region.u0, region.v0, region.u1, region.v1, gui::GuiRenderer::ITEM_TEXTURE_COLOR);
}

bool ItemRenderer::isBlockItem(const Item* item) const
{
    // dynamic_cast 对 nullptr 输入返回 nullptr，无需额外判空
    return dynamic_cast<const BlockItem*>(item) != nullptr;
}

const TextureRegion* ItemRenderer::getItemTextureRegion(const Item* item) const
{
    MC_ASSERT_RELEASE(item != nullptr);
    MC_ASSERT_RELEASE(m_initialized);

    // 统一优先使用物品图集，避免将方块图集UV错误用于GUI物品图集采样。
    if (const TextureRegion* region = m_itemTextureAtlas->getItemTexture(item->itemId())) {
        return region;
    }

    const ResourceLocation& itemId = item->itemLocation();
    const ResourceLocation itemPath(itemId.namespace_(), "item/" + itemId.path());
    if (const TextureRegion* region = m_itemTextureAtlas->getItemTexture(itemPath)) {
        return region;
    }

    const ResourceLocation itemTexturePath(itemId.namespace_(), "textures/item/" + itemId.path());
    if (const TextureRegion* region = m_itemTextureAtlas->getItemTexture(itemTexturePath)) {
        return region;
    }

    // 检查是否为方块物品
    const BlockItem* blockItem = dynamic_cast<const BlockItem*>(item);
    if (blockItem != nullptr) {
        // 方块物品：尝试方块纹理路径别名（ItemTextureAtlas 在加载时会建立别名）
        const ResourceLocation& blockId = blockItem->block().blockLocation();
        const ResourceLocation blockPath(blockId.namespace_(), "block/" + blockId.path());
        if (const TextureRegion* region = m_itemTextureAtlas->getItemTexture(blockPath)) {
            return region;
        }

        const ResourceLocation blockTexturePath(blockId.namespace_(), "textures/block/" + blockId.path());
        if (const TextureRegion* region = m_itemTextureAtlas->getItemTexture(blockTexturePath)) {
            return region;
        }

        // 注意：不要回退到方块图集UV。
        // GUI物品绘制固定采样"物品图集"，若返回方块图集的UV会导致采样错位。
    }

    return nullptr;
}

} // namespace mc::client::renderer::trident::item
