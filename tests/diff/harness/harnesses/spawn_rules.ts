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

/** Horizon 风格的受控世界场景；比较独立生成谓词及其后一次随机抽样。 */
interface SpawnCase {
    name: string;
    blocks: Record<string, string>;
    x: number;
    y: number;
    z: number;
    sky: number;
    light: number;
    darken: number;
    thunder: boolean;
    sea: number;
    surface: number;
    difficulty: string;
    dimension: string;
    biome: string;
    slimeChance: number;
    worldSeed: string;
    player: { x: number; y: number; z: number; mode: string } | null;
    reason: string;
    seed: number;
}

const BASE: SpawnCase = {
    name: "", blocks: { "-1": "minecraft:grass_block" }, x: 8, y: 64, z: 8,
    sky: 15, light: 0, darken: 0, thunder: false, sea: 63, surface: 64,
    difficulty: "normal", dimension: "minecraft:overworld", biome: "minecraft:plains",
    slimeChance: 0, worldSeed: "0", player: null, reason: "natural", seed: 0,
};

const SCENES: readonly Partial<SpawnCase>[] = [
    { name: "grass-day" },
    { name: "grass-night", darken: 11 },
    { name: "grass-dusk", darken: 7 },
    { name: "grass-thunder", thunder: true },
    { name: "peaceful", difficulty: "peaceful", darken: 11 },
    { name: "easy", difficulty: "easy", darken: 11 },
    { name: "hard", difficulty: "hard", darken: 11 },
    { name: "cave", blocks: { "-1": "minecraft:stone" }, sky: 0, y: 20, surface: 80 },
    { name: "cave-sky3", blocks: { "-1": "minecraft:stone" }, sky: 3, y: 20, surface: 80 },
    { name: "no-ground", blocks: {}, sky: 0, y: 20, surface: 80 },
    { name: "water-surface", blocks: { "-1": "minecraft:water", "0": "minecraft:water", "1": "minecraft:water" }, y: 60, biome: "minecraft:ocean" },
    { name: "water-shallow", blocks: { "-1": "minecraft:sand", "0": "minecraft:water", "1": "minecraft:water" }, y: 60, biome: "minecraft:ocean" },
    { name: "water-deep", blocks: { "-1": "minecraft:water", "0": "minecraft:water", "1": "minecraft:water" }, sky: 0, y: 25, biome: "minecraft:deep_ocean" },
    { name: "water-lush", blocks: { "-1": "minecraft:clay", "0": "minecraft:water", "1": "minecraft:water" }, sky: 0, y: 10, biome: "minecraft:lush_caves" },
    { name: "water-warm", blocks: { "-1": "minecraft:water", "0": "minecraft:water", "1": "minecraft:water" }, y: 20, biome: "minecraft:warm_ocean" },
    { name: "water-river", blocks: { "-1": "minecraft:water", "0": "minecraft:water", "1": "minecraft:water" }, sky: 0, y: 50, biome: "minecraft:river" },
    { name: "water-covered", blocks: { "-1": "minecraft:water", "0": "minecraft:water", "1": "minecraft:water", "3": "minecraft:stone" }, y: 50, sky: 0, biome: "minecraft:ocean" },
    { name: "water-air-above", blocks: { "-1": "minecraft:water", "0": "minecraft:water" }, y: 60 },
    { name: "water-leaves-canopy", blocks: { "-1": "minecraft:water", "0": "minecraft:water", "1": "minecraft:water", "2": "minecraft:oak_leaves[waterlogged=true]" }, y: 60 },
    { name: "lava-open", blocks: { "-1": "minecraft:lava", "0": "minecraft:lava" }, sky: 0, light: 15, y: 31, sea: 32, dimension: "minecraft:the_nether", biome: "minecraft:nether_wastes" },
    { name: "lava-covered", blocks: { "-1": "minecraft:lava", "0": "minecraft:lava", "1": "minecraft:lava", "2": "minecraft:stone" }, sky: 0, light: 15, y: 31, sea: 32, dimension: "minecraft:the_nether", biome: "minecraft:basalt_deltas" },
    { name: "nether", blocks: { "-1": "minecraft:netherrack" }, sky: 0, sea: 32, dimension: "minecraft:the_nether", biome: "minecraft:nether_wastes" },
    { name: "nether-wart", blocks: { "-1": "minecraft:nether_wart_block" }, sky: 0, sea: 32, dimension: "minecraft:the_nether", biome: "minecraft:crimson_forest" },
    { name: "end", blocks: { "-1": "minecraft:end_stone" }, sky: 0, dimension: "minecraft:the_end", biome: "minecraft:the_end" },
    { name: "swamp", biome: "minecraft:swamp", y: 62, darken: 11, slimeChance: 0.5 },
    { name: "swamp-no-slime", biome: "minecraft:swamp", y: 62, darken: 11, slimeChance: 0 },
    { name: "swamp-full-chance", biome: "minecraft:swamp", y: 62, sky: 0, slimeChance: 1 },
    { name: "bat-surface", blocks: { "-1": "minecraft:stone" }, sky: 0, y: 80, surface: 80 },
    { name: "stray-powder", blocks: { "-1": "minecraft:snow_block", "1": "minecraft:powder_snow", "2": "minecraft:powder_snow" }, darken: 11, biome: "minecraft:frozen_peaks" },
    { name: "ice-frozen", blocks: { "-1": "minecraft:ice" }, biome: "minecraft:frozen_ocean" },
    ...["sand", "red_sand", "ice", "packed_ice", "snow_block", "jungle_leaves", "mycelium", "podzol", "mud", "soul_sand", "magma_block", "glass", "oak_slab[type=top]", "oak_slab[type=bottom]"].flatMap((block) =>
        [0, 15].map((sky) => ({ name: `surface/${block}/${sky}`, blocks: { "-1": `minecraft:${block}` }, sky }))),
    ...["fire", "soul_fire", "wither_rose", "sweet_berry_bush", "cactus", "powder_snow", "campfire[lit=true]", "campfire[lit=false]", "rail", "redstone_torch", "short_grass"].map((block) => ({
        name: `feet/${block}`, blocks: { "-1": "minecraft:stone", "0": `minecraft:${block}` }, sky: 0,
    })),
    ...["overworld", "the_nether", "the_end"].flatMap((dimension) =>
        [0, 1, 7, 8, 9, 15].map((light) => ({
            name: `light/${dimension}/${light}`, dimension: `minecraft:${dimension}`, sky: 0, light,
            blocks: { "-1": "minecraft:stone" },
        }))),
    ...["ocean", "frozen_ocean", "snowy_plains", "jungle", "mushroom_fields"].map((biome) => ({
        name: `biome/${biome}`, biome: `minecraft:${biome}`,
    })),
    ...["survival", "creative", "spectator", "adventure"].flatMap((mode) =>
        [1, 4.99, 5, 5.01].map((distance) => ({
            name: `player/${mode}/${distance}`, sky: 0, blocks: { "-1": "minecraft:stone" },
            player: { x: distance + 0.5, y: 0.5, z: 0.5, mode },
        }))),
    ...[29, 30, 31, 37, 38, 39, 40, 49, 50, 51, 58, 59, 60, 62, 63, 64, 66, 67, 69, 70].map((y) => ({
        name: `height/${y}`, y, sky: 0, biome: "minecraft:swamp", slimeChance: 0.5,
        blocks: { "-1": "minecraft:water", "0": "minecraft:water", "1": "minecraft:water" },
    })),
    ...[-63, 0, 32, 63, 128].flatMap((sea) =>
        [-34, -33, -26, -25, -14, -13, -5, 0, 3, 4].map((offset) => ({
            name: `sea/${sea}/${offset}`, sea, y: sea + offset, sky: 0,
            blocks: { "-1": "minecraft:water", "0": "minecraft:water", "1": "minecraft:water" },
        }))),
];

const REASONS = ["natural", "chunk_generation", "spawner", "trial_spawner", "reinforcement", "patrol", "structure", "event", "command", "spawn_item_use", "mob_summoned", "conversion", "breeding", "triggered", "jockey", "dispenser", "bucket", "load", "dimension_travel"];

export function spawnRulesInput(entity: string): unknown {
    const cases: SpawnCase[] = [];
    for (let index = 0; index < SCENES.length; index++) {
        for (let sample = 0; sample < 6; sample++) {
            const reason = sample < 3 ? ["natural", "spawner", "trial_spawner"][sample]
                : REASONS[(index * 7 + sample) % REASONS.length];
            cases.push({ ...BASE, ...SCENES[index], reason, seed: (index * 104729 + sample * 49999) ^ 0x5350574e,
                name: `${SCENES[index].name}/${sample}/${reason}` });
        }
    }
    if (entity === "minecraft:slime" || entity === "minecraft:zombie") {
        for (let index = 0; index < 600; index++) {
            cases.push({ ...BASE, name: `slime-chunk/${index}`, blocks: { "-1": "minecraft:stone" },
                x: (index * 1543) % 8001 - 4000, z: (index * 2741) % 8001 - 4000,
                y: index % 106 - 60, sky: 0, surface: 80, seed: index * 13337,
                worldSeed: String(BigInt(index) * 9876543219876543n - 3000000000000000000n) });
        }
    }
    return { entity, cases };
}
