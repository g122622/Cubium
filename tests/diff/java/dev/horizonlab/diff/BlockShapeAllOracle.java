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
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Locale;
import net.minecraft.SharedConstants;
import net.minecraft.core.BlockPos;
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
 * 方块三种形状（外形 / 碰撞 / 支撑）的**全部状态** oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BlockShapeAllOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 {@code BlockShapeOracle}（只测默认状态）互补：这里枚举每个方块的全部属性组合，
 * 单元名采用 {@code minecraft:name[key=value,...]}（属性按字典序，与原版
 * {@code BlockState.toString()} 的属性顺序一致，只是去掉 {@code Block{}} 外壳）。
 */
public final class BlockShapeAllOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private BlockShapeAllOracle() {}

    /** 最小只读 {@code BlockGetter}：所有查询返回「无」。 */
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

    /**
     * 把状态渲染成 {@code minecraft:name[key=value,...]}。
     *
     * <p>原版 {@code BlockState.toString()} 输出 {@code Block{minecraft:name}[...]}，
     * 这里只做外壳剥离，属性顺序直接沿用（原版本身即为字典序）。
     */
    private static String unitName(BlockState state) {
        String text = state.toString();
        int open = text.indexOf('{');
        int close = text.indexOf('}');
        if (open < 0 || close < 0) {
            throw new IllegalStateException("unexpected block state string: " + text);
        }
        return text.substring(open + 1, close) + text.substring(close + 1);
    }

    /** 与探针侧 {@code describe} 完全一致的体素化 + 游程压缩。 */
    private static String describe(VoxelShape shape) {
        if (shape.isEmpty()) {
            return "EMPTY";
        }
        boolean[] grid = new boolean[16 * 16 * 16];
        for (int y = 0; y < 16; y++) {
            for (int z = 0; z < 16; z++) {
                for (int x = 0; x < 16; x++) {
                    final double fx = (x + 0.5) / 16.0;
                    final double fy = (y + 0.5) / 16.0;
                    final double fz = (z + 0.5) / 16.0;
                    final int idx = (y * 16 + z) * 16 + x;
                    shape.forAllBoxes((x1, y1, z1, x2, y2, z2) -> {
                        if (fx > x1 && fx < x2 && fy > y1 && fy < y2 && fz > z1 && fz < z2) {
                            grid[idx] = true;
                        }
                    });
                }
            }
        }
        StringBuilder packed = new StringBuilder();
        for (int i = 0; i < grid.length; ) {
            boolean ch = grid[i];
            int run = 1;
            while (i + run < grid.length && grid[i + run] == ch) {
                run++;
            }
            packed.append(ch ? '1' : '0').append(':').append(run);
            i += run;
        }
        return packed.toString();
    }

    /** 枚举一个方块的全部状态。 */
    private static List<BlockState> allStates(Block block) {
        List<BlockState> result = new ArrayList<>(block.getStateDefinition().getPossibleStates());
        return result;
    }

    public static void main(String[] args) throws IOException {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();
        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (Block block : BuiltInRegistries.BLOCK) {
                for (BlockState state : allStates(block)) {
                    units.add(unitName(state));
                }
            }
            Collections.sort(units);
            StringBuilder text = new StringBuilder();
            for (String unit : units) {
                text.append(unit).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BlockShapeAllOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        // 单元是完整状态字符串，先建全量索引，避免每行都重新枚举。
        Map<String, BlockState> index = new HashMap<>();
        for (Block block : BuiltInRegistries.BLOCK) {
            for (BlockState state : allStates(block)) {
                index.put(unitName(state), state);
            }
        }
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            BlockState state = index.get(name);
            if (state == null) {
                throw new IllegalStateException("BlockShapeAllOracle: unknown block state " + name);
            }
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