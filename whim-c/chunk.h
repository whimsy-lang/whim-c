#ifndef whimsy_chunk_h
#define whimsy_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
	OP_CONSTANT,
	OP_NIL,
	OP_TRUE,
	OP_FALSE,
	OP_POP,
	OP_DEFINE_GLOBAL_CONST,
	OP_DEFINE_GLOBAL_VAR,
	OP_GET_GLOBAL,
	OP_SET_GLOBAL,
	OP_GET_LOCAL,
	OP_SET_LOCAL,
	OP_GET_UPVALUE,
	OP_SET_UPVALUE,
	OP_DEFINE_PROPERTY_CONST,
	OP_DEFINE_PROPERTY_CONST_POP,
	OP_DEFINE_PROPERTY_VAR,
	OP_DEFINE_PROPERTY_VAR_POP,
	OP_GET_PROPERTY,
	OP_GET_PROPERTY_POP,
	OP_SET_PROPERTY,
	OP_EQUAL,
	OP_NOT_EQUAL,
	OP_GREATER,
	OP_GREATER_EQUAL,
	OP_LESS,
	OP_LESS_EQUAL,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_MODULUS,
	OP_NEGATE,
	OP_NOT,
	OP_JUMP,
	OP_JUMP_BACK,
	OP_JUMP_IF_TRUE,
	OP_JUMP_IF_FALSE,
	OP_JUMP_IF_TRUE_POP,
	OP_JUMP_IF_FALSE_POP,
	OP_CALL,
	OP_INVOKE,
	OP_CLOSURE,
	OP_CLOSE_UPVALUE,
	OP_RETURN,
	OP_CLASS,
	OP_ANON_CLASS,
} OpCode;

typedef struct {
	int count;
	int capacity;
	uint8_t* code;
	int* lines;
	ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(VM* vm, Chunk* chunk);
void writeChunk(VM* vm, Chunk* chunk, uint8_t byte, int line);
int addConstant(VM* vm, Chunk* chunk, Value value);

#endif
