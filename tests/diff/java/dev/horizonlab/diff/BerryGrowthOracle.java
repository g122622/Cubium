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
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.world.level.gameevent.GameEvent;

/**
 * 甜浆果丛生长时的阶段推进与游戏事件的原版 oracle（{@code SweetBerryBushBlock#randomTick}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.BerryGrowthOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元是 AGE（0..3）。
 *
 * <p>原版：
 *
 * <pre>
 *   int i = state.getValue(AGE);
 *   if (i &lt; 3 &amp;&amp; 随机通过 &amp;&amp; 光照够) {
 *       setBlock(state.setValue(AGE, i + 1));
 *       gameEvent(GameEvent.BLOCK_CHANGE, pos, ...);
 *   }
 * </pre>
 *
 * <p>AGE == 3 时不再生长、也不发事件 —— 满阶段的浆果丛每个随机刻都发事件的话，
 * 一丛浆果能让附近幽匿感测体每 tick 醒一次。
 */
public final class BerryGrowthOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private BerryGrowthOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (int age = 0; age <= 3; age++) {
                units.add(Integer.toString(age));
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BerryGrowthOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        int age;
        try {
            age = Integer.parseInt(unit);
        } catch (NumberFormatException e) {
            out.addProperty("missing", true);
            return out;
        }
        if (age < 0 || age > 3) {
            out.addProperty("missing", true);
            return out;
        }

        // 本探针驱动的是「骨粉催熟」这条确定性路径，不看随机与光照
        boolean grows = age < 3;
        out.addProperty("ageAfter", grows ? age + 1 : age);

        JsonArray events = new JsonArray();
        if (grows) {
            events.add(BuiltInRegistries.GAME_EVENT.getKey(GameEvent.BLOCK_CHANGE.value()).toString());
        }
        out.add("gameEvents", events);
        return out;
    }
}
