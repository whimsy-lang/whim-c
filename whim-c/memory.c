#include <stdlib.h>

#include "compiler.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

void* reallocate(VM* vm, void* pointer, size_t oldSize, size_t newSize) {
	if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
		collectGarbage(vm);
#endif
	}

	if (newSize == 0) {
		free(pointer);
		return NULL;
	}

	void* result = realloc(pointer, newSize);
	if (result == NULL) exit(1);
	return result;
}

void markObject(VM* vm, Obj* object) {
	if (object == NULL) return;
	if (object->isMarked) return;

#ifdef DEBUG_LOG_GC
	printf("%p mark ", (void*)object);
	printValue(OBJ_VAL(object));
	printf("\n");
#endif

	object->isMarked = true;

	// don't bother queueing up native functions or strings since they
	// do not have references to check
	if (object->type == OBJ_NATIVE || object->type == OBJ_STRING) return;

	if (vm->grayCapacity < vm->grayCount + 1) {
		vm->grayCapacity = GROW_CAPACITY(vm->grayCapacity);
		vm->grayStack = (Obj**)realloc(vm->grayStack, sizeof(Obj*) * vm->grayCapacity);

		if (vm->grayStack == NULL) exit(1);
	}
	vm->grayStack[vm->grayCount++] = object;
}

void markValue(VM* vm, Value value) {
	if (IS_OBJ(value)) markObject(vm, AS_OBJ(value));
}

static void markArray(VM* vm, ValueArray* array) {
	for (int i = 0; i < array->count; i++) {
		markValue(vm, array->values[i]);
	}
}

static void blackenObject(VM* vm, Obj* object) {
#ifdef DEBUG_LOG_GC
	printf("%p blacken ", (void*)object);
	printValue(OBJ_VAL(object));
	printf("\n");
#endif

	switch (object->type) {
	case OBJ_CLOSURE: {
		ObjClosure* closure = (ObjClosure*)object;
		markObject(vm, (Obj*)closure->function);
		for (int i = 0; i < closure->upvalueCount; i++) {
			markObject(vm, (Obj*)closure->upvalues[i]);
		}
		break;
	}
	case OBJ_FUNCTION: {
		ObjFunction* function = (ObjFunction*)object;
		markObject(vm, (Obj*)function->name);
		markArray(vm, &function->chunk.constants);
		break;
	}
	case OBJ_UPVALUE:
		markValue(vm, ((ObjUpvalue*)object)->closed);
		break;
	}
}

static void freeObject(VM* vm, Obj* object) {
#ifdef DEBUG_LOG_GC
	printf("%p free type %d\n", (void*)object, object->type);
#endif

	switch (object->type) {
	case OBJ_CLOSURE: {
		ObjClosure* closure = (ObjClosure*)object;
		FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
		FREE(ObjClosure, object);
		break;
	}
	case OBJ_FUNCTION: {
		ObjFunction* function = (ObjFunction*)object;
		freeChunk(vm, &function->chunk);
		FREE(ObjFunction, object);
		break;
	}
	case OBJ_NATIVE:
		FREE(ObjNative, object);
		break;
	case OBJ_STRING: {
		ObjString* string = (ObjString*)object;
		FREE_ARRAY(char, string->chars, string->length + 1);
		FREE(ObjString, object);
		break;
	}
	case OBJ_UPVALUE:
		FREE(ObjUpvalue, object);
		break;
	}
}

static void markRoots(VM* vm) {
	for (Value* slot = vm->stack; slot < vm->stackTop; slot++) {
		markValue(vm, *slot);
	}

	for (int i = 0; i < vm->frameCount; i++) {
		markObject(vm, (Obj*)vm->frames[i].closure);
	}

	for (ObjUpvalue* upvalue = vm->openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
		markObject(vm, (Obj*)upvalue);
	}

	markTable(vm, &vm->globals);
	markCompilerRoots(vm);
}

static void traceReferences(VM* vm) {
	while (vm->grayCount > 0) {
		Obj* object = vm->grayStack[--vm->grayCount];
		blackenObject(vm, object);
	}
}

void collectGarbage(VM* vm) {
#ifdef DEBUG_LOG_GC
	printf("-- gc begin\n");
#endif

	markRoots(vm);
	traceReferences(vm);

#ifdef DEBUG_LOG_GC
	printf("-- gc end\n");
#endif
}

void freeObjects(VM* vm) {
	Obj* object = vm->objects;
	while (object != NULL) {
		Obj* next = object->next;
		freeObject(vm, object);
		object = next;
	}

	free(vm->grayStack);
}
