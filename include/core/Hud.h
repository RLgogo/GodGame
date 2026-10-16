#ifndef GODGAME_HUD_H
#define GODGAME_HUD_H

#include <cstdint>

// 逻辑线程 -> 渲染层的第二块数据（第一块是 RenderItem 数组）。
//
// HUD 的每个数字都必须由逻辑说了算。渲染层不许自己算血量百分比、自己数还剩
// 几个敌人、自己判"是不是该结束了" —— 一旦两边各算一遍，迟早算出不一样的
// 结果，而且那种 bug 只在某一帧闪一下，最难查。
enum class GameStatus : std::uint8_t {
    Playing = 0,
    Paused,
    GameOver,
};

struct HudState {
    GameStatus status = GameStatus::Playing;

    int   hp         = 0;
    int   maxHp      = 0;
    float hpPending  = 0.0f;     // 待结算伤害占最大血量的比例：血条上那一截白条
    int   ammo       = 0;
    int   maxAmmo    = 0;
    float stamina    = 0.0f;
    float maxStamina = 0.0f;
    bool  exhausted  = false;    // 体力见底：跑不动了，画出提示

    int   level        = 1;
    int   score        = 0;
    int   exp          = 0;      // 当前等级已攒的经验
    int   expNext      = 0;      // 升到下一级还差多少
    float expRatio     = 0.0f;
    int   levelUpFlash = 0;      // > 0 时画升级提示（逻辑侧递减）

    int   round       = 1;
    int   enemiesLeft = 0;
    int   roundWait   = 0;       // > 0 时画"本回合清空，准备下一波"

    bool  bossActive     = false;
    int   bossWarning    = 0;    // > 0 时画 Boss 警告（逻辑侧递减）
    int   bossHp         = 0;
    int   bossMaxHp      = 0;
    float bossHpRatio    = 1.0f;
    float bossPending    = 0.0f;   // 同上，Boss 那条血条上的白条

    double elapsed = 0.0;        // 存活时间（秒），结算画面用
};

#endif //GODGAME_HUD_H
