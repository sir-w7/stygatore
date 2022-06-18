#ifndef PARSER_H
#define PARSER_H

struct Symbol
{
	Str8 str;
	StyxToken *definition;
	
	// TODO(sir->w7): Implement line numbers in the file string.
	int line_number;
	
	Symbol *next;
};

struct SymbolTable
{
	Symbol *syms;
	
	u32 size;
	u32 capacity;
};

#define INITIAL_CAPACITY 32
#define GROWTH_RATE 2

SymbolTable create_symbol_table(MemoryArena *arena);
void symbol_table_push(SymbolTable *table, MemoryArena *arena, Symbol sym);

#endif
