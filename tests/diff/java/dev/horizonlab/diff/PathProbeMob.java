/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

package dev.horizonlab.diff;

import java.util.EnumMap;
import java.util.Map;
import net.minecraft.world.entity.EntityDimensions;
import net.minecraft.world.entity.EntityType;
import net.minecraft.world.entity.HumanoidArm;
import net.minecraft.world.entity.Mob;
import net.minecraft.world.entity.Pose;
import net.minecraft.world.level.Level;
import net.minecraft.world.level.pathfinder.PathType;

/**
 * 受控寻路实体：尺寸、抬脚高度、代价表、地面/水中状态全部由场景输入指定。
 *
 * <p>求值器只通过这些属性认识实体，因此差分只要把这些属性两侧对齐，就能把「求值器算得
 * 对不对」与「某个具体原版生物配置得对不对」分开验证。这里刻意继承真实的 {@link Mob}
 * （而不是另写一个接口的子类）：原版的 {@code NodeEvaluator} 只接受 {@code Mob}，
 * 必须走同一条代码路径才叫真值。
 *
 * <p>与原版 {@code Mob} 的差异只有「属性来自外部输入」这一条，其余（代价表回退到
 * {@code PathType#getMalus()}、乘客继承语义、下落距离公式）都是继承来的原版实现。
 */
final class PathProbeMob extends Mob {
    private EntityDimensions dimensions;
    private float stepHeight;
    private final Map<PathType, Float> malus = new EnumMap<>(PathType.class);
    private final boolean onGroundFlag;
    private final boolean inWaterFlag;
    private final int maxFallDistanceOverride;

    /**
     * 构造受控实体。
     *
     * @param level 世界
     * @param width 碰撞箱宽度
     * @param height 碰撞箱高度
     * @param stepHeight 抬脚高度
     * @param onGround 是否站在地面上
     * @param inWater 是否在水里
     * @param maxFallDistance 若要覆盖原版下落距离公式则给非负值；-1 表示走原版公式
     */
    PathProbeMob(
        Level level,
        float width,
        float height,
        float stepHeight,
        boolean onGround,
        boolean inWater,
        int maxFallDistance) {
        super(EntityType.PIG, level);
        float scale = this.getScale();
        this.dimensions = EntityDimensions.fixed(width / scale, height / scale);
        // 注意：getScale() 是 final，且 sanitizeScale 会把过小值夹到下限，光设 SCALE 不足以完全消除缩放，
        // 所以 dimensions 里存的是「已除掉缩放」的尺寸，让 getDimensions 乘回来正好等于请求值。
        this.stepHeight = stepHeight;
        this.onGroundFlag = onGround;
        this.inWaterFlag = inWater;
        this.maxFallDistanceOverride = maxFallDistance;
    }

    /** 按 {@code PathType} 名称设置代价。 */
    void setMalus(PathType type, float value) {
        malus.put(type, value);
    }

    @Override
    public float getPathfindingMalus(PathType type) {
        Float value = malus.get(type);
        return value == null ? type.getMalus() : value;
    }

    @Override
    protected EntityDimensions getDefaultDimensions(Pose pose) {
        return dimensions;
    }

    @Override
    public float maxUpStep() {
        return stepHeight;
    }

    @Override
    public boolean onGround() {
        return onGroundFlag;
    }

    @Override
    public boolean isInWater() {
        return inWaterFlag;
    }

    @Override
    public int getMaxFallDistance() {
        return maxFallDistanceOverride >= 0 ? maxFallDistanceOverride : super.getMaxFallDistance();
    }

    @Override
    public HumanoidArm getMainArm() {
        return HumanoidArm.RIGHT;
    }

    /** 修改尺寸（场景切换实体时复用同一个实例，避免反复构造）。 */
    void resize(float width, float height, float stepHeight) {
        this.dimensions = EntityDimensions.fixed(width, height);
        this.stepHeight = stepHeight;
    }

    /**
     * 站到指定坐标。
     *
     * <p>{@code setPos} 会把碰撞箱按当前尺寸重新算出来，因此必须先 {@link #resize}。
     */
    void placeAt(double x, double y, double z, float yaw) {
        setPos(x, y, z);
        setYRot(yaw);
        // setPos 只挪坐标，不重算包围盒；显式刷新一次，让碰撞箱与尺寸声明一致。
        refreshDimensions();
    }
}
