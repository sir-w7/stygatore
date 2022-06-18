#include "parser.h"

SymbolTable
create_symbol_table(MemoryArena *arena)
{
	SymbolTable table = {0};
	
	table.capacity = INITIAL_CAPACITY;
	table.syms = arena_push_array(arena, sizeof(Symbol), table.capacity);
	
	return table;
}


void symbol_table_push(SymbolTable *table, 
					   MemoryArena *arena, Symbol sym)
{
	u64 bucket = djb2_hash(sym.str);
	u64 idx = table->capacity % bucket;
	
	if (str8_is_nil(table->syms[idx].str)) {
		table->syms[idx] = sym;
		return;
	}
	
	Symbol *sym_new = arena_push_struct(arena, Symbol);
	
	Symbol *tail = &table->syms[idx];
	while (tail->next != NULL) tail = tail->next;
	
	tail->next = sym_new;
}
