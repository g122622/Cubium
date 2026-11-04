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
import net.minecraft.sounds.SoundEvents;
import net.minecraft.world.level.gameevent.GameEvent;

/**
 * 拨动拉杆时发出的游戏事件与音效的原版 oracle（{@code LeverBlock#play}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.LeverGameEventOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元是拨动前的 {@code POWERED} 值（拨动必然翻转）。
 *
 * <p>原版 {@code LeverBlock#play} 在改完状态后做两件事（源码顺序）：
 *
 * <ol>
 *   <li>{@code playSound(null, pos, state.getValue(POWERED) ? SoundEvents.LEVER_CLICK : ...)}
 *   <li>{@code gameEvent(null, state.getValue(POWERED) ? GameEvent.BLOCK_ACTIVATE
 *       : GameEvent.BLOCK_DEACTIVATE, pos)}
 * </ol>
 *
 * <p>游戏事件是幽匿感测体等「听声音」机制的唯一输入，漏发不会崩溃、不掉日志，
 * 方块本身的状态和音效都正常，只有红石侦测端完全没反应。
 */
public final class LeverGameEventOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private LeverGameEventOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            units.add("false");
            units.add("true");
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: LeverGameEventOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        boolean initiallyPowered = unit.equals("true");
        // 拨动必然翻转
        boolean afterPowered = !initiallyPowered;

        out.addProperty("sound", BuiltInRegistries.SOUND_EVENT.getKey(SoundEvents.LEVER_CLICK).toString());
        out.addProperty("gameEvent",
            eventId((afterPowered ? GameEvent.BLOCK_ACTIVATE : GameEvent.BLOCK_DEACTIVATE).value()));
        return out;
    }

    private static String eventId(GameEvent event) {
        return BuiltInRegistries.GAME_EVENT.getKey(event).toString();
    }
}
