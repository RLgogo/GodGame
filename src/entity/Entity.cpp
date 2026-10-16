#include "entity/Entity.h"

#include "core/Kind.h"

// Kind 的值本身就是位，分类索引按位号开槽 —— 最高位不能超过槽位数
// （用 1ull：kKindSlots 就是 32，1u << 32 本身就是未定义行为，拿它做上界反而写不出断言）
static_assert(static_cast<std::uint64_t>(Kind::Damageable) < (1ull << kKindSlots),
              "Kind 的位数超出了 kKindSlots");

RenderItem Entity::renderItem() const {
    RenderItem item;
    // 这两行是渲染层认人的依据，必须在基类就填好：
    // 派生类只该覆盖"长什么样"，不该负责"我是谁"
    item.id     = id_;       // 槽位会被复用，所以句柄里带世代号
    item.kinds  = kinds();
    item.shape = RenderShape::Rect;
    item.x = static_cast<float>(precisePos_.x());
    item.y = static_cast<float>(precisePos_.y());
    item.w = static_cast<float>(rect_.width());
    item.h = static_cast<float>(rect_.height());
    return item;
}
