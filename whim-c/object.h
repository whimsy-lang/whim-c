#ifndef whimsy_object_h
#define whimsy_object_h

#include "chunk.h"
#include "table.h"
#include "value.h"

#define OBJ_TYPE(value)			(AS_OBJ(value)->type)

#define IS_BOUND_METHOD(value)	isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value)			isObjType(value, OBJ_CLASS)
#define IS_CLOSURE(value)		isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)		isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)		isObjType(value, OBJ_INSTANCE)
#define IS_NATIVE(value)		isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)		isObjType(value, OBJ_STRING)

#define AS_BOUND_METHOD(value)	((ObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value)			((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value)		((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value)		((ObjFunction*)AS_OBJ(value))
#define AS_INSTANCE(value)		((ObjInstance*)AS_OBJ(value))
#define AS_NATIVE(value)		(((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value)		((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)		(((ObjString*)AS_OBJ(value))->chars)

typedef struct VM VM;

typedef enum {
	OBJ_BOUND_METHOD,
	OBJ_CLASS,
	OBJ_CLOSURE,
	OBJ_FUNCTION,
	OBJ_INSTANCE,
	OBJ_NATIVE,
	OBJ_STRING,
	OBJ_UPVALUE,
} ObjType;

struct Obj {
	ObjType type;
	bool isMarked;
	struct Obj* next;
};

typedef struct {
	Obj obj;
	int arity;
	int upvalueCount;
	Chunk chunk;
	ObjString* name;
} ObjFunction;

typedef Value(*NativeFn)(int argCount, Value* args);

typedef struct {
	Obj obj;
	NativeFn function;
} ObjNative;

struct ObjString {
	Obj obj;
	int length;
	char* chars;
	uint32_t hash;
};

typedef struct ObjUpvalue {
	Obj obj;
	Value* location;
	Value closed;
	struct ObjUpvalue* next;
} ObjUpvalue;

typedef struct {
	Obj obj;
	ObjFunction* function;
	ObjUpvalue** upvalues;
	int upvalueCount;
} ObjClosure;

typedef struct ObjClass {
	Obj obj;
	ObjString* name;
	struct ObjClass* super;
	Table fields;
} ObjClass;

typedef struct {
	Obj obj;
	ObjClass* type;
	Table fields;
} ObjInstance;

typedef struct {
	Obj obj;
	Value receiver;
	ObjClosure* method;
} ObjBoundMethod;

ObjBoundMethod* newBoundMethod(VM* vm, Value receiver, ObjClosure* method);
ObjClass* newClass(VM* vm, ObjString* name);
ObjClosure* newClosure(VM* vm, ObjFunction* function);
ObjFunction* newFunction(VM* vm);
ObjInstance* newInstance(VM* vm, ObjClass* _class);
ObjNative* newNative(VM* vm, NativeFn function);
ObjString* takeString(VM* vm, char* chars, int length);
ObjString* copyString(VM* vm, const char* chars, int length);
ObjString* copyEscapeString(VM* vm, const char* chars, int length);
ObjUpvalue* newUpvalue(VM* vm, Value* slot);
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
