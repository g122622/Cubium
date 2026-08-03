## CI / GitHub Actions

项目配置了 GitHub Actions 持续集成，包含主CI工作流和自愈工作流。

### 主CI工作流 (`ci.yml`)

位于 `.github/workflows/ci.yml`，包含以下 Job：

| Job | 平台 | 说明 |
|-----|------|------|
| **build-windows** | Windows / Clang | 客户端+服务端 RelWithDebInfo 构建（快速反馈） |
| **build-linux** | Linux / Clang | 服务端 RelWithDebInfo 构建并运行测试 |
| **asan-ubsan** | Linux / Clang | AddressSanitizer + UndefinedBehaviorSanitizer 测试 |
| **tsan** | Linux / Clang | ThreadSanitizer 测试 |
| **stack-protect** | Linux / Clang | 栈保护 + 硬化构建（`-fstack-protector-strong`、`-D_FORTIFY_SOURCE=2`、PIE、RELRO、不可执行栈），并验证二进制安全属性 |
| **format-check** | Linux | clang-format 格式检查（仅检查变更文件） |

#### 关键设计

- **ASan+UBSan 与 TSan 分离**：两种 sanitizer 互斥，必须独立运行
- **Sanitizer 构建使用 Debug 模式**：`MC_ENABLE_SANITIZERS=ON` 时自动切换到 `-O1` 并禁用 `-march=native`、LTO、`-fno-stack-protector` 等优化选项
- **Linux Job 关闭客户端**：CI 无 GPU/Vulkan，所有 Linux Job 使用 `MC_BUILD_CLIENT=OFF`
- **vcpkg 缓存**：使用 `lukka/run-vcpkg@v11` 并锁定 baseline commit
- **并发控制**：同一分支/PR 的重复运行会自动取消
- **失败诊断输出**：每个 Job 在失败时输出结构化摘要信息，供自愈工作流分析

### 自愈CI工作流 (`self-heal.yml`)

位于 `.github/workflows/self-heal.yml`，当主CI工作流失败时自动触发，实现检测→诊断→修复的闭环：

#### 工作流程

```
CI失败 → 自愈工作流触发 → AI分析日志 → 分类
                                            ├─ 瞬时故障 → 自动重跑失败Jobs
                                            └─ 非瞬时故障 → 创建Issue分配给@copilot
                                                             → Copilot Agent自主修复
                                                             → 创建PR等待人工审查
```

#### 故障分类

| 类别 | 说明 | 处理方式 |
|------|------|----------|
| **transient** | 网络超时、vcpkg缓存损坏、runner临时问题 | 自动重跑失败的Jobs |
| **formatting** | clang-format格式违规 | 创建Issue，Copilot运行clang-format修复 |
| **build-error** | 编译错误（缺少include、类型不匹配、链接错误） | 创建Issue，Copilot修复代码 |
| **test-failure** | 单元测试断言失败 | 创建Issue，Copilot修复逻辑或测试 |
| **sanitizer** | ASan/UBSan/TSan违规（内存越界、UAF、数据竞争） | 创建Issue，Copilot修复内存/线程问题 |
| **infrastructure** | runner故障、磁盘空间不足、工具链问题 | 创建Issue，标记需人工干预 |

#### 安全机制

- **防循环**：同一分支最近20次提交中bot提交>=5则停止创建新Issue
- **去重检查**：同一CI run不创建重复的修复Issue
- **人工审查**：所有Copilot创建的PR必须经过人工review，不自动合并
- **最小权限**：工作流仅有`contents:read`、`actions:read`、`issues:write`、`models:read`权限

#### 启用自愈CI的前置条件

1. **创建 PAT**：在 GitHub Settings → Developer settings → Fine-grained tokens 创建 `auto-remediation` token，权限：Issues(Read/Write)、Actions(Read)、Models(Read)、Contents(Read)
2. **添加 Secret**：在仓库 Settings → Secrets → Actions 中添加 `AUTO_REMEDIATION_PAT`
3. **启用 GitHub Models**：仓库 Settings → Copilot/Models，确保 GitHub Models 已启用
4. **启用 Copilot Coding Agent**：仓库 Settings → Copilot，确保 Coding Agent 已启用
5. **分支保护**：`main` 分支要求 PR review + status checks
