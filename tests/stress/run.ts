import { parseOptions, printHelp } from "./src/config.ts";

try {
    const options = parseOptions(process.argv.slice(2));
    if (options === null) {
        printHelp();
    } else {
        // 帮助与参数校验不依赖已安装的机器人包或服务端构建产物。
        const { runStressTests } = await import("./src/runner.ts");
        process.exitCode = await runStressTests(options);
    }
} catch (error) {
    console.error(error instanceof Error ? error.message : String(error));
    process.exitCode = 2;
}
