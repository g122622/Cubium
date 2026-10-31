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

/*
 * types.ts — 编排层与能力声明之间的公共类型、共享常量。
 *
 * 依赖方向是单向的：能力声明（harnesses/index.ts）只依赖这里的类型；运行层
 * （runner/*）实现这些类型。反过来会让声明表被迫引入运行层，主进程也就无法在
 * 「不构建、不跑 Java」的前提下读取元数据。
 *
 * `HarnessContext` 用结构化接口描述 `beforeUnits` 能用的动作，具体实现由 runner/session.ts
 * 提供：声明不需要知道 session 类的内部结构，只需要「能编译 oracle、能枚举单元」。
 */

/** Java oracle 源码目录（仓库内）。 */
export const ORACLE_SOURCE_DIR = "tests/diff/java";

/** Java oracle 的包名，与目录一一对应。 */
export const ORACLE_PACKAGE = "dev.horizonlab.diff";

/**
 * 每次运行都必须计入的输入：编排层自身的代码、构建入口与版本锁定文件。
 *
 * 它们被所有 harness 共用，任何一项变了都意味着「上一次的结论不再可信」。例如改了比对语义
 * （core/comparison.ts）只影响比较结果，不会出现在任何 harness 的运行期模块里，必须显式计入。
 */
export const SHARED_INPUTS: readonly string[] = [
    "tests/diff/harness/runner.ts",
    "tests/diff/harness/worker.ts",
    "tests/diff/harness/harnesses/index.ts",
    "tests/diff/harness/core/types.ts",
    "tests/diff/harness/core/hash.ts",
    "tests/diff/harness/core/comparison.ts",
    "tests/diff/harness/core/snbt.ts",
    "tests/diff/harness/runner/session.ts",
    "tests/diff/harness/runner/graph.ts",
    "tests/diff/harness/runner/layout.ts",
    "tests/diff/harness/runner/state.ts",
    "tests/diff/harness/runner/module-trace.ts",
    "tests/diff/harness/runner/scheduler.ts",
    "tests/diff/harness/runner/build.ts",
    "tests/diff/harness/runner/select.ts",
    "tests/diff/harness/oracle/environment.ts",
    "tests/diff/harness/oracle/cache.ts",
    "tests/diff/harness/data/vanilla.ts",
    "tests/diff/harness/data/probes.ts",
    "tests/diff/tools/ninja.ts",
    "tests/diff/probes/probe.hpp",
    "tests/diff/probes/runner_main.cpp",
    "vcpkg.json",
    "CMakeLists.txt",
];

/**
 * `beforeUnits` 能用的运行上下文（由 runner/session.ts 实现）。
 *
 * 只暴露枚举单元需要的动作：产物目录、编译 oracle、枚举单元。声明不需要（也不应该）
 * 拿到整个 session。
 */
export interface HarnessContext {
    /** 一个能力的产物目录：`probe` 为运行工作目录，`oracle` 为 Java 侧工作目录。 */
    outputDirectory(name: string, kind?: "probe" | "oracle"): string;
    /** oracle 的类目录（每个能力一份）。 */
    classesDirectory(domain: string): string;
    /** 编译 oracle（javac 幂等：没有变化的类不会重新编译）。 */
    compileOracle(oracle: string, domain: string): void;
    /** 带类目录的 classpath 字符串。 */
    oracleClasspath(classes: string): string;
    /** 子进程环境（已叠加原版数据包目录，两侧读同一份）。 */
    readonly environment: NodeJS.ProcessEnv;
    /** oracle 的 `list` 子命令：枚举单元。 */
    listOracleUnits(oracle: string, domain: string): string[];
    /** 运行 probe 的 `list` 子命令：枚举被测侧能提供的全部单元。 */
    listProbeUnits(probe: string): string[];
    /** minecraft-data 的加载器（离线参照数据；只有纯数据能力用它）。 */
    readonly minecraftData?: () => unknown;
}

/**
 * 单元来源：
 *   - `listing`：由 oracle（有 oracle 时）或 probe 的 `list` 子命令枚举；
 *   - `computed`：由能力自己计算（例如「表 × 场景」展开，或两侧枚举口径不同时自行组合）。
 */
export type UnitsSource =
    | { readonly kind: "listing" }
    | { readonly kind: "computed"; readonly units: () => string[] };

/**
 * 一个差分能力的完整声明。
 *
 * `oracle` 与 `probe` 至少有一个：纯数据能力没有 Java 侧，用 `reference` 计算参照值。
 */
export interface HarnessDeclaration {
    readonly name: string;
    /** 探针域名（可执行文件 diff_<domain>）。 */
    readonly probe?: string;
    /** oracle 类名。 */
    readonly oracle?: string;
    /** 输出中标识单元的字段名，如 "block"。 */
    readonly key: string;
    readonly units: UnitsSource;
    /** 单元 → 探针的一行输入。缺省时输入是 `{"<key>": unit}`。 */
    readonly input?: (unit: string) => unknown;
    /** 比较前对两侧输出的规范化。 */
    readonly normalise?: (value: unknown) => unknown;
    /** 语义无序的数组路径。 */
    readonly unorderedArrays?: readonly string[];
    /**
     * 调 oracle 时追加的参数（放在 `run` 子命令之后）。
     *
     * 用途是让参照侧「跳过本项目的已知缺口」：例如 command_tree 的原版命令树包含九个
     * Cubium 尚未注册的根命令，它们的子节点会把同一批参数类型节点撑成形，导致其余节点
     * 也处于不同状态。把这些命令名传给 oracle，它会在注册阶段就跳过它们，两侧从此可比。
     */
    readonly oracleArgs?: readonly string[];
    /**
     * 原版有、本项目尚未实现的单元（显式缺口）。
     *
     * 明确给定的数组优先；没有给时，若声明里给了 `deriveMissing` 则由它按两侧实际列出的
     * 单元推导（用于「静态缺口清单会滞后于注册表」的场合）。
     */
    readonly oracleMissing?: readonly string[];
    /** 由两侧实际列出的单元推导缺口（`oracleMissing` 缺省时生效）。 */
    readonly deriveMissing?: (oracleUnits: readonly string[], probeUnits: readonly string[]) => string[];
    /** 成员级缺口：从原版一侧剔除的成员。 */
    readonly memberGaps?: { readonly field: string; readonly missing: readonly string[] };
    /**
     * 已知差异：登记后**不判失败**，但会在报告与摘要里点名条数与原因。
     *
     * 与 `oracleMissing`（剔除整个单元）和 `memberGaps`（剔除成员）的区别：这里不剔除任何
     * 数据，差异照常产生、照常记录，只是「预期之内」。用法仅限「已定位根因、修法已写进 TODO、
     * 但需要单独一轮工作」的缺口；修好后必须删掉登记，否则新缺陷会被一起吞掉。
     *
     * 一条差异命中的是「完整单元键」或「单元键#字段路径」（第一层字段名）两者之一，
     * 因此 `{ key: "minecraft:chests/x#plain", field: "slots" }` 只放过该单元的 slots 段。
     * `key: "*"` 表示所有单元（单元名要问 oracle 才能知道时用），但**必须**带 `field`：
     * 整单元放行的通配等于关掉整个能力，不允许。
     */
    readonly knownDifferences?: readonly { readonly key: string; readonly field?: string; readonly reason: string }[];
    /** 本项目独有、原版没有的成员：从被测一侧剔除。 */
    readonly projectOnly?: {
        readonly field: string;
        readonly extra: readonly string[];
        readonly drop?: (record: Record<string, unknown>, matches: (name: string) => boolean) => Record<string, unknown>;
    };
    /** 纯数据能力的参照实现：单元 → 参照输出对象。 */
    readonly reference?: (unit: string) => unknown;
    /**
     * 互斥组：同组能力永不并发运行。
     *
     * 用于「oracle 占用进程外的固定资源」的情形：战利品族 oracle 在 `java.io.tmpdir` 下构建
     * 原版服务端世界并持有区域文件锁，两个同类 oracle 并行会互相锁死。资源路径由原版决定，
     * 编排层只能靠串行化规避。
     */
    readonly exclusiveGroup?: string;
    /** 在枚举单元之前执行（例如导出 oracle 侧夹具）。 */
    readonly beforeUnits?: (context: HarnessContext) => void;
    /** 额外依赖（例如导出夹具后新增的文件），相对仓库根。 */
    readonly extraDependencies?: () => readonly string[];
}

/** 一次运行的结果（执行进程回报给主进程）。 */
export interface RunOutcome {
    readonly name: string;
    readonly status: "pass" | "fail" | "error";
    readonly units: number;
    readonly matched: number;
    /** 意外差异的条数（决定 `status`）。 */
    readonly differences: number;
    /** 已登记的已知缺口条数（记录在案，不判失败）。 */
    readonly knownDifferences?: number;
    /** 已知缺口涉及的单元键（受报告条数上限约束）。 */
    readonly knownKeys?: readonly string[];
    /** oracle 缓存键（没有 oracle 时 undefined）。 */
    readonly oracleKey?: string;
    /** 原版输出文件（调试与生成参考表用）。 */
    readonly oracleOutput?: string;
    readonly reportPath: string;
    readonly seconds: number;
    /** 失败摘要（差异处数或异常信息）。 */
    readonly failure?: string;
    /** 本次实际加载的 TS 模块（相对仓库根）。 */
    readonly modules: readonly string[];
}