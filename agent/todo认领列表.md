# TODO 认领列表

本文档记录项目中被认领的 TODO 项，避免重复工作和冲突。

## 认领记录

| 时间 | 文件路径 | 行号 | TODO 内容摘要 | 状态 | 认领人 |
|------|----------|------|---------------|------|--------|
| 2026-05-12 | src/common/entity/loot/LootFunctions.cpp | 1130 | 实现 SetStewEffectFunction::apply 方法 | 已完成 | Claude |

## 状态说明

- **进行中**: 正在实现
- **暂停**: 暂时难以解决，等待依赖完成
- **已完成**: 已收敛完成
- **放弃**: 放弃此 TODO，原因见备注

## 已完成的 TODO 详情

### 2026-05-12: SetStewEffectFunction::apply 实现

**原始位置**: `src/common/entity/loot/LootFunctions.cpp:1130`

**完成内容**:
1. 实现了 `SetStewEffectFunction::apply()` 方法，可以为谜之炖菜添加状态效果
2. 新增了 `EffectType` 字符串转换工具函数：
   - `getEffectById(i32)` - 从数值 ID 获取效果类型
   - `getEffectByResourceLocation(ResourceLocation)` - 从资源位置获取效果类型
   - `getEffectResourceLocation(EffectType)` - 获取效果的资源位置
   - `getEffectResourceName(EffectType)` - 获取效果的资源名称
   - `isInstantEffect(EffectType)` - 检查效果是否为瞬间效果
3. 编写了完整的单元测试
