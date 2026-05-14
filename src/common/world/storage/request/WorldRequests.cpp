#include "WorldRequests.hpp"

namespace mc::world::storage {

CreateWorldRequest::CreateWorldRequest(std::string displayName,
    std::string requestedLevelId,
    u64 seed,
    WorldType worldType,
    GameMode gameMode,
    Difficulty difficulty,
    bool hardcore,
    bool allowCommands,
    i32 viewDistance)
    : displayName(std::move(displayName))
    , requestedLevelId(std::move(requestedLevelId))
    , seed(seed)
    , worldType(worldType)
    , gameMode(gameMode)
    , difficulty(difficulty)
    , hardcore(hardcore)
    , allowCommands(allowCommands)
    , viewDistance(viewDistance)
{}

LoadWorldRequest::LoadWorldRequest(
    std::string levelId, bool allowFutureVersion, bool createBackupBeforeUpgrade, bool allowStorageConversion)
    : levelId(std::move(levelId))
    , allowFutureVersion(allowFutureVersion)
    , createBackupBeforeUpgrade(createBackupBeforeUpgrade)
    , allowStorageConversion(allowStorageConversion)
{}

RenameWorldRequest::RenameWorldRequest(std::string levelId, std::string newDisplayName)
    : levelId(std::move(levelId))
    , newDisplayName(std::move(newDisplayName))
{}

DeleteWorldRequest::DeleteWorldRequest(std::string levelId)
    : levelId(std::move(levelId))
{}

BackupWorldRequest::BackupWorldRequest(std::string levelId, std::string reason)
    : levelId(std::move(levelId))
    , reason(std::move(reason))
{}

BackupWorldResult::BackupWorldResult(std::filesystem::path zipPath, u64 sizeBytes)
    : zipPath(std::move(zipPath))
    , sizeBytes(sizeBytes)
{}

} // namespace mc::world::storage
