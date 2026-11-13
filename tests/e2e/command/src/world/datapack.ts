/*
 * datapack.ts — 用例用的最小数据包。
 *
 * 函数、`/schedule`、`/datapack` 这些指令只有在数据包里存在对应资源时才有意义。
 * 这里生成的是**固定内容**的数据包（不依赖任何用例），因为它同时要装进两侧：
 *   - Cubium：`<游戏目录>/datapacks`（GameDirectory::dataPacksDirectory）；
 *   - vanilla：`<cwd>/<worldName>/datapacks`（世界级数据包）。
 *
 * 包名固定为 `e2e_command`，函数命名空间固定为 `e2e`，用例直接引用 `e2e:xxx`。
 */

import fs from "node:fs";
import path from "node:path";

/** 数据包目录名。 */
const DATAPACK_NAME = "e2e_command";

/** 数据包格式版本（与仓库内置的原版数据包一致）。 */
const PACK_FORMAT = 94;

/** 一个要写入数据包的文件。 */
interface DatapackFile {
    /** 相对数据包根的路径（POSIX 分隔符）。 */
    readonly relativePath: string;
    readonly content: string;
}

/**
 * 数据包的固定内容。
 *
 * 函数只做**最小可观察动作**：say 用于观察反馈是否被抑制，scoreboard 用于观察返回值，
 * 嵌套函数用于观察 `/function` 的结果计数是否累加。
 */
function datapackFiles(): readonly DatapackFile[] {
    const packMeta = {
        pack: {
            description: "Cubium command e2e",
            pack_format: PACK_FORMAT,
            supported_formats: { min_inclusive: PACK_FORMAT, max_inclusive: PACK_FORMAT },
        },
    };
    return [
        { relativePath: "pack.mcmeta", content: `${JSON.stringify(packMeta, null, 2)}\n` },
        { relativePath: "data/e2e/function/hello.mcfunction", content: "say function-hello\n" },
        {
            relativePath: "data/e2e/function/nested.mcfunction",
            content: ["say nested-outer", "function e2e:hello"].join("\n") + "\n",
        },
        {
            relativePath: "data/e2e/function/count.mcfunction",
            content: ["scoreboard players add Counter e2e_fn 1", "scoreboard players add Counter e2e_fn 1"].join("\n") + "\n",
        },
        { relativePath: "data/e2e/function/broken.mcfunction", content: "e2e_no_such_command\n" },
    ];
}

/**
 * 把数据包写入指定目录。
 *
 * @param parentDir 数据包根目录（其下会创建 `e2e_command/`）。
 */
export function installDatapack(parentDir: string): void {
    const root = path.join(parentDir, DATAPACK_NAME);
    for (const file of datapackFiles()) {
        const target = path.join(root, ...file.relativePath.split("/"));
        fs.mkdirSync(path.dirname(target), { recursive: true });
        fs.writeFileSync(target, file.content, "utf8");
    }
}
