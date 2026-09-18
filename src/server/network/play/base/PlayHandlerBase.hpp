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

namespace mc::server {
class MinecraftServer;
} // namespace mc::server

namespace mc::server::net {

/**
 * @brief Play 包族处理器的公共基座
 *
 * 只提供对 `MinecraftServer` 的引用，是本子树的依赖末梢：只前置声明 `MinecraftServer`，
 * 任何处理器头文件因此都不必拖入 `server/application/MinecraftServer.hpp`。
 *
 * 持 `MinecraftServer&` 而非 `IServer&`：部分处理体要调 `MinecraftServer` 自身的纯虚
 * （`getHeldItemForPlacement` / `getSelectedHotbarSlot` / `setInventoryItem` /
 * `syncPlayerInventory` / `tryOpenCraftingContainer`），这些不在 `IServer` 上。
 */
class PlayHandlerBase {
public:
    PlayHandlerBase(const PlayHandlerBase&) = delete;
    PlayHandlerBase& operator=(const PlayHandlerBase&) = delete;
    PlayHandlerBase(PlayHandlerBase&&) = delete;
    PlayHandlerBase& operator=(PlayHandlerBase&&) = delete;
    ~PlayHandlerBase() = default;

protected:
    explicit PlayHandlerBase(MinecraftServer& server)
        : m_server(server)
    {}

    /// 服务端门面引用（非拥有，生命周期由 MinecraftServer 保证）。
    MinecraftServer& m_server;
};

} // namespace mc::server::net
