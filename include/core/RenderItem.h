#ifndef GODGAME_RENDERITEM_H
#define GODGAME_RENDERITEM_H

#include <cstdint>

#include "core/EntityId.h"
#include "core/Kind.h"

// 逻辑层 -> 渲染层的唯一通道。
//
// 逻辑线程每帧把世界"拍扁"成一份这样的 POD 数组发布出去，渲染线程读它。
// 两边不再共享任何对象，于是：
//   1) 不存在"逻辑线程在 update、渲染线程在 draw 同一个对象"的数据竞争；
//   2) Entity 上不需要 draw(QPainter&)，逻辑层彻底不依赖 Qt Gui，能无窗口跑测试。
enum class RenderShape : std::uint8_t {
    Rect = 0,
    Circle,
};

// 外观类别。逻辑只说"我是个啥、长成哪一档"，具体配色和画法全归渲染层 ——
// 这样换一套配色不用碰任何一条玩法逻辑。
enum class RenderStyle : std::uint8_t {
    Generic = 0,
    Player,
    Enemy,
    EnemyDasher,
    Boss,
    PlayerBullet,
    EnemyBullet,
    AmmoBox,
    HealthPack,
};

struct RenderItem {
    EntityId    id;
    KindMask    kinds    = 0;
    RenderShape shape    = RenderShape::Rect;
    RenderStyle style    = RenderStyle::Generic;
    std::uint8_t variant = 0;   // 同类里的档位（敌人按速度分档 0/1/2）
    float       angle    = 0.0f; // 朝向角（弧度），玩家的枪管要用

    float x = 0.0f;         // 左上角（Rect）/ 圆心（Circle）
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    float facing  = 1.0f;   // 朝向 x 分量：-1 / 1
    float hpRatio = 1.0f;   // 血条。<= 0 或 >= 1 时渲染层可以不画
    float gauge   = 0.0f;   // 白条/护盾长度，来自 Buff::gauge()
    float pending = 0.0f;   // 待结算伤害占最大血量的比例（白条那一截），来自 WhiteShieldBuff
    bool  flash   = false;  // 无敌帧闪烁：读的是 buff 状态，不是另存一个计时器
};

#endif //GODGAME_RENDERITEM_H
