#ifndef GODGAME_PLAYER_H
#define GODGAME_PLAYER_H

#include "core/RenderItem.h"
#include "entity/actor/Actor.h"

// 升级奖励的那一包东西。让 Player 不必知道 Config 里都有哪些字段 ——
// 算账的是 Game（这一局的规则），套用的是 Player（它自己的身体）
struct LevelUpReward {
    float healAmount        = 0.0f;
    int   ammoBonus         = 0;
    float maxStaminaBonus   = 0.0f;
    float staminaRegenDelta = 0.0f;
    float staminaDrainDelta = 0.0f;
    int   maxAmmoBonus      = 0;
};

// 玩家。
// 它自己负责：读输入、走路、管体力和弹药、开枪、挨打之后进无敌帧。
// 它不负责：分数、等级、经验、回合 —— 那些是"这一局"的账，归 Game。
class Player : public Actor {
public:
    explicit Player(const QPointF& pos = QPointF(0.0, 0.0));

    KindMask kinds() const override {
        return kindBit(Kind::Player) | kindBit(Kind::Damageable);
    }
    RenderItem renderItem() const override;

    int  ammo() const    { return ammo_; }
    int  maxAmmo() const { return maxAmmo_; }
    void addAmmo(int amount);          // 拾取和升级都走这里（会 clamp 到上限）

    float stamina() const    { return stamina_; }
    float maxStamina() const { return maxStamina_; }
    bool  exhausted() const  { return exhausted_; }

    void applyLevelUp(const LevelUpReward& reward);

    QPointF aimDirection() const { return aimDir_; }

protected:
    void tick(float dt, World& world) override;
    void onDamaged(const DamageResult& result, const DamageInfo& damage) override;

private:
    void updateAim(World& world);
    void updateStamina(float dt, World& world);
    void move(float dt, World& world);
    void shoot(float dt, World& world);

    int     ammo_         = 60;
    int     maxAmmo_      = 360;
    float   stamina_      = 100.0f;
    float   maxStamina_   = 100.0f;
    float   staminaRegen_ = 25.0f;
    float   staminaDrain_ = 50.0f;
    float   fireTimer_    = 0.0f;   // 距离下次能开火还剩几秒
    bool    exhausted_    = false;

    QPointF velocity_;
    QPointF aimDir_{1.0, 0.0};
    float   angle_ = 0.0f;
};

#endif //GODGAME_PLAYER_H
