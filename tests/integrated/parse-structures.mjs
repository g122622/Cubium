// 解析 Bedrock .mcstructure（little-endian NBT）并输出每层二维图。
// 供 docs/test/ 结构文档生成参考。运行：node parse-structures.mjs
import * as fs from "node:fs";
import * as path from "node:path";

function readNBT(buf) {
  let off = 0;
  const rd = () => buf[off++];
  const rs = () => { const v = buf.readInt16LE(off); off += 2; return v; };
  const ru16 = () => { const v = buf.readUInt16LE(off); off += 2; return v; };
  const ri = () => { const v = buf.readInt32LE(off); off += 4; return v; };
  const rf = () => { const v = buf.readFloatLE(off); off += 4; return v; };
  const rdbl = () => { const v = buf.readDoubleLE(off); off += 8; return v; };
  const rstr = () => { const len = ru16(); const s = buf.toString("utf8", off, off + len); off += len; return s; };
  function rpayload(type) {
    switch (type) {
      case 1: { const v = buf.readInt8(off); off++; return v; }
      case 2: { return rs(); }
      case 3: { return ri(); }
      case 4: { const lo = buf.readInt32LE(off); const hi = buf.readInt32LE(off + 4); off += 8; return [lo, hi]; } // TAG_Long (int64)
      case 5: { return rf(); } // TAG_Float
      case 6: { return rdbl(); } // TAG_Double
      case 7: { const len = ri(); const arr = []; for (let i = 0; i < len; i++) arr.push(buf.readInt8(off++)); return arr; }
      case 8: { return rstr(); }
      case 9: { const et = rd(); const len = ri(); const arr = []; for (let i = 0; i < len; i++) arr.push(et === 0 ? null : rpayload(et)); return arr; }
      case 10: { const m = {}; while (true) { const t = rd(); if (t === 0) break; const name = rstr(); m[name] = rpayload(t); } return m; }
      case 11: { const len = ri(); const arr = []; for (let i = 0; i < len; i++) arr.push(ri()); return arr; }
      case 12: { const len = ri(); const arr = []; for (let i = 0; i < len; i++) { const lo = buf.readInt32LE(off); const hi = buf.readInt32LE(off + 4); off += 8; arr.push([lo, hi]); } return arr; }
      default: throw new Error("unknown NBT type " + type + " at off " + (off - 1));
    }
  }
  const rootType = rd();
  rstr(); // root name
  return rpayload(rootType);
}

// 方块名 → 单字符图例（用于二维图）。优先用语义化字母。
const LEGEND = {
  "minecraft:air": "·",
  "minecraft:glass": "G",
  "minecraft:glass_pane": "g",
  "minecraft:bedrock": "B",
  "minecraft:stone": "S",
  "minecraft:cobblestone": "c",
  "minecraft:dirt": "d",
  "minecraft:grass_block": "g",
  "minecraft:grass": ",",
  "minecraft:grass_path": "p",
  "minecraft:wood": "W",
  "minecraft:oak_log": "L",
  "minecraft:oak_leaves": "l",
  "minecraft:oak_fence": "f",
  "minecraft:oak_planks": "P",
  "minecraft:oak_stairs": "s",
  "minecraft:water": "~",
  "minecraft:lava": "v",
  "minecraft:sand": "n",
  "minecraft:sandstone": "N",
  "minecraft:gravel": "r",
  "minecraft:clay": "C",
  "minecraft:ice": "I",
  "minecraft:packed_ice": "i",
  "minecraft:blue_ice": "b",
  "minecraft:snow_layer": "#",
  "minecraft:snow": "#",
  "minecraft:snow_block": "#",
  "minecraft:iron_block": "F",
  "minecraft:gold_block": "O",
  "minecraft:diamond_block": "D",
  "minecraft:emerald_block": "E",
  "minecraft:lapis_block": "U",
  "minecraft:redstone_block": "R",
  "minecraft:coal_block": "K",
  "minecraft:quartz_block": "Q",
  "minecraft:obsidian": "X",
  "minecraft:glowstone": "*",
  "minecraft:sea_lantern": "*",
  "minecraft:beacon": "!",
  "minecraft:iron_bars": "|",
  "minecraft:barrier": "?",
  "minecraft:structure_block": "T",
  "minecraft:command_block": ">",
  "minecraft:chain_command_block": "-",
  "minecraft:repeating_command_block": "=",
  "minecraft:spawner": "@",
  "minecraft:chest": "h",
  "minecraft:bookshelf": "x",
  "minecraft:crafting_table": "+",
  "minecraft:furnace": "u",
  "minecraft:torch": "t",
  "minecraft:ladder": "H",
  "minecraft:wool": "w",
  "minecraft:white_wool": "w",
  "minecraft:black_wool": "k",
  "minecraft:red_wool": "r",
  "minecraft:blue_wool": "q",
  "minecraft:green_wool": "e",
  "minecraft:yellow_wool": "y",
  "minecraft:concrete": "m",
  "minecraft:terracotta": "z",
  "minecraft:bricks": "b",
  "minecraft:nether_bricks": "b",
  "minecraft:netherrack": "n",
  "minecraft:soul_sand": "&",
  "minecraft:glow_lichen": "j",
};

function charFor(name, state) {
  if (LEGEND[name]) return LEGEND[name];
  // 未知方块用首字母大写
  const base = name.replace("minecraft:", "");
  return base[0] ? base[0].toUpperCase() : "?";
}

function parseStructure(filePath) {
  const buf = fs.readFileSync(filePath);
  const root = readNBT(buf);
  const size = root.size; // [x, y, z]
  const structure = root.structure;
  // palette 可能是 { default: { block_palette: [...] } } 或 { block_palette: [...] }
  let blockPalette;
  let paletteKey;
  const palObj = structure.palette;
  if (palObj.default && palObj.default.block_palette) {
    blockPalette = palObj.default.block_palette;
    paletteKey = "default";
  } else if (Array.isArray(palObj)) {
    blockPalette = palObj;
    paletteKey = "(root)";
  } else if (palObj.block_palette) {
    blockPalette = palObj.block_palette;
    paletteKey = "(root)";
  } else {
    // 单 palette 对象，键为 palette 名
    const keys = Object.keys(palObj);
    paletteKey = keys[0];
    blockPalette = palObj[paletteKey].block_palette;
  }
  // blockPalette: [{ name: string, states: {...} }, ...]（Bedrock 用小写 name）
  const paletteNames = blockPalette.map(b => b.name);
  // block_indices: [layer0, layer1]，layer0 是主层（实心方块），layer1 是 waterlog 层
  const layer0 = structure.block_indices[0];
  const layer1 = structure.block_indices[1];
  const [sx, sy, sz] = size;
  // 索引：x + z*sx + y*sx*sz
  const at = (layer, x, y, z) => layer[x + z * sx + y * sx * sz];

  return { size: [sx, sy, sz], paletteNames, paletteKey, layer0, layer1, at, sx, sy, sz };
}

function renderStructure(s) {
  const lines = [];
  lines.push(`尺寸: ${s.sx} × ${s.sy} × ${s.sz} (X × Y × Z)`);
  // palette 去重统计
  const paletteUsed = new Set();
  for (let i = 0; i < s.layer0.length; i++) {
    if (s.layer0[i] >= 0) paletteUsed.add(s.paletteNames[s.layer0[i]]);
  }
  for (let i = 0; i < s.layer1.length; i++) {
    if (s.layer1[i] >= 0) paletteUsed.add(s.paletteNames[s.layer1[i]] + "(waterlog)");
  }
  lines.push(`palette (${s.paletteNames.length} 项, 实际使用 ${paletteUsed.size}):`);
  s.paletteNames.forEach((n, i) => lines.push(`  [${i}] ${n}`));

  // 每层二维图（俯视，X 横向 Z 纵向）
  for (let y = 0; y < s.sy; y++) {
    lines.push("");
    lines.push(`── Y=${y} ── (俯视: 列=X, 行=Z)`);
    // 列标
    let header = "    ";
    for (let x = 0; x < s.sx; x++) header += (x % 10).toString();
    lines.push(header);
    for (let z = 0; z < s.sz; z++) {
      let row = `Z=${z} `;
      if (z < 10) row += " ";
      for (let x = 0; x < s.sx; x++) {
        const idx = s.at(s.layer0, x, y, z);
        if (idx < 0) {
          row += " ";
        } else {
          row += charFor(s.paletteNames[idx]);
        }
      }
      lines.push(row);
    }
  }
  return lines.join("\n");
}

// 主：解析命令行参数指定的所有结构
const files = process.argv.slice(2);
for (const f of files) {
  console.log("");
  console.log("════════════════════════════════════════════════════════════════");
  console.log(`📄 ${f}`);
  console.log("════════════════════════════════════════════════════════════════");
  try {
    const s = parseStructure(f);
    console.log(renderStructure(s));
  } catch (e) {
    console.log("PARSE ERROR: " + e.message);
    console.log(e.stack);
  }
}
