#pragma once

// 自动生成的版本和构建信息文件 - 不要手动编辑

// ==================== 版本信息 ====================
#define MC_VERSION_MAJOR 0
#define MC_VERSION_MINOR 1
#define MC_VERSION_PATCH 0

#define MC_VERSION_STRING "0.1.0"
#define MC_VERSION_NAME "Cubium"

// ==================== Git 信息 ====================
#define MC_GIT_COMMIT_HASH "93c69e5"
#define MC_GIT_COMMIT_HASH_FULL "93c69e5434d511bd1a80d566b40a85d8e27988f7"
#define MC_GIT_BRANCH "main"
#define MC_GIT_DIRTY

// ==================== 构建信息 ====================
#define MC_BUILD_TIME "2026-07-20T13:34:07Z"
#define MC_BUILD_TYPE "RelWithDebInfo"
#define MC_BUILD_PLATFORM "Windows"
#define MC_BUILD_ARCH "x64"

// ==================== 编译器信息 ====================
#define MC_COMPILER_ID "Clang"
#define MC_COMPILER_VERSION "20.1.8"
#define MC_COMPILER_STRING "Clang 20.1.8"

namespace mc {

// 版本常量
constexpr int VERSION_MAJOR = MC_VERSION_MAJOR;
constexpr int VERSION_MINOR = MC_VERSION_MINOR;
constexpr int VERSION_PATCH = MC_VERSION_PATCH;

// 构建信息常量
constexpr const char* GIT_COMMIT_HASH = MC_GIT_COMMIT_HASH;
constexpr const char* GIT_COMMIT_HASH_FULL = MC_GIT_COMMIT_HASH_FULL;
constexpr const char* GIT_BRANCH = MC_GIT_BRANCH;
constexpr bool GIT_DIRTY =
# ifdef MC_GIT_DIRTY
    true;
# else
    false;
# endif

constexpr const char* BUILD_TIME = MC_BUILD_TIME;
constexpr const char* BUILD_TYPE = MC_BUILD_TYPE;
constexpr const char* BUILD_PLATFORM = MC_BUILD_PLATFORM;
constexpr const char* BUILD_ARCH = MC_BUILD_ARCH;

constexpr const char* COMPILER_ID = MC_COMPILER_ID;
constexpr const char* COMPILER_VERSION = MC_COMPILER_VERSION;
constexpr const char* COMPILER_STRING = MC_COMPILER_STRING;

} // namespace mc
