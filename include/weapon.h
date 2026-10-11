#ifndef WEAPON_H
#define WEAPON_H

class Weapon {
private:

public:
    Weapon() = default;
    ~Weapon() = default;
    virtual void model();
    virtual void action();
};

class danqiang : public Weapon {


};


class Shuanqiang : public Weapon {
public: 
    Shuanqiang() = default;
};

#endif