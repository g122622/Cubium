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
 * 潮涌核心框架等级（{@code ConduitBlockEntity.updateShape}）的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ConduitFrameLevelOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <h2>为什么不直接调原版方法</h2>
 *
 * {@code updateShape} 是 private static，且需要一个 {@code Level} 来读方块。本能力把它的
 * 两段判定完整搬到参数上，逐行对应 {@code ConduitBlockEntity.java:127-162}；方块判定
 * 由单元里的位图字符串提供，两侧用同一份布局。
 */
public final class ConduitFrameLevelOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final int MIN_ACTIVE_SIZE = 16;
    private static final int MIN_KILL_SIZE = 42;

    private ConduitFrameLevelOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> scenes = new ArrayList<>();
            scenes.add("0;" + "1".repeat(42));
            scenes.add("0;" + "0".repeat(42));
            scenes.add("0;" + "1".repeat(41) + "0");
            scenes.add("0;" + "1".repeat(16) + "0".repeat(26));
            scenes.add("0;" + "1".repeat(15) + "0".repeat(27));
            scenes.add("0;" + "1".repeat(17) + "0".repeat(25));
            scenes.add("1;" + "1".repeat(42));
            scenes.add("1;" + "0".repeat(42));
            Files.write(Path.of(args[1]), scenes, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
            "usage: ConduitFrameLevelOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        int sep = unit.indexOf(';');
        boolean drained = unit.substring(0, sep).equals("1");
        String bitmap = unit.substring(sep + 1);

        int frameCount = countFrameBlocks(drained, bitmap);

        out.addProperty("frameCount", frameCount);
        out.addProperty("active", frameCount >= MIN_ACTIVE_SIZE);
        out.addProperty("eyeOpen", frameCount >= MIN_KILL_SIZE);
        out.addProperty("killReady", frameCount >= MIN_KILL_SIZE);
        return out;
    }

    /** 原版 {@code updateShape} 的两段判定。 */
    private static int countFrameBlocks(boolean drained, String bitmap) {
        // 第 1 段：中心 3x3x3 必须全是水。
        if (drained) {
            return -1;
        }

        // 第 2 段：只认框架环（至少一个分量绝对值 > 1，且某个分量为 0），共 42 格。
        List<int[]> ring = new ArrayList<>();
        for (int dx = -2; dx <= 2; dx++) {
            for (int dy = -2; dy <= 2; dy++) {
                for (int dz = -2; dz <= 2; dz++) {
                    int adx = Math.abs(dx);
                    int ady = Math.abs(dy);
                    int adz = Math.abs(dz);
                    if (adx <= 1 && ady <= 1 && adz <= 1) {
                        continue;
                    }
                    boolean isFrame = (dx == 0 && (ady == 2 || adz == 2))
                        || (dy == 0 && (adx == 2 || adz == 2))
                        || (dz == 0 && (adx == 2 || ady == 2));
                    if (isFrame) {
                        ring.add(new int[] {dx, dy, dz});
                    }
                }
            }
        }

        if (bitmap.length() != ring.size()) {
            throw new IllegalArgumentException("bitmap length " + bitmap.length() + " != " + ring.size());
        }

        int count = 0;
        for (int i = 0; i < ring.size(); i++) {
            if (bitmap.charAt(i) == '1') {
                count++;
            }
        }
        return count;
    }
}
