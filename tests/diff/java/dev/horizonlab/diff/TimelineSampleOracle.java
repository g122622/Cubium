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
import java.util.Optional;
import net.minecraft.util.EasingType;
import net.minecraft.util.Keyframe;
import net.minecraft.util.KeyframeTrack;
import net.minecraft.util.KeyframeTrackSampler;
import net.minecraft.world.attribute.LerpFunction;

/**
 * 关键帧轨道采样的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.TimelineSampleOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.TimelineSampleOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>这里**直接调用原版的 {@link KeyframeTrackSampler}**，不重写公式——轨道采样拆开
 * 重写容易在分段边界、周期回绕这些地方与实现悄悄分叉，而恰好这些地方才是要测的。
 * 探针侧同一份轨道定义也要与 {@code timeline_sample.cpp} 逐项一致。
 */
public final class TimelineSampleOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private TimelineSampleOracle() {}

    private static String floatBits(float value) {
        return String.format("0x%08x", Float.floatToRawIntBits(value));
    }

    /** 与原版 LerpFunction 同式：`from + f * (to - from)`。 */
    private static final LerpFunction<Float> LERP = (f, from, to) -> from + f * (to - from);

    /** 一条待测轨道。 */
    private record TrackCase(String id, List<Keyframe<Float>> keyframes,
            EasingType easing, Optional<Integer> period) {}

    private static List<Keyframe<Float>> keyframes(float... pairs) {
        List<Keyframe<Float>> list = new ArrayList<>();
        for (int i = 0; i < pairs.length; i += 2) {
            list.add(new Keyframe<>((int) pairs[i], pairs[i + 1]));
        }
        return list;
    }

    private static List<TrackCase> trackCases() {
        List<TrackCase> list = new ArrayList<>();
        list.add(new TrackCase("single", keyframes(0, 5.0F), EasingType.LINEAR, Optional.empty()));
        list.add(new TrackCase("two_linear", keyframes(0, 0.0F, 100, 10.0F), EasingType.LINEAR, Optional.empty()));
        list.add(new TrackCase("two_eased", keyframes(0, 0.0F, 100, 10.0F), EasingType.IN_OUT_SINE, Optional.empty()));
        list.add(new TrackCase("three_linear", keyframes(0, 0.0F, 50, 10.0F, 100, 0.0F), EasingType.LINEAR, Optional.empty()));
        list.add(new TrackCase("three_out_bounce", keyframes(0, 0.0F, 50, 10.0F, 100, 0.0F), EasingType.OUT_BOUNCE, Optional.empty()));
        list.add(new TrackCase("period_24000", keyframes(0, 0.0F, 12000, 1.0F), EasingType.LINEAR, Optional.of(24000)));
        list.add(new TrackCase("period_24000_eased", keyframes(0, 0.0F, 12000, 1.0F), EasingType.IN_OUT_QUAD, Optional.of(24000)));
        list.add(new TrackCase("period_24000_three", keyframes(0, 0.0F, 6000, 1.0F, 18000, 0.0F), EasingType.LINEAR, Optional.of(24000)));
        list.add(new TrackCase("period_offset", keyframes(1000, 0.0F, 13000, 1.0F), EasingType.LINEAR, Optional.of(24000)));
        list.add(new TrackCase("villager_schedule", keyframes(0, 0.0F, 10, 1.0F, 100, 1.0F), EasingType.CONSTANT, Optional.of(24000)));
        return list;
    }

    /** 采样时刻，必须与探针侧逐项一致。 */
    private static final long[] SAMPLE_TICKS = {
        -24001L, -24000L, -12000L, -1L, 0L, 1L, 25L, 49L, 50L, 51L, 75L,
        99L, 100L, 101L, 6000L, 12000L, 18000L, 23999L, 24000L, 24001L, 48000L, 100000L,
    };

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (TrackCase trackCase : trackCases()) {
                for (int i = 0; i < SAMPLE_TICKS.length; i++) {
                    units.add(trackCase.id() + "@" + i);
                }
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
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

        throw new IllegalArgumentException("usage: TimelineSampleOracle list <units.txt> | run <in> <out>");
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        int at = requested.lastIndexOf('@');
        if (at < 0) {
            return missing(requested);
        }
        String trackId = requested.substring(0, at);
        int tickIndex;
        try {
            tickIndex = Integer.parseInt(requested.substring(at + 1));
        } catch (NumberFormatException error) {
            return missing(requested);
        }
        if (tickIndex < 0 || tickIndex >= SAMPLE_TICKS.length) {
            return missing(requested);
        }

        for (TrackCase trackCase : trackCases()) {
            if (trackId.equals(trackCase.id())) {
                KeyframeTrack<Float> track = new KeyframeTrack<>(trackCase.keyframes(), trackCase.easing());
                // 构造函数是包私有的，走公开的 bakeSampler —— 生产代码也是这么建的。
                KeyframeTrackSampler<Float> sampler = track.bakeSampler(trackCase.period(), LERP);
                JsonObject output = new JsonObject();
                output.addProperty("unit", requested);
                output.addProperty("value", floatBits(sampler.sample(SAMPLE_TICKS[tickIndex])));
                return GSON.toJson(output);
            }
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