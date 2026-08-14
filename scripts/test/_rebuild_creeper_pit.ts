// 重建 creeper_pit.mcstructure 为正确的基岩 little-endian NBT 格式。
//
// 背景：creeper_pit.mcstructure 的 size list 字段格式损坏（缺 count 字段，Cubium 容错解析但
// 基岩 BDS 严格解析失败，报 "Failed to spawn test structure"），影响 26 个用该结构的测试在基岩
// 无法加载。本脚本参照 glass_pit.mcstructure（基岩可正常加载）的正确格式重建 creeper_pit。
//
// creeper_pit 布局：7×5×7 开放坑，y=0 满铺 grass_block 地板，y=1..4 全 air（无围墙）。
// 索引公式（基岩 mcstructure 标准）：index = (x * size_y + y) * size_z + z，方块遍历顺序
// x→y→z（外层 x，内层 z）。block_indices 两层：layer0=方块索引，layer1=-1（无液体层）。
//
// 用法：node scripts/test/_rebuild_creeper_pit.ts
// 临时脚本，提交前删除。

import fs from "node:fs";

const OUT = "E:/dev/minecraft-reborn-branch-1/tests/integrated/mob_behavior/structures/gametests/creeper_pit.mcstructure";

const SIZE_X = 7;
const SIZE_Y = 5;
const SIZE_Z = 7;
const TOTAL = SIZE_X * SIZE_Y * SIZE_Z;

// palette：0=grass_block, 1=air。version 17879555 对齐 glass_pit（基岩 1.21 方块版本号）。
const VERSION = 17879555;
const palette = [
    { name: "minecraft:grass_block", states: {}, version: VERSION },
    { name: "minecraft:air", states: {}, version: VERSION },
];

// 方块索引：y=0 是 grass_block(0)，y=1..4 是 air(1)。
// index = (x * SIZE_Y + y) * SIZE_Z + z
const layer0 = new Array(TOTAL);
for (let x = 0; x < SIZE_X; x++) {
    for (let y = 0; y < SIZE_Y; y++) {
        for (let z = 0; z < SIZE_Z; z++) {
            const idx = (x * SIZE_Y + y) * SIZE_Z + z;
            layer0[idx] = (y === 0) ? 0 : 1; // y=0 grass_block, 其余 air
        }
    }
}
const layer1 = new Array(TOTAL).fill(-1); // 液体层全空

// ===== little-endian NBT 序列化 =====
const bufs: Buffer[] = [];

function u8(v: number): void { bufs.push(Buffer.from([v & 0xff])); }
function i32(v: number): void { const b = Buffer.alloc(4); b.writeInt32LE(v, 0); bufs.push(b); }
function u16(v: number): void { const b = Buffer.alloc(2); b.writeUInt16LE(v, 0); bufs.push(b); }
function str(s: string): void { u16(Buffer.byteLength(s, "utf8")); bufs.push(Buffer.from(s, "utf8")); }

// tag types
const TAG_COMPOUND = 10;
const TAG_LIST = 9;
const TAG_INT = 3;
const TAG_STRING = 8;
const TAG_END = 0;

function writeName(name: string): void { str(name); }

function writeIntPayload(v: number): void { i32(v); }

function writeListInt(values: number[]): void {
    u8(TAG_INT); // element type
    i32(values.length);
    for (const v of values) { i32(v); }
}

function writeListCompound(items: Record<string, unknown>[]): void {
    u8(TAG_COMPOUND); // element type
    i32(items.length);
    for (const it of items) {
        writeCompoundPayload(it);
    }
}

function writeListListInt(layers: number[][]): void {
    u8(TAG_LIST); // element type
    i32(layers.length);
    for (const layer of layers) {
        writeListInt(layer);
    }
}

function writeCompoundPayload(obj: Record<string, unknown>): void {
    for (const [key, val] of Object.entries(obj)) {
        if (typeof val === "number" && Number.isInteger(val)) {
            u8(TAG_INT); writeName(key); writeIntPayload(val);
        } else if (Array.isArray(val) && val.length > 0 && typeof val[0] === "number") {
            u8(TAG_LIST); writeName(key); writeListInt(val as number[]);
        } else if (Array.isArray(val) && val.length > 0 && Array.isArray(val[0])) {
            u8(TAG_LIST); writeName(key); writeListListInt(val as number[][]);
        } else if (Array.isArray(val) && val.length > 0 && typeof val[0] === "object") {
            u8(TAG_LIST); writeName(key); writeListCompound(val as Record<string, unknown>[]);
        } else if (Array.isArray(val) && val.length === 0) {
            // 空列表：元素类型用 END(0)
            u8(TAG_LIST); writeName(key); u8(TAG_END); i32(0);
        } else if (typeof val === "object" && val !== null) {
            u8(TAG_COMPOUND); writeName(key); writeCompoundPayload(val as Record<string, unknown>);
        } else if (typeof val === "string") {
            u8(TAG_STRING); writeName(key); str(val);
        }
    }
    u8(TAG_END);
}

// root: compound(无名)
u8(TAG_COMPOUND);
writeName(""); // root name 空

const root: Record<string, unknown> = {
    format_version: 1,
    size: [SIZE_X, SIZE_Y, SIZE_Z],
    structure: {
        block_indices: [layer0, layer1],
        entities: [],
        palette: {
            default: {
                block_palette: palette,
                block_position_data: {},
            },
        },
    },
};
writeCompoundPayload(root);

const out = Buffer.concat(bufs);
fs.writeFileSync(OUT, out);
console.log(`wrote ${out.length} bytes to ${OUT}`);
console.log(`palette: ${palette.length} blocks, layer0 len: ${layer0.length}, grass_block count: ${layer0.filter(v=>v===0).length}`);
