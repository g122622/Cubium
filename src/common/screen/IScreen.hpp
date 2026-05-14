#pragma once

#include "core/Types.hpp"
#include <string>

namespace mc {

/**
 * @brief 屏幕接口
 *
 * 所有客户端UI屏幕的基类接口。
 * 定义屏幕的基本生命周期和交互方法。
 *
 * 屏幕生命周期：
 * 1. 创建 - 通过工厂方法创建
 * 2. 初始化 - init() 设置UI元素
 * 3. 显示 - 显示给玩家
 * 4. 交互 - 玩家点击/按键
 * 5. 关闭 - onClose() 清理资源
 */
class IScreen {
public:
    virtual ~IScreen() = default;

    /**
     * @brief 初始化屏幕
     *
     * 在屏幕显示前调用，用于设置UI元素和状态。
     */
    virtual void init() = 0;

    /**
     * @brief 渲染屏幕
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param partialTick 部分tick时间
     */
    virtual void render(i32 mouseX, i32 mouseY, f32 partialTick) = 0;

    /**
     * @brief 处理鼠标点击
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param button 鼠标按钮 (0=左键, 1=右键, 2=中键)
     * @return 如果事件被处理返回true
     */
    virtual bool onClick(i32 mouseX, i32 mouseY, i32 button) = 0;

    /**
     * @brief 处理鼠标释放
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param button 鼠标按钮
     * @return 如果事件被处理返回true
     */
    virtual bool onRelease(i32 mouseX, i32 mouseY, i32 button)
    {
        (void)mouseX;
        (void)mouseY;
        (void)button;
        return false;
    }

    /**
     * @brief 处理鼠标拖动
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param deltaX X方向移动量
     * @param deltaY Y方向移动量
     * @return 如果事件被处理返回true
     */
    virtual bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY)
    {
        (void)mouseX;
        (void)mouseY;
        (void)deltaX;
        (void)deltaY;
        return false;
    }

    /**
     * @brief 处理鼠标滚轮
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param delta 滚轮增量
     * @return 如果事件被处理返回true
     */
    virtual bool onScroll(i32 mouseX, i32 mouseY, f64 delta)
    {
        (void)mouseX;
        (void)mouseY;
        (void)delta;
        return false;
    }

    /**
     * @brief 处理键盘按键
     * @param key 键码
     * @param scanCode 扫描码
     * @param action 动作 (0=释放, 1=按下, 2=重复)
     * @param mods 修饰键
     * @return 如果事件被处理返回true
     */
    virtual bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) = 0;

    /**
     * @brief 处理字符输入
     * @param codePoint Unicode码点
     * @return 如果事件被处理返回true
     */
    virtual bool onChar(u32 codePoint)
    {
        (void)codePoint;
        return false;
    }

    /**
     * @brief 屏幕关闭时调用
     *
     * 用于清理资源和保存状态。
     */
    virtual void onClose() = 0;

    /**
     * @brief 检查屏幕是否暂停游戏
     * @return 如果屏幕暂停游戏返回true
     *
     * 如菜单屏幕暂停游戏，背包屏幕不暂停。
     */
    [[nodiscard]] virtual bool isPauseScreen() const { return false; }

    /**
     * @brief 获取屏幕标题
     * @return 标题文本
     */
    [[nodiscard]] virtual std::string getTitle() const { return ""; }

    /**
     * @brief 检查屏幕是否应该渲染背景
     * @return 如果渲染背景暗化返回true
     */
    [[nodiscard]] virtual bool shouldRenderBackground() const { return true; }

    /**
     * @brief 屏幕尺寸改变时调用
     * @param width 新宽度
     * @param height 新高度
     */
    virtual void onResize(i32 width, i32 height)
    {
        (void)width;
        (void)height;
    }

    /**
     * @brief 每帧更新
     * @param dt 增量时间
     */
    virtual void tick(f32 dt) { (void)dt; }
};

} // namespace mc
