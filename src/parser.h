#ifndef PARSER_H
#define PARSER_H

struct symbol
{
	struct str8 str;
	struct token *definition;

	// TODO(sir->w7): Implement line numbers in the file string.
	int line_number;

	struct symbol *next;
};

struct symbol_table
{
	struct symbol *syms;

	u32 size;
	u32 capacity;
};

#define INITIAL_CAPACITY 32
#define GROWTH_RATE 2

struct symbol_table create_symbol_table(struct memory_arena *arena);
void symbol_table_push(struct symbol_table *table, struct memory_arena *arena, struct symbol sym);

#endif
