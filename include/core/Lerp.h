#ifndef GODGAME_LERP_H
#define GODGAME_LERP_H

#include <cmath>

// 线性插值：t = 0 给 a，t = 1 给 b。
//
// 为什么这对本项目不是"锦上添花"：逻辑固定 20 TPS，位置每秒只更新 20 次，
// 而画面每秒画 60 次。只按 tick 更新的东西如果直接画，中间那些帧就是同一份
// 数据重复画三遍 —— 冲刺时一 tick 跳 31 像素，看起来就是一格一格地闪。
// 所有"连续量"（位置、角度）都得靠它补成连续运动。
inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// 角度插值：走最短的那一边。
// 直接对 -179° 和 +179° 做线性插值会绕一大圈（穿过 0°），
// 而它们其实只差 2°。弧度制，和 RenderItem::angle 一致。
inline float lerpAngle(float a, float b, float t) {
    constexpr float kPi  = 3.14159265358979323846f;
    constexpr float kTau = 2.0f * kPi;

    float d = std::fmod(b - a, kTau);
    if (d >  kPi) d -= kTau;
    if (d < -kPi) d += kTau;

    return a + d * t;
}

#endif //GODGAME_LERP_H
