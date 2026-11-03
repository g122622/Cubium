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
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.state.properties.BlockStateProperties;
import net.minecraft.world.level.block.state.properties.NoteBlockInstrument;

/**
 * 音符盒触发音效三元组的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.NoteBlockTriggerOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元名形如 {@code harp:12}（乐器名:音符值）。
 *
 * <p>原版把播放放在 {@code NoteBlock#triggerEvent} 里，而 oracle 的进程内没有 {@code Level}，
 * 无法真的调它。这里改为**逐项复现** triggerEvent 里做的四件事，四个值都从原版的公开 API 取：
 *
 * <ol>
 *   <li>声音事件：{@code NoteBlockInstrument#getSoundEvent()}（头颅类是 {@code hasCustomSound}）
 *   <li>音量：原版写死 {@code 3.0F}
 *   <li>音高：{@code isTunable()} 时 {@code NoteBlock.getPitchFromNote(note)}，否则 {@code 1.0F}
 *   <li>音符粒子：仅 {@code isTunable()} 时发出，颜色偏移 {@code note / 24.0}
 * </ol>
 *
 * <p>f32 一律输出位模式，避免 Gson 把 float 展开成 double 制造伪差异。
 */
public final class NoteBlockTriggerOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 原版 {@code NoteBlock.triggerEvent} 里的常量音量 */
    private static final float NOTE_VOLUME = 3.0F;

    private static final int[] SAMPLE_NOTES = {0, 12, 24};

    private NoteBlockTriggerOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (NoteBlockInstrument instrument : NoteBlockInstrument.values()) {
                for (int note : SAMPLE_NOTES) {
                    units.add(instrument.getSerializedName() + ":" + note);
                }
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: NoteBlockTriggerOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        int colon = unit.lastIndexOf(':');
        if (colon < 0) {
            out.addProperty("missing", true);
            return out;
        }
        String instrumentName = unit.substring(0, colon);
        int note = Integer.parseInt(unit.substring(colon + 1));

        NoteBlockInstrument instrument = null;
        for (NoteBlockInstrument candidate : NoteBlockInstrument.values()) {
            if (candidate.getSerializedName().equals(instrumentName)) {
                instrument = candidate;
                break;
            }
        }
        if (instrument == null) {
            out.addProperty("missing", true);
            return out;
        }

        // 组装目标状态，与 C++ 侧一致
        BlockState state = Blocks.NOTE_BLOCK
            .defaultBlockState()
            .setValue(BlockStateProperties.NOTEBLOCK_INSTRUMENT, instrument)
            .setValue(BlockStateProperties.NOTE, note);

        // 复现 triggerEvent：可调音乐器按音符值定音高并发出音符粒子，头颅类固定 1.0 无粒子
        boolean tunable = instrument.isTunable();
        float pitch = tunable ? net.minecraft.world.level.block.NoteBlock.getPitchFromNote(note) : 1.0F;

        // 头颅类里只有 custom_head 走自定义音效（需要上方头颅方块实体的 note_block_sound）；
        // 没有方块实体时原版 return false —— 这里如实反映「取不到自定义音效」的情况
        if (instrument.hasCustomSound()) {
            // 场景里音符盒上方是空气，取不到 SkullBlockEntity
            out.addProperty("handled", false);
            out.addProperty("hasParticle", false);
            return out;
        }

        SoundEvent soundEvent = instrument.getSoundEvent().value();
        String soundId = BuiltInRegistries.SOUND_EVENT.getKey(soundEvent).toString();

        out.addProperty("handled", true);
        out.addProperty("sound", soundId);
        out.addProperty("volume", bits(NOTE_VOLUME));
        out.addProperty("pitch", bits(pitch));
        out.addProperty("hasParticle", tunable);
        if (tunable) {
            // 粒子类型固定为 NOTE；颜色偏移是 note / 24.0（原版用 double 算出后转 float）
            out.addProperty("particleType", "note");
            out.addProperty("particleDx", bits((float) (note / 24.0)));
        }
        return out;
    }

    private static String bits(float value) {
        return String.format("0x%08x", Float.floatToRawIntBits(value));
    }
}
