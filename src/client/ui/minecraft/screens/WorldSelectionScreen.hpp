#pragma once

#include "../../kagero/widget/ListWidget.hpp"
#include "TemplateScreen.hpp"
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

    world::storage::WorldListService m_worldListService;
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
