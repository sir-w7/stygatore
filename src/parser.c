#include "common.h"
#include "platform.h"
#include "tokenizer.h"
#include "parser.h"

struct symbol_table
create_symbol_table(struct memory_arena *arena)
{
	struct symbol_table table = {0};

	table.capacity = INITIAL_CAPACITY;
	table.syms = arena_push_array(arena, sizeof(struct symbol), table.capacity);

	return table;
}

void symbol_table_push(struct symbol_table *table, struct symbol sym)
{

}
