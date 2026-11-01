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
import java.util.List;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.world.level.block.Block;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.levelgen.Heightmap;

/**
 * {@code Heightmap.Types} 谓词的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.HeightmapTypesOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <h2>属性四元组如何映射到真实方块</h2>
 *
 * <p>探针用 `<air>|<blocksMotion>|<hasFluid>|<isLeaves>` 四元组描述方块，oracle 必须找到
 * **属性完全匹配**的真实 `BlockState` 才能调用原版 `isOpaque()`。映射表：
 *
 * <table>
 *   <tr><th>四元组 (air,motion,fluid,leaves)</th><th>方块</th></tr>
 *   <tr><td>1,0,0,0</td><td>AIR</td></tr>
 *   <tr><td>0,1,0,0</td><td>STONE</td></tr>
 *   <tr><td>0,0,1,0</td><td>WATER</td></tr>
 *   <tr><td>0,0,0,0</td><td>SHORT_GRASS（非空气、不阻挡、无流体）</td></tr>
 *   <tr><td>0,0,0,1</td><td>OAK_LEAVES（普通树叶）</td></tr>
 *   <tr><td>0,1,0,1</td><td>无真实方块——用 STONE 代替并按「树叶」处理</td></tr>
 *   <tr><td>0,0,1,1</td><td>无真实方块——用 WATER 代替并按「树叶」处理</td></tr>
 * </table>
 *
 * <p>后两行的组合在原版里不存在（没有带流体的树叶，也没有 overriding blocksMotion 的树叶），
 * 因此 oracle 侧无法用真实状态验证。这两组用例会记为**已知缺口**，理由是「原版无对应方块」。
 *
 * <p>单元表必须与 probes/heightmap_types.cpp 逐项一致。
 */
public final class HeightmapTypesOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "info|WORLD_SURFACE_WG",
                    "info|WORLD_SURFACE",
                    "info|OCEAN_FLOOR_WG",
                    "info|OCEAN_FLOOR",
                    "info|MOTION_BLOCKING",
                    "info|MOTION_BLOCKING_NO_LEAVES",
                    "test|WORLD_SURFACE|1|0|0|0",
                    "test|MOTION_BLOCKING|1|0|0|0",
                    "test|MOTION_BLOCKING_NO_LEAVES|1|0|0|0",
                    "test|WORLD_SURFACE|0|1|0|0",
                    "test|OCEAN_FLOOR|0|1|0|0",
                    "test|MOTION_BLOCKING|0|1|0|0",
                    "test|MOTION_BLOCKING_NO_LEAVES|0|1|0|0",
                    "test|WORLD_SURFACE|0|0|1|0",
                    "test|OCEAN_FLOOR|0|0|1|0",
                    "test|MOTION_BLOCKING|0|0|1|0",
                    "test|MOTION_BLOCKING_NO_LEAVES|0|0|1|0",
                    "test|WORLD_SURFACE|0|0|0|0",
                    "test|OCEAN_FLOOR|0|0|0|0",
                    "test|MOTION_BLOCKING|0|0|0|0",
"test|WORLD_SURFACE|0|1|0|1",
"test|OCEAN_FLOOR|0|1|0|1",
                    "test|MOTION_BLOCKING|0|1|0|1",
                    "test|MOTION_BLOCKING_NO_LEAVES|0|1|0|1",
                    "byId|0",
                    "byId|1",
                    "byId|2",
                    "byId|3",
                    "byId|4",
                    "byId|5",
                    "byId|6",
                    "byId|-1",
                    "byId|999");

    private HeightmapTypesOracle() {}

    public static void main(String[] args) throws IOException {
        DataPackRegistries.load();
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String unit : UNITS) {
                text.append(unit).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
                "usage: HeightmapTypesOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            text.append(GSON.toJson(summarise(unit))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static Heightmap.Types findType(String name) {
        for (Heightmap.Types type : Heightmap.Types.values()) {
            if (type.name().equals(name)) {
                return type;
            }
        }
        return null;
    }

    /**
     * 找到属性四元组对应的真实 `BlockState`。
     *
     * @return 匹配的状态；若无真实方块对应则返回 `null`
     */
    private static BlockState stateFor(
            boolean air, boolean blocksMotion, boolean hasFluid, boolean isLeaves) {
        Block block;
        if (air) {
            block = Blocks.AIR;
        } else if (isLeaves) {
            // 普通树叶：不阻挡、无流体。
            // 原版 LeavesBlock 的 blocksMotion() 为 true；带流体的树叶原版不存在。
            if (hasFluid) {
                return null; // 原版无此类树叶
            }
            block = Blocks.OAK_LEAVES;
        } else if (hasFluid) {
            if (blocksMotion) {
                return null; // 带流体的固体不常见，也不在本能力用例里
            }
            block = Blocks.WATER;
        } else if (blocksMotion) {
            block = Blocks.STONE;
        } else {
            block = Blocks.SHORT_GRASS;
        }

        // 校验推导出的状态确实满足四元组，否则说明映射表写错了。
        BlockState state = block.defaultBlockState();
        boolean actualAir = state.isAir();
        boolean actualMotion = state.blocksMotion();
        boolean actualFluid = !state.getFluidState().isEmpty();
        boolean actualLeaves = block instanceof net.minecraft.world.level.block.LeavesBlock;
        if (actualAir != air || actualMotion != blocksMotion || actualFluid != hasFluid
                || actualLeaves != isLeaves) {
            throw new IllegalStateException(
                    "heightmap_types: 方块的属性与四元组不符：" + unitOf(air, blocksMotion, hasFluid,
                            isLeaves) + " -> " + BuiltInRegistries.BLOCK.getKey(block)
                            + " (air=" + actualAir + ",motion=" + actualMotion
                            + ",fluid=" + actualFluid + ",leaves=" + actualLeaves + ")");
        }
        return state;
    }

    private static String unitOf(boolean air, boolean motion, boolean fluid, boolean leaves) {
        return (air ? "1" : "0") + (motion ? "1" : "0") + (fluid ? "1" : "0")
                + (leaves ? "1" : "0");
    }

    /** 由两个公开布尔方法反推 `Usage`。 */
    private static String usageName(Heightmap.Types type) {
        if (type.sendToClient()) {
            return "CLIENT";
        }
        return type.keepAfterWorldgen() ? "LIVE_WORLD" : "WORLDGEN";
    }
    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("heightmap_types: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "info" -> {
                Heightmap.Types type = findType(parts[1]);
                if (type == null) {
                    out.addProperty("value", "null");
                } else {
                    out.addProperty("value",
                            type.ordinal() + "," + usageName(type) + ","
                                    + predicateName(type));
                }
            }
            case "test" -> {
                Heightmap.Types type = findType(parts[1]);
                if (type == null) {
                    out.addProperty("value", "null");
                } else {
                    BlockState state = stateFor(
                            parts[2].equals("1"), parts[3].equals("1"),
                            parts[4].equals("1"), parts[5].equals("1"));
                    if (state == null) {
                        // 原版无对应方块：交给框架按「已知缺口」处理。
                        out.addProperty("value", "noSuchBlock");
                    } else {
                        out.addProperty("value", type.isOpaque().test(state) ? "true" : "false");
                    }
                }
            }
            case "byId" -> {
                int id = Integer.parseInt(parts[1]);
                // BY_ID 是私有的；ZERO 策略等价于「越界取 values()[0]」。
                Heightmap.Types[] all = Heightmap.Types.values();
                Heightmap.Types type = (id >= 0 && id < all.length) ? all[id] : all[0];
                out.addProperty("value", type.name());
            }
            default -> throw new IllegalArgumentException("heightmap_types: 无法识别的单元: " + unit);
        }

        return out;
    }

    /**
     * 把谓词反推成名字。
     *
     * <p>原版没有公开谓词名，这里按 `Types` 的声明逐个对应。
     */
    private static String predicateName(Heightmap.Types type) {
        return switch (type) {
            case WORLD_SURFACE_WG, WORLD_SURFACE -> "NOT_AIR";
            case OCEAN_FLOOR_WG, OCEAN_FLOOR -> "MATERIAL_MOTION_BLOCKING";
            case MOTION_BLOCKING -> "MOTION_BLOCKING";
            case MOTION_BLOCKING_NO_LEAVES -> "MOTION_BLOCKING_NO_LEAVES";
        };
    }
}
