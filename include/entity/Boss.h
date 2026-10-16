#ifndef GODGAME_BOSS_H
#define GODGAME_BOSS_H

#include "core/RenderItem.h"
#include "entity/actor/Actor.h"

// Boss。
//
// 旧版它带着四个字段：bossWhiteHp / bossWhiteDelay / bossInvincible /
// bossInvincibleTimer，语义全靠 update 里的分支拼出来。现在：
//   - 无敌阶段 = InvincibleBuff（带时长的状态，本来就该是个 buff）
//   - 白条     = WhiteShieldBuff（伤害先记在它账上，冻结期过后按固定速度转成真血）
//   - 半血暴走 = 一次性的行为分支，由 enraged_ 记住
//
// 最下面那三个白条字段能删掉，就是这一层的意义：Boss 现在只管"我是 Boss、我会
// 追人、我半血变身"，"挨打之后账怎么记"完全不归它管 —— 换成护盾、换成免疫、
// 换成双层白条，都是换一个 buff 的事
class Boss : public Actor {
public:
    Boss(const QPointF& pos, float hp);

    KindMask kinds() const override {
        return kindBit(Kind::Boss) | kindBit(Kind::Damageable);
    }
    RenderItem renderItem() const override;

    bool enraged() const { return enraged_; }

protected:
    void tick(float dt, World& world) override;

private:
    void enterPhaseTwo(World& world);
    void summonAdds(World& world);
    void chase(float dt, World& world);
    void hitPlayerIfTouching(World& world);

    bool enraged_ = false;
};

#endif //GODGAME_BOSS_H
