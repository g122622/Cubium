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
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/**
 * 振动信息 NBT 结构的原版 oracle（{@code VibrationInfo.CODEC}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.VibrationInfoNbtOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元是字段组合：none / source / owner / both。
 *
 * <p>原版 {@code VibrationInfo.CODEC}：
 *
 * <pre>
 *   game_event        string            必填
 *   distance          float             必填
 *   pos               double[3]         必填
 *   source            UUID(int[4])      可选
 *   projectile_owner  UUID(int[4])      可选
 * </pre>
 *
 * <p><b>UUID 不是 long</b>：原版 {@code UUIDUtil.CODEC} 是
 * {@code Codec.INT_STREAM.comapFlatMap(fixedSize(4))}，把 16 字节按大端拆成 4 个有符号 int。
 * 写成单个 long 会让存档与网络包都和原版对不上。
 */
public final class VibrationInfoNbtOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final String[] UNITS = {"none", "source", "owner", "both"};

    private static final String SOURCE_UUID = "0f0e0d0c-0b0a-0908-0706-050403020100";
    private static final String OWNER_UUID = "ffffffff-ffff-ffff-8000-000000000001";

    /** 测试用的振动距离，与探针保持一致 */
    private static final float DISTANCE = 12.5F;

    private VibrationInfoNbtOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (String unit : UNITS) {
                units.add(unit);
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: VibrationInfoNbtOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 原版 UUIDUtil.uuidToIntArray：大端 16 字节拆成 4 个 int */
    private static JsonArray uuidToIntArray(UUID uuid) {
        long msb = uuid.getMostSignificantBits();
        long lsb = uuid.getLeastSignificantBits();
        JsonArray array = new JsonArray();
        array.add((int) (msb >> 32));
        array.add((int) msb);
        array.add((int) (lsb >> 32));
        array.add((int) lsb);
        return array;
    }

    private static JsonObject solve(String unit) {
        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        boolean known = Arrays.asList(UNITS).contains(unit);
        if (!known) {
            out.addProperty("missing", true);
            return out;
        }
        boolean withSource = unit.equals("source") || unit.equals("both");
        boolean withOwner = unit.equals("owner") || unit.equals("both");

        // 原版用 GameEvent.BLOCK_CHANGE.key()，序列化成完整 id
        out.addProperty("gameEvent", "minecraft:block_change");

        // distance 是 f32，按位模式输出避免 Gson 展开成 double 制造伪差异
        out.addProperty("distanceBits", Float.floatToRawIntBits(DISTANCE));

        JsonArray pos = new JsonArray();
        pos.add(1.25);
        pos.add(64.0);
        pos.add(-3.75);
        out.add("pos", pos);

        if (withSource) {
            out.add("source", uuidToIntArray(UUID.fromString(SOURCE_UUID)));
        } else {
            out.add("source", null);
        }
        if (withOwner) {
            out.add("projectileOwner", uuidToIntArray(UUID.fromString(OWNER_UUID)));
        } else {
            out.add("projectileOwner", null);
        }
        return out;
    }
}
