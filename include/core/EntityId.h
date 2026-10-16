#ifndef GODGAME_ENTITYID_H
#define GODGAME_ENTITYID_H

#include <cstddef>
#include <cstdint>

// 实体句柄。
//
// 为什么不用 Entity*：目标死了、unique_ptr 析构了，裸指针就悬垂。
// 句柄里的 gen（世代号）在槽位被回收时会 +1，于是老句柄自动失配，
// World::get() 替你返回 nullptr —— 一类 bug 直接从类型层面消掉。
struct EntityId {
    std::uint32_t index = 0;
    std::uint32_t gen   = 0;

    bool valid() const { return gen != 0; }
};

inline bool operator==(EntityId a, EntityId b) {
    return a.index == b.index && a.gen == b.gen;
}
inline bool operator!=(EntityId a, EntityId b) { return !(a == b); }

struct EntityIdHash {
    std::size_t operator()(EntityId id) const {
        return (static_cast<std::size_t>(id.index) << 32) ^ id.gen;
    }
};

#endif //GODGAME_ENTITYID_H
