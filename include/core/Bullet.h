#ifndef GODGAME_BULLET_H
#define GODGAME_BULLET_H

#include <cstddef>
#include <vector>

#include "core/Damage.h"
#include "core/EntityId.h"
#include "core/Kind.h"
#include "core/RenderItem.h"

// 子弹不进实体表，这里是纯 POD 数组。
//
// 理由：子弹是"海量、同质、行为完全一致"的东西 —— 每颗子弹的一生就是
// "沿速度走、撞到东西、消失"，没有任何一颗需要自己的 buff、自己的状态、
// 自己的生命周期回调。给它一个 vtable + 一次堆分配 + 一个槽位 + 一份世代号，
// 全是白付的开销；而 200 颗子弹各朝 200 个方向散开时，那一堆指针追逐才是
// 真正的成本。
//
// 于是：一个连续数组，顺序遍历（对缓存友好），删除用 swap-and-pop（不用
// 整体搬移），容量只长到历史峰值为止 —— push_back 到峰值后就不再分配，
// 这本身就是最朴素的对象池。
//
// 代价（写在这里免得以后忘）：子弹不能挂 buff、不能有自己的 update 行为。
// 哪颗子弹要变成"会追踪、会分裂、会留火"的东西，它就该升级成实体。
struct Bullet {
    float      x      = 0.0f;                 // 圆心
    float      y      = 0.0f;
    float      vx     = 0.0f;                 // 像素 / 秒
    float      vy     = 0.0f;
    float      radius = 4.0f;
    float      damage = 10.0f;
    float      life   = 2.0f;                 // 剩余寿命（秒）
    DamageType type   = DamageType::Bullet;
    bool       fromPlayer = true;             // 决定它打谁
    EntityId   owner;                         // 打出去的人（避免自伤 / 记击杀归属）
};

class BulletField {
public:
    void        spawn(const Bullet& bullet) { bullets_.push_back(bullet); }
    void        clear() { bullets_.clear(); }

    std::size_t size() const { return bullets_.size(); }
    bool        empty() const { return bullets_.empty(); }

    // 走一步：位移 + 掉寿命；出界或过期的 swap-and-pop 掉
    void update(float dt, float worldW, float worldH);

    // 命中判定要遍历，所以直接开给调用方；杀掉某一颗用 killAt
    std::vector<Bullet>&       all()       { return bullets_; }
    const std::vector<Bullet>& all() const { return bullets_; }

    // swap-and-pop：把最后一颗换到 i 再 pop。顺序会变，子弹之间没有顺序语义
    void killAt(std::size_t i);

    // 把这一帧的子弹塞进快照（子弹不是实体，得自己来）
    void appendItems(std::vector<RenderItem>& out) const;

private:
    std::vector<Bullet> bullets_;
};

#endif //GODGAME_BULLET_H
