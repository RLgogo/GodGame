#ifndef GODGAME_ENTITY_H
#define GODGAME_ENTITY_H

#include <QPointF>
#include <QRect>

#include "core/EntityId.h"
#include "core/Kind.h"
#include "core/RenderItem.h"

class World;

class Entity {
public:
    Entity() = default;
    virtual ~Entity() = default;

    // dt 按值传：实体的 update 不该改到游戏循环的步长。
    // world 提供"世界上有谁/生成/造成伤害"的全部能力 ——
    // 实体不许自己去摸别的实体（那会变成悬垂指针和上帝对象 Game）。
    virtual void update(float dt, World& world) = 0;

    // 我是哪类东西。Game 用它维护分类索引，所以必须由每个实体自己说清楚
    virtual KindMask kinds() const = 0;

    // 渲染层要的数据。默认按 bounds() 画个方块，需要别的样子就重写。
    // 注意这里返回的是纯数据，不含 QPainter —— 逻辑层不依赖 Qt Gui
    virtual RenderItem renderItem() const;

    bool alive() const { return alive_; }
    void kill()       { alive_ = false; }   // 只做标记，真正的移除由 Game 统一做

    EntityId id() const { return id_; }
    QRect bounds() const { return rect_; }

    // 身体中心（浮点）。别的实体要"朝我走"时用它 ——
    // precisePos_ 是 protected，派生类没法读另一个对象的 protected 成员
    QPointF center() const {
        return precisePos_ + QPointF(rect_.width() * 0.5, rect_.height() * 0.5);
    }

protected:
    explicit Entity(const QPointF& pos, const QSize& size)
        : precisePos_(pos)
        , rect_(QRect(pos.toPoint(), size)) {}

    void syncRect() { rect_.moveTo(qRound(precisePos_.x()), qRound(precisePos_.y())); }

    // 出生就是活的。
    // （原来这里是 false：任何忘了在构造里赋值的派生类，第一帧就被清场逻辑抹掉，
    //   而且不报错、不崩，只是"这东西怎么不显示"。）
    bool    alive_ = true;
    QPointF precisePos_;
    QRect   rect_;

private:
    friend class Game;   // 只有 Game 有权发号并回填
    EntityId id_;

    // 这一条命结过账没有（分数/经验/掉落）。死亡可能被两个地方看见：
    // 伤害管线当场发现 killed，或者 sweep 事后发现躺着个死的。
    // 结算写在一处、用这个标志去重，谁先看到都行，但只算一次
    bool rewarded_ = false;
};

#endif //GODGAME_ENTITY_H
