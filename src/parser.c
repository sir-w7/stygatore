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


void symbol_table_push(struct symbol_table *table, 
		       struct memory_arena *arena, struct symbol sym)
{
	u64 bucket = djb2_hash(sym.str);
	u64 idx = table->capacity % bucket;

	if (str8_is_nil(table->syms[idx].str)) {
		table->syms[idx] = sym;
		return;
	}

	struct symbol *sym_new = arena_push_struct(arena, struct symbol);

	struct symbol *tail = &table->syms[idx];
	while (tail->next != NULL) tail = tail->next;

	tail->next = sym_new;
}
