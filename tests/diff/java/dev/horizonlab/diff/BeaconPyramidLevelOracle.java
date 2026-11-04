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
import java.util.List;

/**
 * 信标金字塔等级（{@code BeaconBlockEntity.updateBase}）的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BeaconPyramidLevelOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <h2>为什么不直接调原版方法</h2>
 *
 * {@code updateBase} 是 private static，且需要一个 {@code Level} 来读方块。本能力把它的
 * 循环结构完整搬到参数上，逐行对应 {@code BeaconBlockEntity.java:198-224}；方块判定
 * 由单元里的布局字符串提供，两侧用同一份布局，因此结果完全可比。
 */
public final class BeaconPyramidLevelOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 与 C++ 探针 writeList 完全同构的场景表。 */
    private static final String L1 = "#########";
    private static final String L2 = "#########################";
    private static final String L3 = "###########################################";
    private static final String L4 = "#########################################################";

    /** 与 C++ 探针 buildScenes() 逐一对应，**恰好 5 段**。 */
    private static final String[] SCENES = {
        "-64;" + "" + ";" + "" + ";" + "" + ";" + "",
        "-64;" + L1 + ";" + "" + ";" + "" + ";" + "",
        "-64;" + "###.#.###" + ";" + "" + ";" + "" + ";" + "",
        "-64;" + ".###.####" + ";" + "" + ";" + "" + ";" + "",
        "-64;" + L1 + ";" + L2 + ";" + "" + ";" + "",
        "-64;" + L1 + ";" + L2 + ";" + L3 + ";" + "",
        "-64;" + L1 + ";" + L2 + ";" + L3 + ";" + L4,
        "-64;" + L1 + ";" + "#########.###############" + ";" + L3 + ";" + "",
        "-1;" + L1 + ";" + "" + ";" + "" + ";" + "",
        "-3;" + L1 + ";" + L2 + ";" + "" + ";" + "",
        "-2;" + L1 + ";" + L2 + ";" + L3 + ";" + "",
    };

    private static final int BEACON_Y = 0;
    private static final int MAX_LEVELS = 4;

    private BeaconPyramidLevelOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            Files.write(Path.of(args[1]), new ArrayList<>(List.of(SCENES)), StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
            "usage: BeaconPyramidLevelOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        List<String> lines = Files.readAllLines(input, StandardCharsets.UTF_8);
        List<String> results = new ArrayList<>(lines.size());
        for (String line : lines) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject obj = JsonParser.parseString(line).getAsJsonObject();
            results.add(GSON.toJson(solve(obj.get("unit").getAsString())));
        }
        Files.write(output, results, StandardCharsets.UTF_8);
    }

    private static JsonObject solve(String unit) {
        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        String[] parts = unit.split(";", -1);
        int minY = Integer.parseInt(parts[0]);
        String[] layers = new String[4];
        for (int i = 0; i < 4; i++) {
            layers[i] = i + 1 < parts.length ? parts[i + 1] : "";
        }

        out.addProperty("level", computeLevel(minY, layers));
        return out;
    }

    /** 原版 {@code updateBase} 的循环结构。 */
    private static int computeLevel(int minY, String[] layers) {
        int result = 0;

        for (int j = 1; j <= MAX_LEVELS; j++) {
            int layerY = BEACON_Y - j;

            // 低于世界底部即停止。
            if (layerY < minY) {
                break;
            }

            boolean valid = true;
            int side = 2 * j + 1;
            String layout = layers[j - 1];

            for (int dx = -j; dx <= j && valid; dx++) {
                for (int dz = -j; dz <= j; dz++) {
                    int row = dz + j;
                    int col = dx + j;
                    boolean isBase = layout.length() == side * side
                        && layout.charAt(row * side + col) == '#';
                    if (!isBase) {
                        valid = false;
                        break;
                    }
                }
            }

            if (!valid) {
                break;
            }
            result = j;
        }

        return result;
    }
}
