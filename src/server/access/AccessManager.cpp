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

#include "AccessManager.hpp"
#include "admission/BannedIpList.hpp"
#include "admission/BannedPlayerList.hpp"
#include "admission/WhitelistManager.hpp"
#include "permission/OpListManager.hpp"

namespace mc::server {
struct AccessManager::Lists {
    WhitelistManager whitelist;
    BannedPlayerList playerBan;
    BannedIpList ipBan;
    OpListManager operators;
};

AccessManager::AccessManager()
    : m_lists(std::make_unique<Lists>())
{}

AccessManager::~AccessManager() = default;

bool AccessManager::whitelistIsEnabled() const
{
    return m_lists->whitelist.isEnabled();
}

void AccessManager::whitelistSetEnabled(bool enabled)
{
    return m_lists->whitelist.setEnabled(enabled);
}

bool AccessManager::whitelistAddEntry(const WhitelistEntry& entry)
{
    return m_lists->whitelist.addEntry(entry);
}

bool AccessManager::whitelistRemoveEntry(const std::string& uuid)
{
    return m_lists->whitelist.removeEntry(uuid);
}

bool AccessManager::whitelistRemoveEntryByName(const std::string& name)
{
    return m_lists->whitelist.removeEntryByName(name);
}

bool AccessManager::whitelistIsWhitelisted(const std::string& uuid) const
{
    return m_lists->whitelist.isWhitelisted(uuid);
}

bool AccessManager::whitelistIsNameWhitelisted(const std::string& name) const
{
    return m_lists->whitelist.isNameWhitelisted(name);
}

std::optional<WhitelistEntry> AccessManager::whitelistGetEntry(const std::string& uuid) const
{
    return m_lists->whitelist.getEntry(uuid);
}

std::optional<WhitelistEntry> AccessManager::whitelistGetEntryByName(const std::string& name) const
{
    return m_lists->whitelist.getEntryByName(name);
}

std::vector<WhitelistEntry> AccessManager::whitelistGetAllEntries() const
{
    return m_lists->whitelist.getAllEntries();
}

std::vector<std::string> AccessManager::whitelistGetAllNames() const
{
    return m_lists->whitelist.getAllNames();
}

size_t AccessManager::whitelistSize() const
{
    return m_lists->whitelist.size();
}

bool AccessManager::whitelistEmpty() const
{
    return m_lists->whitelist.empty();
}

void AccessManager::whitelistClear()
{
    return m_lists->whitelist.clear();
}

Result<void> AccessManager::whitelistLoad(const std::filesystem::path& path)
{
    return m_lists->whitelist.load(path);
}

Result<void> AccessManager::whitelistSave(const std::filesystem::path& path)
{
    return m_lists->whitelist.save(path);
}

Result<void> AccessManager::whitelistSave()
{
    return m_lists->whitelist.save(std::filesystem::path{});
}

Result<void> AccessManager::whitelistReload()
{
    return m_lists->whitelist.reload();
}

std::filesystem::path AccessManager::whitelistFilePath() const
{
    return m_lists->whitelist.filePath();
}

bool AccessManager::playerBanAddEntry(const BannedPlayerEntry& entry)
{
    return m_lists->playerBan.addEntry(entry);
}

bool AccessManager::playerBanRemoveEntry(const std::string& uuid)
{
    return m_lists->playerBan.removeEntry(uuid);
}

bool AccessManager::playerBanRemoveEntryByName(const std::string& name)
{
    return m_lists->playerBan.removeEntryByName(name);
}

bool AccessManager::playerBanIsBanned(const std::string& uuid) const
{
    return m_lists->playerBan.isBanned(uuid);
}

bool AccessManager::playerBanIsNameBanned(const std::string& name) const
{
    return m_lists->playerBan.isNameBanned(name);
}

std::optional<BannedPlayerEntry> AccessManager::playerBanGetEntry(const std::string& uuid) const
{
    return m_lists->playerBan.getEntry(uuid);
}

std::optional<BannedPlayerEntry> AccessManager::playerBanGetEntryByName(const std::string& name) const
{
    return m_lists->playerBan.getEntryByName(name);
}

std::vector<BannedPlayerEntry> AccessManager::playerBanGetAllEntries() const
{
    return m_lists->playerBan.getAllEntries();
}

std::vector<std::string> AccessManager::playerBanGetAllBannedNames() const
{
    return m_lists->playerBan.getAllBannedNames();
}

size_t AccessManager::playerBanSize() const
{
    return m_lists->playerBan.size();
}

bool AccessManager::playerBanEmpty() const
{
    return m_lists->playerBan.empty();
}

void AccessManager::playerBanClear()
{
    return m_lists->playerBan.clear();
}

Result<void> AccessManager::playerBanLoad(const std::filesystem::path& path)
{
    return m_lists->playerBan.load(path);
}

Result<void> AccessManager::playerBanSave(const std::filesystem::path& path)
{
    return m_lists->playerBan.save(path);
}

Result<void> AccessManager::playerBanSave()
{
    return m_lists->playerBan.save(std::filesystem::path{});
}

Result<void> AccessManager::playerBanReload()
{
    return m_lists->playerBan.reload();
}

const std::filesystem::path& AccessManager::playerBanFilePath() const
{
    return m_lists->playerBan.filePath();
}

bool AccessManager::ipBanAddEntry(const BannedIpEntry& entry)
{
    return m_lists->ipBan.addEntry(entry);
}

bool AccessManager::ipBanRemoveEntry(const std::string& ip)
{
    return m_lists->ipBan.removeEntry(ip);
}

bool AccessManager::ipBanIsBanned(const std::string& ip) const
{
    return m_lists->ipBan.isBanned(ip);
}

std::optional<BannedIpEntry> AccessManager::ipBanGetEntry(const std::string& ip) const
{
    return m_lists->ipBan.getEntry(ip);
}

std::vector<BannedIpEntry> AccessManager::ipBanGetAllEntries() const
{
    return m_lists->ipBan.getAllEntries();
}

std::vector<std::string> AccessManager::ipBanGetAllBannedIps() const
{
    return m_lists->ipBan.getAllBannedIps();
}

size_t AccessManager::ipBanSize() const
{
    return m_lists->ipBan.size();
}

bool AccessManager::ipBanEmpty() const
{
    return m_lists->ipBan.empty();
}

void AccessManager::ipBanClear()
{
    return m_lists->ipBan.clear();
}

Result<void> AccessManager::ipBanLoad(const std::filesystem::path& path)
{
    return m_lists->ipBan.load(path);
}

Result<void> AccessManager::ipBanSave(const std::filesystem::path& path)
{
    return m_lists->ipBan.save(path);
}

Result<void> AccessManager::ipBanSave()
{
    return m_lists->ipBan.save(std::filesystem::path{});
}

Result<void> AccessManager::ipBanReload()
{
    return m_lists->ipBan.reload();
}

const std::filesystem::path& AccessManager::ipBanFilePath() const
{
    return m_lists->ipBan.filePath();
}

bool AccessManager::operatorSetEntry(const OpEntry& entry)
{
    return m_lists->operators.setEntry(entry);
}

bool AccessManager::operatorRemoveEntry(const std::string& uuid)
{
    return m_lists->operators.removeEntry(uuid);
}

bool AccessManager::operatorRemoveEntryByName(const std::string& name)
{
    return m_lists->operators.removeEntryByName(name);
}

bool AccessManager::operatorIsOp(const std::string& uuid) const
{
    return m_lists->operators.isOp(uuid);
}

bool AccessManager::operatorIsNameOp(const std::string& name) const
{
    return m_lists->operators.isNameOp(name);
}

OpLevel AccessManager::operatorGetLevel(const std::string& uuid) const
{
    return m_lists->operators.getLevel(uuid);
}

OpLevel AccessManager::operatorGetLevelByName(const std::string& name) const
{
    return m_lists->operators.getLevelByName(name);
}

bool AccessManager::operatorBypassesPlayerLimit(const std::string& uuid) const
{
    return m_lists->operators.bypassesPlayerLimit(uuid);
}

std::optional<OpEntry> AccessManager::operatorGetEntry(const std::string& uuid) const
{
    return m_lists->operators.getEntry(uuid);
}

std::optional<OpEntry> AccessManager::operatorGetEntryByName(const std::string& name) const
{
    return m_lists->operators.getEntryByName(name);
}

std::vector<OpEntry> AccessManager::operatorGetAllEntries() const
{
    return m_lists->operators.getAllEntries();
}

std::vector<std::string> AccessManager::operatorGetAllNames() const
{
    return m_lists->operators.getAllNames();
}

size_t AccessManager::operatorSize() const
{
    return m_lists->operators.size();
}

bool AccessManager::operatorEmpty() const
{
    return m_lists->operators.empty();
}

Result<void> AccessManager::operatorLoad(const std::filesystem::path& path)
{
    return m_lists->operators.load(path);
}

Result<void> AccessManager::operatorSave(const std::filesystem::path& path)
{
    return m_lists->operators.save(path);
}

Result<void> AccessManager::operatorSave()
{
    return m_lists->operators.save(std::filesystem::path{});
}

Result<void> AccessManager::operatorReload()
{
    return m_lists->operators.reload();
}

const std::filesystem::path& AccessManager::operatorFilePath() const
{
    return m_lists->operators.filePath();
}
} // namespace mc::server
