#ifndef GODGAME_ENEMY_H
#define GODGAME_ENEMY_H

#include "core/RenderItem.h"
#include "entity/actor/Actor.h"

// 普通敌人。
//
// 旧版它是一堆字段（isDasher / dashTimer / respawnTimer / speed）摊在一个
// Enemy 结构体里，行为写在 GameWidget 的 updategame() 里；现在行为就在它自己
// 身上，Game 只负责"什么时候造它"。
//
// 所谓的 respawnTimer 其实只有两个值：0 = 刚生成，-1 = 已阵亡。那个 >0 的
// 复活分支是死代码（死敌人不会被清理，一直挂到回合结束 enemies.clear()，
// 于是单回合能累积 150*round 个死对象）。这里直接用 alive_ 表示生死。
class Enemy : public Actor {
public:
    Enemy(const QPointF& pos, float speed, bool dasher, float hp);

    KindMask kinds() const override {
        return kindBit(Kind::Enemy) | kindBit(Kind::Damageable);
    }
    RenderItem renderItem() const override;

    bool  isDasher() const { return dasher_; }
    float speed() const    { return speed_; }

protected:
    void tick(float dt, World& world) override;

private:
    void chase(float dt, World& world);
    void blinkIfReady(float dt, World& world);
    void hitPlayerIfTouching(World& world);

    // 外观档位：旧版按速度分三档红，越快越亮
    std::uint8_t speedTier() const;

    float speed_      = 125.0f;
    bool  dasher_     = false;
    float blinkTimer_ = 4.8f;
};

#endif //GODGAME_ENEMY_H
