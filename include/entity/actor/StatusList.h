#ifndef GODGAME_STATUSLIST_H
#define GODGAME_STATUSLIST_H

#include <memory>
#include <vector>

#include "entity/actor/Buff.h"

class Actor;

// buff 容器 + 生命周期调度。
//
// 唯一的铁律：DeferScope 作用域内 buffs_ 只能被读，所有增删都进队列。
// 状态推进和伤害结算都用它把表结构冻结住 —— 否则某个 buff 的回调里再增删
// 一个 buff，就会当场让遍历用的迭代器（或 ordered_ 里的裸指针）失效。
class StatusList {
public:
    class DeferScope {
    public:
        explicit DeferScope(StatusList& list) : list_(list), previous_(list.busy_) {
            list_.busy_ = true;
        }
        ~DeferScope() { list_.busy_ = previous_; }
        DeferScope(const DeferScope&) = delete;
        DeferScope& operator=(const DeferScope&) = delete;
    private:
        StatusList& list_;
        bool        previous_;
    };

    // 返回 false = 被状态免疫否决，或按叠层策略被忽略
    bool add(std::unique_ptr<Buff> buff, Actor& owner);

    // 净化/驱散。只是记下请求，owner 下次 update() 时才真正摘掉
    void remove(BuffId id);
    void clear();

    bool        has(BuffId id) const;
    int         stacks(BuffId id) const;
    const Buff* find(BuffId id) const;

    void update(float dt, Actor& owner);

    // 已按 priority 升序，伤害管线直接遍历
    const std::vector<Buff*>& ordered() const;

private:
    // false = 同类不存在，调用方需要把它当新 buff 入库
    bool addNow(std::unique_ptr<Buff>& buff, Actor& owner);
    void rebuildOrder() const;

    // 必须存指针：按值存会切片 —— 派生类的钩子全部退化成空实现，而且不报错。
    // 这是这套模式在 C++ 里最经典的一次翻车。
    std::vector<std::unique_ptr<Buff>> buffs_;

    std::vector<std::unique_ptr<Buff>> pendingAdded_;
    std::vector<BuffId>                pendingRemoved_;
    bool clearRequested_ = false;
    bool busy_           = false;

    mutable std::vector<Buff*> ordered_;
    mutable bool orderDirty_ = true;
};

#endif //GODGAME_STATUSLIST_H
