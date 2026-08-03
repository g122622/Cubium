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

#include "WorldSelectionScreen.hpp"
#include "ConfirmScreen.hpp"
#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include "client/ui/kagero/template/binder/BindingContext.hpp"
#include "client/ui/kagero/widget/ListWidget.hpp"
#include "client/ui/minecraft/screens/Screen.hpp"
#include "client/ui/minecraft/screens/TemplateScreen.hpp"
#include "client/ui/minecraft/widgets/ScreenStackWidget.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>
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
    _cacheWidgets();
    _registerCallbacks();
    refreshWorldList();
}

void WorldSelectionScreen::onOpen()
{
    TemplateScreen::onOpen();
    refreshWorldList();
}

void WorldSelectionScreen::_registerCallbacks()
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

    exposeSimpleCallback("onDelete", [this]() { _requestDeleteWorld(); });

    exposeSimpleCallback("onBack", [this]() {
        if (m_onBack) {
            m_onBack();
        }
    });
}

void WorldSelectionScreen::_cacheWidgets()
{
    if (auto* widget = findWidget("worldList")) {
        m_worldListWidget = dynamic_cast<kagero::widget::ListWidget*>(widget);
    }

    if (m_worldListWidget) {
        m_worldListWidget->setSelectionMode(kagero::widget::ListWidget::SelectionMode::Single);
        m_worldListWidget->setOnSelect(
            [this](size_t index, kagero::widget::IListItem*) { _updateSelection(static_cast<i32>(index)); });
        m_worldListWidget->setOnDoubleClick([this](size_t index, kagero::widget::IListItem*) {
            _updateSelection(static_cast<i32>(index));
            if (m_selectedWorld != nullptr && m_onSelectWorld) {
                m_onSelectWorld(*m_selectedWorld);
            }
        });
    }
}

void WorldSelectionScreen::_updateSelection(i32 index)
{
    if (index < 0 || static_cast<size_t>(index) >= m_worlds.size()) {
        m_selectedIndex = -1;
        m_selectedWorld = nullptr;
        m_hasSelection = false;
        _updateBindingValues();
        return;
    }

    m_selectedIndex = index;
    m_selectedWorld = &m_worlds[static_cast<size_t>(index)];
    m_hasSelection = true;
    _updateBindingValues();
}

void WorldSelectionScreen::_publishWorldCollection()
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

void WorldSelectionScreen::_updateBindingValues()
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
        _publishWorldCollection();
        _updateBindingValues();

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
        _publishWorldCollection();
        _updateBindingValues();

        if (m_worldListWidget != nullptr) {
            m_worldListWidget->clearItems();
        }
    }
}

void WorldSelectionScreen::_requestDeleteWorld()
{
    if (m_selectedWorld == nullptr) {
        return;
    }

    // 获取选中世界的信息
    const std::string worldName = m_selectedWorld->displayName;
    const std::string levelId = m_selectedWorld->levelId;

    // 如果世界被锁定（正在运行），不允许删除
    if (m_selectedWorld->locked) {
        spdlog::warn("[WorldSelectionScreen] Cannot delete locked world: {}", worldName);
        return;
    }

    // 检查屏幕栈是否可用，如果不可用则直接执行删除
    if (m_screenStack == nullptr) {
        spdlog::warn("[WorldSelectionScreen] No ScreenStackWidget available, performing delete without confirmation");
        _doDeleteWorld(levelId);
        return;
    }

    // 保存当前选中世界的 levelId 用于回调
    // 创建确认对话框
    auto confirmScreen = std::make_unique<ConfirmScreen>("Delete World",
        worldName + " will be permanently deleted!",
        "Delete",
        "Cancel",
        [this, levelId](bool confirmed) {
            if (confirmed) {
                _doDeleteWorld(levelId);
            }
            // 无论确认还是取消，都弹出确认对话框返回世界选择界面
            if (m_screenStack != nullptr) {
                m_screenStack->pop();
            }
        });

    // 设置确认对话框的大小与当前屏幕一致
    confirmScreen->setBounds(bounds());

    // 将确认对话框推入屏幕栈
    m_screenStack->push(std::move(confirmScreen));
}

void WorldSelectionScreen::_doDeleteWorld(const std::string& levelId)
{
    auto result = m_globalStorage.deleteWorld(levelId);
    if (result.success()) {
        spdlog::info("[WorldSelectionScreen] Deleted world: {}", levelId);
    } else {
        spdlog::error("[WorldSelectionScreen] Failed to delete world '{}': {}", levelId, result.error().toString());
    }

    // 刷新世界列表
    refreshWorldList();
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
