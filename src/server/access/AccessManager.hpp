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
#include "common/core/Result.hpp"
#include "server/access/base/BannedIpListTypes.hpp"
#include "server/access/base/BannedPlayerListTypes.hpp"
#include "server/access/base/OpListManagerTypes.hpp"
#include "server/access/base/WhitelistManagerTypes.hpp"
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc::server {
/** @brief 服务端准入和权限门面，名单实现仅在本领域内部持有。 */
class AccessManager {
public:
    AccessManager();
    ~AccessManager();
    /** @brief 白名单的 isEnabled 操作。 */
    bool whitelistIsEnabled() const;
    /** @brief 白名单的 setEnabled 操作。 */
    void whitelistSetEnabled(bool enabled);
    /** @brief 白名单的 addEntry 操作。 */
    bool whitelistAddEntry(const WhitelistEntry& entry);
    /** @brief 白名单的 removeEntry 操作。 */
    bool whitelistRemoveEntry(const std::string& uuid);
    /** @brief 白名单的 removeEntryByName 操作。 */
    bool whitelistRemoveEntryByName(const std::string& name);
    /** @brief 白名单的 isWhitelisted 操作。 */
    bool whitelistIsWhitelisted(const std::string& uuid) const;
    /** @brief 白名单的 isNameWhitelisted 操作。 */
    bool whitelistIsNameWhitelisted(const std::string& name) const;
    /** @brief 白名单的 getEntry 操作。 */
    std::optional<WhitelistEntry> whitelistGetEntry(const std::string& uuid) const;
    /** @brief 白名单的 getEntryByName 操作。 */
    std::optional<WhitelistEntry> whitelistGetEntryByName(const std::string& name) const;
    /** @brief 白名单的 getAllEntries 操作。 */
    std::vector<WhitelistEntry> whitelistGetAllEntries() const;
    /** @brief 白名单的 getAllNames 操作。 */
    std::vector<std::string> whitelistGetAllNames() const;
    /** @brief 白名单的 size 操作。 */
    size_t whitelistSize() const;
    /** @brief 白名单的 empty 操作。 */
    bool whitelistEmpty() const;
    /** @brief 白名单的 clear 操作。 */
    void whitelistClear();
    /** @brief 白名单的 load 操作。 */
    Result<void> whitelistLoad(const std::filesystem::path& path);
    /** @brief 白名单的 save 操作。 */
    Result<void> whitelistSave(const std::filesystem::path& path);
    Result<void> whitelistSave();
    /** @brief 白名单的 reload 操作。 */
    Result<void> whitelistReload();
    /** @brief 白名单的 filePath 操作。 */
    std::filesystem::path whitelistFilePath() const;
    /** @brief 玩家封禁的 addEntry 操作。 */
    bool playerBanAddEntry(const BannedPlayerEntry& entry);
    /** @brief 玩家封禁的 removeEntry 操作。 */
    bool playerBanRemoveEntry(const std::string& uuid);
    /** @brief 玩家封禁的 removeEntryByName 操作。 */
    bool playerBanRemoveEntryByName(const std::string& name);
    /** @brief 玩家封禁的 isBanned 操作。 */
    bool playerBanIsBanned(const std::string& uuid) const;
    /** @brief 玩家封禁的 isNameBanned 操作。 */
    bool playerBanIsNameBanned(const std::string& name) const;
    /** @brief 玩家封禁的 getEntry 操作。 */
    std::optional<BannedPlayerEntry> playerBanGetEntry(const std::string& uuid) const;
    /** @brief 玩家封禁的 getEntryByName 操作。 */
    std::optional<BannedPlayerEntry> playerBanGetEntryByName(const std::string& name) const;
    /** @brief 玩家封禁的 getAllEntries 操作。 */
    std::vector<BannedPlayerEntry> playerBanGetAllEntries() const;
    /** @brief 玩家封禁的 getAllBannedNames 操作。 */
    std::vector<std::string> playerBanGetAllBannedNames() const;
    /** @brief 玩家封禁的 size 操作。 */
    size_t playerBanSize() const;
    /** @brief 玩家封禁的 empty 操作。 */
    bool playerBanEmpty() const;
    /** @brief 玩家封禁的 clear 操作。 */
    void playerBanClear();
    /** @brief 玩家封禁的 load 操作。 */
    Result<void> playerBanLoad(const std::filesystem::path& path);
    /** @brief 玩家封禁的 save 操作。 */
    Result<void> playerBanSave(const std::filesystem::path& path);
    Result<void> playerBanSave();
    /** @brief 玩家封禁的 reload 操作。 */
    Result<void> playerBanReload();
    /** @brief 玩家封禁的 filePath 操作。 */
    const std::filesystem::path& playerBanFilePath() const;
    /** @brief 地址封禁的 addEntry 操作。 */
    bool ipBanAddEntry(const BannedIpEntry& entry);
    /** @brief 地址封禁的 removeEntry 操作。 */
    bool ipBanRemoveEntry(const std::string& ip);
    /** @brief 地址封禁的 isBanned 操作。 */
    bool ipBanIsBanned(const std::string& ip) const;
    /** @brief 地址封禁的 getEntry 操作。 */
    std::optional<BannedIpEntry> ipBanGetEntry(const std::string& ip) const;
    /** @brief 地址封禁的 getAllEntries 操作。 */
    std::vector<BannedIpEntry> ipBanGetAllEntries() const;
    /** @brief 地址封禁的 getAllBannedIps 操作。 */
    std::vector<std::string> ipBanGetAllBannedIps() const;
    /** @brief 地址封禁的 size 操作。 */
    size_t ipBanSize() const;
    /** @brief 地址封禁的 empty 操作。 */
    bool ipBanEmpty() const;
    /** @brief 地址封禁的 clear 操作。 */
    void ipBanClear();
    /** @brief 地址封禁的 load 操作。 */
    Result<void> ipBanLoad(const std::filesystem::path& path);
    /** @brief 地址封禁的 save 操作。 */
    Result<void> ipBanSave(const std::filesystem::path& path);
    Result<void> ipBanSave();
    /** @brief 地址封禁的 reload 操作。 */
    Result<void> ipBanReload();
    /** @brief 地址封禁的 filePath 操作。 */
    const std::filesystem::path& ipBanFilePath() const;
    /** @brief 操作员权限的 setEntry 操作。 */
    bool operatorSetEntry(const OpEntry& entry);
    /** @brief 操作员权限的 removeEntry 操作。 */
    bool operatorRemoveEntry(const std::string& uuid);
    /** @brief 操作员权限的 removeEntryByName 操作。 */
    bool operatorRemoveEntryByName(const std::string& name);
    /** @brief 操作员权限的 isOp 操作。 */
    bool operatorIsOp(const std::string& uuid) const;
    /** @brief 操作员权限的 isNameOp 操作。 */
    bool operatorIsNameOp(const std::string& name) const;
    /** @brief 操作员权限的 getLevel 操作。 */
    OpLevel operatorGetLevel(const std::string& uuid) const;
    /** @brief 操作员权限的 getLevelByName 操作。 */
    OpLevel operatorGetLevelByName(const std::string& name) const;
    /** @brief 操作员权限的 bypassesPlayerLimit 操作。 */
    bool operatorBypassesPlayerLimit(const std::string& uuid) const;
    /** @brief 操作员权限的 getEntry 操作。 */
    std::optional<OpEntry> operatorGetEntry(const std::string& uuid) const;
    /** @brief 操作员权限的 getEntryByName 操作。 */
    std::optional<OpEntry> operatorGetEntryByName(const std::string& name) const;
    /** @brief 操作员权限的 getAllEntries 操作。 */
    std::vector<OpEntry> operatorGetAllEntries() const;
    /** @brief 操作员权限的 getAllNames 操作。 */
    std::vector<std::string> operatorGetAllNames() const;
    /** @brief 操作员权限的 size 操作。 */
    size_t operatorSize() const;
    /** @brief 操作员权限的 empty 操作。 */
    bool operatorEmpty() const;
    /** @brief 操作员权限的 load 操作。 */
    Result<void> operatorLoad(const std::filesystem::path& path);
    /** @brief 操作员权限的 save 操作。 */
    Result<void> operatorSave(const std::filesystem::path& path);
    Result<void> operatorSave();
    /** @brief 操作员权限的 reload 操作。 */
    Result<void> operatorReload();
    /** @brief 操作员权限的 filePath 操作。 */
    const std::filesystem::path& operatorFilePath() const;

private:
    struct Lists;
    std::unique_ptr<Lists> m_lists;
};
} // namespace mc::server
