/**
 * @brief Flexible mapping container.
 * @author K. Lange
 *
 * @copyright
 * This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2013-2021 K. Lange
 */


#include <libk_reduced/string.h>
#include <kernel/list.h>
#include <kernel/hashmap.h>

extern void kfree(void *);



unsigned int hashmap_string_hash(const void * _key) {
	unsigned int hash = 0;
	char * key = (char *)_key;
	int c;
	/* This is the so-called "sdbm" hash. It comes from a piece of
	 * public domain code from a clone of ndbm. */
	while ((c = *key++)) {
		hash = c + (hash << 6) + (hash << 16) - hash;
	}
	return hash;
}

int hashmap_string_comp(const void * a, const void * b) {
	return !strcmp(a, (char*)b); // the lack of const char* in strcmp strikes again!
}

void * hashmap_string_dupe(const void * key) {
	char *out = kmalloc(strlen((char*)key) + 1);
    memcpy(out, key, strlen((char*)key) + 1);
    return out;
}

unsigned int hashmap_int_hash(const void * key) {
	return (intptr_t)key;
}

int hashmap_int_comp(const void * a, const void * b) {
	return (intptr_t)a == (intptr_t)b;
}

void * hashmap_int_dupe(const void * key) {
	return (void*)key;
}

static void hashmap_int_free(void * ptr) {
	(void)ptr;
	return;
}


hashmap_t * hashmap_create(int size) {
	hashmap_t * map = kmalloc(sizeof(hashmap_t));

	map->hash_func     = &hashmap_string_hash;
	map->hash_comp     = &hashmap_string_comp;
	map->hash_key_dup  = &hashmap_string_dupe;
	map->hash_key_free = &kfree;
	map->hash_val_free = &kfree;

	map->size = size;
	map->entries = kmalloc(sizeof(hashmap_entry_t *) * size);
	memset(map->entries, 0x00, sizeof(hashmap_entry_t *) * size);

	return map;
}

hashmap_t * hashmap_create_int(int size) {
	hashmap_t * map = kmalloc(sizeof(hashmap_t));

	map->hash_func     = &hashmap_int_hash;
	map->hash_comp     = &hashmap_int_comp;
	map->hash_key_dup  = &hashmap_int_dupe;
	map->hash_key_free = &hashmap_int_free;
	map->hash_val_free = &kfree;

	map->size = size;
	map->entries = kmalloc(sizeof(hashmap_entry_t *) * size);
	memset(map->entries, 0x00, sizeof(hashmap_entry_t *) * size);

	return map;
}

void * hashmap_set(hashmap_t * map, const void * key, void * value) {
	unsigned int hash = map->hash_func(key) % map->size;

	hashmap_entry_t * x = map->entries[hash];
	if (!x) {
		hashmap_entry_t * e = kmalloc(sizeof(hashmap_entry_t));
		e->key   = map->hash_key_dup(key);
		e->value = value;
		e->next = NULL;
		map->entries[hash] = e;
		return NULL;
	} else {
		hashmap_entry_t * p = NULL;
		do {
			if (map->hash_comp(x->key, key)) {
				void * out = x->value;
				x->value = value;
				return out;
			} else {
				p = x;
				x = x->next;
			}
		} while (x);
		hashmap_entry_t * e = kmalloc(sizeof(hashmap_entry_t));
		e->key   = map->hash_key_dup(key);
		e->value = value;
		e->next = NULL;

		p->next = e;
		return NULL;
	}
}

void * hashmap_get(hashmap_t * map, const void * key) {
	unsigned int hash = map->hash_func(key) % map->size;

	hashmap_entry_t * x = map->entries[hash];
	if (!x) {
		return NULL;
	} else {
		do {
			if (map->hash_comp(x->key, key)) {
				return x->value;
			}
			x = x->next;
		} while (x);
		return NULL;
	}
}

void * hashmap_remove(hashmap_t * map, const void * key) {
	unsigned int hash = map->hash_func(key) % map->size;

	hashmap_entry_t * x = map->entries[hash];
	if (!x) {
		return NULL;
	} else {
		if (map->hash_comp(x->key, key)) {
			void * out = x->value;
			map->entries[hash] = x->next;
			map->hash_key_free(x->key);
			map->hash_val_free(x);
			return out;
		} else {
			hashmap_entry_t * p = x;
			x = x->next;
			do {
				if (map->hash_comp(x->key, key)) {
					void * out = x->value;
					p->next = x->next;
					map->hash_key_free(x->key);
					map->hash_val_free(x);
					return out;
				}
				p = x;
				x = x->next;
			} while (x);
		}
		return NULL;
	}
}

int hashmap_has(hashmap_t * map, const void * key) {
	unsigned int hash = map->hash_func(key) % map->size;

	hashmap_entry_t * x = map->entries[hash];
	if (!x) {
		return 0;
	} else {
		do {
			if (map->hash_comp(x->key, key)) {
				return 1;
			}
			x = x->next;
		} while (x);
		return 0;
	}

}

list_t * hashmap_keys(hashmap_t * map) {
	list_t * l = list_create();

	for (unsigned int i = 0; i < map->size; ++i) {
		hashmap_entry_t * x = map->entries[i];
		while (x) {
			list_insert(l, x->key);
			x = x->next;
		}
	}

	return l;
}

list_t * hashmap_values(hashmap_t * map) {
	list_t * l = list_create();

	for (unsigned int i = 0; i < map->size; ++i) {
		hashmap_entry_t * x = map->entries[i];
		while (x) {
			list_insert(l, x->value);
			x = x->next;
		}
	}

	return l;
}

void hashmap_free(hashmap_t * map) {
	for (unsigned int i = 0; i < map->size; ++i) {
		hashmap_entry_t * x = map->entries[i], * p;
		while (x) {
			p = x;
			x = x->next;
			map->hash_key_free(p->key);
			map->hash_val_free(p);
		}
	}
	kfree(map->entries);
}

int hashmap_is_empty(hashmap_t * map) {
	for (unsigned int i = 0; i < map->size; ++i) {
		if (map->entries[i]) return 0;
	}
	return 1;
}
