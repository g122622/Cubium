/**
 * 找出“游离源文件”：仓库内有、但没被任何 CMake 目标收集、也没被任何聚合单元包含的 C/C++ 源文件。
 *
 * 数据来源（三路交叉，逐级兜底）：
 *   1. CMake 脚本静态解析：按语义执行——变量（`set`/`list`/`math(EXPR)`）、`foreach`、`function`/`macro`、
 *      `include()` 引入的脚本、`add_subdirectory` 子树、`file(GLOB[_RECURSE])` 通配，以及
 *      `add_library`/`add_executable`/`target_sources` 命令里的源清单。能识别注释与带引号参数。
 *   2. 聚合单元：`Xxx.cpp` 里 `#include "Other.cpp"` 的成员不单独登记，属既定约定
 *      （见 src/server/world/gen/feature/tree/foliage/README.md）。
 *   3. compile_commands.json（构建目录存在时自动启用）：CMake 真正喂给编译器的结果，作为最终真值。
 *
 * 用法：
 *   node --experimental-strip-types scripts/diag/orphan_sources.ts            # 只列出游离文件
 *   node --experimental-strip-types scripts/diag/orphan_sources.ts --all      # 列出全部文件与状态
 *   node --experimental-strip-types scripts/diag/orphan_sources.ts --json     # 机器可读输出
 *   node --experimental-strip-types scripts/diag/orphan_sources.ts --build-dir build
 *
 * 退出码：存在游离文件为 1，否则为 0（可直接当门禁用）。
 */
import { existsSync, readdirSync, readFileSync, statSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const SCRIPT_DIR = path.dirname(fileURLToPath(import.meta.url));
const REPO_ROOT = path.resolve(SCRIPT_DIR, "..", "..");

/** 参与判定的源码扩展名。 */
const SOURCE_EXTENSIONS = new Set([".cpp", ".cc", ".cxx", ".c"]);

/** 不参与扫描的目录。 */
const SKIP_DIR_NAMES = new Set(["third_party", "build", ".git", "node_modules", ".vs", ".idea", ".vscode"]);

/**
 * 参数中出现的源文件视为“被收集”的命令。
 * 不含 add_custom_command/add_custom_target：它们只是构建步骤的依赖或命令参数，不产生编译单元。
 */
const COLLECTING_COMMANDS = new Set(["add_library", "add_executable", "target_sources"]);

/** 单次求值的命令上限，防止 `foreach(... RANGE 1e9)` 之类的爆炸。 */
const MAX_EVALUATION_STEPS = 500_000;

/** 生成器表达式 `$<CONFIG:Debug>` 在路径判定时忽略。 */
const GENERATOR_EXPRESSION = /\$<[^>]*>/g;

/** 块起始命令 -> 该块结束后下一条命令的偏移（parseRange 用，避免重复扫描）。 */
const blockEndOffsets = new WeakMap<Command, number>();

/** 块级命令与其结束命令。 */
const BLOCK_END: ReadonlyMap<string, string> = new Map([
    ["if", "endif"],
    ["foreach", "endforeach"],
    ["while", "endwhile"],
    ["function", "endfunction"],
    ["macro", "endmacro"],
    ["block", "endblock"],
]);

/** if 块内部的分支分隔命令：不是独立命令，但把 if 体切成多段。 */
const BRANCH_DELIMITERS = new Set(["elseif", "else"]);

/** 块结束命令：只有所属的块才消费它，其余场景一律视为边界。 */
const END_DELIMITERS = new Set(["endif", "endforeach", "endwhile", "endfunction", "endmacro", "endblock"]);

interface CliOptions {
    readonly showAll: boolean;
    readonly json: boolean;
    readonly buildDir: string;
}

// ============================================================================
// CMake 解析：命令树
// ============================================================================

interface Command {
    readonly name: string;
    /** 原始参数（保留 `${}` 与引号标记），在求值阶段展开。 */
    readonly rawArgs: string[];
    /** 起止偏移，用于截取块原文。 */
    readonly start: number;
    readonly end: number;
}

interface BlockCommand {
    readonly name: string;
    /** 块起始命令自身的原始参数（如 `foreach(<var> ${LIST})` 的 `<var> ${LIST}`）。 */
    readonly headerArgs: string[];
    /** 起止命令之间的原文，用于 foreach 循环体重复执行。 */
    readonly bodyText: string;
    readonly child: CommandNode;
    /**
     * 仅 if 块：`then` / `elseif` / `else` 各自的分支体。
     * 每项依次返回，静态求值时所有分支都会执行，但 `return()` 只结束**当前分支**。
     */
    readonly branches?: readonly CommandNode[];
}

type CommandNode = readonly (Command | BlockCommand)[];

function isBlockCommand(node: Command | BlockCommand): node is BlockCommand {
    return (node as BlockCommand).bodyText !== undefined;
}

/**
 * 去掉 CMake 注释（`#[[ ... ]]` 块注释与 `#` 行注释）。
 *
 * 本仓库的源文件清单通常逐行带注释说明，注释里也常出现文件名，不清除会被误判成登记项。
 * 字符串字面量**原样保留**：它既是真实参数（`MC_SOURCE_DIR="${CMAKE_SOURCE_DIR}"`、
 * `math(EXPR out "${N} - 1")`），也可能出现在正则里；清空会丢信息。
 * 去注释时保留换行，保证命令偏移与原文一致。
 */
function stripComments(text: string): string {
    const chars = text.split("");
    const protect = (from: number, to: number): void => {
        for (let i = from; i < to; i += 1) {
            if (chars[i] !== "\n" && chars[i] !== "\r") {
                chars[i] = " ";
            }
        }
    };
    let index = 0;
    while (index < chars.length) {
        const ch = chars[index];
        // 字符串字面量在前：其中的 `#`（如 `"#include ..."`）不是注释起点，
        // 反过来的话会把同行后续的引号一起吞掉，导致之后所有命令的引号配对全部错位。
        if (ch === '"') {
            let j = index + 1;
            while (j < chars.length && chars[j] !== '"') {
                j += chars[j] === "\\" ? 2 : 1;
            }
            index = j + 1;
            continue;
        }
        if (ch === "#") {
            const block = /^#\[(=*)\[/.exec(text.slice(index, index + 8));
            if (block !== null) {
                const closingToken = `]${block[1]}]`;
                const closeIndex = text.indexOf(closingToken, index + block[0].length);
                const end = closeIndex < 0 ? text.length : closeIndex + closingToken.length;
                protect(index, end);
                index = end;
                continue;
            }
            const lineEnd = text.indexOf("\n", index);
            const commentEnd = lineEnd < 0 ? text.length : lineEnd;
            protect(index, commentEnd);
            index = commentEnd;
            continue;
        }
        index += 1;
    }
    return chars.join("");
}

/** 从 `(` 位置读到配对的 `)`，返回参数体原文与结束位置。 */
function readArgumentBody(text: string, openParenIndex: number): { body: string; end: number } {
    let depth = 0;
    let quoted = false;
    for (let i = openParenIndex; i < text.length; i += 1) {
        const ch = text[i];
        if (quoted) {
            if (ch === "\\") {
                i += 1;
            } else if (ch === "\"") {
                quoted = false;
            }
            continue;
        }
        if (ch === "\"") {
            quoted = true;
        } else if (ch === "(") {
            depth += 1;
        } else if (ch === ")") {
            depth -= 1;
            if (depth === 0) {
                return { body: text.slice(openParenIndex + 1, i), end: i + 1 };
            }
        }
    }
    return { body: text.slice(openParenIndex + 1), end: text.length };
}

/**
 * 带引号的参数前缀标记：用于区分“带引号（不切分列表）”与普通参数。
 *
 * 必须用普通字符串：`\u0000` 在 Node 的类型剥离下会被改写成字面量 `u0000`，标记随之失效。
 */
const QUOTED_MARK = "\u0001q";

function isQuotedToken(token: string): boolean {
    return token.startsWith(QUOTED_MARK);
}

function unquoteToken(token: string): string {
    return isQuotedToken(token) ? token.slice(QUOTED_MARK.length) : token;
}

/** 把参数体切成原始参数项：去掉包裹引号并打上标记，保留 `${}` 原文。 */
function splitRawArguments(body: string): string[] {
    const tokens: string[] = [];
    let current = "";
    let quoted = false;
    let braceDepth = 0;
    let hasContent = false;
    for (let i = 0; i < body.length; i += 1) {
        const ch = body[i];
        if (braceDepth === 0 && ch === "\"") {
            quoted = !quoted;
            hasContent = true;
            continue;
        }
        if (ch === "$" && body[i + 1] === "{") {
            braceDepth += 1;
            current += ch;
            hasContent = true;
            continue;
        }
        if (ch === "}" && braceDepth > 0) {
            braceDepth -= 1;
            current += ch;
            hasContent = true;
            continue;
        }
        if (braceDepth === 0 && !quoted && /\s/.test(ch)) {
            if (hasContent) {
                tokens.push(quoted ? `${QUOTED_MARK}${current}` : current);
                current = "";
                hasContent = false;
            }
            continue;
        }
        current += ch;
        hasContent = true;
    }
    if (hasContent) {
        tokens.push(quoted ? `${QUOTED_MARK}${current}` : current);
    }
    return tokens;
}

/** 把脚本文本解析成命令树（块级命令嵌套成子节点）。 */
function parseCommandTree(text: string): CommandNode {
    return parseRange(text, 0, text.length, true).nodes;
}

/**
 * 解析 `[startOffset, text.length)` 范围内的命令序列。
 *
 * @param root 是否为脚本根作用域。根作用域里孤立的 elseif/else/endif 等没有归属，直接丢弃；
 *             非根作用域一律把边界命令交还调用方（块体需要据此切分）。
 * @returns 命令树，以及遇到边界命令时的停止位置
 */
function parseRange(
    text: string,
    startOffset: number,
    endOffset: number,
    root: boolean,
): { nodes: (Command | BlockCommand)[]; stoppedAt: Command | null } {
    const nodes: (Command | BlockCommand)[] = [];
    let index = startOffset;
    const regex = /\b([A-Za-z_][A-Za-z0-9_]*)[ \t]*\(/g;
    while (index < endOffset) {
        regex.lastIndex = index;
        const match = regex.exec(text);
        if (match === null || match.index >= endOffset) {
            break;
        }
        const name = match[1].toLowerCase();
        const openParenIndex = match.index + match[0].length - 1;
        const { body, end } = readArgumentBody(text, openParenIndex);
        const command: Command = { name, rawArgs: splitRawArguments(body), start: match.index, end };

        // 属于外层的分支/结束命令。根作用域里这类命令没有所属块（等价于语法错误），跳过即可；
        // 块体内则交还调用方处理。
        if (BRANCH_DELIMITERS.has(name) || END_DELIMITERS.has(name)) {
            if (!root) {
                return { nodes, stoppedAt: command };
            }
            index = end;
            continue;
        }

        // 用 Map 查表：BLOCK_END 是普通对象，`BLOCK_END["constructor"]` 会取到原型链成员，
        // 使普通命令被误判成块，这里用 Map 避免该问题。
        const endName = BLOCK_END.get(name);
        if (endName === undefined) {
            nodes.push(command);
            index = end;
            continue;
        }
        // 块节点自带起止命令的参数；块内部由 executeBlock 单独执行，绝不能在这里把
        // 内部命令（尤其是嵌套的 if/function 内容）当成本层命令交给 execute。
        nodes.push(parseBlock(text, command, name, endName));
        index = blockEndOffsets.get(command) ?? command.end;
    }
    return { nodes, stoppedAt: null };
}

/** 解析一个块级命令（if/foreach/function/...），返回折叠后的块节点。 */
function parseBlock(text: string, opener: Command, name: string, endName: string): BlockCommand {
    // 一次扫描同时得到配对结果，避免 parseRange 里重复扫描同一段文本。
    const closing = findClosingCommand(text, opener.end, name, endName);
    blockEndOffsets.set(opener, closing === null ? opener.end : closing.end);
    if (closing === null) {
        // 没有配对的结束命令（脚本被截断或写法非法）：按普通命令处理，避免吞掉后续内容。
        return { name, headerArgs: opener.rawArgs, bodyText: "", child: [] };
    }
    const bodyText = text.slice(opener.end, closing.start);
    const branches = name === "if" ? splitIfBranches(bodyText) : undefined;
    return {
        name,
        headerArgs: opener.rawArgs,
        bodyText,
        // 块体按原文重新解析；if 块额外按 elseif/else 切成互不影响的分支。
        child: branches === undefined ? parseRange(bodyText, 0, bodyText.length, false).nodes : branches.flat(),
        ...(branches === undefined ? {} : { branches }),
    };
}

/**
 * 把 if 体按 elseif/else 切成彼此独立的分支。
 *
 * `if` 的每个分支都是独立作用域：分支里的 `return()` 只结束该分支，不会终止外层函数。
 */
function splitIfBranches(bodyText: string): CommandNode[] {
    const branches: CommandNode[] = [];
    let segmentStart = 0;
    let scanFrom = 0;
    const regex = /\b([A-Za-z_][A-Za-z0-9_]*)[ \t]*\(/g;
    while (true) {
        regex.lastIndex = scanFrom;
        const match = regex.exec(bodyText);
        if (match === null) {
            break;
        }
        const openParenIndex = match.index + match[0].length - 1;
        const { end } = readArgumentBody(bodyText, openParenIndex);
        const name = match[1].toLowerCase();
        if (BRANCH_DELIMITERS.has(name)) {
            // 分支体是 [segmentStart, match.index)；分隔命令自身（elseif 的条件、else）不再执行。
            branches.push(parseRange(bodyText, segmentStart, match.index, false).nodes);
            segmentStart = end;
        }
        scanFrom = end;
    }
    branches.push(parseRange(bodyText, segmentStart, bodyText.length, false).nodes);
    return branches;
}

/** 从块起始命令之后，找到与之配对的结束命令（嵌套同名块按深度计数）。 */
function findClosingCommand(
    text: string,
    fromOffset: number,
    opener: string,
    closer: string,
): { start: number; end: number } | null {
    let depth = 1;
    const regex = /\b([A-Za-z_][A-Za-z0-9_]*)[ \t]*\(/g;
    regex.lastIndex = fromOffset;
    let match: RegExpExecArray | null;
    while ((match = regex.exec(text)) !== null) {
        const openParenIndex = match.index + match[0].length - 1;
        const { end } = readArgumentBody(text, openParenIndex);
        const name = match[1].toLowerCase();
        if (name === opener) {
            depth += 1;
        } else if (name === closer) {
            depth -= 1;
            if (depth === 0) {
                return { start: match.index, end };
            }
        }
        regex.lastIndex = end;
    }
    return null;
}

// ============================================================================
// 求值状态
// ============================================================================

interface FunctionDefinition {
    readonly name: string;
    /** 宏替换：参数名换成传入值原文，其余文本原样保留。C 风格接收器额外使用 `<prefix>_ARGN/<prefix>_ARGV...`。 */
    readonly isMacro: boolean;
    readonly params: string[];
    readonly body: CommandNode;
    /** 宏体的原文（宏需要在调用时做文本替换）；函数为 null。 */
    readonly rawBody: string | null;
    readonly directory: string;
}

interface DeferredPropertyWrite {
    readonly mode: "APPEND" | "SET";
    readonly values: string[];
}

/**
 * 变量作用域：每个函数调用 / 脚本 / add_subdirectory 各自一层。
 *
 * CMake 的 `set()` 默认写入**当前**作用域，函数体里写的新变量在函数返回后消失；
 * 源清单常写成 `set(X ...)` + `target_sources(... ${X})`，两层都在同一个函数里，
 * 若不分层，函数里的临时列表会泄漏到顶层，甚至覆盖同名的顶层列表。
 */
interface VariableFrame {
    readonly values: Map<string, string[]>;
    /** 函数/目录/脚本创建的作用域在新帧里查找；宏复用调用方作用域。 */
    readonly parent: VariableFrame | null;
}

interface EvaluationState {
    /** 当前作用域帧；读写一律经 readVariable/writeVariable，不要直接改它的值。 */
    frame: VariableFrame;
    /** 正在执行宏体时的临时帧（`frame` 指向它）；其余时刻为 null。 */
    macroFrame: VariableFrame | null;
    readonly functions: Map<string, FunctionDefinition>;
    readonly collected: Set<string>;
    readonly warnings: Set<string>;
    readonly visitedScripts: Set<string>;
    readonly callStack: string[];
    /** set_property(GLOBAL ...) 的待写入项：等脚本执行完、函数参数还原后再统一应用。 */
    readonly deferredProperties: Map<string, DeferredPropertyWrite[]>;
    steps: number;
}

function createState(): EvaluationState {
    const root: VariableFrame = {
        values: new Map<string, string[]>([
            ["CMAKE_SOURCE_DIR", [toPosixAbsolute(REPO_ROOT)]],
            ["PROJECT_SOURCE_DIR", [toPosixAbsolute(REPO_ROOT)]],
        ]),
        parent: null,
    };
    return {
        frame: root,
        macroFrame: null,
        functions: new Map<string, FunctionDefinition>(),
        collected: new Set<string>(),
        warnings: new Set<string>(),
        visitedScripts: new Set<string>(),
        callStack: [],
        deferredProperties: new Map<string, DeferredPropertyWrite[]>(),
        steps: 0,
    };
}

// ============================================================================
// 变量作用域
// ============================================================================

/** 沿作用域链查找变量；未定义返回 undefined。 */
function readVariable(state: EvaluationState, name: string): string[] | undefined {
    for (let frame: VariableFrame | null = state.frame; frame !== null; frame = frame.parent) {
        const value = frame.values.get(name);
        if (value !== undefined) {
            return value;
        }
    }
    return undefined;
}

/** 写入当前作用域。 */
function writeVariable(state: EvaluationState, name: string, value: string[]): void {
    // 一律复制：变量数组之间共享引用会让后续 `list(APPEND ...)` 就地改写别的变量与全局属性。
    state.frame.values.set(name, [...value]);
}

/** 删除当前作用域里的变量（CMake 语义：不影响外层同名变量）。 */
function unsetVariable(state: EvaluationState, name: string): void {
    state.frame.values.delete(name);
}

/** 进入新作用域执行 body，结束后自动退出。 */
function withScope(state: EvaluationState, body: () => void, shared = false): void {
    const previousFrame = state.frame;
    const nextFrame: VariableFrame = { values: new Map(), parent: shared ? previousFrame.parent : previousFrame };
    state.frame = nextFrame;
    try {
        body();
    } finally {
        state.frame = previousFrame;
    }
}

// ============================================================================
// 变量展开
// ============================================================================

function findMatchingBrace(text: string, openIndex: number): number {
    let depth = 0;
    for (let i = openIndex; i < text.length; i += 1) {
        if (text[i] === "{") {
            depth += 1;
        } else if (text[i] === "}") {
            depth -= 1;
            if (depth === 0) {
                return i;
            }
        }
    }
    return -1;
}

/**
 * 变量名里出现这些片段时，它的值可能是源文件列表（未定义就必须告警）。
 *
 * 构建目录、安装目录、工具链目录与第三方 include/lib 目录都不承载源清单，排除在外，
 * 否则每次运行都会刷出一大屏无关告警。
 */
const SOURCE_BEARING_VARIABLE = /^_?[A-Za-z0-9_]*(SOURCES?|SCRIPTS?|FILES)(_ABS)?$|_DIR$/i;

/** 构建与工具链目录类变量：它们的值不参与源清单，未定义不必告警。 */
const IGNORED_DIRECTORY_VARIABLE = /(INSTALL|BINARY|OUTPUT|CLANG|STB|DEBUGINFOD|UNWIND|VCPKG|CMAKE_MODULE_PATH|MODULE_PATH)/i;

/**
 * 展开参数中的 `${VAR}`；`$ENV{...}` 与本脚本无关，按空串处理。
 *
 * 未知变量按 CMake 语义展开为空串，同时记一条告警——空串会让路径判定静默吞掉文件，
 * 这类告警正是“解析结果可能不完整”的信号。
 */
function expandVariables(token: string, state: EvaluationState, context: string): string {
    let result = "";
    for (let i = 0; i < token.length; i += 1) {
        const ch = token[i];
        if (ch === "$" && token[i + 1] === "{") {
            const closeIndex = findMatchingBrace(token, i + 1);
            if (closeIndex < 0) {
                result += token.slice(i);
                break;
            }
            result += resolveVariableReference(token.slice(i + 2, closeIndex), state, context);
            i = closeIndex;
            continue;
        }
        if (ch === "$" && token.startsWith("$ENV{", i)) {
            const closeIndex = token.indexOf("}", i);
            i = closeIndex < 0 ? token.length : closeIndex;
            continue;
        }
        result += ch;
    }
    return result;
}

function resolveVariableReference(inner: string, state: EvaluationState, context: string): string {
    if (inner.startsWith("ENV{") || inner.startsWith("CACHE{")) {
        return "";
    }
    const name = inner.includes("${") ? expandVariables(inner, state, context) : inner;
    const value = readVariable(state, name);
    if (value === undefined) {
        // 只有“可能承载源文件列表”的未定义变量才值得告警：编译器路径、构建目录、版本号这类
        // 变量被展开为空不影响源清单，逐一告警只会淹没真正的问题。
        if (SOURCE_BEARING_VARIABLE.test(name) && !IGNORED_DIRECTORY_VARIABLE.test(name)) {
            state.warnings.add(`${context}: undefined variable \${${name}} expanded to empty string`);
        }
        return "";
    }
    return value.join(";");
}

/** 展开成实际参数列表：先展开变量，再按未加引号的 `;` 切分列表。 */
function expandArguments(rawArgs: string[], state: EvaluationState, context: string): string[] {
    const result: string[] = [];
    for (const raw of rawArgs) {
        const quoted = isQuotedToken(raw);
        const expanded = expandVariables(unquoteToken(raw), state, context);
        if (quoted) {
            result.push(expanded);
            continue;
        }
        result.push(...expanded.split(";"));
    }
    return result;
}

/**
 * 展开参数但**保留**未加引号参数内部的 `;`（不切分）。
 *
 * 用于 `cmake_parse_arguments` 的关键字列表：`"SOURCES;EXTRA_LIBS;DEFINES"` 需要原样
 * 交给解析器，由它按 `;` 拆成多个关键字。
 */
function expandArgumentsPreservingSemicolons(rawArgs: string[], state: EvaluationState, context: string): string[] {
    return rawArgs.map((raw) => expandVariables(unquoteToken(raw), state, context));
}

// ============================================================================
// 执行
// ============================================================================

/**
 * 执行命令树。
 *
 * @param directory 等价于 CMAKE_CURRENT_SOURCE_DIR
 */
/**
 * 命令执行的控制流信号。
 *
 * `return()` 只结束**当前节点序列**（一个函数体、一个分支），而 `foreach` 里的 `return()`
 * 在 CMake 中只结束当前迭代——因此用返回值逐层传递，由各处按语义处理。
 */
type Flow = "continue" | "return";

function execute(node: CommandNode, directory: string, state: EvaluationState, depth: number): Flow {
    if (depth > 32) {
        state.warnings.add(`${toRepoRelative(directory)}: include/function nesting deeper than 32 levels, skipped`);
        return "continue";
    }
    for (const item of node) {
        state.steps += 1;
        if (state.steps > MAX_EVALUATION_STEPS) {
            state.warnings.add("evaluation step budget exhausted, remaining commands skipped");
            return "continue";
        }
        if (isBlockCommand(item)) {
            // 块级命令的作用域由 executeBlock 自己管理（函数/宏/if 分支），
            // 块内出现的 return() 只结束该块，绝不能一路上传到脚本顶层。
            executeBlock(item, directory, state, depth);
            continue;
        }
        if (executeCommand(item, directory, state, depth) === "return") {
            return "return";
        }
    }
    return "continue";
}

/** 报错/告警用的位置标签（当前脚本的 CMakeLists.txt）。 */
function contextOf(directory: string): string {
    return toRepoRelative(path.join(directory, "CMakeLists.txt"));
}

function executeBlock(block: BlockCommand, directory: string, state: EvaluationState, depth: number): Flow {
    const context = contextOf(directory);
    const resolvedArgs = expandArguments(block.headerArgs, state, context);

    switch (block.name) {
        case "function": {
            const name = (resolvedArgs[0] ?? "").toLowerCase();
            if (name.length > 0) {
                state.functions.set(name, {
                    name,
                    params: resolvedArgs.slice(1),
                    body: block.child,
                    rawBody: null,
                    directory,
                    isMacro: false,
                });
            }
            return "continue";
        }
        case "macro": {
            // 宏是文本替换：执行时按调用值替换参数名与 ARGN/ARGV 后重新解析宏体。
            const name = (resolvedArgs[0] ?? "").toLowerCase();
            if (name.length > 0) {
                state.functions.set(name, {
                    name,
                    params: resolvedArgs.slice(1),
                    body: block.child,
                    rawBody: block.bodyText,
                    directory,
                    isMacro: true,
                });
            }
            return "continue";
        }
        case "foreach":
            executeForeach(block, resolvedArgs, directory, state, depth);
            return "continue";
        case "if":
            // 条件无法静态判定，所有分支都执行：多收集只会让判定更宽松，不会把真正游离的
            // 文件误判成已收集。每个分支独立执行，分支内的 return() 不会终止外层函数。
            for (const branch of block.branches ?? [block.child]) {
                withScope(state, () => execute(branch, directory, state, depth));
            }
            return "continue";
        default:
            // while/block：静态无法判定，跳过。
            return "continue";
    }
}

function executeForeach(
    block: BlockCommand,
    resolvedArgs: string[],
    directory: string,
    state: EvaluationState,
    depth: number,
): void {
    const context = contextOf(directory);
    if (resolvedArgs.length === 0) {
        return;
    }
    const loopVariableName = resolvedArgs[0];
    const values = resolveForeachValues(resolvedArgs.slice(1), state, context);
    const previous = readVariable(state, loopVariableName);
    for (const value of values) {
        state.steps += 1;
        if (state.steps > MAX_EVALUATION_STEPS) {
            state.warnings.add("evaluation step budget exhausted inside foreach");
            break;
        }
        writeVariable(state, loopVariableName, [value]);
        // CMake 里 foreach 体内的 return() 只结束当前迭代，循环继续。
        execute(block.child, directory, state, depth);
    }
    if (previous === undefined) {
        unsetVariable(state, loopVariableName);
    } else {
        writeVariable(state, loopVariableName, previous);
    }
}

function resolveForeachValues(args: string[], state: EvaluationState, context: string): string[] {
    if (args.length === 0) {
        return [];
    }
    const first = args[0];
    const keyword = first.toUpperCase();
    if (keyword === "RANGE") {
        const numbers = args
            .slice(1)
            .flatMap((value) => value.split(";"))
            .map((value) => Number.parseInt(value, 10))
            .filter((value) => Number.isFinite(value));
        if (numbers.length === 0) {
            return [];
        }
        // `RANGE [start] stop [step]`：只有一个数时 start=0，只有两个数时 step=1。
        const start = numbers.length >= 2 ? numbers[0] : 0;
        const stop = numbers.length >= 2 ? numbers[1] : numbers[0];
        const step = numbers.length >= 3 ? numbers[2] : 1;
        if (step === 0) {
            state.warnings.add(`${context}: foreach RANGE with step 0 skipped`);
            return [];
        }
        const values: string[] = [];
        for (let value = start; step > 0 ? value <= stop : value >= stop; value += step) {
            values.push(String(value));
            if (values.length > 100_000) {
                state.warnings.add(`${context}: foreach RANGE too large, truncated`);
                break;
            }
        }
        return values;
    }
    if (keyword === "IN") {
        const values: string[] = [];
        let mode: "lists" | "items" = "lists";
        for (let i = 1; i < args.length; i += 1) {
            const upper = args[i].toUpperCase();
            if (upper === "LISTS") {
                mode = "lists";
                continue;
            }
            if (upper === "ITEMS") {
                mode = "items";
                continue;
            }
            if (mode === "items") {
                values.push(args[i]);
                continue;
            }
            // IN LISTS 的参数已经是展开后的列表项（`${LIST}` 展开后按 `;` 切开）。
            for (const name of args[i].split(";")) {
                const list = readVariable(state, name);
                if (list === undefined) {
                    state.warnings.add(`${context}: foreach(IN LISTS) references unknown list '${name}'`);
                    continue;
                }
                values.push(...list);
            }
        }
        return values;
    }
    return args;
}

/** 把参数解析为仓库内的绝对源文件路径；非源文件或含未展开内容时返回 null。 */
function toAbsoluteSource(argument: string, directory: string): string | null {
    const cleaned = argument.replace(GENERATOR_EXPRESSION, "").trim();
    if (cleaned.length === 0 || cleaned.includes("${") || !isSourceFile(cleaned)) {
        return null;
    }
    return toPosixAbsolute(path.isAbsolute(cleaned) ? cleaned : path.resolve(directory, cleaned));
}

function executeCommand(command: Command, directory: string, state: EvaluationState, depth: number): Flow {
    // 相对路径一律相对“当前脚本目录”解析（与 CMake 的 CMAKE_CURRENT_SOURCE_DIR 一致）。
    const relativeBase = currentDirectory(state);
    const context = contextOf(relativeBase);
    if (command.name === "return") {
        return "return";
    }
    if (command.name === "cmake_parse_arguments") {
        // 关键字列表用引号写成 `"SOURCES;EXTRA_LIBS;DEFINES"`：必须先按原始参数展开成列表，
        // 否则整串会变成一个关键字。其余参数按普通规则展开。
        handleParseArguments(expandArgumentsPreservingSemicolons(command.rawArgs, state, context), state);
        return "continue";
    }
    // 每次调用都展开成新数组：函数体里的 `${param}` 会把参数原样带入，若直接复用
    // rawArgs/ARGV 的数组引用，后续 `list(APPEND ...)` 之类的原地修改会污染命令树。
    const args = [...expandArguments(command.rawArgs, state, context)];

    switch (command.name) {
        case "set":
            handleSet(args, state);
            return "continue";
        case "option":
            // `option(<name> "<doc>" [<default>])`：未定义时写入默认值。顶层 `if(MC_BUILD_*)`
            // 全靠它，遗漏会让整个子目录树都不被求值。
            if (args.length > 0 && readVariable(state, args[0]) === undefined) {
                writeVariable(state, args[0], args.length >= 3 && args[2].length > 0 ? [args[2]] : ["OFF"]);
            }
            return "continue";
        case "list":
            handleList(args, state, context);
            return "continue";
        case "unset":
            if (args.length > 0) {
                unsetVariable(state, args[0]);
            }
            return "continue";
        case "math":
            handleMath(args, state, context);
            return "continue";
        case "get_property":
            handleGetProperty(args, state);
            return "continue";
        case "set_property":
            handleSetProperty(args, relativeBase, state);
            return "continue";
        case "file":
            handleFile(args, relativeBase, state);
            return "continue";
        case "include":
            handleInclude(args, relativeBase, state, depth);
            return "continue";
        case "add_subdirectory":
            handleAddSubdirectory(args, relativeBase, state, depth);
            return "continue";
        default:
            break;
    }

    if (COLLECTING_COMMANDS.has(command.name)) {
        for (const argument of args) {
            const absolute = toAbsoluteSource(argument, relativeBase);
            if (absolute !== null) {
                state.collected.add(absolute);
            }
        }
        return "continue";
    }

    const userFunction = state.functions.get(command.name);
    if (userFunction !== undefined) {
        invokeFunction(userFunction, args, state, depth);
    }
    return "continue";
}

function handleSet(args: string[], state: EvaluationState): void {
    if (args.length === 0) {
        return;
    }
    const name = args[0];
    // 列表值里的空元素没有意义，还会让“参数是否为空”的判定出错，统一剔除。
    const rest = args.slice(1).filter((value) => value.length > 0);

    // `set(X v1 v2 CACHE <type> "<doc>" [FORCE])`：CACHE 之后的都是元数据。
    const cacheIndex = rest.indexOf("CACHE");
    if (cacheIndex >= 0) {
        writeVariable(state, name, rest.slice(0, cacheIndex));
        return;
    }
    // `set(X v1 v2 PARENT_SCOPE)`：本脚本不对作用域建模，按当前作用域处理即可。
    const parentIndex = rest.indexOf("PARENT_SCOPE");
    if (parentIndex >= 0) {
        writeVariable(state, name, rest.slice(0, parentIndex));
        return;
    }
    writeVariable(state, name, rest);
}

/** 会改变列表内容的 list() 子命令；其余子命令走 handleReadOnlyList 或直接忽略。 */
const MUTATING_LIST_SUBCOMMANDS = new Set(["APPEND", "PREPEND", "REMOVE_ITEM", "REMOVE_AT", "TRANSFORM", "FILTER", "INSERT"]);

/**
 * 只读 list() 子命令里会影响后续逻辑的两个：`LENGTH`（常被
 * `foreach(... RANGE 0 ${N}-1)` 使用）与 `GET`（用于取下标）。
 * 其余（FIND/REVERSE/SORT/JOIN/SUBLIST）只产生中间值，不影响源清单。
 */
function handleReadOnlyList(subCommand: string, args: string[], state: EvaluationState): void {
    const name = args[1] ?? "";
    const current = readVariable(state, name) ?? [];
    if (subCommand === "LENGTH" && args.length >= 3) {
        writeVariable(state, args[2], [String(current.length)]);
        return;
    }
    if (subCommand === "GET" && args.length >= 4) {
        const outputVariable = args[3];
        const picked: string[] = [];
        for (const rawIndex of args.slice(2, 3).flatMap((value) => value.split(";"))) {
            const index = normalizeIndex(rawIndex, current.length);
            if (index >= 0 && index < current.length) {
                picked.push(current[index]);
            }
        }
        writeVariable(state, outputVariable, picked);
    }
}

function handleList(args: string[], state: EvaluationState, context: string): void {
    if (args.length < 2) {
        return;
    }
    const subCommand = args[0].toUpperCase();
    if (!MUTATING_LIST_SUBCOMMANDS.has(subCommand)) {
        handleReadOnlyList(subCommand, args, state);
        return;
    }
    const name = args[1];
    const current = readVariable(state, name) ?? [];
    const values = args.slice(2).filter((value) => value.length > 0);
    switch (subCommand) {
        case "APPEND":
            writeVariable(state, name, [...current, ...values]);
            return;
        case "PREPEND":
            writeVariable(state, name, [...values, ...current]);
            return;
        case "REMOVE_ITEM":
            writeVariable(state, 
                name,
                current.filter((item) => !values.includes(item)),
            );
            return;
        case "REMOVE_AT": {
            const indexes = new Set(values.map((value) => normalizeIndex(value, current.length)));
            writeVariable(state, name, current.filter((_, index) => !indexes.has(index)));
            return;
        }
        case "TRANSFORM":
            writeVariable(state, name, transformList(current, values, state, context));
            return;
        case "FILTER":
            writeVariable(state, name, filterList(current, values, state, context));
            return;
        case "INSERT": {
            const position = normalizeIndex(values[0] ?? "0", current.length);
            const next = [...current];
            next.splice(Math.max(position, 0), 0, ...values.slice(1));
            writeVariable(state, name, next);
            return;
        }
        default:
            return;
    }
}

function normalizeIndex(rawValue: string, length: number): number {
    const value = Number.parseInt(rawValue, 10);
    if (!Number.isFinite(value)) {
        return -1;
    }
    return value < 0 ? value + length : value;
}

function transformList(current: string[], args: string[], state: EvaluationState, context: string): string[] {
    const verb = (args[0] ?? "").toUpperCase();
    const operand = args[1] ?? "";
    switch (verb) {
        case "PREPEND":
            return current.map((item) => `${operand}${item}`);
        case "APPEND":
            return current.map((item) => `${item}${operand}`);
        case "REPLACE":
            return current.map((item) => (operand.length === 0 ? item : item.split(operand).join(args[2] ?? "")));
        case "TOUPPER":
            return current.map((item) => item.toUpperCase());
        case "TOLOWER":
            return current.map((item) => item.toLowerCase());
        case "STRIP":
            return current.map((item) => item.trim());
        default:
            state.warnings.add(`${context}: unsupported list(TRANSFORM ${verb}), list kept as-is`);
            return current;
    }
}

/**
 * `list(FILTER <list> INCLUDE|EXCLUDE REGEX <regex>)` / `list(FILTER <list> INCLUDE <glob>)`。
 */
function filterList(current: string[], args: string[], state: EvaluationState, context: string): string[] {
    const mode = (args[0] ?? "").toUpperCase();
    const kind = (args[1] ?? "").toUpperCase();
    const pattern = args[2] ?? "";
    if ((mode !== "INCLUDE" && mode !== "EXCLUDE") || pattern.length === 0) {
        return current;
    }
    let matcher: RegExp;
    if (kind === "REGEX") {
        try {
            matcher = new RegExp(pattern);
        } catch {
            state.warnings.add(`${context}: invalid regex '${pattern}' in list(FILTER), list kept as-is`);
            return current;
        }
    } else {
        // 默认是 glob 匹配（CMake 的 glob 用 `*` 通配任意字符）。
        matcher = new RegExp(`^${pattern.replace(/[.+^${}()|[\]\\]/g, "\\$&").replace(/\*/g, ".*").replace(/\?/g, ".")}$`);
    }
    const keepMatching = mode === "INCLUDE";
    return current.filter((item) => matcher.test(item) === keepMatching);
}

function handleMath(args: string[], state: EvaluationState, context: string): void {
    // `math(EXPR out "<expr>" [OUTPUT_FORMAT ...])`：只支持整数四则，够用即可。
    if (args.length < 3 || args[0].toUpperCase() !== "EXPR") {
        return;
    }
    const expression = args.slice(2).join("");
    if (!/^[\s0-9+\-*/%()]+$/.test(expression)) {
        state.warnings.add(`${context}: math(EXPR) with unsupported expression '${expression}' skipped`);
        return;
    }
    try {
        const value = Function(`"use strict";return (${expression});`)() as number;
        if (typeof value === "number" && Number.isFinite(value)) {
            writeVariable(state, args[1], [String(Math.trunc(value))]);
        }
    } catch {
        // 表达式语法超出支持范围时静默跳过：它不参与源清单。
    }
}

/** 全局属性表：GLOBAL 作用域上的 `APPEND`/`SET` 属性，源清单常借它跨函数汇总。 */
const globalProperties = new Map<string, string[]>();

/**
 * `get_property(<out> GLOBAL PROPERTY <name> [SET|DEFINED|BRIEF_DOCS...])`。
 *
 * 只还原 `GLOBAL PROPERTY` 形式；TARGET/DIRECTORY 作用域的属性不影响源清单。
 */
function handleGetProperty(args: string[], state: EvaluationState): void {
    if (args.length < 4 || args[1].toUpperCase() !== "GLOBAL" || args[2].toUpperCase() !== "PROPERTY") {
        return;
    }
    const outputVariable = args[0];
    const propertyName = args[3];
    const option = (args[4] ?? "").toUpperCase();
    if (option === "SET" || option === "DEFINED") {
        writeVariable(state, outputVariable, [globalProperties.has(propertyName) ? "1" : "0"]);
        return;
    }
    writeVariable(state, outputVariable, [...(globalProperties.get(propertyName) ?? [])]);
}

/**
 * `set_property(GLOBAL APPEND|SET PROPERTY <name> <values...>)`。
 *
 * 属性值里常带函数参数（`${source}`），若立即写入，函数返回后参数被还原也不影响已写入的
 * 值——但同一个属性会被后续调用反复覆盖，所以先记录待写项，在参数还原之前统一应用，
 * 保证拿到的是**调用当时**的真实路径。
 */
function handleSetProperty(args: string[], directory: string, state: EvaluationState): void {
    const globalIndex = args.findIndex((value) => value.toUpperCase() === "GLOBAL");
    const propertyIndex = args.findIndex((value) => value.toUpperCase() === "PROPERTY");
    if (globalIndex < 0 || propertyIndex < 0 || propertyIndex + 1 >= args.length) {
        return;
    }
    const mode: "APPEND" | "SET" = args.some((value) => value.toUpperCase() === "APPEND") ? "APPEND" : "SET";
    const propertyName = args[propertyIndex + 1];
    const values = args
        .slice(propertyIndex + 2)
        .filter((value) => value.length > 0)
        .map((value) => toAbsoluteSource(value, directory) ?? value);
    const pending = state.deferredProperties.get(propertyName) ?? [];
    pending.push({ mode, values });
    state.deferredProperties.set(propertyName, pending);
}

/** 把待写入的 GLOBAL 属性应用到全局属性表。 */
function flushDeferredProperties(state: EvaluationState): void {
    for (const [propertyName, writes] of state.deferredProperties) {
        let current = globalProperties.get(propertyName) ?? [];
        for (const write of writes) {
            current = write.mode === "APPEND" ? [...current, ...write.values] : [...write.values];
        }
        globalProperties.set(propertyName, current);
    }
    state.deferredProperties.clear();
}

/**
 * `cmake_parse_arguments(<prefix> <options> <one_value> <multi_value> <args...>)`。
 *
 * 本仓库的自定义函数（如 `mc_add_fuzz_target`）用它把 `SOURCES;...` 之类的关键词参数
 * 解析成 `${ARG_SOURCES}` 等变量，源清单就藏在这些变量里，因此必须还原。
 */
function handleParseArguments(args: string[], state: EvaluationState): void {
    // 正常情况写当前作用域；宏体运行在“临时帧”里，解析结果要留在调用方作用域。
    const target = state.frame.parent ?? state.frame;
    const writeParsed = (name: string, value: string[]): void => {
        target.values.set(name, [...value]);
    };
    if (args.length < 4) {
        return;
    }
    const prefix = args[0];
    const optionKeywords = splitKeywordList(args[1]);
    const singleKeywords = splitKeywordList(args[2]);
    const multiKeywords = splitKeywordList(args[3]);
    // `PARSE_ARGV <N>` 形式从第 N 个参数开始解析，本仓库的调用方不使用，按 0 处理即可。
    const remaining = args
        .slice(4)
        .flatMap((value) => splitListArgument(value))
        .filter((value) => value.length > 0 && value !== "PARSE_ARGV");

    const isKnownKeyword = (token: string): boolean =>
        optionKeywords.includes(token) || singleKeywords.includes(token) || multiKeywords.includes(token);

    const options = new Set<string>();
    const singleValues = new Map<string, string>();
    const multiValues = new Map<string, string[]>();
    // 多值关键词遇到下一个关键字才结束；遇到未知的大写标识符（常见于 `SOURCES;...` 被
    // 展开切开后残留的枚举值）时也结束，避免把它吞进上一个多值列表。
    const endsValueList = (token: string): boolean =>
        isKnownKeyword(token) || /^[A-Za-z_][A-Za-z0-9_]*$/.test(token);
    const unparsed: string[] = [];

    let index = 0;
    while (index < remaining.length) {
        const token = remaining[index];
        if (optionKeywords.includes(token)) {
            options.add(token);
            index += 1;
            continue;
        }
        if (singleKeywords.includes(token)) {
            singleValues.set(token, remaining[index + 1] ?? "");
            index += 2;
            continue;
        }
        if (multiKeywords.includes(token)) {
            const collected: string[] = [];
            index += 1;
            while (index < remaining.length && !endsValueList(remaining[index])) {
                collected.push(remaining[index]);
                index += 1;
            }
            multiValues.set(token, collected);
            continue;
        }
        unparsed.push(token);
        index += 1;
    }

    for (const keyword of optionKeywords) {
        writeParsed(`${prefix}_${keyword}`, options.has(keyword) ? ["TRUE"] : ["FALSE"]);
    }
    for (const keyword of singleKeywords) {
        writeParsed(`${prefix}_${keyword}`, singleValues.has(keyword) ? [singleValues.get(keyword) ?? ""] : []);
    }
    for (const keyword of multiKeywords) {
        writeParsed(`${prefix}_${keyword}`, [...(multiValues.get(keyword) ?? [])]);
    }
    writeParsed(`${prefix}_UNPARSED_ARGUMENTS`, unparsed);
    writeVariable(
        state,
        `${prefix}_KEYWORDS_MISSING_VALUES`,
        singleKeywords.filter((keyword) => singleValues.get(keyword) === ""),
    );
}

/** 关键词列表参数可能是 `A;B;C`，也可能被展开成多个参数，统一按 `;` 切分。 */
function splitKeywordList(value: string): string[] {
    return value
        .split(";")
        .map((item) => item.trim())
        .filter((item) => item.length > 0);
}

/** 把可能含 `;` 的单个参数展开成列表项（保留带引号的 `"A;B"` 语义时由调用方自行处理）。 */
function splitListArgument(value: string): string[] {
    return value.split(";");
}

function handleFile(args: string[], directory: string, state: EvaluationState): void {
    if (args.length < 2) {
        return;
    }
    const mode = args[0].toUpperCase();
    if (mode !== "GLOB" && mode !== "GLOB_RECURSE") {
        return;
    }
    const outputVariable = args[1];
    let relativeTo: string | null = null;
    let listDirectories = false;
    const patterns: string[] = [];

    for (let i = 2; i < args.length; i += 1) {
        const upper = args[i].toUpperCase();
        if (upper === "RELATIVE") {
            relativeTo = args[(i += 1)] ?? "";
            continue;
        }
        if (upper === "LIST_DIRECTORIES") {
            listDirectories = (args[(i += 1)] ?? "false").toUpperCase() !== "FALSE";
            continue;
        }
        if (upper === "CONFIGURE_DEPENDS" || upper === "FOLLOW_SYMLINKS") {
            continue;
        }
        if (args[i].length > 0) {
            patterns.push(args[i]);
        }
    }
    if (patterns.length === 0) {
        return;
    }

    // `file(GLOB out dir/*.cpp)`：若只有一个模式，其目录部分即基准目录。
    let baseDir = directory;
    if (patterns.length === 1) {
        baseDir = path.resolve(directory, path.dirname(patterns[0]));
        patterns[0] = path.basename(patterns[0]);
    } else if (!patterns[0].includes("*") && !patterns[0].includes("?")) {
        baseDir = path.resolve(directory, patterns[0]);
        patterns.shift();
    }

    const matches: string[] = [];
    for (const pattern of patterns) {
        for (const matched of expandGlob(baseDir, pattern, listDirectories)) {
            if (isSourceFile(matched)) {
                matches.push(toPosixAbsolute(matched));
            }
        }
    }
    const wantsSources = patterns.some((pattern) => isSourceFile(pattern));
    if (matches.length === 0 && wantsSources && readVariable(state, outputVariable) === undefined) {
        // 通配为空会让下游 target_sources 静默缺文件，必须显式记录。
        state.warnings.add(
            `${toRepoRelative(directory)}: file(${mode}) for '${outputVariable}' matched no source file (base='${toRepoRelative(baseDir)}')`,
        );
    }
    const relativeBase = relativeTo === null ? null : path.resolve(directory, relativeTo);
    writeVariable(state, 
        outputVariable,
        matches.map((item) => (relativeBase === null ? item : path.relative(relativeBase, item).split(path.sep).join("/"))),
    );
}

function handleInclude(args: string[], directory: string, state: EvaluationState, depth: number): void {
    if (args.length === 0 || args[0].length === 0) {
        return;
    }
    const moduleName = args[0];
    const candidates: string[] = [];
    if (path.isAbsolute(moduleName) || moduleName.includes("/")) {
        const base = path.resolve(directory, moduleName);
        candidates.push(base, `${base}.cmake`);
    } else {
        // 模块名形式（如 `include(BuildOptions)` / `include(GoogleTest)`）：按 CMAKE_MODULE_PATH
        // 逐项查找。仓库自己的模块都在 cmake/ 下，遗漏它们会让 MC_BUILD_* 等开关全部未定义。
        const visited = new Set<string>();
        const extendSearchPath = (value: string | undefined): void => {
            if (value === undefined) {
                return;
            }
            for (const item of value.split(";")) {
                if (item.length > 0 && !visited.has(item)) {
                    visited.add(item);
                    candidates.push(path.join(item, `${moduleName}.cmake`));
                }
            }
        };
        extendSearchPath(readVariable(state, "CMAKE_MODULE_PATH")?.join(";"));
        candidates.push(path.resolve(directory, `${moduleName}.cmake`));
        candidates.push(path.join(REPO_ROOT, "cmake", `${moduleName}.cmake`));
    }
    const target = candidates.find((candidate) => isFile(candidate));
    if (target === undefined) {
        // CMake 内置模块（GoogleTest/FetchContent/...）与源清单无关，找不到属于正常情况。
        return;
    }
    runScriptFile(target, state, depth + 1);
}

function handleAddSubdirectory(args: string[], directory: string, state: EvaluationState, depth: number): void {
    if (args.length === 0 || args[0].length === 0) {
        return;
    }
    const subDir = path.resolve(directory, args[0]);
    const cmakeFile = path.join(subDir, "CMakeLists.txt");
    if (!isFile(cmakeFile)) {
        state.warnings.add(`${toRepoRelative(directory)}: add_subdirectory(${args[0]}) has no CMakeLists.txt`);
        return;
    }
    runScriptFile(cmakeFile, state, depth + 1);
}

/** 命令树缓存，键为脚本绝对路径。 */
const commandTreeCache = new Map<string, CommandNode>();

function runScriptFile(filePath: string, state: EvaluationState, depth: number): void {
    const key = toPosixAbsolute(filePath);
    if (state.visitedScripts.has(key)) {
        return;
    }
    state.visitedScripts.add(key);
    const script = loadScript(filePath);
    if (script === null) {
        return;
    }
    let tree = commandTreeCache.get(key);
    if (tree === undefined) {
        tree = parseCommandTree(script);
        commandTreeCache.set(key, tree);
    }
    const scriptDir = path.dirname(filePath);
    const previousDir = currentDirectory(state);
    setCurrentDirectory(state, scriptDir);
    try {
        // CMake 语义：include()/add_subdirectory() 里的 set() 写入**调用方**作用域
        // （只有函数调用才有独立作用域，见 invokeFunction）。
        execute(tree, scriptDir, state, depth);
    } finally {
        // CMake 里形如 `client/renderer/X.cpp` 的相对源路径是按**当前目录**解析的，
        // 而当前目录在被 include 的脚本执行期间就是该脚本所在目录。脚本执行完恢复调用方目录。
        setCurrentDirectory(state, previousDir);
    }
}

/** 当前脚本目录（等价于 CMAKE_CURRENT_SOURCE_DIR）。 */
function currentDirectory(state: EvaluationState): string {
    return readVariable(state, "CMAKE_CURRENT_SOURCE_DIR")?.[0] ?? REPO_ROOT;
}

/**
 * 切换 CMAKE_CURRENT_SOURCE_DIR / CMAKE_CURRENT_LIST_DIR。
 *
 * 直接写在 `state.frame` 上：调用方（如 runScriptFile 的恢复步骤）需要它跨 execute 可见，
 * 而 execute 内部的块会在自己的帧里执行，必须在**进入之前**就落到当前帧。
 */
function setCurrentDirectory(state: EvaluationState, directory: string): void {
    const value = [toPosixAbsolute(directory)];
    state.frame.values.set("CMAKE_CURRENT_SOURCE_DIR", value);
    state.frame.values.set("CMAKE_CURRENT_LIST_DIR", value);
}

function invokeFunction(definition: FunctionDefinition, args: string[], state: EvaluationState, depth: number): void {
    if (state.callStack.includes(definition.name)) {
        state.warnings.add(`recursive call to '${definition.name}' skipped`);
        return;
    }
    state.callStack.push(definition.name);
    try {
        if (definition.isMacro && definition.rawBody !== null) {
            invokeMacro(definition, args, state, depth);
            return;
        }
        // 函数使用独立作用域；参数写入后随作用域一起销毁，无需手工还原。
        // 相对路径以**调用点**所在脚本目录为基准（与该函数定义在哪个文件无关）。
        withScope(state, () => {
            setCurrentDirectory(state, currentDirectory(state));
            bindArguments(definition, args, state);
            execute(definition.body, currentDirectory(state), state, depth + 1);
        });
    } finally {
        flushDeferredProperties(state);
        state.callStack.pop();
    }
}

/**
 * 执行宏：CMake 的宏是**文本替换**，参数与 `ARGN`/`ARGV`/`ARGC` 在展开时按变量名替换成调用值，
 * 其余文本原样保留，因此宏体里的 `cmake_parse_arguments(... SOURCES;EXTRA_LIBS;DEFINES)` 能拿到关键词。
 * 宏体在调用方作用域执行（与函数不同）。
 */
function invokeMacro(definition: FunctionDefinition, args: string[], state: EvaluationState, depth: number): void {
    if (definition.rawBody === null) {
        // 没有记下宏原文（不应发生）：退化为按函数处理，至少能覆盖最简形式。
        withScope(
            state,
            () => {
                bindArguments(definition, args, state);
                execute(definition.body, definition.directory, state, depth + 1);
            },
            true,
        );
        return;
    }
    const extraArgs = args.slice(definition.params.length);
    const substitutions = new Map<string, string>();
    for (let i = 0; i < definition.params.length; i += 1) {
        substitutions.set(definition.params[i], args[i] ?? "");
    }
    substitutions.set("ARGC", String(args.length));
    substitutions.set("ARGV", args.join(";"));
    substitutions.set("ARGN", extraArgs.join(";"));
    if (definition.params.length > 0) {
        const prefix = definition.params[0];
        substitutions.set(`${prefix}_ARGN`, extraArgs.join(";"));
        substitutions.set(`${prefix}_ARGV`, args.join(";"));
        substitutions.set(`${prefix}_ARGC`, String(args.length));
        for (let i = 0; i < args.length; i += 1) {
            substitutions.set(`${prefix}_ARG${i}`, args[i]);
        }
    }

    const expandedText = definition.rawBody.replace(/\$\{([A-Za-z0-9_]+)\}/g, (match, name: string) => {
        const replacement = substitutions.get(name);
        return replacement === undefined ? match : replacement;
    });
    // 宏体在调用方作用域执行（可以读到调用方的变量），但宏内的普通 set() 不应泄漏回去：
    // ExecuteState 的 scope 语义里宏是“同一作用域”，这里用一个临时帧 + 结束后丢弃来近似，
    // cmake_parse_arguments 的结果会被提升到父作用域保留下来。
    const macroCallDir = currentDirectory(state);
    const macroFrame: VariableFrame = { values: new Map(), parent: state.frame };
    const savedFrame = state.frame;
    state.frame = macroFrame;
    state.macroFrame = macroFrame;
    setCurrentDirectory(state, macroCallDir);
    try {
        execute(parseCommandTree(expandedText), macroCallDir, state, depth + 1);
    } finally {
        state.frame = savedFrame;
        state.macroFrame = null;
    }
}

/** 绑定 `ARGC`/`ARGV`/`ARGN` 与形式参数（CMake 语义：未传入的参数展开为空）。 */
function bindArguments(definition: FunctionDefinition, args: string[], state: EvaluationState): void {
    writeVariable(state, "ARGC", [String(args.length)]);
    writeVariable(state, "ARGV0", args.length > 0 ? [args[0]] : []);
    writeVariable(state, "CMAKE_ARGC", [String(args.length)]);
    writeVariable(state, "CMAKE_ARGV0", args.length > 0 ? [args[0]] : []);
    // 参数必须复制：变量数组与全局属性数组一旦共享同一个引用，后续
    // `list(APPEND 属性 ...)` / `set_property(APPEND ...)` 会把参数数组撑大。
    writeVariable(state, "ARGV", [...args]);
    writeVariable(state, "CMAKE_CURRENT_FUNCTION", [definition.name]);
    // ARGN 是“形式参数之外的全部实参”，不论形参表里是否显式写了 ARGN 都必须绑定。
    writeVariable(state, "ARGN", args.slice(definition.params.filter((name) => name !== "ARGN").length));
    for (let i = 0; i < definition.params.length; i += 1) {
        const paramName = definition.params[i];
        if (paramName === "ARGN") {
            continue;
        }
        writeVariable(state, paramName, args[i] === undefined || args[i].length === 0 ? [] : [args[i]]);
    }
}

/** 脚本原文缓存，避免重复读盘与去注释。 */
const scriptCache = new Map<string, string>();

function loadScript(filePath: string): string | null {
    const cached = scriptCache.get(filePath);
    if (cached !== undefined) {
        return cached;
    }
    try {
        const text = stripComments(readFileSync(filePath, "utf8"));
        scriptCache.set(filePath, text);
        return text;
    } catch (error) {
        console.warn(`Failed to read ${toRepoRelative(filePath)}: ${String(error)}`);
        return null;
    }
}

// ============================================================================
// 通用工具
// ============================================================================

function toPosixAbsolute(absolutePath: string): string {
    return path.resolve(absolutePath).split(path.sep).join("/");
}

function toRepoRelative(absolutePath: string): string {
    return path.relative(REPO_ROOT, absolutePath).split(path.sep).join("/");
}

/** 是否为本脚本认定的 C/C++ 源文件。 */
function isSourceFile(filePath: string): boolean {
    return SOURCE_EXTENSIONS.has(path.extname(filePath).toLowerCase());
}

function isFile(candidate: string): boolean {
    try {
        return statSync(candidate).isFile();
    } catch {
        return false;
    }
}

function isInsideSkippedDir(absolutePath: string): boolean {
    const relative = path.relative(REPO_ROOT, absolutePath);
    if (relative.startsWith("..")) {
        return false;
    }
    return relative.split(path.sep).some((segment: string) => SKIP_DIR_NAMES.has(segment));
}

function safeReaddir(dir: string): ReturnType<typeof readdirSync> {
    try {
        return readdirSync(dir, { withFileTypes: true });
    } catch {
        return [];
    }
}

function collectFiles(dir: string, out: string[], predicate: (name: string) => boolean): void {
    for (const entry of safeReaddir(dir)) {
        if (SKIP_DIR_NAMES.has(entry.name)) {
            continue;
        }
        const fullPath = path.join(dir, entry.name);
        if (entry.isDirectory()) {
            collectFiles(fullPath, out, predicate);
        } else if (entry.isFile() && predicate(entry.name)) {
            out.push(fullPath);
        }
    }
}

function globToRegExp(segment: string): RegExp {
    const escaped = segment.replace(/[.+^${}()|[\]\\]/g, "\\$&").replace(/\*/g, "[^/]*").replace(/\?/g, "[^/]");
    return new RegExp(`^${escaped}$`);
}

/** 展开 glob 模式（支持 `*`、`?`、`**`）。 */
function expandGlob(baseDir: string, pattern: string, listDirectories: boolean): string[] {
    const segments = pattern.split("/").filter((segment) => segment.length > 0 && segment !== ".");
    const results: string[] = [];

    const walk = (currentDir: string, index: number): void => {
        if (index >= segments.length) {
            return;
        }
        const segment = segments[index];
        const isLast = index === segments.length - 1;
        if (segment === "**") {
            if (isLast) {
                collectAll(currentDir, results, listDirectories);
                return;
            }
            walk(currentDir, index + 1);
            for (const entry of safeReaddir(currentDir)) {
                if (entry.isDirectory() && !SKIP_DIR_NAMES.has(entry.name)) {
                    walk(path.join(currentDir, entry.name), index);
                }
            }
            return;
        }
        if (segment.includes("*") || segment.includes("?")) {
            const matcher = globToRegExp(segment);
            for (const entry of safeReaddir(currentDir)) {
                if (!matcher.test(entry.name)) {
                    continue;
                }
                const next = path.join(currentDir, entry.name);
                if (isLast) {
                    if (entry.isFile() || (entry.isDirectory() && listDirectories)) {
                        results.push(next);
                    }
                } else if (entry.isDirectory()) {
                    walk(next, index + 1);
                }
            }
            return;
        }
        const next = path.join(currentDir, segment);
        if (!existsSync(next)) {
            return;
        }
        if (isLast) {
            results.push(next);
            return;
        }
        walk(next, index + 1);
    };

    walk(baseDir, 0);
    return results;
}

function collectAll(dir: string, out: string[], listDirectories: boolean): void {
    for (const entry of safeReaddir(dir)) {
        if (SKIP_DIR_NAMES.has(entry.name)) {
            continue;
        }
        const fullPath = path.join(dir, entry.name);
        if (entry.isDirectory()) {
            if (listDirectories) {
                out.push(fullPath);
            }
            collectAll(fullPath, out, listDirectories);
        } else if (entry.isFile()) {
            out.push(fullPath);
        }
    }
}

// ============================================================================
// 聚合单元与编译数据库
// ============================================================================

/** 找出 `.cpp` 内 `#include "xxx.cpp"` 的聚合关系：成员 -> 包含它的聚合单元。 */
function findAggregatedMembers(sourceFiles: string[]): Map<string, string[]> {
    const members = new Map<string, string[]>();
    const includePattern = /^[ \t]*#[ \t]*include[ \t]*"([^"\n]+\.(?:cpp|cc|cxx|c))"/gm;
    for (const file of sourceFiles) {
        let text: string;
        try {
            text = readFileSync(file, "utf8");
        } catch {
            continue;
        }
        includePattern.lastIndex = 0;
        let match: RegExpExecArray | null;
        while ((match = includePattern.exec(text)) !== null) {
            const memberPath = toPosixAbsolute(path.resolve(path.dirname(file), match[1]));
            if (memberPath === toPosixAbsolute(file)) {
                continue;
            }
            const list = members.get(memberPath) ?? [];
            list.push(toPosixAbsolute(file));
            members.set(memberPath, list);
        }
    }
    return members;
}

function readCompileCommands(buildDir: string): Set<string> | null {
    const candidate = path.join(buildDir, "compile_commands.json");
    if (!isFile(candidate)) {
        return null;
    }
    const entries = JSON.parse(readFileSync(candidate, "utf8")) as Array<{ directory: string; file: string }>;
    const compiled = new Set<string>();
    for (const entry of entries) {
        const absolute = path.isAbsolute(entry.file) ? entry.file : path.resolve(entry.directory, entry.file);
        if (isInsideSkippedDir(absolute)) {
            continue;
        }
        compiled.add(toPosixAbsolute(absolute));
    }
    return compiled;
}

// ============================================================================
// 主流程
// ============================================================================

function scanCmake(): { collected: Set<string>; warnings: string[] } {
    const state = createState();
    const rootCmake = path.join(REPO_ROOT, "CMakeLists.txt");
    runScriptFile(rootCmake, state, 0);
    return { collected: state.collected, warnings: [...state.warnings].sort() };
}

type SourceStatus = "compiled" | "declared" | "aggregated" | "orphan";

interface SourceReport {
    readonly file: string;
    readonly status: SourceStatus;
    readonly aggregatedBy: string[];
}

function classify(
    sourceFiles: string[],
    collected: Set<string>,
    compiled: Set<string> | null,
    aggregated: Map<string, string[]>,
): SourceReport[] {
    const liveUnit = (unit: string): boolean => collected.has(unit) || (compiled?.has(unit) ?? false);
    const reports: SourceReport[] = sourceFiles.map((absolute) => {
        const aggregators = aggregated.get(absolute) ?? [];
        let status: SourceStatus;
        if (compiled?.has(absolute)) {
            status = "compiled";
        } else if (collected.has(absolute)) {
            status = "declared";
        } else if (aggregators.some(liveUnit)) {
            status = "aggregated";
        } else {
            status = "orphan";
        }
        return { file: toRepoRelative(absolute), status, aggregatedBy: aggregators.map(toRepoRelative) };
    });
    reports.sort((a, b) => a.file.localeCompare(b.file));
    return reports;
}

function parseArgs(argv: string[]): CliOptions {
    let showAll = false;
    let json = false;
    let buildDir = path.join(REPO_ROOT, "build");
    for (let i = 0; i < argv.length; i += 1) {
        const arg = argv[i];
        if (arg === "--all") {
            showAll = true;
        } else if (arg === "--json") {
            json = true;

        } else if (arg === "--build-dir") {
            buildDir = path.resolve(REPO_ROOT, argv[(i += 1)] ?? "build");
        } else if (arg.startsWith("--build-dir=")) {
            buildDir = path.resolve(REPO_ROOT, arg.slice("--build-dir=".length));
        } else if (arg === "--help" || arg === "-h") {
            printUsage();
            process.exit(0);
        } else {
            console.error(`Unknown argument: ${arg}`);
            printUsage();
            process.exit(2);
        }
    }
    return { showAll, json, buildDir };
}

function printUsage(): void {
    console.log("Usage: node --experimental-strip-types scripts/diag/orphan_sources.ts [--all] [--json] [--build-dir <dir>]");
}

function main(): void {
    const options = parseArgs(process.argv.slice(2));

    const discovered: string[] = [];
    collectFiles(REPO_ROOT, discovered, isSourceFile);
    // 仓库自身的源文件统一成 posix 绝对路径；判断“是否被收集”时就是用这同一套表示。
    const absoluteSources = discovered.map(toPosixAbsolute);

    const { collected, warnings } = scanCmake();
    const aggregated = findAggregatedMembers(discovered);
    const compiled = readCompileCommands(options.buildDir);

    const reports = classify(absoluteSources, collected, compiled, aggregated);
    const counts: Record<SourceStatus, number> = { compiled: 0, declared: 0, aggregated: 0, orphan: 0 };
    for (const report of reports) {
        counts[report.status] += 1;
    }
    const orphans = reports.filter((report) => report.status === "orphan");
    const exitCode = orphans.length > 0 ? 1 : 0;

    if (options.json) {
        console.log(
            JSON.stringify(
                {
                    compileCommandsUsed: compiled !== null,
                    counts: { total: reports.length, ...counts },
                    warnings,
                    sources: options.showAll ? reports : orphans,
                },
                null,
                2,
            ),
        );
        process.exit(exitCode);
        return;
    }

    for (const report of options.showAll ? reports : orphans) {
        if (options.showAll) {
            const suffix = report.status === "aggregated" ? `  <- aggregated by ${report.aggregatedBy.join(", ")}` : "";
            console.log(`${report.status.padEnd(10)} ${report.file}${suffix}`);
        } else {
            console.log(report.file);
        }
    }
    const visible = options.showAll ? reports : orphans;
    if (visible.length > 0) {
        console.log("");
    }

    console.log("=== Source collection summary ===");
    console.log(`Total source files: ${reports.length}`);
    console.log(
        `  Compiled (compile_commands.json): ${counts.compiled}${compiled === null ? "  [compile_commands.json not found]" : ""}`,
    );
    console.log(`  Declared by CMake: ${counts.declared}`);
    console.log(`  Included by aggregate units: ${counts.aggregated}`);
    console.log(`  Orphan (not part of any target): ${counts.orphan}`);

    if (compiled === null) {
        console.log("");
        console.log("compile_commands.json was not found, so the verdict relies purely on static CMake parsing.");
        console.log("Configure the project once (e.g. ./scripts/configure.sh build) to enable cross-checking.");
    }
    if (warnings.length > 0) {
        console.log("");
        console.log(`Parsing warnings (${warnings.length}), the verdict may be incomplete:`);
        for (const warning of warnings) {
            console.log(`  ${warning}`);
        }
    }

    process.exit(exitCode);
}

main();