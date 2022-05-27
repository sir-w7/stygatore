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

struct str8 
str8_token_type(enum token_type type)
{
	static struct str8 token_type_str[] = {
#define token_type_decl(enum_val) str8_lit(#enum_val),
		#include "token_type.h"
#undef token_type_decl
	};
	
	return token_type_str[type];
}

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
struct str8
str8_get_token(enum token_type type,
	            struct tokenizer *tokenizer)
{
	struct str8 result = {0};
	int prev_offset = tokenizer->offset;
	result.str = tokenizer->file_data.str + prev_offset;

	switch (type) {
	case TOKEN_WHITESPACE:
		while ((tokenizer->offset++ < tokenizer->file_data.len) &&
			(tokenizer->file_data.str[tokenizer->offset] == ' ' ||
			tokenizer->file_data.str[tokenizer->offset] == '\n' ||
			tokenizer->file_data.str[tokenizer->offset] == '\r' ||
			tokenizer->file_data.str[tokenizer->offset] == '\t'));

		break;
	default:
		while ((tokenizer->offset++ < tokenizer->file_data.len) &&
			(tokenizer->file_data.str[tokenizer->offset] != ' ' &&
			tokenizer->file_data.str[tokenizer->offset] != '\n' &&
			tokenizer->file_data.str[tokenizer->offset] != '\r' &&
			tokenizer->file_data.str[tokenizer->offset] != '\t'));
		break;
	}

	result.len = tokenizer->offset - prev_offset;
	return result;
}

struct tokenizer
tokenizer_file(struct memory_arena *allocator, struct str8 filename)
{
	struct tokenizer tokenizer = {0};
	tokenizer.file_data = read_file(allocator, filename);
	return tokenizer;
}

struct token
get_tokenizer_at(struct tokenizer *tokenizer)
{
	struct token token = {0};

	switch (tokenizer->file_data.str[tokenizer->offset]) {
	case '\0':
		token.type = TOKEN_END_OF_FILE;
		// We don't need a string for this.
		break;
	case '@':
		token.type = TOKEN_TEMPLATE_DIRECTIVE;
		token.str = str8_get_token(token.type, tokenizer);
		break;
	case ' ':
	case '\t':
	case '\r':
	case '\n':
		token.type = TOKEN_WHITESPACE;
		token.str = str8_get_token(token.type, tokenizer);
		break;
	default:
		token.type = TOKEN_IDENTIFIER;
		token.str = str8_get_token(token.type, tokenizer);
		break;
	};

	return token;
}

static inline b32
tokenizer_inc_all(struct tokenizer *tokenizer)
{
	tokenizer->offset++;
	return tokenizer->offset < tokenizer->file_data.len;
}

void print_token(struct token token)
{
	println("token type: %.*s", str8_exp(str8_token_type(token.type)));

	printf("token_str: ");
	for (int i = 0; i < token.str.len; ++i) {
		if (token.str.str[i] == ' ') {
			printf("<space>");
		} else if (token.str.str[i] == '\t') {
			printf("\\t");
		} else if (token.str.str[i] == '\r') {
			printf("\\r");
		} else if (token.str.str[i] == '\n') {
			printf("\\n");
		} else {
			printf("%c", token.str.str[i]);
		}
	}
	printnl();
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

	struct tokenizer tokenizer = tokenizer_file(allocator, file);
	struct token token = get_tokenizer_at(&tokenizer);
	print_token(token);
	while (tokenizer_inc_all(&tokenizer)) {
		token = get_tokenizer_at(&tokenizer);
		print_token(token);
	}

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
