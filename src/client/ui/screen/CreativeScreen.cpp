#include "CreativeScreen.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/renderer/trident/gui/GuiTextureManager.hpp"
#include "client/renderer/trident/item/ItemRenderer.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <utility>

namespace mc::client {

namespace {

[[nodiscard]] String toLowerAscii(StringView text)
{
    String lowered;
    lowered.reserve(text.size());
    for (const char character : text) {
        lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
    }
    return lowered;
}

} // namespace

CreativeScreen::CreativeScreen(PlayerInventory& inventory, CreativeActionSender actionSender)
    : m_inventory(&inventory)
    , m_actionSender(std::move(actionSender))
{
    updateLayout();
}

void CreativeScreen::setRenderers(renderer::trident::gui::GuiRenderer* gui,
                                  renderer::trident::gui::GuiTextureManager* textureManager,
                                  renderer::trident::item::ItemRenderer* itemRenderer)
{
    m_gui = gui;
    m_textureManager = textureManager;
    m_itemRenderer = itemRenderer;
}

void CreativeScreen::setScreenSize(i32 width, i32 height)
{
    m_screenWidth = width;
    m_screenHeight = height;
    updateLayout();
}

void CreativeScreen::init()
{
    if (m_initialized) {
        return;
    }

    m_initialized = true;
    m_paletteEntries = buildCreativePaletteEntries();
    rebuildVisibleEntries();
    updateLayout();
}

void CreativeScreen::render(i32 mouseX, i32 mouseY, f32 partialTick)
{
    (void)partialTick;

    if (m_gui == nullptr) {
        return;
    }

    m_gui->beginFrame(static_cast<f64>(m_screenWidth), static_cast<f64>(m_screenHeight));
    renderBackground();
    renderPanelBackground();
    renderSearchBox();
    renderPaletteGrid(mouseX, mouseY);
    renderPlayerInventory(mouseX, mouseY);
    renderCarriedItem(mouseX, mouseY);
}

bool CreativeScreen::onClick(i32 mouseX, i32 mouseY, i32 button)
{
    if (m_gui == nullptr) {
        return false;
    }

    const i32 paletteIndex = getPaletteIndexAt(mouseX, mouseY);
    if (paletteIndex >= 0) {
        handlePaletteClick(paletteIndex, button);
        return true;
    }

    if (isMouseOver(mouseX, mouseY, m_leftPos + TRASH_X, m_topPos + TRASH_Y, SLOT_SIZE, SLOT_SIZE)) {
        m_carriedItem = ItemStack::EMPTY;
        return true;
    }

    const i32 inventorySlot = getInventorySlotAt(mouseX, mouseY);
    if (inventorySlot >= 0) {
        handleInventoryClick(inventorySlot, button);
        return true;
    }

    return false;
}

bool CreativeScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    (void)scanCode;
    (void)mods;

    if (action != GLFW_PRESS) {
        return false;
    }

    if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_E) {
        onClose();
        return true;
    }

    if (key == GLFW_KEY_BACKSPACE) {
        if (!m_searchText.empty()) {
            m_searchText.pop_back();
            m_scrollRows = 0;
            rebuildVisibleEntries();
        }
        return true;
    }

    return false;
}

bool CreativeScreen::onChar(u32 codePoint)
{
    if (codePoint < 32U || codePoint == 127U) {
        return false;
    }

    if (m_searchText.size() >= 64) {
        return true;
    }

    if (codePoint < 128U) {
        m_searchText.push_back(static_cast<char>(codePoint));
        m_scrollRows = 0;
        rebuildVisibleEntries();
        return true;
    }

    return false;
}

bool CreativeScreen::onScroll(i32 mouseX, i32 mouseY, f64 delta)
{
    (void)mouseX;
    (void)mouseY;

    const i32 maxScrollRows = getMaxScrollRows();
    if (maxScrollRows <= 0) {
        return true;
    }

    const i32 step = delta > 0.0 ? -1 : 1;
    m_scrollRows = std::clamp(m_scrollRows + step, 0, maxScrollRows);
    return true;
}

void CreativeScreen::onClose()
{
    m_carriedItem = ItemStack::EMPTY;
}

bool CreativeScreen::isPauseScreen() const
{
    return false;
}

String CreativeScreen::getTitle() const
{
    return "Creative Inventory";
}

void CreativeScreen::onResize(i32 width, i32 height)
{
    setScreenSize(width, height);
}

void CreativeScreen::updateLayout()
{
    if (m_screenWidth > 0 && m_screenHeight > 0) {
        m_leftPos = (m_screenWidth - GUI_WIDTH) / 2;
        m_topPos = (m_screenHeight - GUI_HEIGHT) / 2;
    } else {
        m_leftPos = 0;
        m_topPos = 0;
    }
}

void CreativeScreen::rebuildVisibleEntries()
{
    m_visibleEntries.clear();

    const String filter = normalizeSearchText(m_searchText);
    for (i32 index = 0; index < static_cast<i32>(m_paletteEntries.size()); ++index) {
        if (filter.empty() || matchesSearch(m_paletteEntries[static_cast<std::size_t>(index)])) {
            m_visibleEntries.push_back(index);
        }
    }

    m_scrollRows = std::clamp(m_scrollRows, 0, getMaxScrollRows());
}

void CreativeScreen::renderBackground()
{
    if (m_gui == nullptr) {
        return;
    }

    m_gui->fillGradientRect(0.0, 0.0,
                            static_cast<f64>(m_screenWidth), static_cast<f64>(m_screenHeight),
                            0xD0141A1F, 0xE00C1016);
}

void CreativeScreen::renderPanelBackground()
{
    if (m_gui == nullptr) {
        return;
    }

    const f64 palettePanelWidth = 172.0;
    const f64 inventoryPanelWidth = static_cast<f64>(176);

    m_gui->fillRect(static_cast<f64>(m_leftPos + 4), static_cast<f64>(m_topPos + 18), palettePanelWidth, 170.0, 0xCC23262C);
    m_gui->fillRect(static_cast<f64>(m_leftPos + INVENTORY_X), static_cast<f64>(m_topPos + INVENTORY_Y), inventoryPanelWidth, 176.0, 0xCC2A2F37);
    m_gui->drawRect(static_cast<f64>(m_leftPos + 4), static_cast<f64>(m_topPos + 18), palettePanelWidth, 170.0, 0xFF4DA3FF);
    m_gui->drawRect(static_cast<f64>(m_leftPos + INVENTORY_X), static_cast<f64>(m_topPos + INVENTORY_Y), inventoryPanelWidth, 176.0, 0xFFFFB84D);

    if (m_gui->font() != nullptr) {
        m_gui->drawText(getTitle(), static_cast<f64>(m_leftPos + TITLE_X), static_cast<f64>(m_topPos + TITLE_Y), 0xFFF5F7FA, false);
        m_gui->drawText("Search", static_cast<f64>(m_leftPos + SEARCH_X), static_cast<f64>(m_topPos + SEARCH_Y - 10), 0xFFB9C1CC, false);
        m_gui->drawText("Inventory", static_cast<f64>(m_leftPos + INVENTORY_X + 8), static_cast<f64>(m_topPos + INVENTORY_Y - 10), 0xFFD8CFA3, false);
    }
}

void CreativeScreen::renderSearchBox()
{
    if (m_gui == nullptr) {
        return;
    }

    const i32 searchX = m_leftPos + SEARCH_X;
    const i32 searchY = m_topPos + SEARCH_Y;
    m_gui->fillRect(static_cast<f64>(searchX), static_cast<f64>(searchY), static_cast<f64>(SEARCH_WIDTH), static_cast<f64>(SEARCH_HEIGHT), 0xFF0F1318);
    m_gui->drawRect(static_cast<f64>(searchX), static_cast<f64>(searchY), static_cast<f64>(SEARCH_WIDTH), static_cast<f64>(SEARCH_HEIGHT), 0xFF4DA3FF);

    if (m_gui->font() != nullptr) {
        const String displayText = m_searchText.empty() ? String("Search creative items...") : m_searchText;
        const u32 color = m_searchText.empty() ? 0xFF6B7785 : 0xFFF5F7FA;
        m_gui->drawText(displayText, static_cast<f64>(searchX + 4), static_cast<f64>(searchY + 4), color, false);
    }

    const i32 trashX = m_leftPos + TRASH_X;
    const i32 trashY = m_topPos + TRASH_Y;
    renderSlotFrame(trashX, trashY, 0xFFB84D4D, 0xFF271717);
    if (m_gui->font() != nullptr) {
        m_gui->drawText("X", static_cast<f64>(trashX + 5), static_cast<f64>(trashY + 3), 0xFFF2D6D6, false);
    }
}

void CreativeScreen::renderPaletteGrid(i32 mouseX, i32 mouseY)
{
    if (m_gui == nullptr) {
        return;
    }

    const i32 visibleCount = PALETTE_COLUMNS * PALETTE_VISIBLE_ROWS;

    for (i32 row = 0; row < PALETTE_VISIBLE_ROWS; ++row) {
        for (i32 column = 0; column < PALETTE_COLUMNS; ++column) {
            const i32 visibleIndex = m_scrollRows * PALETTE_COLUMNS + row * PALETTE_COLUMNS + column;
            const i32 cellX = getPaletteCellX(column);
            const i32 cellY = getPaletteCellY(row);
            const bool hovered = isMouseOver(mouseX, mouseY, cellX, cellY, SLOT_SIZE, SLOT_SIZE);

            if (visibleIndex < static_cast<i32>(m_visibleEntries.size())) {
                renderSlotFrame(m_leftPos + cellX, m_topPos + cellY, hovered ? 0xFF4DA3FF : 0xFF3C4654, 0xFF151A20);

                const i32 paletteEntryIndex = m_visibleEntries[static_cast<std::size_t>(visibleIndex)];
                const CreativeInventoryEntry& entry = m_paletteEntries[static_cast<std::size_t>(paletteEntryIndex)];
                if (!entry.stack.isEmpty()) {
                    renderItemIcon(entry.stack, m_leftPos + cellX, m_topPos + cellY);
                    if (entry.stack.getCount() > 1) {
                        renderItemCount(entry.stack.getCount(), m_leftPos + cellX + SLOT_SIZE - 2, m_topPos + cellY + SLOT_SIZE - 8);
                    }
                }
            } else if (visibleIndex < visibleCount) {
                renderSlotFrame(m_leftPos + cellX, m_topPos + cellY, hovered ? 0xFF4DA3FF : 0xFF2A2F37, 0xFF10141A);
            }
        }
    }
}

void CreativeScreen::renderPlayerInventory(i32 mouseX, i32 mouseY)
{
    if (m_inventory == nullptr || m_gui == nullptr) {
        return;
    }

    for (i32 i = 0; i < PlayerInventory::ARMOR_SIZE; ++i) {
        const i32 slotIndex = InventorySlots::ARMOR_START + i;
        const i32 x = m_leftPos + INVENTORY_X + ARMOR_X;
        const i32 y = m_topPos + INVENTORY_Y + (i == 0 ? ARMOR_Y_HEAD : i == 1 ? ARMOR_Y_CHEST : i == 2 ? ARMOR_Y_LEGS : ARMOR_Y_FEET);
        const bool hovered = isMouseOver(mouseX, mouseY, x, y, SLOT_SIZE, SLOT_SIZE);
        renderSlotFrame(x, y, hovered ? 0xFFFFB84D : 0xFF63502E, 0xFF1A1712);

        const ItemStack stack = m_inventory->getItem(slotIndex);
        if (!stack.isEmpty()) {
            renderItemIcon(stack, x, y);
        }
    }

    const i32 offhandSlot = InventorySlots::OFFHAND;
    const i32 offhandX = m_leftPos + INVENTORY_X + OFFHAND_X;
    const i32 offhandY = m_topPos + INVENTORY_Y + OFFHAND_Y;
    renderSlotFrame(offhandX, offhandY, isMouseOver(mouseX, mouseY, offhandX, offhandY, SLOT_SIZE, SLOT_SIZE) ? 0xFFFFB84D : 0xFF63502E, 0xFF1A1712);
    const ItemStack offhandStack = m_inventory->getItem(offhandSlot);
    if (!offhandStack.isEmpty()) {
        renderItemIcon(offhandStack, offhandX, offhandY);
    }

    for (i32 row = 0; row < 3; ++row) {
        for (i32 column = 0; column < PlayerInventory::HOTBAR_SIZE; ++column) {
            const i32 slotIndex = InventorySlots::MAIN_START + row * PlayerInventory::HOTBAR_SIZE + column;
            const i32 x = m_leftPos + INVENTORY_X + PLAYER_INV_X + column * SLOT_SPACING;
            const i32 y = m_topPos + INVENTORY_Y + PLAYER_INV_Y + row * SLOT_SPACING;
            const bool hovered = isMouseOver(mouseX, mouseY, x, y, SLOT_SIZE, SLOT_SIZE);
            renderSlotFrame(x, y, hovered ? 0xFFFFB84D : 0xFF5A5F69, 0xFF1C2028);

            const ItemStack stack = m_inventory->getItem(slotIndex);
            if (!stack.isEmpty()) {
                renderItemIcon(stack, x, y);
                if (stack.getCount() > 1) {
                    renderItemCount(stack.getCount(), x + SLOT_SIZE - 2, y + SLOT_SIZE - 8);
                }
            }
        }
    }

    for (i32 column = 0; column < PlayerInventory::HOTBAR_SIZE; ++column) {
        const i32 slotIndex = InventorySlots::HOTBAR_START + column;
        const i32 x = m_leftPos + INVENTORY_X + HOTBAR_X + column * SLOT_SPACING;
        const i32 y = m_topPos + INVENTORY_Y + HOTBAR_Y;
        const bool hovered = isMouseOver(mouseX, mouseY, x, y, SLOT_SIZE, SLOT_SIZE);
        const bool selected = slotIndex == m_inventory->getSelectedSlot();
        renderSlotFrame(x, y, selected ? 0xFFF7D26A : hovered ? 0xFFFFB84D : 0xFF5A5F69, selected ? 0xFF2B2516 : 0xFF1C2028);

        const ItemStack stack = m_inventory->getItem(slotIndex);
        if (!stack.isEmpty()) {
            renderItemIcon(stack, x, y);
            if (stack.getCount() > 1) {
                renderItemCount(stack.getCount(), x + SLOT_SIZE - 2, y + SLOT_SIZE - 8);
            }
        }
    }
}

void CreativeScreen::renderItemIcon(const ItemStack& stack, i32 screenX, i32 screenY)
{
    if (m_gui == nullptr || stack.isEmpty()) {
        return;
    }

    if (m_itemRenderer != nullptr) {
        m_itemRenderer->renderItem(*m_gui, stack,
                                   static_cast<f32>(screenX),
                                   static_cast<f32>(screenY),
                                   static_cast<f32>(SLOT_SIZE));
        return;
    }

    m_gui->fillRect(static_cast<f64>(screenX), static_cast<f64>(screenY), static_cast<f64>(SLOT_SIZE), static_cast<f64>(SLOT_SIZE), 0x80FFFFFF);
}

void CreativeScreen::renderItemCount(i32 count, i32 screenX, i32 screenY)
{
    if (m_gui == nullptr || m_gui->font() == nullptr || count <= 1) {
        return;
    }

    m_gui->drawText(std::to_string(count), static_cast<f64>(screenX), static_cast<f64>(screenY), 0xFFF5F7FA, true);
}

void CreativeScreen::renderSlotFrame(i32 screenX, i32 screenY, u32 borderColor, u32 fillColor)
{
    if (m_gui == nullptr) {
        return;
    }

    m_gui->fillRect(static_cast<f64>(screenX), static_cast<f64>(screenY), static_cast<f64>(SLOT_SIZE), static_cast<f64>(SLOT_SIZE), fillColor);
    m_gui->drawRect(static_cast<f64>(screenX), static_cast<f64>(screenY), static_cast<f64>(SLOT_SIZE), static_cast<f64>(SLOT_SIZE), borderColor);
}

void CreativeScreen::renderItemTooltip(const ItemStack& stack, i32 mouseX, i32 mouseY)
{
    if (m_gui == nullptr || m_gui->font() == nullptr || stack.isEmpty()) {
        return;
    }

    std::vector<String> lines;
    auto displayName = stack.getDisplayName();
    lines.emplace_back(displayName ? displayName->getUnformattedText() : "");

    if (stack.getCount() > 1) {
        lines.emplace_back("Count: " + std::to_string(stack.getCount()));
    }

    if (stack.isDamageable() && stack.getMaxDamage() > 0) {
        const i32 remainingDurability = std::max(0, stack.getMaxDamage() - stack.getDamage());
        lines.emplace_back("Durability: " + std::to_string(remainingDurability) + "/" + std::to_string(stack.getMaxDamage()));
    }

    f64 maxTextWidth = 0.0;
    for (const auto& line : lines) {
        maxTextWidth = std::max(maxTextWidth, m_gui->getTextWidth(line));
    }

    constexpr f64 PADDING = 4.0;
    constexpr f64 MARGIN = 12.0;
    const f64 fontHeight = static_cast<f64>(m_gui->getFontHeight());
    const f64 tooltipWidth = maxTextWidth + PADDING * 2.0;
    const f64 tooltipHeight = static_cast<f64>(lines.size()) * fontHeight + PADDING * 2.0;

    f64 tooltipX = static_cast<f64>(mouseX) + MARGIN;
    f64 tooltipY = static_cast<f64>(mouseY) + MARGIN;
    const f64 screenWidth = static_cast<f64>(m_screenWidth);
    const f64 screenHeight = static_cast<f64>(m_screenHeight);

    if (tooltipX + tooltipWidth > screenWidth) {
        tooltipX = static_cast<f64>(mouseX) - MARGIN - tooltipWidth;
    }
    if (tooltipY + tooltipHeight > screenHeight) {
        tooltipY = static_cast<f64>(mouseY) - MARGIN - tooltipHeight;
    }

    tooltipX = std::max(4.0, tooltipX);
    tooltipY = std::max(4.0, tooltipY);

    m_gui->fillRect(tooltipX, tooltipY, tooltipWidth, tooltipHeight, 0xF0100010);
    m_gui->drawRect(tooltipX, tooltipY, tooltipWidth, tooltipHeight, 0xFF4DA3FF);

    const f64 textX = tooltipX + PADDING;
    f64 textY = tooltipY + PADDING;
    for (const auto& line : lines) {
        m_gui->drawText(line, textX, textY, 0xFFF5F7FA, true);
        textY += fontHeight;
    }
}

void CreativeScreen::renderCarriedItem(i32 mouseX, i32 mouseY)
{
    if (m_carriedItem.isEmpty()) {
        return;
    }

    renderItemIcon(m_carriedItem, mouseX - SLOT_SIZE / 2, mouseY - SLOT_SIZE / 2);
    if (m_carriedItem.getCount() > 1) {
        renderItemCount(m_carriedItem.getCount(), mouseX + SLOT_SIZE / 2 - 2, mouseY + SLOT_SIZE / 2 - 8);
    }
}

void CreativeScreen::handlePaletteClick(i32 paletteIndex, i32 button)
{
    if (paletteIndex < 0 || paletteIndex >= static_cast<i32>(m_visibleEntries.size())) {
        return;
    }

    const i32 entryIndex = m_visibleEntries[static_cast<std::size_t>(paletteIndex)];
    if (entryIndex < 0 || entryIndex >= static_cast<i32>(m_paletteEntries.size())) {
        return;
    }

    ItemStack stack = m_paletteEntries[static_cast<std::size_t>(entryIndex)].stack;
    if (button == GLFW_MOUSE_BUTTON_RIGHT && stack.getCount() > 1) {
        stack.setCount(1);
    }

    m_carriedItem = stack;
}

void CreativeScreen::handleInventoryClick(i32 slotIndex, i32 button)
{
    if (m_inventory == nullptr || slotIndex < 0 || slotIndex >= PlayerInventory::TOTAL_SIZE) {
        return;
    }

    if (!m_carriedItem.isEmpty()) {
        m_carriedItem = m_inventory->placeItem(slotIndex, m_carriedItem);
        sendInventorySlotUpdate(slotIndex);
        return;
    }

    const ItemStack current = m_inventory->getItem(slotIndex);
    if (current.isEmpty()) {
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        const i32 amount = std::max(1, (current.getCount() + 1) / 2);
        m_carriedItem = m_inventory->removeItem(slotIndex, amount);
    } else {
        m_carriedItem = m_inventory->removeItemNoUpdate(slotIndex);
    }

    sendInventorySlotUpdate(slotIndex);
}

void CreativeScreen::sendInventorySlotUpdate(i32 slotIndex)
{
    if (m_actionSender) {
        m_actionSender(slotIndex, m_inventory->getItem(slotIndex));
    }
}

i32 CreativeScreen::getPaletteIndexAt(i32 mouseX, i32 mouseY) const
{
    const i32 localX = mouseX - (m_leftPos + PALETTE_X);
    const i32 localY = mouseY - (m_topPos + PALETTE_Y);
    if (localX < 0 || localY < 0) {
        return -1;
    }

    const i32 column = localX / SLOT_SPACING;
    const i32 row = localY / SLOT_SPACING;
    if (column < 0 || column >= PALETTE_COLUMNS || row < 0 || row >= PALETTE_VISIBLE_ROWS) {
        return -1;
    }

    const i32 slotX = localX % SLOT_SPACING;
    const i32 slotY = localY % SLOT_SPACING;
    if (slotX >= SLOT_SIZE || slotY >= SLOT_SIZE) {
        return -1;
    }

    return m_scrollRows * PALETTE_COLUMNS + row * PALETTE_COLUMNS + column;
}

i32 CreativeScreen::getInventorySlotAt(i32 mouseX, i32 mouseY) const
{
    for (i32 i = 0; i < PlayerInventory::ARMOR_SIZE; ++i) {
        const i32 x = m_leftPos + INVENTORY_X + ARMOR_X;
        const i32 y = m_topPos + INVENTORY_Y + (i == 0 ? ARMOR_Y_HEAD : i == 1 ? ARMOR_Y_CHEST : i == 2 ? ARMOR_Y_LEGS : ARMOR_Y_FEET);
        if (isMouseOver(mouseX, mouseY, x, y, SLOT_SIZE, SLOT_SIZE)) {
            return InventorySlots::ARMOR_START + i;
        }
    }

    const i32 offhandX = m_leftPos + INVENTORY_X + OFFHAND_X;
    const i32 offhandY = m_topPos + INVENTORY_Y + OFFHAND_Y;
    if (isMouseOver(mouseX, mouseY, offhandX, offhandY, SLOT_SIZE, SLOT_SIZE)) {
        return InventorySlots::OFFHAND;
    }

    for (i32 row = 0; row < 3; ++row) {
        for (i32 column = 0; column < PlayerInventory::HOTBAR_SIZE; ++column) {
            const i32 slotIndex = InventorySlots::MAIN_START + row * PlayerInventory::HOTBAR_SIZE + column;
            const i32 x = m_leftPos + INVENTORY_X + PLAYER_INV_X + column * SLOT_SPACING;
            const i32 y = m_topPos + INVENTORY_Y + PLAYER_INV_Y + row * SLOT_SPACING;
            if (isMouseOver(mouseX, mouseY, x, y, SLOT_SIZE, SLOT_SIZE)) {
                return slotIndex;
            }
        }
    }

    for (i32 column = 0; column < PlayerInventory::HOTBAR_SIZE; ++column) {
        const i32 slotIndex = InventorySlots::HOTBAR_START + column;
        const i32 x = m_leftPos + INVENTORY_X + HOTBAR_X + column * SLOT_SPACING;
        const i32 y = m_topPos + INVENTORY_Y + HOTBAR_Y;
        if (isMouseOver(mouseX, mouseY, x, y, SLOT_SIZE, SLOT_SIZE)) {
            return slotIndex;
        }
    }

    return -1;
}

i32 CreativeScreen::getPaletteCellX(i32 column) const
{
    return PALETTE_X + column * SLOT_SPACING;
}

i32 CreativeScreen::getPaletteCellY(i32 row) const
{
    return PALETTE_Y + row * SLOT_SPACING;
}

i32 CreativeScreen::getMaxScrollRows() const
{
    const i32 visibleCount = static_cast<i32>(m_visibleEntries.size());
    const i32 totalRows = (visibleCount + PALETTE_COLUMNS - 1) / PALETTE_COLUMNS;
    return std::max(0, totalRows - PALETTE_VISIBLE_ROWS);
}

bool CreativeScreen::isMouseOver(i32 mouseX, i32 mouseY, i32 x, i32 y, i32 width, i32 height) const
{
    return mouseX >= x && mouseX < x + width && mouseY >= y && mouseY < y + height;
}

String CreativeScreen::normalizeSearchText(StringView text) const
{
    return toLowerAscii(text);
}

bool CreativeScreen::matchesSearch(const CreativeInventoryEntry& entry) const
{
    const String filter = normalizeSearchText(m_searchText);
    if (filter.empty()) {
        return true;
    }

    return entry.searchKey.find(filter) != String::npos;
}

} // namespace mc::client