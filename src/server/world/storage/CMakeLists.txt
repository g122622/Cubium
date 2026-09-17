# src/server/world/storage/CMakeLists.txt
#
# mc_server_storage - world/storage 实现 OBJECT 库
#
# 架构规则：src/common 不得依赖 src/server。world/storage 整体（含 core/、list/、
# request/ 纯文件系统层与 db/、backend/、blockentity/、entity/、player/、save/、
# section/、snapshot/、task/、reader/ RocksDB 运行时层）从 common 迁移到 server 后，
# 既被 minecraft-server（生产服务端）编译，也被 minecraft-client（内置服，client
# CMakeLists.txt 直接编译 IntegratedServer.cpp/MinecraftServer.cpp）/ mc_tests /
# mc_benchmarks / mc_trident_tests 编译。抽成 OBJECT 库后，同一批 .o 被各 target
# 复用，避免重复编译与重复符号。
#
# mc_server_storage PRIVATE 链接 mc_common：编译 mc_server_storage 自身 .cpp 时需要
# common 符号与头路径（Result.hpp/各 world 类/CompressionUtils 等），但 mc_server_storage
# 是纯实现 OBJECT 库，不对外暴露头接口，故用 PRIVATE 避免向使用方传递 mc_common 链接
# （minecraft-server / minecraft-client / mc_tests / mc_benchmarks / mc_trident_tests
# 本就各自直接链接 mc_common，传递会造成 "duplicate libraries" 警告）。
#
# 外部库归属（随 storage 从 mc_common 迁移至 mc_server_storage）：
# - RocksDB::rocksdb：db/、section/、snapshot/、entity/、blockentity/ 等 RocksDB 运行时
# - leveldb::leveldb：backend/BedrockLDBBackend、reader/bedrock/BedrockLevelDb 等
# - zstd::libzstd：RocksDB 压缩、reader/bedrock/ PaletteUtil 等
# - LibArchive::LibArchive：list/WorldListService.cpp backupWorld() 压缩存档
# - ZLIB::ZLIB：reader 解压 level.dat（gzip）、LibArchive 间接依赖
add_library(mc_server_storage OBJECT
    GlobalStorageManager.cpp
    SingleLevelStorageManager.cpp
    backend/BedrockLDBBackend.cpp
    backend/JavaAnvilBackend.cpp
    blockentity/BlockEntityStorageManager.cpp
    core/LevelDatCodec.cpp
    core/SaveFormat.cpp
    core/WorldSessionLock.cpp
    core/WorldStoragePaths.cpp
    db/RocksDBDatabase.cpp
    db/SectionCodec.cpp
    entity/EntityStorageManager.cpp
    list/WorldListEntry.cpp
    list/WorldListService.cpp
    list/WorldNameSanitizer.cpp
    player/PlayerDataManager.cpp
    player/PlayerSaveData.cpp
    reader/bedrock/BedrockBiomeMapper.cpp
    reader/bedrock/BedrockChunkReader.cpp
    reader/bedrock/BedrockColumnReader.cpp
    reader/bedrock/BedrockLevelDatReader.cpp
    reader/bedrock/BedrockLevelDb.cpp
    reader/bedrock/BedrockWorldReader.cpp
    reader/bedrock/LevelDBKey.cpp
    reader/bedrock/PaletteUtil.cpp
    reader/java/JavaBiomeMapper.cpp
    reader/java/JavaBlockStateMapper.cpp
    reader/java/JavaChunkReader.cpp
    reader/java/JavaColumnReader.cpp
    reader/java/JavaLevelDatReader.cpp
    reader/java/JavaWorldReader.cpp
    reader/java/RegionFile.cpp
    request/WorldRequests.cpp
    save/AutoSave.cpp
    save/DirtyTracker.cpp
    section/SectionCache.cpp
    section/SectionManager.cpp
    snapshot/BackupManager.cpp
    task/StorageTask.cpp
    task/StorageTaskManager.cpp
)

target_link_libraries(mc_server_storage PRIVATE
    mc_common
    RocksDB::rocksdb
    leveldb::leveldb
    zstd::libzstd
    LibArchive::LibArchive
    ZLIB::ZLIB
)

mc_set_compiler_warnings(mc_server_storage)
