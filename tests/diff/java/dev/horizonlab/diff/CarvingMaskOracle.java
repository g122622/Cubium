package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.world.level.chunk.CarvingMask;
import net.minecraft.world.level.ChunkPos;

/**
 * 雕刻掩码的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.CarvingMaskOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.CarvingMaskOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接驱动原版 {@link CarvingMask}，不重写索引公式。索引是
 * {@code x & 15 | (z & 15) << 4 | (y - minY) << 8}：X、Z 被掩码到 0..15，Y 原样参与位移
 * 且**没有任何范围检查**。这两点决定了越界坐标不是报错而是回绕/扩张，是与实现分叉的
 * 高发区，必须由原版自己给出答案。
 */
public final class CarvingMaskOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private CarvingMaskOracle() {}

    /** 一个场景：与探针侧逐项一致。 */
    private record MaskScene(String id, int minY, int height, List<int[]> operations) {}

    private static MaskScene scene(String id, int minY, int height, int[]... flat) {
        List<int[]> operations = new ArrayList<>();
        for (int[] operation : flat) {
            operations.add(operation);
        }
        return new MaskScene(id, minY, height, operations);
    }

    /**
     * 场景表：与 {@code probes/carving_mask.cpp} 的 {@code maskScenes()} 逐项一致。
     *
     * <p>每个操作是 {@code {kind, x, y, z}}，kind 为 0 表示 set、1 表示 get。
     */
    private static List<MaskScene> maskScenes() {
        List<MaskScene> list = new ArrayList<>();
        list.add(scene("overworld_inside", -64, 384,
                new int[] {0, 0, -64, 0}, new int[] {0, 15, 319, 15}, new int[] {0, 7, 64, 8},
                new int[] {1, 0, -64, 0}, new int[] {1, 15, 319, 15}, new int[] {1, 7, 64, 8},
                new int[] {1, 1, -64, 0}, new int[] {1, 0, -63, 0}, new int[] {1, 0, -64, 1}));
        list.add(scene("overworld_wrap_xz", -64, 384,
                new int[] {0, 16, 0, 0}, new int[] {0, 0, 0, 16}, new int[] {0, -1, 0, -1},
                new int[] {1, 0, 0, 0}, new int[] {1, 16, 0, 0}, new int[] {1, -1, 0, -1},
                new int[] {1, 15, 0, 15}));
        list.add(scene("overworld_y_edges", -64, 384,
                new int[] {0, 0, -64, 0}, new int[] {0, 0, 319, 0},
                new int[] {1, 0, -64, 0}, new int[] {1, 0, 319, 0}, new int[] {1, 0, -63, 0}, new int[] {1, 0, 318, 0},
                new int[] {0, 5, -63, 5}, new int[] {0, 5, 312, 5},
                new int[] {1, 5, -63, 5}, new int[] {1, 5, 312, 5}));
        list.add(scene("overworld_collide", -64, 384,
                new int[] {0, 17, 0, -3}, new int[] {1, 1, 0, 13}, new int[] {1, 17, 0, -3},
                new int[] {0, 33, 64, 18}, new int[] {1, 1, 64, 2},
                new int[] {0, 0, 1, 0}, new int[] {1, 0, 1, 0}));
        list.add(scene("nether_inside", 0, 128,
                new int[] {0, 0, 0, 0}, new int[] {0, 15, 127, 15},
                new int[] {1, 0, 127, 0}, new int[] {1, 15, 0, 15}, new int[] {1, 8, 64, 8}));
        list.add(scene("nether_edges", 0, 128,
                new int[] {0, 3, 0, 3}, new int[] {0, 3, 120, 3},
                new int[] {1, 3, 0, 3}, new int[] {1, 3, 120, 3}, new int[] {1, 3, 1, 3}));
        list.add(scene("end_inside", 0, 256,
                new int[] {0, 5, 100, 9}, new int[] {0, 9, 100, 5},
                new int[] {1, 5, 100, 9}, new int[] {1, 9, 100, 5}, new int[] {1, 9, 100, 9}));
        return list;
    }

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (MaskScene scene : maskScenes()) {
                text.append(scene.id()).append('\n');
            }
            Files.writeString(Path.of(args[1]), text.toString(), StandardCharsets.UTF_8);
            return;
        }

        if (args.length == 3 && args[0].equals("run")) {
            List<String> inputs = Files.readAllLines(Path.of(args[1]), StandardCharsets.UTF_8);
            List<String> outputs = new ArrayList<>(inputs.size());
            for (String line : inputs) {
                if (line.isBlank()) {
                    continue;
                }
                outputs.add(runLine(line));
            }
            Files.write(Path.of(args[2]), outputs, StandardCharsets.UTF_8);
            return;
        }

        throw new IllegalArgumentException("usage: CarvingMaskOracle list <units.txt> | run <in> <out>");
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        for (MaskScene scene : maskScenes()) {
            if (!requested.equals(scene.id())) {
                continue;
            }

            // 原版构造器是 (height, minY)——与 Cubium 的 (chunkX, chunkZ, minY, height) 顺序不同，
            // 传反了会让整张掩码的 Y 位移基准错位。
            CarvingMask mask = new CarvingMask(scene.height(), scene.minY());

            JsonArray reads = new JsonArray();
            for (int[] operation : scene.operations()) {
                int kind = operation[0];
                int x = operation[1];
                int y = operation[2];
                int z = operation[3];
                if (kind == 0) {
                    mask.set(x, y, z);
                } else {
                    reads.add(mask.get(x, y, z));
                }
            }

            // 与探针侧相同的扫描：统计预分配范围内被标记的格数。
            int marked = 0;
            for (int y = scene.minY(); y < scene.minY() + scene.height(); y++) {
                for (int x = 0; x < 16; x++) {
                    for (int z = 0; z < 16; z++) {
                        if (mask.get(x, y, z)) {
                            marked++;
                        }
                    }
                }
            }

            JsonObject output = new JsonObject();
            output.addProperty("unit", requested);
            output.addProperty("minY", scene.minY());
            output.addProperty("height", scene.height());
            output.add("reads", reads);
            output.addProperty("marked", marked);
            return GSON.toJson(output);
        }
        return missing(requested);
    }

    private static String missing(String requested) {
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}