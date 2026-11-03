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
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.world.level.gameevent.GameEvent;

/**
 * 振动选择器 NBT 结构与往返的原版 oracle（{@code VibrationSelector.CODEC}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.VibrationSelectorNbtOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>原版 CODEC 只有两项：
 *
 * <pre>
 *   event  VibrationInfo.CODEC.lenientOptionalFieldOf("event")   可选
 *   tick   Codec.LONG.fieldOf("tick")  orElse(-1L)              无候选时写 -1
 * </pre>
 *
 * <p>两个易错点：
 *
 * <ol>
 *   <li>无候选时 {@code tick} 必须写 -1 而不是省略字段 —— 原版用 {@code orElse(-1L)}，
 *       字段存在但值是哨兵；省略会让「读取失败」和「没有候选」语义混淆
 *   <li>{@code event} 存在但 {@code tick < 0} 时不能恢复候选 —— 两字段是一对，
 *       只信一个会把损坏存档当成有效候选
 * </ol>
 */
public final class VibrationSelectorNbtOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final String[] UNITS = {"empty", "candidate"};

    /** 探针里候选用的游戏 tick */
    private static final long CANDIDATE_TICK = 4200L;
    /** 探针里候选用的距离 */
    private static final float CANDIDATE_DISTANCE = 6.5F;

    private VibrationSelectorNbtOracle() {}

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
        System.err.println("usage: VibrationSelectorNbtOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        boolean isEmpty = unit.equals("empty");
        boolean isCandidate = unit.equals("candidate");
        if (!isEmpty && !isCandidate) {
            out.addProperty("missing", true);
            return out;
        }

        // 原版 getter：currentVibrationData.map(getRight).orElse(-1L)
        long tick = isCandidate ? CANDIDATE_TICK : -1L;
        out.addProperty("savedTick", tick);
        // tick 字段永远存在（fieldOf 而非 optionalFieldOf）
        out.addProperty("hasTickField", true);

        if (isCandidate) {
            out.addProperty("hasEventField", true);
            JsonObject event = new JsonObject();
            event.addProperty("gameEvent",
                BuiltInRegistries.GAME_EVENT.getKey(GameEvent.SHEAR.value()).toString());
            event.addProperty("distance", (double) CANDIDATE_DISTANCE);
            out.add("savedEvent", event);
        } else {
            out.addProperty("hasEventField", false);
            out.add("savedEvent", null);
        }

        // 往返：只有 event 存在且 tick >= 0 时才恢复候选
        if (isCandidate) {
            out.addProperty("roundTripEvent",
                BuiltInRegistries.GAME_EVENT.getKey(GameEvent.SHEAR.value()).toString());
            out.addProperty("roundTripDistance", (double) CANDIDATE_DISTANCE);
        } else {
            out.add("roundTripEvent", null);
            out.add("roundTripDistance", null);
        }
        return out;
    }
}
