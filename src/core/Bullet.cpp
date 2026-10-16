#include "core/Bullet.h"

#include "core/Lerp.h"

void BulletField::update(float dt, float worldW, float worldH) {
    // 倒着遍历：killAt 是 swap-and-pop，会把末尾那颗换到当前位置。
    // 倒序时末尾那颗本帧已经处理过了，换过来不再处理正好是对的
    for (std::size_t i = bullets_.size(); i-- > 0;) {
        Bullet& b = bullets_[i];

        b.px = b.x;             // 先记下起点，供渲染插值
        b.py = b.y;

        b.x += b.vx * dt;
        b.y += b.vy * dt;
        b.life -= dt;

        // 出界判定带上半径：让子弹整个离开画面再销毁，
        // 否则会在边缘"闪一下就不见"
        const bool out = b.x + b.radius < 0.0f || b.x - b.radius > worldW ||
                         b.y + b.radius < 0.0f || b.y - b.radius > worldH;

        if (out || b.life <= 0.0f) killAt(i);
    }
}

void BulletField::killAt(std::size_t i) {
    if (i >= bullets_.size()) return;

    if (i + 1 != bullets_.size()) bullets_[i] = bullets_.back();
    bullets_.pop_back();
}

void BulletField::appendItems(std::vector<RenderItem>& out, float alpha) const {
    out.reserve(out.size() + bullets_.size());

    for (const Bullet& b : bullets_) {
        RenderItem item;
        item.kinds = b.fromPlayer ? kindBit(Kind::PlayerBullet) : kindBit(Kind::EnemyBullet);
        item.style = b.fromPlayer ? RenderStyle::PlayerBullet : RenderStyle::EnemyBullet;
        item.shape = RenderShape::Circle;
        item.x     = lerp(b.px, b.x, alpha);
        item.y     = lerp(b.py, b.y, alpha);
        item.w     = b.radius * 2.0f;
        item.h     = b.radius * 2.0f;
        out.push_back(item);
    }
}
