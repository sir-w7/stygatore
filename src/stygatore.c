/* TODO(winston): error output */
/* TODO(winston): efficient memory usage */
/* TODO(winston): one-pass lexing */

/* TODO(sir->w7): A strong powerful string type for easier parsing. */
// TODO(sir->w7): Template concatenation for even more powerful templates.

// TODO(sir->w7): Unity/jumbo compilation?
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "common.h"
#include "platform.h"

#define STYX_EXT "styxgen"

//--------------------------------------------------------------------
//-----------------------------Stygatore------------------------------
//--------------------------------------------------------------------
enum token_type
{
#define token_type_decl(enum_val) enum_val,
	#include "token_type.h"
#undef token_type_decl
};

struct str8 token_type_str[] = {
#define token_type_decl(enum_val) str8_lit(#enum_val),
	#include "token_type.h"
#undef token_type_decl
};

struct token
{
	enum token_type type;
	struct str8 str;

	struct token *next;
};

struct token_list
{
	struct token *head, *tail;
	struct token *at;
};

void tokenlist_push(struct token_list *list,
                    struct memory_arena *allocator, struct token token)
{
	if (list->head == NULL) {
		list->head = arena_push_struct(allocator, struct token);
		memory_copy(list->head, &token, sizeof(struct token));

		list->at = list->head;
		list->tail = list->head;
		return;
	}

	struct token *new_node = arena_push_struct(allocator, struct token);
	memory_copy(new_node, &token, sizeof(struct token));

	list->tail->next = new_node;
	list->tail = new_node;
}

struct str8
str8_get_token(enum token_type type,
	       struct str8 data, int *idx)
{
	struct str8 result = {0};
	int offset = *idx;
	result.str = data.str + offset;

	switch (type) {
	case TOKEN_WHITESPACE:
		while ((*idx)++ < data.len) {
			if (data.str[*idx] != ' ' ||
				data.str[*idx] != '\n' ||
				data.str[*idx] != '\r' ||
				data.str[*idx] != '\t') break;
		}
		break;
	default:
		while ((*idx)++ < data.len) {
			if (data.str[*idx] == ' ' ||
				data.str[*idx] == '\n' ||
				data.str[*idx] == '\r' ||
				data.str[*idx] == '\t') break;
		}
		break;
	}

	result.len = *idx - offset;
	return result;
}

void print_token(struct token *token)
{
	println("token type: %.*s", str8_exp(token_type_str[token->type]));

	printf("token_str: ");
	for (int i = 0; i < token->str.len; ++i) {
		if (token->str.str[i] == ' ') {
			printf("<space>");
		} else if (token->str.str[i] == '\t') {
			printf("\\t");
		} else if (token->str.str[i] == '\r') {
			printf("\\r");
		} else if (token->str.str[i] == '\n') {
			printf("\\n");
		} else {
			printf("%c", token->str.str[i]);
		}
	}
	printnl();
}

struct token_list
index_file(struct memory_arena *allocator, struct str8 filename)
{
	struct token_list tokenizer = {0};

	struct str8 file_data = read_file(allocator, filename);
	if (file_data.len == 0) return tokenizer;

	for (int i = 0; i < file_data.len;) {
		struct token token = {0};
		char ch = file_data.str[i];

		switch (ch) {
		case '@':
			token.type = TOKEN_TEMPLATE_DIRECTIVE;
			token.str = str8_get_token(token.type, file_data, &i);
			break;
		case '\t':
		case '\r':
		case '\n':
		case ' ':
			token.type = TOKEN_WHITESPACE;
			token.str = str8_get_token(token.type, file_data, &i);
			break;
		default:
			token.type = TOKEN_IDENTIFIER;
			token.str = str8_get_token(token.type, file_data, &i);
			break;
		};

		tokenlist_push(&tokenizer, allocator, token);
	}

	return tokenizer;
}

void handle_file(struct memory_arena *allocator,
		 struct memory_arena *temp_allocator,
		 struct str8 file)
{
	struct str8 working_dir = file_working_dir(file);
	struct str8 base_name = file_base_name(file);
	struct str8 ext = file_ext(file);

	println("full path: %.*s", str8_exp(file));
	println("working_dir: %.*s", str8_exp(working_dir));
	println("base_name: %.*s", str8_exp(base_name));
	println("ext: %.*s", str8_exp(ext));
	printnl();
	
	struct token_list tokenizer = index_file(temp_allocator, file);
	for (struct token *token = tokenizer.head; token; token = token->next) {
		print_token(token);
	}
	printnl();
	printnl();
}

void handle_dir(struct memory_arena *allocator,
		struct memory_arena *temp_allocator,
		struct str8 dir)
{
	struct str8list files = get_dir_list_ext(allocator, dir, str8_lit(STYX_EXT));
	//struct str8list files = get_dir_list_ext(allocator, dir, str8_lit("c"));
	for (struct str8node *file = files.head; file; file = file->next) {
		handle_file(allocator, temp_allocator, file->data);
	}
}

int main(int argc, char **argv)
{
	if (argc == 1) {
		println("stygatore is a sane, performant metaprogramming tool for language-agnostic generics\n"
			"with readable diagnostics and viewable output free from compiler/vendor control\n" 
			"for maximum developer productivity.");
		printnl();
		println("Usage: %s [files/directories]", argv[0]);
	}

	struct memory_arena allocator = init_arena(megabytes(256));
	struct memory_arena temp_allocator = init_arena(megabytes(512));

	struct str8list args = arg_list(&allocator, argc, argv);
	for (struct str8node *arg = args.head; arg; arg = arg->next) {
		arena_reset(&temp_allocator);
		if (is_file(arg->data)) {
			handle_file(&allocator, &temp_allocator, arg->data);
		} else if (is_dir(arg->data)) {
			handle_dir(&allocator, &temp_allocator, arg->data);	
		} else {
			fprintln(stderr, "Argument is neither a file nor a directory.");
		}
	}

	free_arena(&temp_allocator);
	free_arena(&allocator);
}
