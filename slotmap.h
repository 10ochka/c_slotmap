
#ifndef SLOTMAP_H
#define SLOTMAP_H


#include <stddef.h>
#include <stdint.h>
#include <string.h>



#ifndef slotmap_panic
#   include <stdio.h>
#   define slotmap_panic(exc, fmt, ...) (fprintf(stderr, "\033[31m%s:\033[0m " fmt, exc __VA_OPT__(,) __VA_ARGS__), abort())
#endif // slotmap_panic

#ifndef slotmap_realloc
#   include <stdlib.h>
#   define slotmap_realloc(ptr, itemsize, nitems) realloc((ptr), itemsize * (nitems))
#endif // slotmap_realloc

#ifndef slotmap_free
#   include <stdlib.h>
#   define slotmap_free(ptr) free((ptr))
#endif // slotmap_free

#ifndef SLOTMAP_INITIAL_CAPACITY
#   define SLOTMAP_INITIAL_CAPACITY 8
#endif // SLOTMAP_INITIAL_CAPACITY



typedef uint32_t slotmap_index_t;

typedef struct {
    uint32_t index;
    uint32_t version;
} slotmap_key_t;

typedef struct {
    uint32_t index;
    uint32_t version;
} slotmap_slot_t;

#define slotmap_t(T) struct { \
    slotmap_slot_t *slots;  \
    uint32_t length;        \
    uint32_t capacity;      \
    T *items;              \
}



static inline slotmap_index_t slotmap_impl_follow(void *any_sm, slotmap_index_t idx) {
    slotmap_t(void) *sm = any_sm;
    return sm->slots[idx].index;
}

static inline slotmap_index_t slotmap_impl_get_true_idx(void *any_sm, slotmap_index_t expected_idx) {
    slotmap_index_t slot_idx = slotmap_impl_follow(any_sm, expected_idx);
    slotmap_index_t item_idx = slotmap_impl_follow(any_sm, slot_idx);
    return item_idx;
}

static inline slotmap_key_t slotmap_impl_create_key(void *any_sm, slotmap_index_t expected_inserted_idx) {
    slotmap_t(void) *sm = any_sm;
    slotmap_index_t slot_idx = slotmap_impl_follow(sm, expected_inserted_idx);
    uint32_t version = ++(sm)->slots[slot_idx].version;
    return (slotmap_key_t){ slot_idx, version};
}

static inline void slotmap_impl_grow_if_nessesary(void *any_sm, size_t itemsize) {
    slotmap_t(void) *sm = any_sm;

    if (sm->capacity > sm->length) return;

    sm->capacity = sm->capacity
        ? (sm)->capacity * 2
        : SLOTMAP_INITIAL_CAPACITY;

    (sm)->slots = slotmap_realloc((sm)->slots, sizeof(slotmap_slot_t), (sm)->capacity);
    (sm)->items = slotmap_realloc((sm)->items, itemsize, (sm)->capacity);

    for (uint32_t i = sm->length; i < sm->capacity; ++i) {
        sm->slots[i] = (slotmap_slot_t){ i, 0 };
    }
}

static inline void slotmap_impl_clear(void *any_sm) {
    slotmap_t(void) *sm = any_sm;
    slotmap_free(sm->slots);
    slotmap_free(sm->items);
    sm->slots = sm->items = NULL;
    sm->length = sm->capacity = 0;
}

static inline void slotmap_impl_check_key(void *any_sm, slotmap_key_t key) {
    slotmap_t(void) *sm = any_sm;

    if (key.index >= sm->capacity) {
        slotmap_panic("Slotmap.ImpossibleKey",
            "Key index out of bounds. Key: (idx: %u, ver: %u), capacity: %u",
            key.index, key.version,
            sm->capacity
        );
    }

    slotmap_index_t item_idx = slotmap_impl_follow(sm, key.index);

    uint32_t version = sm->slots[item_idx].version;

    if (version != key.version) {
        slotmap_panic("Slotmap.ExpiredKey",
              "Cannot delete key (idx: %u, ver: %u), expected version: %u\n",
              (key).index, (key).version,
              version
        );
    }
}

static inline slotmap_index_t slotmap_impl_remove(void *any_sm, size_t itemsize, slotmap_key_t key) {
    slotmap_t(char) *sm = any_sm;
    slotmap_impl_check_key((sm), (key));

    slotmap_index_t item_idx = slotmap_impl_follow(sm, key.index);
    sm->length--;

    void *rem = &sm->items[item_idx * itemsize];
    void *end = &sm->items[sm->length * itemsize];
    void *tmp = alloca(itemsize);
    memmove(tmp, rem, itemsize);
    memmove(rem, end, itemsize);
    memmove(end, rem, itemsize);

    slotmap_slot_t tmpslot = sm->slots[item_idx];
    sm->slots[item_idx].index = sm->length;
    sm->slots[item_idx].version++;
    sm->slots[sm->length] = tmpslot;

    return sm->length;
}



#define slotmap_insert(sm, value) (                                         \
    slotmap_impl_grow_if_nessesary((sm), sizeof((sm)->items[0])),           \
    (sm)->items[slotmap_impl_get_true_idx((sm), (sm)->length)] = (value),   \
    slotmap_impl_create_key((sm), (sm)->length++)                           \
)

#define slotmap_remove(sm, key) (sm)->items[slotmap_impl_remove((sm), sizeof((sm)->items[0]), key)]

#define slotmap_at(sm, key) (                           \
    slotmap_impl_check_key((sm), (key)),                \
    (sm)->items[slotmap_impl_follow((sm), (key).index)] \
)

#define slotmap_clear(sm) slotmap_impl_clear((sm))




#endif // SLOTMAP_H
