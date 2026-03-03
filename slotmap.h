/*
    This is a sigle-header library implementing type-generic
    slotmap data structute.



TABLE OF CONTENTS

    - Options
    - Documentation



OPTIONS

    #define VNL_SLOTMAP_IMPLEMENTATION

        Set this flag to include implementation for internal functions

    #define VNL_SLOTMAP_UNSAFE_KEYS

        This flag removes key validations like index bounds check and key version check.
        It also affects definition for the slots and keys, since key/slot version is not needed.

    #define VNL_SLOTMAP_PANIC(file, line, exc, fmt, ...) better_panic

        Redefine to provide alternative panic action. Default version prints source location,
        exception message and exits via abort()

    #define VNL_SLOTMAP_REALLOC(ptr, size) better_realloc
    #define VNL_SLOTMAP_FREE(ptr, size) better_free

        By default slotmap uses stdlib realloc/free. If you want to redefine one of these,
        you must redefine the other macro too.

    #define VNL_SLOTMAP_STRIP_PREFIX

        This flag removes `vnl_` prefix from funciton



DOCUMENTATION
    Slotmap functions (actually macros):

    slotmap_t(T)
        Define a slotmap structure with elemtns of type T.

    slotmap_free(sm)
        void slotmap_free(slotmap_t *sm);
            Free the memory held by the slotmap structure. After that,
            slotmap can be used again.

    slotmap_insert(sm, value)
        slotmap_key_t slotmap_insert(slotmap_t *sm, T value);
            Insert value into the slotmap. Returns the key
            to be able to access this element.

    slotmap_remove(sm, key)
        T slotmap_remove(slotmap_t *sm, slotmap_key_t key);
            Remove and return value under the key.

    slotmap_get(sm, key)
        T slotmap_get(slotmap_t *sm, slotmap_key_t key);
            Get value associated with the key.

    slotmap_contains(sm, key)
        bool slotmap_contains(slotmap_t *sm, slotmap_key_t key);
            Check if key is present in the slotmap. Note that with
            VNL_SLOTMAP_UNSAFE_KEYS option defined this function
            will return true if the slot is accupied, regardless of
            the value stored in.

    slotmap_reserve(sm, nitems)
        void slotmap_reserve(slotmap_t *sm, size_t nitems);
            Reserve space for the nitems additional values in the slotmap.


*/


#ifndef VNL_SLOTMAP_H
#define VNL_SLOTMAP_H



#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>


typedef uint32_t vnl_slotmap_index_t;

#ifdef VNL_SLOTMAP_UNSAFE_KEYS

typedef struct { vnl_slotmap_index_t index; } vnl_slotmap_key_t;
typedef struct { vnl_slotmap_index_t index; } vnl_slotmap_slot_t;

#else // !VNL_SLOTMAP_UNSAFE_KEYS

typedef struct { vnl_slotmap_index_t index; uint32_t version; } vnl_slotmap_key_t;
typedef struct { vnl_slotmap_index_t index; uint32_t version; } vnl_slotmap_slot_t;

#endif // VNL_SLOTMAP_UNSAFE_KEYS



#ifndef VNL_SLOTMAP_PANIC

#include <stdio.h>
#define VNL_SLOTMAP_PANIC(file, line, exc, fmt, ...)                        \
    do {                                                                    \
        fprintf(stderr, "%s:%d: %s: " fmt, file, line, exc, __VA_ARGS__);   \
        abort();                                                            \
    } while(0)

#endif // VNL_SLOTMAP_PANIC



#if !defined(VNL_SLOTMAP_REALLOC) && !defined (VNL_SLOTMAP_FREE)

#include <stdlib.h>
#define VNL_SLOTMAP_REALLOC(ptr, size) realloc((ptr), (size))
#define VNL_SLOTMAP_FREE(ptr, size) (free((ptr)), (void)(size))

#elif !defined(VNL_SLOTMAP_REALLOC) || !defined (VNL_SLOTMAP_FREE)

#error "Partial redefinition of VNL_SLOTMAP_REALLOC / VNL_SLOTMAP_FREE is not supported."

#endif



#define vnl_slotmap_t(T)            \
    struct {                        \
        vnl_slotmap_slot_t *slots;  \
        uint32_t length;            \
        uint32_t capacity;          \
        T *items;                   \
    }


#define vnl_slotmap_impl_itemsize(sm) sizeof((sm)->items[0])

#define vnl_slotmap_free(sm) \
    vnl_slotmap_impl_free((sm), vnl_slotmap_impl_itemsize((sm)))

#define vnl_slotmap_insert(sm, value) \
    (                                                                           \
        vnl_slotmap_impl_maybegrow((sm), vnl_slotmap_impl_itemsize((sm)), 1),   \
        (sm)->items[vnl_slotmap_impl_get_insert_index((sm))] = (value),         \
        (sm)->length++,                                                         \
        vnl_slotmap_impl_get_insert_key((sm))                                   \
    )

#define vnl_slotmap_remove(sm, key) \
    (                                                                   \
        vnl_slotmap_impl_check_key((sm), (key), __FILE__, __LINE__),    \
        vnl_slotmap_impl_remove(                                        \
            (sm),                                                       \
            (key),                                                      \
            vnl_slotmap_impl_itemsize((sm))                             \
        ),                                                              \
        (sm)->items[(sm)->length]                                       \
    )

#define vnl_slotmap_get(sm, key) \
    (                                                                   \
        vnl_slotmap_impl_check_key((sm), (key), __FILE__, __LINE__),    \
        (sm)->items[vnl_slotmap_impl_follow_index((sm), (key).index)]   \
    )

#define vnl_slotmap_contains(sm, key) \
    vnl_slotmap_impl_contains_key((sm), (key))

#define vnl_slotmap_reserve(sm, nitems) \
    vnl_slotmap_impl_maybegrow((sm), vnl_slotmap_impl_itemsize((sm)), (nitems))



inline void vnl_slotmap_impl_free(void *sm, size_t itemsize);
inline void vnl_slotmap_impl_maybegrow(void *sm, size_t itemsize, size_t nitems);
inline vnl_slotmap_index_t vnl_slotmap_impl_follow_index(void *sm, vnl_slotmap_index_t index);
inline vnl_slotmap_index_t vnl_slotmap_impl_get_insert_index(void *sm);
inline vnl_slotmap_key_t vnl_slotmap_impl_get_insert_key(void *sm);
inline void vnl_slotmap_impl_check_key(void *sm, vnl_slotmap_key_t key, const char *file, int line);
inline bool vnl_slotmap_impl_contains_key(void *sm, vnl_slotmap_key_t key);
inline void vnl_slotmap_impl_remove(void *sm, vnl_slotmap_key_t key, size_t itemsize);




#ifdef VNL_SLOTMAP_STRIP_PREFIX

typedef vnl_slotmap_index_t slotmap_index_t;
typedef vnl_slotmap_key_t slotmap_key_t;
typedef vnl_slotmap_slot_t slotmap_slot_t;

#define slotmap_t(T)                vnl_slotmap_t(T)
#define slotmap_free(sm)            vnl_slotmap_free((sm))
#define slotmap_insert(sm, value)   vnl_slotmap_insert((sm), (value))
#define slotmap_remove(sm, key)     vnl_slotmap_remove((sm), (key))
#define slotmap_get(sm, key)        vnl_slotmap_get((sm), (key))
#define slotmap_contains(sm, key)   vnl_slotmap_contains((sm), (key))
#define slotmap_reserve(sm, nitems) vnl_slotmap_reserve((sm), (nitems))

#endif // VNL_SLOTMAP_STRIP_PREFIX


#endif // VNL_SLOTMAP_H



#ifdef VNL_SLOTMAP_IMPLEMENTATION

void vnl_slotmap_impl_free(void *any_sm, size_t itemsize) {
    vnl_slotmap_t(void) *sm = any_sm;
    size_t slotsize = sizeof(sm->slots[0]);
    VNL_SLOTMAP_FREE(sm->slots, sm->capacity * slotsize);
    VNL_SLOTMAP_FREE(sm->items, sm->capacity * itemsize);
    sm->length = sm->length = 0;
    sm->items = sm->slots = NULL;
}

void vnl_slotmap_impl_maybegrow(void *any_sm, size_t itemsize, size_t nitems) {
    vnl_slotmap_t(void) *sm = any_sm;

    const size_t CAP_ALIGN = 8;

    size_t min_cap = sm->length + nitems;
    if (min_cap < 8) {
        min_cap = 8;
    }

    if (min_cap <= sm->capacity) return;

    if (min_cap <= sm->capacity * 2) {
        sm->capacity = sm->capacity * 2;
    } else {
        // align to multiple of CAP_ALIGN
        sm->capacity = (min_cap + (CAP_ALIGN - 1)) & ~(CAP_ALIGN - 1);
    }

    sm->slots = VNL_SLOTMAP_REALLOC(sm->slots, sm->capacity * itemsize);
    sm->items = VNL_SLOTMAP_REALLOC(sm->items, sm->capacity * itemsize);
    // Fill new slots
    for (vnl_slotmap_index_t i = sm->length; i < sm->capacity; ++i) {
        sm->slots[i] = (vnl_slotmap_slot_t){ .index = i };
    }

}


vnl_slotmap_index_t vnl_slotmap_impl_follow_index(void *any_sm, vnl_slotmap_index_t index) {
    vnl_slotmap_t(void) *sm = any_sm;
    return sm->slots[index].index;
}

vnl_slotmap_index_t vnl_slotmap_impl_get_insert_index(void *any_sm) {
    vnl_slotmap_t(void) *sm = any_sm;
    vnl_slotmap_index_t insert_item_index = sm->length;
    vnl_slotmap_index_t insert_slot_index = vnl_slotmap_impl_follow_index(sm, insert_item_index);
                        insert_item_index = vnl_slotmap_impl_follow_index(sm, insert_slot_index);
    return insert_item_index;
}

vnl_slotmap_key_t vnl_slotmap_impl_get_insert_key(void *any_sm) {
    vnl_slotmap_t(void) *sm = any_sm;
    vnl_slotmap_index_t insert_item_index = sm->length - 1;
    vnl_slotmap_index_t insert_slot_index = vnl_slotmap_impl_follow_index(sm, insert_item_index);

#ifndef VNL_SLOTMAP_UNSAFE_KEYS

    vnl_slotmap_key_t key = {
        insert_slot_index,
        ++sm->slots[insert_slot_index].version
    };

#else

    vnl_slotmap_key_t key = {
        insert_slot_index
    };

#endif

    return key;
}

void vnl_slotmap_impl_check_key(void *any_sm, vnl_slotmap_key_t key, const char *file, int line) {
#ifndef VNL_SLOTMAP_UNSAFE_KEYS

    vnl_slotmap_t(void) *sm = any_sm;

    if (key.index >= sm->capacity)
        VNL_SLOTMAP_PANIC(file, line, "Slotmap.KeyOutOfBounds",
            "Key index cannot exceed slotmap capacity: key=(%u, %u), capacity=%u\n",
            key.index, key.version,
            sm->capacity
        );

    vnl_slotmap_slot_t slot = sm->slots[key.index];

    if (slot.version > key.version)
        VNL_SLOTMAP_PANIC(file, line, "Slotmap.ExpiredKey",
            "Key is expired: key=(%u, %u), slot=(%u, %u)\n",
            key.index, key.version,
            key.index, slot.version
        );

    if (slot.version < key.version)
        VNL_SLOTMAP_PANIC(file, line, "Slotmap.YoungKey",
            "Key is too young: key=(%u, %u), slot=(%u, %u)\n",
            key.index, key.version,
            key.index, slot.version
        );

#else

    (void)any_sm;
    (void)key;
    (void)file;
    (void)line;

#endif // VNL_SLOTMAP_UNSAFE_KEYS
}


bool vnl_slotmap_impl_contains_key(void *any_sm, vnl_slotmap_key_t key) {
    vnl_slotmap_t(void) *sm = any_sm;

#ifdef VNL_SLOTMAP_UNSAFE_KEYS

    return key.index < sm->capacity
        && vnl_slotmap_impl_follow_index(sm, key.index) < sm->length;

#else // VNL_SLOTMAP_UNSAFE_KEYS

    return key.index < sm->capacity
        && sm->slots[key.index].version == key.version
        && vnl_slotmap_impl_follow_index(sm, key.index) < sm->length;

#endif // VNL_SLOTMAP_UNSAFE_KEYS
}

void vnl_slotmap_impl_remove(void *any_sm, vnl_slotmap_key_t key, size_t itemsize) {
    vnl_slotmap_t(char) *sm = any_sm;

    vnl_slotmap_index_t item_idx = vnl_slotmap_impl_follow_index(sm, key.index);
    sm->length--;

    void *rem = &sm->items[item_idx * itemsize];
    void *end = &sm->items[sm->length * itemsize];
    void *tmp = alloca(itemsize);
    memmove(tmp, end, itemsize);
    memmove(end, rem, itemsize);
    memmove(rem, tmp, itemsize);

    vnl_slotmap_slot_t tmpslot = sm->slots[item_idx];
    sm->slots[item_idx].index = sm->length;

#ifndef VNL_SLOTMAP_UNSAFE_KEYS
    sm->slots[item_idx].version++;
#endif // VNL_SLOTMAP_UNSAFE_KEYS

    sm->slots[sm->length] = tmpslot;
}

#endif // VNL_SLOTMAP_IMPLEMENTATION
