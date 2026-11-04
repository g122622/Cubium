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
package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;
import net.minecraft.SharedConstants;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.server.Bootstrap;
import net.minecraft.world.level.BlockGetter;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.entity.BlockEntity;
import net.minecraft.world.level.material.FluidState;
import net.minecraft.world.level.material.Fluids;
import net.minecraft.world.level.levelgen.Heightmap;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.phys.shapes.CollisionContext;
import net.minecraft.world.phys.shapes.VoxelShape;

/**
 * 方块三种形状（外形 / 碰撞 / 支撑）的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockShapeOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>三种形状各自独立取样，用固定顺序拼成扁平字符串，便于逐字节比对。
 */
public final class BlockShapeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private BlockShapeOracle() {}

    /**
     * 最小只读 {@code BlockGetter}：所有查询返回「无」。
     *
     * 形状计算大多不访问世界；少数会调 {@code getBlockEntity} / {@code getFluidState}，
     * 给它一个恒空的实现比传 {@code null} 安全。
     */
    private static final BlockGetter EMPTY_LEVEL = new BlockGetter() {
        @Override
        public BlockEntity getBlockEntity(BlockPos pos) {
            return null;
        }

        @Override
        public BlockState getBlockState(BlockPos pos) {
            return Blocks.AIR.defaultBlockState();
        }

        @Override
        public FluidState getFluidState(BlockPos pos) {
            return Fluids.EMPTY.defaultFluidState();
        }

        @Override
        public int getMinY() {
            return 0;
        }

        @Override
        public int getHeight() {
            return 1;
        }
    };

    /** 与探针侧 {@code describe} 保持完全相同的格式：体素化后按游程压缩输出。 */
    private static String describe(VoxelShape shape) {
        if (shape.isEmpty()) {
            return "EMPTY";
        }
        // 体素化到 16³ 网格：形状的语义是「覆盖了方块内的哪些点」，而不是「由几个盒子拼出来」。
        // 同一几何可以有无数种盒子拆分方式，逐盒输出会把表示差异误报成不一致。
        boolean[] grid = new boolean[16 * 16 * 16];
        for (int y = 0; y < 16; y++) {
            for (int z = 0; z < 16; z++) {
                for (int x = 0; x < 16; x++) {
                    double px = (x + 0.5) / 16.0;
                    double py = (y + 0.5) / 16.0;
                    double pz = (z + 0.5) / 16.0;
                    final double fx = px;
                    final double fy = py;
                    final double fz = pz;
                    final int idx = (y * 16 + z) * 16 + x;
                    // 逐个盒子检查体素中心是否落在盒内；命中一次即可，用返回值立即跳出遍历。
                    final boolean[] hit = {false};
                    shape.forAllBoxes((x1, y1, z1, x2, y2, z2) -> {
                        if (!hit[0] && fx > x1 && fx < x2 && fy > y1 && fy < y2 && fz > z1 && fz < z2) {
                            hit[0] = true;
                        }
                    });
                    grid[idx] = hit[0];
                }
            }
        }
        // 与探针侧相同的游程压缩：每段写成 `字符*次数`，段间用逗号分隔。
        // 不加分隔符时次数与次数相邻会产生歧义（`1`+`4096` 与 `14`+`096` 无法区分）。
        // 展开顺序必须与探针完全一致：探针按 y、z、x 三层循环把网格线性展开，
        // 这里先把网格复制到同一线性顺序，再按游程压缩。
        boolean[] linear = new boolean[grid.length];
        for (int y = 0; y < 16; y++) {
            for (int z = 0; z < 16; z++) {
                for (int x = 0; x < 16; x++) {
                    linear[(y * 16 + z) * 16 + x] = grid[(y * 16 + z) * 16 + x];
                }
            }
        }
        StringBuilder packed = new StringBuilder();
        boolean first = true;
        for (int i = 0; i < linear.length; ) {
            boolean ch = linear[i];
            int run = 1;
            while (i + run < linear.length && linear[i + run] == ch) {
                run++;
            }
            if (!first) {
                packed.append(',');
            }
            first = false;
            packed.append(ch ? '1' : '0').append(run);
            i += run;
        }
        return packed.toString();
    }

    public static void main(String[] args) throws IOException {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();
        if (args.length == 2 && args[0].equals("list")) {
            List<String> names = new ArrayList<>();
            for (Block block : BuiltInRegistries.BLOCK) {
                names.add(BuiltInRegistries.BLOCK.getKey(block).toString());
            }
            Collections.sort(names);
            StringBuilder text = new StringBuilder();
            for (String name : names) {
                text.append(name).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BlockShapeOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            Block block = BuiltInRegistries.BLOCK.get(Identifier.parse(name)).map(h -> h.value()).orElse(null);
            if (block == null) {
                throw new IllegalStateException("BlockShapeOracle: unknown block " + name);
            }
            BlockState state = block.defaultBlockState();

            // 部分方块的形状实现会查询 BlockGetter（如 getBlockEntity），传 null 会崩溃。
            // 用最小只读实现代替：所有查询都返回 null / 空气。
            String result = "shape=" + describe(state.getShape(EMPTY_LEVEL, BlockPos.ZERO))
                + "|collision=" + describe(state.getCollisionShape(EMPTY_LEVEL, BlockPos.ZERO, CollisionContext.empty()))
                + "|support=" + describe(state.getBlockSupportShape(EMPTY_LEVEL, BlockPos.ZERO));

            JsonObject row = new JsonObject();
            row.addProperty("result", result);
            row.addProperty("unit", name);
            text.append(GSON.toJson(row)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }
}