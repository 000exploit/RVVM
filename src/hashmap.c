/*
hashmap.c - Open-addressing hashmap implementation
Copyright (C) 2021  LekKit <github.com/LekKit>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "hashmap.h"
#include "utils.h"
#include "bit_ops.h"
#include <string.h>

void hashmap_init(hashmap_t* map, size_t size)
{
    if (!size) size = 16;
    map->entries = 0;
    map->entry_balance = 0;
    map->size = bit_next_pow2(size) - 1;
    map->buckets = safe_new_arr(hashmap_bucket_t, map->size + 1);
}

void hashmap_destroy(hashmap_t* map)
{
    free(map->buckets);
    memset(map, 0, sizeof(hashmap_t));
}

void hashmap_resize(hashmap_t* map, size_t size)
{
    hashmap_t tmp;
    hashmap_init(&tmp, size);
    hashmap_foreach(map, k, v)
        hashmap_put(&tmp, k, v);
    free(map->buckets);
    map->buckets = tmp.buckets;
    map->size = tmp.size;
    map->entry_balance = map->entries;
}

void hashmap_grow(hashmap_t* map, size_t key, size_t val)
{
    hashmap_resize(map, (map->size + 1) << 1);
    hashmap_put(map, key, val);
}

static size_t hashmap_calc_shrink(hashmap_t* map)
{
    if (unlikely(map->entries && map->entry_balance > map->entries)) {
        return map->size / (map->entry_balance / map->entries);
    }
    return map->size;
}

void hashmap_shrink(hashmap_t* map)
{
    size_t size = hashmap_calc_shrink(map);
    if (unlikely(size < map->size)) {
        hashmap_resize(map, size);
    }
}

void hashmap_clear(hashmap_t* map)
{
    size_t size = bit_next_pow2(hashmap_calc_shrink(map)) - 1;
    if (size < map->size) {
        map->size = size;
        map->buckets = safe_realloc(map->buckets, (map->size + 1) * sizeof(hashmap_bucket_t));
        map->entry_balance = map->entries;
    }
    memset(map->buckets, 0, (map->size + 1) * sizeof(hashmap_bucket_t));
    map->entries = 0;
}

void hashmap_rebalance(hashmap_t* map, size_t index)
{
    size_t j = index, k;
    while (true) {
        map->buckets[index].val = 0;
        do {
            j = (j + 1) & map->size;
            if (!map->buckets[j].val) return;
            k = hashmap_hash(map->buckets[j].key) & map->size;
        } while ((index <= j) ? (index < k && k <= j) : (index < k || k <= j));
        map->buckets[index] = map->buckets[j];
        index = j;
    }
}
