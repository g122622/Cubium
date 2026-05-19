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

#include "WorldSelectionScreen.hpp"
#include "../../kagero/event/EventBus.hpp"
#include "../../kagero/state/StateStore.hpp"
#include <algorithm>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace mc::client::ui::minecraft {

WorldSelectionScreen::WorldSelectionScreen()
    : TemplateScreen(std::make_unique<kagero::tpl::binder::BindingContext>(
                         kagero::state::StateStore::instance(), kagero::event::EventBus::instance()),
          "worldSelection")
{
    expose("worlds.empty", &m_worldsEmpty);
    expose("worlds.selected", &m_hasSelection);

    loadTemplateFile("src/client/ui/minecraft/templates/world_selection.tpl");
    cacheWidgets();
    registerCallbacks();
    refreshWorldList();
}

void WorldSelectionScreen::onOpen()
{
    TemplateScreen::onOpen();
    refreshWorldList();
}

void WorldSelectionScreen::registerCallbacks()
{
    exposeSimpleCallback("onPlay", [this]() {
        if (m_selectedWorld != nullptr && m_onSelectWorld) {
            m_onSelectWorld(*m_selectedWorld);
        }
    });

    exposeSimpleCallback("onCreateWorld", [this]() {
        if (m_onCreateWorld) {
            m_onCreateWorld();
        }
    });

    exposeSimpleCallback("onDelete", [this]() {});

    exposeSimpleCallback("onBack", [this]() {
        if (m_onBack) {
            m_onBack();
        }
    });
}

void WorldSelectionScreen::cacheWidgets()
{
    if (auto* widget = findWidget("worldList")) {
        m_worldListWidget = dynamic_cast<kagero::widget::ListWidget*>(widget);
    }

    if (m_worldListWidget) {
        m_worldListWidget->setSelectionMode(kagero::widget::ListWidget::SelectionMode::Single);
        m_worldListWidget->setOnSelect(
            [this](size_t index, kagero::widget::IListItem*) { updateSelection(static_cast<i32>(index)); });
        m_worldListWidget->setOnDoubleClick([this](size_t index, kagero::widget::IListItem*) {
            updateSelection(static_cast<i32>(index));
            if (m_selectedWorld != nullptr && m_onSelectWorld) {
                m_onSelectWorld(*m_selectedWorld);
            }
        });
    }
}

void WorldSelectionScreen::updateSelection(i32 index)
{
    if (index < 0 || static_cast<size_t>(index) >= m_worlds.size()) {
        m_selectedIndex = -1;
        m_selectedWorld = nullptr;
        m_hasSelection = false;
        updateBindingValues();
        return;
    }

    m_selectedIndex = index;
    m_selectedWorld = &m_worlds[static_cast<size_t>(index)];
    m_hasSelection = true;
    updateBindingValues();
}

void WorldSelectionScreen::publishWorldCollection()
{
    if (auto* ctx = bindingContext()) {
        std::vector<kagero::tpl::binder::Value> values;
        values.reserve(m_worldNames.size());
        for (const auto& name : m_worldNames) {
            values.emplace_back(name);
        }
        ctx->setCollectionValue("worlds", values);
    }
}

void WorldSelectionScreen::updateBindingValues()
{
    expose("worlds.empty", &m_worldsEmpty);
    expose("worlds.selected", &m_hasSelection);
    refresh();
}

void WorldSelectionScreen::refreshWorldList()
{
    auto result = m_globalStorage.listWorlds();
    if (result.success()) {
        m_worlds = std::move(result.value());
        m_worldNames.clear();
        m_worldNames.reserve(m_worlds.size());
        for (const auto& world : m_worlds) {
            m_worldNames.push_back(world.displayName);
        }
        m_worldsEmpty = m_worlds.empty();
        m_selectedIndex = -1;
        m_selectedWorld = nullptr;
        m_hasSelection = false;
        publishWorldCollection();
        updateBindingValues();

        if (m_worldListWidget != nullptr) {
            std::vector<kagero::tpl::binder::Value> values;
            values.reserve(m_worldNames.size());
            for (const auto& name : m_worldNames) {
                values.emplace_back(name);
            }
            m_worldListWidget->setItemsFromValue(kagero::tpl::binder::Value::fromArray(std::move(values)));
        }
    } else {
        spdlog::error("[WorldSelectionScreen] Failed to list worlds: {}", result.error().toString());
        m_worlds.clear();
        m_worldNames.clear();
        m_worldsEmpty = true;
        m_selectedIndex = -1;
        m_selectedWorld = nullptr;
        m_hasSelection = false;
        publishWorldCollection();
        updateBindingValues();

        if (m_worldListWidget != nullptr) {
            m_worldListWidget->clearItems();
        }
    }
}

bool WorldSelectionScreen::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    (void)scanCode;
    (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (m_onBack) {
            m_onBack();
        }
        return true;
    }

    return Screen::onKey(key, scanCode, action, mods);
}

} // namespace mc::client::ui::minecraft
