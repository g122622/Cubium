# 差分测试（differential tests）

同一份输入喂给 Cubium 与参照实现（原版数据或 Java oracle），要求两侧输出**逐字节相同**。

这是项目的第四类测试：

| 目录 | 回答什么 | 期望值从哪来 |
|---|---|---|
| `tests/unit` | 单个函数/类对不对 | 人写的断言 |
| `tests/integrated` | 行为包在真实服务端里跑得对不对 | GameTest 的通过条件 |
| `tests/e2e/bot` | 真客户端连上来能不能玩 | 冻结的快照基线 |
| **`tests/diff`** | **和原版比，同一输入下结果一样吗** | **原版自己跑一遍** |

前三类里「期望值」都由人决定，人写错就查不出来；差分测试的期望值由原版给出，不存在写错。

---

## 结构

```
tests/diff/
├── CMakeLists.txt               # probe 目标定义（EXCLUDE_FROM_ALL）
├── probes/                      # C++ 侧小程序，只链 mc_common
│   ├── probe.hpp                # Probe 接口 + MC_DIFF_DEFINE_PROBE 宏
│   ├── runner_main.cpp          # 所有 probe 共用的 main（命令行、IO、错误处理）
│   └── text_encode.cpp          # 当前唯一能力：方块状态的文本表示
└── harness/                     # TypeScript 编排层（Node 24 原生 type stripping）
    ├── vanilla.ts               # 参照实现：读取 minecraft-data 的方块表
    ├── runner.ts                # 构建 / 运行 / 比对 / 写报告
    └── test_text_encode_parity.ts   # 第一个 harness
```

产物全部落在 `build/diff/<harness>/`：`inputs.jsonl`、`probe_outputs.jsonl`、`report.json`。

---

## 运行

```bash
# 构建并运行（首次会编译 probe；之后增量）
node tests/diff/harness/test_text_encode_parity.ts

# 已经构建过时跳过 cmake
node tests/diff/harness/test_text_encode_parity.ts --no-build

# 只构建
node tests/diff/harness/runner.ts build

# 指定其它构建目录（默认 <仓库>/build）
MC_DIFF_BUILD_DIR=build-noprof node tests/diff/harness/test_text_encode_parity.ts
```

退出码：`0` 全部一致；`1` 存在差异；`2` 环境问题（缺 probe、构建失败、缺原版数据）。

**前置**：`MC_BUILD_DIFF_PROBES` 默认为 `OFF`，构建脚本会自动打开它；Windows 上构建
必须经过 `scripts/configure.sh`（注入 VS 环境），`runner.ts` 已经这样调用。
参照数据来自 `tests/e2e/bot/node_modules/minecraft-data`，若未安装：
`cd tests/e2e/bot && npm install`。

---

## 一个 harness 的协议

C++ probe 只有两个子命令：

```
diff_<domain> list                          # 每行一个单元（方块状态文本）
diff_<domain> run <input.jsonl> <output.jsonl>   # 每行输入 → 一行 JSON 输出
```

`list` 枚举「可被枚举的全集」，`run` 按方块出摘要，两者不必是同一个集合——比对什么由
harness 决定。`runner.ts` 负责按 `key` 对齐两侧输出、规范化、比对、写报告。

**规范化必须在两侧都做**（`compareTransform`）。当前做了三件事：

1. 属性按名字排序 —— vanilla 的 `StateHolder#toString` 按名字字典序输出，注册顺序不是语义；
2. 取值域排序 —— minecraft-data 给字符串序（`"1","10","2"`），Cubium 给枚举声明序；
3. `DirectionProperty` 归一到 `EnumProperty` —— 前者是后者的子类，vanilla 的 `facing` 就是
   `EnumProperty<Direction>`。**只归一类型名，不放开取值比较**。

---

## 新增一个能力

1. 在 `probes/<domain>.cpp` 实现两个函数并调用宏：

   ```cpp
   void writeList(FILE* out);                        // 或不需要就传 nullptr
   std::string runLine(std::string_view inputLine);
   MC_DIFF_DEFINE_PROBE("<domain>", &writeList, &runLine)
   ```

   放在 `namespace mc::diff { ... }` 内（宏本身不开 namespace）。

2. 在 `tests/diff/CMakeLists.txt` 的三张平行列表**同一位置**各加一项：

   ```cmake
   set(MC_DIFF_PROBE_SUFFIXES text_encode encode_nbt)
   set(MC_DIFF_PROBE_SOURCES  probes/text_encode.cpp probes/encode_nbt.cpp)
   set(MC_DIFF_PROBE_DOMAINS  text_encode encode_nbt)
   ```

3. 在 `harness/` 下写 `test_<domain>_parity.ts`，参照实现放在 `<domain>.ts` 或
   `vanilla.ts` 里，调用 `runHarness()` 并自行决定退出码。

**确定性是硬要求**：同一输入必须永远产出同一输出。涉及无序容器、随机数、时间、
地址的，probe 必须在写出前自行收敛（排序、固定种子、去掉地址），否则差分没有意义。

---

## 当前状态与已知边界

`text_encode` 的结果：

```
1166 个方块摘要全部一致
29671 条状态文本全部一致
另有 648 条状态属于参照数据缺失的方块（purpur_wall、quartz_wall），未计入
```

差异清单（`ORACLE_MISSING_BLOCKS`）是**参照数据的缺口**，不是 Cubium 的缺陷；每次运行都会
核对它，参照数据一旦补齐就必须删掉条目，否则测试失败——防止清单变成永久豁免。

**参照实现的局限**：minecraft-data 只给属性与状态数，没有状态级 ID 的完整枚举，
也没有原版的解析语义。因此这一步验证的是「状态集合 + 属性名/类型/值域」，
**没有**验证状态 ID 分配顺序、BlockState→NBT 编码、BlockStateParser 的接受范围。

---

## 下一步：接 Java oracle

最终形态是链接 1.21.11 服务端 jar 的 Java oracle（参照 Horizon 的
`mods/horizon/src/test/java/dev/horizonlab/parity/Vanilla*Oracle.java`）：

```
tests/diff/java/                         # 新增
└── dev/horizonlab/diff/
    ├── OracleMain.java                  # 与 probe 同构：list / run
    └── VanillaBlockStateOracle.java     # 直接调 Blocks/BlockState.toString
```

两侧跑同一份 `inputs.jsonl`，byte-for-byte 比对。到那时 `ORACLE_MISSING_BLOCKS` 应当清空，
共有方块的 `stateId` 分配顺序也能一并纳入比较（目前刻意不比，因为 minecraft-data 无此信息）。

优先级更高的候选能力（按「只读、无状态、易对齐」排序）：

1. **方块状态 → NBT / 数字 ID 编解码** —— 直接落在存档与网络协议的 1:1 对齐上；
2. **物品 → 默认组件 / 堆叠上限** —— 与 `docs/数据驱动差异.md` 里那批硬编码表对应；
3. **命令解析树** —— 从原版导出 command tree，对同一批输入比解析结果；
4. **随机数序列** —— 先锁死 `RandomSource` 的位级等价，后面世界生成的差分才可能成立。