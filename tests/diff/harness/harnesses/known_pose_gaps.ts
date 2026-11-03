/**
 * `entity_pose_dimensions` 的已知缺口：需要实体处于特定状态才能对齐的项。
 *
 * 这些**不是** `getDimensions` 算法错误，而是「同一实体类型在不同状态下尺寸不同」，
 * 而探针构造的是**默认状态实例**，两侧默认状态的初始化路径不同。
 *
 * - `slime`（17 个非睡眠姿态）：原版史莱姆尺寸由 `setSize(n)` 写进 `entityData` 的
 *   `ID_SIZE`，构造时走 `defineSynchedData` 的默认值；Cubium 的 `m_size` 初值不同，
 *   于是未显式 `setSize` 时两侧尺寸不一致。要精确比对必须在两侧显式设置同一体型，
 *   这超出了本能力「默认状态查表」的范围。
 * - `warden`（eyeHeight）：`2.465` 在两侧浮点字面量解析下差 1 ulp
 *   （Java `0x401dc290` vs C++ `0x401dc28f`），属于无法消除的末位差异。
 *
 * TODO: 为 slime 增加「指定体型」的场景维度后删除 slime 那些条目；
 *       warden 的 1 ulp 差异需要在归一化层用「末位 1 ulp 视为相等」处理。
 */
const ALL_POSES = [
    "sleeping",
    "standing",
    "fall_flying",
    "swimming",
    "spin_attack",
    "crouching",
    "long_jumping",
    "dying",
    "croaking",
    "using_tongue",
    "sitting",
    "roaring",
    "sniffing",
    "emerging",
    "digging",
    "sliding",
    "shooting",
    "inhaling",
];

const POSES = ALL_POSES.filter((p) => p !== "sleeping");

const REASON_SLIME = "史莱姆默认体型在两侧初始化路径不同（原版由 setSize 的 entityData 驱动）";

export const KNOWN_POSE_GAPS: readonly { key: string; field: string; reason: string }[] = [
    ...POSES.flatMap((pose) =>
        ["width", "height", "eyeHeight"].map((field) => ({
            key: `minecraft:slime|${pose}`,
            field,
            reason: REASON_SLIME,
        }))),
    // 睡眠姿态也必须登记：史莱姆的覆写绕过了基类睡眠特判，值仍来自体型。
    ...["width", "height", "eyeHeight"].map((field) => ({
        key: "minecraft:slime|sleeping",
        field,
        reason: REASON_SLIME,
    })),
    // warden 的 1 ulp 差异出现在**全部** 18 个姿态（eyeHeight 与姿态无关）。
    ...ALL_POSES.map((pose) => ({
        key: `minecraft:warden|${pose}`,
        field: "eyeHeight",
        reason: "2.465 的浮点字面量在 Java 与 C++ 下差 1 ulp（0x401dc290 vs 0x401dc28f）",
    })),
];
