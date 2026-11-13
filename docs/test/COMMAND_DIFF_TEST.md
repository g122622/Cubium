# 指令端到端 diff 测试（tests/e2e/command）

同一段指令序列同时喂给 **Cubium 服务端**与**真 vanilla 1.21.11 服务端**，逐条比对两者的
聊天反馈、服务端自省探针与客户端视角，差异按用例/步骤定位并落盘。

---

## 1. 为什么需要这一层

| 测试 | 回答什么 | 明确不覆盖 |
|---|---|---|
| `tests/unit` | 单个函数/类对不对 | 不经过命令解析、不经过世界状态 |
| `tests/integrated`（GameTest） | 行为包在进程内跑得通 | 无玩家、无网络、无原版对照 |
| `tests/e2e/bot` | 真客户端连上来能不能玩 | 只覆盖手写的少数交互路径 |
| `tests/diff` | 单个纯函数与原版是否一致 | 不涉及命令执行与世界状态 |
| **`tests/e2e/command`** | **同一指令序列在两侧的可观察结果是否一致** | 不覆盖渲染、不覆盖客户端交互 |

命令系统是图灵完备的世界操作接口：`execute`、`scoreboard`、`data`、`function`、`setblock`
等组合起来能覆盖方块、实体、物品、记分板与世界元数据。指令级 diff 用真实协议、真实 tick、
真实世界状态做判定，是前四层都覆盖不到的。

首轮 32 条用例立刻暴露了 62 处已知差异，其中多条是单元测试与 GameTest 看不见的行为缺陷
（见 §7）。

---

## 2. 结构

```
tests/e2e/command/
├── run.ts                      # CLI 入口（mode / case / strict）
├── package.json                # 依赖：mineflayer + minecraft-protocol + minecraft-data
├── cases/                      # 用例数据（JSON，按分类分子目录）
│   ├── block/ command/ data/ entity/ execute/ function/ item/ scoreboard/ world/
├── baselines/vanilla.json      # record 模式产出的 vanilla 冻结基线
└── src/
    ├── config.ts               # 世界档、bot 用户名、超时、棋盘参数
    ├── case.ts                 # 用例契约、加载校验、坐标占位符、棋盘分配
    ├── runner.ts               # 编排：启动 → 连 bot → 执行 → 比对 → 报告
    ├── driver/chat-driver.ts   # 逐条发送 + 静默窗口收集反馈
    ├── observe/
    │   ├── feedback.ts         # 采集 system_chat 与 profileless_chat
    │   ├── probes.ts           # 只读探针指令
    │   └── bot-view.ts         # 客户端视角（方块/实体/物品栏/天气）
    ├── compare/
    │   ├── normalize.ts        # 文本与 SNBT 规范形
    │   └── diff.ts             # 断言级比对与 knownDiff
    ├── baseline/store.ts       # 冻结基线的读写
    └── world/datapack.ts       # 最小数据包（函数/调度用例用）
```

共享设施在 `tests/e2e/shared/`（服务端进程、端口、ops.json、bot 工厂、等待助手），
与 `tests/e2e/bot` 共用。

---

## 3. 运行

```bash
cd tests/e2e/command

node run.ts                          # 双跑对比（默认）
node run.ts --case=scoreboard        # 只跑 id 含该子串的用例
node run.ts --strict                 # 有未登记差异时以非零码退出
node run.ts --mode=record            # 双跑并刷新 baselines/vanilla.json
node run.ts --mode=regress --strict  # 只跑 Cubium，与冻结基线比对（日常回归）
npm run test:unit                    # 框架纯逻辑单元测试（node --test）
```

`record` 支持增量：`--mode=record --case=<子串>` 只更新选中用例，其余条目保留。

产物：`build/e2e/command/artifacts/<runId>/report.json` + 两侧 `server.log`；
用例工作目录在 `build/e2e/command/runs/<runId>/`。

前置条件：

- 已构建服务端（`./scripts/configure.sh build`）；
- vanilla jar 位于 gradle 缓存（`~/.gradle/caches/fabric-loom/1.21.11/minecraft-server.jar`），
  可用 `MC_VANILLA_JAR` 覆盖；
- 本机有 `java`。

---

## 4. 用例格式

一个用例一个 JSON 文件，放在 `cases/<category>/<name>.json`：

```json
{
  "id": "scoreboard/player_ops",
  "title": "分数的设置、读取、加减、重置与运算",
  "category": "scoreboard",
  "setup": ["/scoreboard objectives add e2e_sb dummy"],
  "steps": [{ "command": "/scoreboard players get Alice e2e_sb" }],
  "probes": [{ "key": "alice", "command": "/scoreboard players get Alice e2e_sb" }],
  "clientView": [{ "key": "block", "kind": "block", "position": "{x} {y} {z}" }],
  "knownDiffs": [
    { "path": "steps[0].feedback", "reason": "目标名未用 [名称] 高亮组件", "evidence": "src/server/...cpp" }
  ]
}
```

- **坐标**：一律写 `{x}` / `{y}` / `{z}`，可带整数偏移（`{x+3}`、`{z-12}`）。runner 按用例
  序号分配独立棋盘并替换；用例里不出现绝对坐标。
- **setup**：准备指令，反馈全部丢弃，不参与判定。
- **steps**：判定主体，逐条执行并收集反馈。
- **probes**：只读查询指令，反馈作为结构化判定值（路径 `probes.<key>`）。
- **clientView**：bot 自己收到的世界状态（路径 `clientView.<key>`），用于捕捉「服务端改了
  但没同步」。`kind` 可为 `block` / `entity` / `inventory` / `time`。
- **knownDiffs**：断言级已知差异，`path` 只能是 `steps[<i>].feedback`、`probes.<key>` 或
  `clientView.<key>`，不允许整条用例豁免。

判定单元是**一条步骤**或**一个探针/观测**，不是整条用例——所以一条豁免不会掩盖同用例里
新出现的差异。

---

## 5. 判定与归一化

- **反馈文本**逐字比对。归一化只做等价改写：行尾空白、换行风格、SNBT 键序与数字写法
  （复用 `tests/diff/harness/core/snbt.ts` 的 `canonicalSnbt`）。
- **刻意不做 NFKC**：两侧消费同一份 `en_us.json`，任何字符形态差异都值得报出来。
- **数字格式、单复数、引号风格都是真实差异**，交给 `knownDiff` 显式登记，而不是在
  归一化里抹平。
- 已登记的差异若本次未出现，会报「登记过期」，提示删除登记。

---

## 6. 运行机制

1. 加载用例并按 id 排序；每条用例按序号分配棋盘（沿 Z 轴，间隔 8）。
2. 两侧**串行**启动：写 ops.json → 安装数据包 → 启动服务端 → 连 bot → 等 spawn。
3. 每条用例：
   - 把玩家钉到棋盘旁的小平台上（`{x}` 工作区正南 8 格的 `y-1` 处放石台，再 `/tp`）；
   - 执行 setup → steps → probes → clientView；
   - 反馈用「静默窗口」收集：收到消息后 120ms 内没有新消息才算本轮结束。
4. 归一化后逐步骤比对，落盘报告。
5. 停服：Cubium 硬杀，vanilla 走 stdin `stop`；vanilla 世界目录用完即删。

---

## 7. 首轮暴露并已修复的差异

首轮 32 条用例共暴露 62 处差异，现已全部修复（仅剩 1 处测试客户端渲染限制，见下）。
值得记录的根因：

| 用例 | 原现象 | 根因 |
|---|---|---|
| `command/*` | 解析错误不下发；字面量不匹配报「Expected literal」 | `ChatHandler` 只写日志；dispatcher 把字面量失败当成给玩家的错误 |
| `command/selector_errors` | 未知实体类型的 `@e[type=...]` 被当作空选择器执行 | 选择器解析期不校验实体注册表 |
| `scoreboard/player_ops` | `operation` 右操作数无分数时失败 | 用了 `getScore` 而不是 `getOrCreateScore`（vanilla 缺失分数按 0） |
| `data/storage` | `data remove storage ... <path>` 未真正删除 | `/data set|remove|modify` 用合并写回，删除的键被复活 |
| `block/*` | `destroy` 误判失败、`force` 不生效、零改动报成功 | 把 `setBlockState` 的「无变化」当成失败；fill/clone 未按改动数判定成败 |
| `command/argument_range` | `time set` 对参数取模 24000 | 回显用了 `dayTimeOfDay` 而不是累积 `dayTime` |
| `function/call` | 函数未抑制内部反馈、返回值与 vanilla 不同 | 未用抑制输出的源执行函数体；返回值取了执行数（vanilla 返回 0） |
| `entity/summon` | 未知实体类型泄漏翻译键 | 执行期才校验类型，且错误文本拼了原始键 |
| `client/inventory_sync` | 首次获得钻石没有成就广播 | 触发器注册晚于成就加载；`PlayerInventory::add` 绕过变更回调；完成成就从不广播 |
| `client/entity_sync` | 召唤盔甲架误触发铁傀儡/凋灵/末影龙成就 | 召唤触发器用了裸实体谓词，未走数据包的「条件列表」上下文 |

文案/组件级：物品名改用 `block.*`/`item.*` 翻译键并包裹方括号、记分板目标名统一
`[名称]` 高亮组件、`difficulty`/`weather`/`clear`/`worldborder`/`gamerule`/`schedule`
等命令改用 vanilla 翻译键与数字格式（`%.1f`/`%.2f`/`%f`）。

唯一保留的 knownDiff：`function/schedule` 的 `gametime` 参数。两侧服务端都发送 NBT
**long** 类型参数，而 mineflayer 无法渲染 long（显示为空）；Cubium 发送文本参数因此多出
数字。真实 Java 客户端两侧都会显示数字，属测试客户端限制。

---

## 8. 容易踩的坑

1. **反馈通道两侧不同**。vanilla 的 `/say` 走 `profileless_chat`（position `chat`），
   Cubium 降级为 `system_chat`（position `system`）。采集必须同时接受两者，否则一侧永远
   「无反馈」。
2. **只等第一条反馈会丢消息**。一条指令可以产生多行（函数调用先报 Running，再报函数体内
   容），必须用静默窗口收集，否则后续行会串到下一步。
3. **玩家与工作区必须分开**。`{x}/{y}/{z}` 是工作区中心；玩家站在旁边小平台上。混在一起时
   `setblock` 会落进玩家身体、`at @s` 的落点被用例本身改动。
4. **玩家会掉下去**。棋盘在空中，runner 每条用例前重建脚下石台；`worldborder` 用例若把边界
   设小，必须先把边界中心移到当前棋盘，否则玩家被边界推出/致死，反馈里混入「Player died」。
5. **不要比对会漂移的值**。`/time add` 的反馈包含结果时间、`/weather` 的随机持续秒数、
   `time query daytime` 的 tick 进度——它们每轮都不同。用例只保留确定性输出（`time set`），
   动态值要么去掉步骤，要么登记 knownDiff。
6. **数据包必须两侧都装**。Cubium 在 `<游戏目录>/datapacks`，vanilla 在
   `<cwd>/<worldName>/datapacks`，且必须在服务端**启动前**写好（vanilla 只在启动时扫描）。
7. **内置 vanilla 包必须始终可用**。原实现只要用户 datapacks 目录非空就不注入内置包，
   导致 worldgen 注册表为空、服务端启动即断言崩溃；已改为始终以最低优先级注入（见
   `fix(resource): 内置原版包始终以最低优先级注入`）。
8. **objective 列表顺序不可对齐**。vanilla 的输出顺序既不是 HashMap 桶序也不是插入序，
   无法从数据结构上复刻。`normalize` 对「There are N objective(s): ...」行做排序（列举是有序
   无关的集合），其余内容仍逐字比对。
9. **解析错误的上下文行不比**。`...<--[HERE]` 那一行是客户端帮助信息，截断/省略规则由
   Brigadier 与客户端渲染共同决定，`normalize` 将其统一成 `<parse-context>`；分号前的错误
   消息本身仍严格比对。
10. **触发器必须先于成就加载注册**。`Criterion::fromJson` 按触发器类型构造条件实例，注册表
   为空时所有条件退化为通用实例（运行时 dynamic_cast 失败、监听器永远不触发、成就无法完成）。
    注册点在 `RegistryBootstrap::initializeAll` 的成就加载之前。
11. **strict 只对未登记差异失败**。已登记差异不算失败，但「登记过期」会打印出来，定期清理。

---

## 9. 尚未实现（TODO）

- **旁观者 / 非 OP 视角**：需要一个常驻的第二 bot 验证广播可见性（`sendCommandSuccess`
  只发 OP）与权限分级。当前只有单个 OP bot，涉及多 bot 的用例需要扩展 runner。
- **崩溃续跑**：批次中断后从进度文件继续，而不是整轮重跑。
- **用例生成器 / fuzz**：用例是纯数据，下一步可以接生成器产出随机指令序列；当前只做手写用例。
- **report 模式**：从已有 `report.json` 重新渲染（当前直接读文件即可，暂不单独实现）。
