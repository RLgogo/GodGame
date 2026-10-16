#ifndef GODGAME_DAMAGE_H
#define GODGAME_DAMAGE_H

#include <QPointF>
#include <cstdint>

#include "core/EntityId.h"

enum class DamageType : std::uint32_t {
    Bullet = 0,
    Explosion,
    Contact,
    Fire,
    Frost,
    Poison,
    True,       // 真伤：跳过多重修正和护盾吸收，但免疫依然挡得住
    Count_
};

using DamageMask = std::uint32_t;

constexpr DamageMask damageBit(DamageType t) {
    return 1u << static_cast<std::uint32_t>(t);
}

// 一次伤害请求。
//
// 注意 multiplier / flatReduce 这两个字段存在的意义：让 buff 不要去直接改 amount。
// 如果 5 个 buff 都去乘 amount，结果就依赖它们的处理顺序；改成"只改 multiplier，
// 最后统一乘一次"，乘法可交换，顺序再也无所谓。
struct DamageInfo {
    float      amount     = 0.0f;
    float      multiplier = 1.0f;   // 减伤/易伤只乘这个
    float      flatReduce = 0.0f;   // 护甲那种固定减伤，最后减
    DamageMask types      = 0;
    EntityId   source;              // 句柄，不是裸指针
    QPointF    hitPos;
    QPointF    knockDir;            // 零向量 = 不击退
    float      knockForce = 0.0f;
    bool       cancelled  = false;  // 免疫：整条作废
};

struct DamageResult {
    float applied   = 0.0f;   // 真扣掉的血
    float absorbed  = 0.0f;   // 被护盾/白条吃掉的
    bool  cancelled = false;  // 被免疫
    bool  killed    = false;
};

#endif //GODGAME_DAMAGE_H
