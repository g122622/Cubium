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
import net.minecraft.sounds.SoundEvent;
import net.minecraft.sounds.SoundEvents;
import net.minecraft.world.level.gameevent.GameEvent;

/**
 * 绊线钩状态变化时音效与游戏事件的原版 oracle（{@code TripWireHookBlock#emitState}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.TripwireStateOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元名形如 {@code false:true:true:true}
 * （变化前 powered:attached : 变化后 powered:attached）。
 *
 * <p>原版 {@code emitState} 是 if / else if 链，四个分支互斥：
 *
 * <pre>
 *   powered &amp;&amp; !wasPowered       -&gt; CLICK_ON  (0.4, 0.6f) + BLOCK_ACTIVATE
 *   !powered &amp;&amp; wasPowered       -&gt; CLICK_OFF (0.4, 0.5f) + BLOCK_DEACTIVATE
 *   attached &amp;&amp; !wasAttached     -&gt; ATTACH    (0.4, 0.7f) + BLOCK_ATTACH
 *   !attached &amp;&amp; wasAttached     -&gt; DETACH    (0.4, 1.2f/(r*0.2f+0.9f)) + BLOCK_DETACH
 * </pre>
 *
 * <p>通电/断电<b>优先于</b>挂上/脱开 —— 两者同时变化时只报电量那次。
 * 写成并列四个 if 会让一次扯断连发两个事件两个音效。
 *
 * <p>音量恒为 0.4（f32），输出位模式避免 Gson 把 float 展开成 double 制造伪差异。
 */
public final class TripwireStateOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 原版 emitState 里写死的音量 */
    private static final float VOLUME = 0.4F;

    private TripwireStateOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (String wasPowered : new String[] {"false", "true"}) {
                for (String wasAttached : new String[] {"false", "true"}) {
                    for (String powered : new String[] {"false", "true"}) {
                        for (String attached : new String[] {"false", "true"}) {
                            units.add(wasPowered + ":" + wasAttached + ":" + powered + ":" + attached);
                        }
                    }
                }
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: TripwireStateOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        String[] parts = unit.split(":");
        if (parts.length != 4) {
            out.addProperty("missing", true);
            return out;
        }
        boolean wasPowered = parts[0].equals("true");
        boolean wasAttached = parts[1].equals("true");
        boolean powered = parts[2].equals("true");
        boolean attached = parts[3].equals("true");

        SoundEvent sound = null;
        GameEvent event = null;
        if (powered && !wasPowered) {
            sound = SoundEvents.TRIPWIRE_CLICK_ON;
            event = GameEvent.BLOCK_ACTIVATE.value();
        } else if (!powered && wasPowered) {
            sound = SoundEvents.TRIPWIRE_CLICK_OFF;
            event = GameEvent.BLOCK_DEACTIVATE.value();
        } else if (attached && !wasAttached) {
            sound = SoundEvents.TRIPWIRE_ATTACH;
            event = GameEvent.BLOCK_ATTACH.value();
        } else if (!attached && wasAttached) {
            sound = SoundEvents.TRIPWIRE_DETACH;
            event = GameEvent.BLOCK_DETACH.value();
        }

        if (sound == null) {
            out.addProperty("sound", (String) null);
            out.addProperty("volume", 0);
            out.addProperty("gameEvent", (String) null);
        } else {
            out.addProperty("sound", BuiltInRegistries.SOUND_EVENT.getKey(sound).toString());
            out.addProperty("volume", Float.floatToRawIntBits(VOLUME));
            out.addProperty("gameEvent", BuiltInRegistries.GAME_EVENT.getKey(event).toString());
        }
        return out;
    }
}
