#pragma once

#include "common/core/Types.hpp"

#include <string_view>

namespace mc::test {

/**
 * @brief 内置结构模板引导：向全局 `TemplateManager` 注入 GameTest 框架依赖的程序化空模板。
 *
 * 框架样例测试（`BuiltinNativeTests`）与 JS `register` 默认结构均引用 `gametest:empty_3x3`。
 * 项目当前未提供 `.nbt` 资源文件，结构放置器（`MinecraftStructurePlacer`）取模板会失败导致测试必 fail。
 * 此引导在服务端启动期（`GameTestServer`/`IntegratedServer`/`StandaloneServer` initialize 末尾）注入
 * 程序化空模板兜底：`createProceduralTemplate` 生成全 air 的 3×3×3 模板，`placeInWorld` 立即返回成功
 * 不写方块，结构放置成功不阻塞测试。
 *
 * 幂等：`addTemplate` 同 key 覆盖，重复调用安全。线程不安全——须在主线程启动期调用。
 */

/**
 * @brief 注入名为 `gametest:<name>` 的程序化空模板（width×height×depth 全 air）。
 *
 * @param name 模板短名（注入为 `gametest:<name>`）。
 * @param width 模板宽度（X 跨度）。
 * @param height 模板高度（Y 跨度）。
 * @param depth 模板深度（Z 跨度）。
 * @return 注入成功返回 true；TemplateManager 单例不可达返回 false。
 */
bool injectProceduralStructure(std::string_view name, i32 width, i32 height, i32 depth);

/**
 * @brief 注入框架内置样例测试依赖的全部程序化空模板。
 *
 * 当前注入 `gametest:empty_3x3`（3×3×3）。新增内置结构在此追加。
 * 在 `GameTestServer::initialize`、生产服务器 `/gametest` 在线路径注册前调用。
 */
void ensureBuiltinStructureTemplates();

} // namespace mc::test
