#ifndef GODGAME_INPUT_H
#define GODGAME_INPUT_H

// 输入层 -> 逻辑层的一帧快照。
//
// 键鼠事件发生在渲染/UI 线程，逻辑跑在自己的线程上，所以两边不能共享一个
// "当前按着哪些键"的容器 —— 只能一方把状态打包投递给另一方。这个结构就是
// 那个包：纯值，拷一份投进去，逻辑线程每 tick 取一份出来读。
//
// 注意它是"瞬时状态"而不是"事件流"：逻辑层不需要知道"这一帧按下了 A"，
// 只需要知道"这一帧 A 是按着的"。按键事件、连按、repeat 都在 UI 层消化掉。
struct InputState {
    bool up      = false;
    bool down    = false;
    bool left    = false;
    bool right   = false;
    bool sprint  = false;   // Shift：加速，消耗体力

    bool fire    = false;   // 鼠标左键 / 空格：朝准星射击

    // 准星位置（窗口坐标）。逻辑层只拿它算方向，不自己转世界坐标 ——
    // 分辨率和世界尺寸一致时才等价，转坐标的活留给调用方
    float aimX = 0.0f;
    float aimY = 0.0f;
    bool  aimValid = false;
};

#endif //GODGAME_INPUT_H
