#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table) {
	table->count = 0;
	table->capacity = 0;
	table->entries = NULL;
}

void freeTable(VM* vm, Table* table) {
	FREE_ARRAY(Entry, table->entries, table->capacity);
	initTable(table);
}

static Entry* findEntry(Entry* entries, int capacity, ObjString* key) {
	uint32_t index = key->hash % capacity;
	Entry* tombstone = NULL;

	for (;;) {
		Entry* entry = &entries[index];
		if (entry->key == NULL) {
			if (IS_NIL(entry->value)) {
				// empty entry
				return tombstone != NULL ? tombstone : entry;
			}
			else {
				// tombstone
				if (tombstone == NULL) tombstone = entry;
			}
		}
		else if (entry->key == key) {
			// found the key
			return entry;
		}

		index = (index + 1) % capacity;
	}
}

bool tableGet(Table* table, ObjString* key, Value* value) {
	if (table->count == 0) return false;

	Entry* entry = findEntry(table->entries, table->capacity, key);
	if (entry->key == NULL) return false;

	*value = entry->value;
	return true;
}

bool tableGetPtr(Table* table, ObjString* key, Value** value) {
	if (table->count == 0) return false;

	Entry* entry = findEntry(table->entries, table->capacity, key);
	if (entry->key == NULL) return false;

	*value = &entry->value;
	return true;
}

static void adjustCapacity(VM* vm, Table* table, int capacity) {
	Entry* entries = ALLOCATE(Entry, capacity);
	for (int i = 0; i < capacity; i++) {
		entries[i].key = NULL;
		entries[i].value = NIL_VAL;
	}

	table->count = 0;
	for (int i = 0; i < table->capacity; i++) {
		Entry* entry = &table->entries[i];
		if (entry->key == NULL) continue;

		Entry* dest = findEntry(entries, capacity, entry->key);
		dest->key = entry->key;
		dest->value = entry->value;
		table->count++;
	}

	FREE_ARRAY(Entry, table->entries, table->capacity);
	table->entries = entries;
	table->capacity = capacity;
}

bool tableSet(VM* vm, Table* table, ObjString* key, Value value) {
	if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
		int capacity = GROW_CAPACITY(table->capacity);
		adjustCapacity(vm, table, capacity);
	}

	Entry* entry = findEntry(table->entries, table->capacity, key);
	bool isNewKey = entry->key == NULL;
	// only increment if it's a new key and not a tombstone
	if (isNewKey && IS_NIL(entry->value)) table->count++;

	entry->key = key;
	entry->value = value;
	return isNewKey;
}

// adds an item if it doesn't already exist, and returns whether the add succeeded
bool tableAdd(VM* vm, Table* table, ObjString* key, Value value) {
	if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
		int capacity = GROW_CAPACITY(table->capacity);
		adjustCapacity(vm, table, capacity);
	}

	Entry* entry = findEntry(table->entries, table->capacity, key);
	bool isNewKey = entry->key == NULL;
	if (isNewKey) {
		// only increment if it's a new key and not a tombstone
		if (IS_NIL(entry->value)) table->count++;
		entry->key = key;
		entry->value = value;
	}
	
	return isNewKey;
}

bool tableDelete(Table* table, ObjString* key) {
	if (table->count == 0) return false;

	// find the entry
	Entry* entry = findEntry(table->entries, table->capacity, key);
	if (entry->key == NULL) return false;

	// tombstone
	entry->key = NULL;
	entry->value = BOOL_VAL(true);
	return true;
}

void tableAddAll(VM* vm, Table* from, Table* to) {
	for (int i = 0; i < from->capacity; i++) {
		Entry* entry = &from->entries[i];
		if (entry->key != NULL) {
			tableSet(vm, to, entry->key, entry->value);
		}
	}
}

ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash) {
	if (table->count == 0) return NULL;

	uint32_t index = hash % table->capacity;
	for (;;) {
		Entry* entry = &table->entries[index];
		if (entry->key == NULL) {
			// stop if we find an empty non-tombstone entry
			if (IS_NIL(entry->value)) return NULL;
		}
		else if (entry->key->length == length && entry->key->hash == hash &&
			memcmp(entry->key->chars, chars, length) == 0) {
			// found
			return entry->key;
		}

		index = (index + 1) % table->capacity;
	}
}

void tableRemoveWhite(VM* vm, Table* table) {
	for (int i = 0; i < table->capacity; i++) {
		Entry* entry = &table->entries[i];
		if (entry->key != NULL && !entry->key->obj.isMarked) {
			tableDelete(table, entry->key);
		}
	}
}

void markTable(VM* vm, Table* table) {
	for (int i = 0; i < table->capacity; i++) {
		Entry* entry = &table->entries[i];
		markObject(vm, (Obj*)entry->key);
		markValue(vm, entry->value);
	}
}
