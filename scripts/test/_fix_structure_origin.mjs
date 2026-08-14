// 通用 mcstructure 结构修复工具：为缺少 structure_world_origin 字段的结构补全该字段。
//
// 背景：基岩 BDS 1.26 严格校验 mcstructure，缺失 structure_world_origin 字段时报
// "structure_world_origin field, a required field, is missing from the structure"，
// 导致结构加载失败、GameTest 报 "Could not find StructureBlockActor"。
// Cubium 容错能读缺该字段的结构，故仅基岩侧受影响。
//
// 本工具读入 mcstructure（little-endian NBT），在 root compound 的 TAG_END 之前
// 插入 structure_world_origin 字段（TAG_LIST of TAG_INT，值为 [ox,oy,oz]），保持
// 其他字段字节不变。纯 JS（Buffer API），无 TS 依赖。
//
// 用法：node scripts/test/_fix_structure_origin.mjs <structure.mcstructure> <ox> <oy> <oz>
// 例：node scripts/test/_fix_structure_origin.mjs .../creeper_pit.mcstructure 0 0 0
// 常备工具：未来若新增结构文件仍缺 structure_world_origin，可复用此工具补全。
// 相关经验已沉淀到 docs/test/INTEGRATED_TEST.md。

import fs from "node:fs";

const [file, oxStr, oyStr, ozStr] = process.argv.slice(2);
if (!file || oxStr === undefined || oyStr === undefined || ozStr === undefined) {
  console.error("usage: _fix_structure_origin.mjs <structure.mcstructure> <ox> <oy> <oz>");
  process.exit(2);
}

const ox = parseInt(oxStr, 10);
const oy = parseInt(oyStr, 10);
const oz = parseInt(ozStr, 10);
const origin = [ox, oy, oz];

const buf = fs.readFileSync(file);

// 解析顶层，定位 root compound 的 TAG_END 位置（即最后一个顶层字段 payload 结束后）。
let pos = 0;
const readByte = () => buf[pos++];
const readUShort = () => { const v = buf.readUInt16LE(pos); pos += 2; return v; };
const readInt = () => { const v = buf.readInt32LE(pos); pos += 4; return v; };
const readString = () => { const len = readUShort(); const s = buf.toString("utf8", pos, pos + len); pos += len; return s; };

// root: TAG_COMPOUND(10) + name
readByte(); // 10
readString(); // root name ""

function skipPayload(t) {
  switch (t) {
    case 1: pos++; break;
    case 2: pos += 2; break;
    case 3: pos += 4; break;
    case 4: pos += 8; break;
    case 5: pos += 4; break;
    case 6: pos += 8; break;
    case 7: { const len = readInt(); pos += len; break; }
    case 8: readString(); break;
    case 9: { const et = readByte(); const len = readInt(); for (let i = 0; i < len; i++) skipPayload(et); break; }
    case 10: { while (true) { const tt = readByte(); if (tt === 0) break; readString(); skipPayload(tt); } break; }
    case 11: { const len = readInt(); pos += len * 4; break; }
    case 12: { const len = readInt(); pos += len * 8; break; }
    default: throw new Error("unknown tag " + t + " at pos " + pos);
  }
}

// 检查是否已有 structure_world_origin 字段
let hasOrigin = false;
const fieldStart = pos;
while (true) {
  const t = readByte();
  if (t === 0) break; // TAG_END
  const name = readString();
  if (name === "structure_world_origin") hasOrigin = true;
  skipPayload(t);
}
const insertPos = pos - 1; // TAG_END 的位置，插入点在其前

if (hasOrigin) {
  console.log(`${file}: already has structure_world_origin, skipping.`);
  process.exit(0);
}

// 构造 structure_world_origin 字段字节：
// TAG_LIST(9) + name("structure_world_origin") + element_type(TAG_INT=3) + count(3) + 3×int32LE
const fieldName = "structure_world_origin";
const nameBytes = Buffer.byteLength(fieldName, "utf8");
const fieldBuf = Buffer.alloc(1 + 2 + nameBytes + 1 + 4 + 4 * 3);
let off = 0;
fieldBuf.writeUInt8(9, off); off += 1; // TAG_LIST
fieldBuf.writeUInt16LE(nameBytes, off); off += 2; // name length
fieldBuf.write(fieldName, off, "utf8"); off += nameBytes; // name
fieldBuf.writeUInt8(3, off); off += 1; // element type = TAG_INT
fieldBuf.writeInt32LE(3, off); off += 4; // count = 3
fieldBuf.writeInt32LE(origin[0], off); off += 4;
fieldBuf.writeInt32LE(origin[1], off); off += 4;
fieldBuf.writeInt32LE(origin[2], off); off += 4;

// 拼接：buf[0..insertPos) + fieldBuf + buf[insertPos..]（buf[insertPos] 是 TAG_END）
const out = Buffer.concat([buf.subarray(0, insertPos), fieldBuf, buf.subarray(insertPos)]);
fs.writeFileSync(file, out);
console.log(`${file}: injected structure_world_origin=[${origin.join(",")}] (${out.length} bytes, was ${buf.length})`);
