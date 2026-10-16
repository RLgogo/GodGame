#ifndef GODGAME_KIND_H
#define GODGAME_KIND_H

#include <cstdint>

// 实体类型标签：位掩码。
//
// Game 会按位维护一份分类索引，于是"遍历所有敌人"是 O(命中数)，
// 既不用 dynamic_cast，也不用往基类上继续堆"isEnemy()"这种虚函数。
enum class Kind : std::uint32_t {
    Player       = 1u << 0,
    Enemy        = 1u << 1,
    PlayerBullet = 1u << 2,
    EnemyBullet  = 1u << 3,
    Pickup       = 1u << 4,
    Boss         = 1u << 5,
    Damageable   = 1u << 6,
};

using KindMask = std::uint32_t;

// 枚举值本身就是位，这里只是给调用处一个好读的名字
constexpr KindMask kindBit(Kind k) { return static_cast<KindMask>(k); }

constexpr int kKindSlots = 32;  // 分类索引按位号开槽

#endif //GODGAME_KIND_H
