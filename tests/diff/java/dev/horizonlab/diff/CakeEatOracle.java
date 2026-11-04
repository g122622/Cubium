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
 * 吃一片蛋糕时发出的游戏事件的原版 oracle（{@code CakeBlock#eat}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.CakeEatOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元是吃之前的 {@code BITES}（0..6）。
 *
 * <p>原版 {@code eat} 的事件是<b>叠加</b>的：
 *
 * <pre>
 *   gameEvent(GameEvent.EAT, pos);            // 每一片都有
 *   if (bites &lt; 6) setBlock(BITES+1);
 *   else { removeBlock(); gameEvent(GameEvent.BLOCK_DESTROY, pos); }
 * </pre>
 *
 * <p>所以 BITES=6 连发两个事件，其余只发一个。
 */
public final class CakeEatOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private CakeEatOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (int bites = 0; bites <= 6; bites++) {
                units.add(Integer.toString(bites));
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: CakeEatOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        int bites;
        try {
            bites = Integer.parseInt(unit);
        } catch (NumberFormatException e) {
            out.addProperty("missing", true);
            return out;
        }
        if (bites < 0 || bites > 6) {
            out.addProperty("missing", true);
            return out;
        }

        JsonArray events = new JsonArray();
        events.add(eventId(GameEvent.EAT.value()));
        if (bites >= 6) {
            events.add(eventId(GameEvent.BLOCK_DESTROY.value()));
        }
        out.add("gameEvents", events);

        if (bites < 6) {
            out.addProperty("bitesAfter", bites + 1);
        } else {
            // 最后一片：蛋糕整块被移除
            out.addProperty("bitesAfter", (Integer) null);
        }
        return out;
    }

    private static String eventId(GameEvent event) {
        return BuiltInRegistries.GAME_EVENT.getKey(event).toString();
    }
}
