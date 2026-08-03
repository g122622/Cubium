/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following further conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "TemplateScreen.hpp"
#include "client/ui/kagero/widget/ListWidget.hpp"
#include "common/core/Types.hpp"
#include "common/world/storage/GlobalStorageManager.hpp"
#include "common/world/storage/list/WorldListEntry.hpp"
#include "common/world/storage/request/WorldRequests.hpp"
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc::client::ui::minecraft {

// 前向声明
namespace widgets {
class ScreenStackWidget;
}

/**
 * @brief 世界选择界面
 *
 * 显示本地存储的世界列表，支持选择、创建和删除世界。
 */
class WorldSelectionScreen : public TemplateScreen {
public:
    using Callback = std::function<void()>;
    using WorldSelectCallback = std::function<void(const world::storage::WorldListEntry&)>;

    WorldSelectionScreen();

    /** 设置选择世界后的回调 */
    void setOnSelectWorld(WorldSelectCallback callback) { m_onSelectWorld = std::move(callback); }
    /** 设置创建世界按钮的回调 */
    void setOnCreateWorld(Callback callback) { m_onCreateWorld = std::move(callback); }
    /** 设置返回按钮的回调 */
    void setOnBack(Callback callback) { m_onBack = std::move(callback); }
    /** 设置屏幕栈Widget，用于弹出确认对话框 */
    void setScreenStack(widgets::ScreenStackWidget* stack) { m_screenStack = stack; }

    void onOpen() override;
    /** 刷新世界列表，从磁盘重新读取 */
    void refreshWorldList();
    /** 获取当前选中的世界，未选中时返回 nullptr */
    [[nodiscard]] const world::storage::WorldListEntry* selectedWorld() const { return m_selectedWorld; }

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

private:
    void _registerCallbacks();
    void _cacheWidgets();
    /** 根据索引更新选中状态和绑定值 */
    void _updateSelection(i32 index);
    /** 重新推送绑定值到模板 */
    void _updateBindingValues();
    /** 将世界名称列表推送到模板集合绑定 */
    void _publishWorldCollection();

    /** 请求删除当前选中的世界（弹出确认对话框） */
    void _requestDeleteWorld();
    /** 执行删除世界的操作 */
    void _doDeleteWorld(const std::string& levelId);

    world::storage::GlobalStorageManager m_globalStorage;
    std::vector<world::storage::WorldListEntry> m_worlds;
    std::vector<std::string> m_worldNames;
    const world::storage::WorldListEntry* m_selectedWorld = nullptr;
    i32 m_selectedIndex = -1;
    bool m_worldsEmpty = true;
    bool m_hasSelection = false;

    kagero::widget::ListWidget* m_worldListWidget = nullptr;
    widgets::ScreenStackWidget* m_screenStack = nullptr;

    WorldSelectCallback m_onSelectWorld;
    Callback m_onCreateWorld;
    Callback m_onBack;
};

} // namespace mc::client::ui::minecraft
