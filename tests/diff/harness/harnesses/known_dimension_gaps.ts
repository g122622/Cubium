/**
 * `dimension_type_params` 的已知缺口。
 *
 * 只剩两项，都是**纯客户端渲染字段**，而本项目的客户端已停止维护（只支持 Java 版客户端
 * 连接本项目的服务端），服务端从不读取它们：
 *
 * - `skybox`：决定客户端画哪种天空盒（`overworld` / `none` / `end`）。
 * - `cardinal_light_type`：决定客户端四个水平方向的面光照衰减模型。
 *
 * 保留在能力里而不是从场景表删掉，是为了让「这两项确实没实现」这件事在测试报告里
 * 一直可见；将来若重新启用客户端，这里会直接变成红灯提醒。
 */
const DIMENSIONS = ["minecraft:overworld", "minecraft:the_nether", "minecraft:the_end"];

export const KNOWN_DIMENSION_GAPS: readonly { key: string; field: string; reason: string }[] =
    DIMENSIONS.flatMap((dimension) =>
        ["skybox", "cardinal_light_type"].map((field) => ({
            key: `${dimension}|${field}`,
            field: "value",
            reason: "纯客户端渲染字段，客户端已停止维护，服务端不读取",
        })));
