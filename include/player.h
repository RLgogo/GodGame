#ifndef PLAYER_H
#define PLAYER_H

#include "weapon.h"

class Player {
private:
    Weapon* weapon;
    double life;
public:
    Player() = default;
    ~Player() = default;

    Player(double life, Weapon* w);

};


// 实际的 屏幕生成玩家 的类
Weapon w = Shuanqiang();
Player aPlayer = Player(200.00, &w);

#endif