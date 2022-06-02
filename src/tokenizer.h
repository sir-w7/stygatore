#ifndef TOKENIZER_H
#define TOKENIZER_H

enum token_type
{
#define token_type_decl(enum_val) enum_val,
	#include "token_type.h"
#undef token_type_decl
};

struct token
{
	enum token_type type;
	struct str8 str;
};

struct tokenizer
{
	struct str8 file_data;
	int offset;
};

struct str8 str8_get_token(enum token_type type, struct tokenizer *tokenizer);
struct tokenizer tokenizer_file(struct memory_arena *allocator, struct str8 filename);
struct token get_tokenizer_at(struct tokenizer *tokenizer);
void print_token(struct token token);

struct token tokenizer_inc_all(struct tokenizer *tokenizer);
struct token tokenizer_inc_no_whitespace(struct tokenizer *tokenizer);

#endif
