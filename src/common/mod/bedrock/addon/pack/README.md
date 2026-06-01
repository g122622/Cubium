# 行为包系统

解析和管理基岩版行为包。

- `AddonManifest` — 基岩版manifest.json解析（format_version 2）
- `AddonModule` — 模块声明（script/data/resources类型）
- `AddonDependency` — 依赖声明
- `BehaviorPack` — 行为包（包含manifest和资源读取接口）
- `BehaviorPackList` — 行为包列表管理（全局+世界级，按优先级排序）
- `PackDependencyResolver` — UUID依赖解析
- `PackVersion` — 版本号（[major,minor,patch]格式）
