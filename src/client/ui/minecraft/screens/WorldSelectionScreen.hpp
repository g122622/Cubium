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

#include "../../kagero/widget/ListWidget.hpp"
#include "TemplateScreen.hpp"
#include "common/world/storage/WorldStorageService.hpp"
#include "common/world/storage/list/WorldListService.hpp"
#include "common/world/storage/request/WorldRequests.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace mc::client::ui::minecraft {

class WorldSelectionScreen : public TemplateScreen {
public:
    using Callback = std::function<void()>;
    using WorldSelectCallback = std::function<void(const world::storage::WorldListEntry&)>;

    WorldSelectionScreen();

    void setOnSelectWorld(WorldSelectCallback callback) { m_onSelectWorld = std::move(callback); }
    void setOnCreateWorld(Callback callback) { m_onCreateWorld = std::move(callback); }
    void setOnBack(Callback callback) { m_onBack = std::move(callback); }

    void onOpen() override;
    void refreshWorldList();
    [[nodiscard]] const world::storage::WorldListEntry* selectedWorld() const { return m_selectedWorld; }

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

private:
    void registerCallbacks();
    void cacheWidgets();
    void updateSelection(i32 index);
    void updateBindingValues();
    void publishWorldCollection();

    world::storage::WorldStorageService m_storageService;
    std::vector<world::storage::WorldListEntry> m_worlds;
    std::vector<std::string> m_worldNames;
    const world::storage::WorldListEntry* m_selectedWorld = nullptr;
    i32 m_selectedIndex = -1;
    bool m_worldsEmpty = true;
    bool m_hasSelection = false;

    kagero::widget::ListWidget* m_worldListWidget = nullptr;

    WorldSelectCallback m_onSelectWorld;
    Callback m_onCreateWorld;
    Callback m_onBack;
};

} // namespace mc::client::ui::minecraft
