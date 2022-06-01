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

// Tokenization
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

static inline void
tokenizer_token_inc_comment_line(struct tokenizer *tokenizer)
{
	while (tokenizer->offset++ < tokenizer->file_data.len &&
	       tokenizer->file_data.str[tokenizer->offset] != '\n');
}

static inline void
tokenizer_token_inc_comment_block(struct tokenizer *tokenizer)
{
	// Increment the tokenizer past the block comment open.
	tokenizer->offset++;
	while ((tokenizer->offset++ < tokenizer->file_data.len)) {
		if (tokenizer->file_data.str[tokenizer->offset] == '*' &&
		    tokenizer->file_data.str[++tokenizer->offset] == '/')
			break;
	}
	// NOTE(sir->w7): The tokenizer works by calculating to the string's end
	// to the character before the offset. Therefore, this is to maintain
	// compatibility with the other increment functions.
	tokenizer->offset++;
}

static inline void
tokenizer_token_inc_whitespace(struct tokenizer *tokenizer)
{
	static char whitespaces[] = {
		' ', '\n', '\r', '\t',
	};
	// Used to do cool pointer increment while loop thing:
	// The classic sir->w7; but changed to other because more readable.
	while (tokenizer->offset++ < tokenizer->file_data.len) {
		for (int i = 0; i < array_count(whitespaces); ++i) {
			char whitespace = whitespaces[i];
			if (tokenizer->file_data.str[tokenizer->offset] !=
			    whitespace)
				return;
		}
	}
}

static inline void
tokenizer_token_inc_default(struct tokenizer *tokenizer)
{
	// Delimiter is likely not even the right term for this.
	static char delimiters[] = {
		' ', '\n', '\r', '\t',
		'(', ')', '{', '}',
	};

	while (tokenizer->offset++ < tokenizer->file_data.len) {
		for (int i = 0; i < array_count(delimiters); ++i) {
			char delimiter = delimiters[i];
			if (tokenizer->file_data.str[tokenizer->offset] ==
			    delimiter)
				return;
		}
	}
}

struct str8
str8_get_token(enum token_type type,
	       struct tokenizer *tokenizer)
{
	if (type == TOKEN_END_OF_FILE) return (struct str8){0};
	
	struct str8 result = {0};
	int prev_offset = tokenizer->offset;
	result.str = tokenizer->file_data.str + prev_offset;

	switch (type) {
	case TOKEN_WHITESPACE:
		tokenizer_token_inc_whitespace(tokenizer);
		break;
	case TOKEN_COMMENT_LINE:
		tokenizer_token_inc_comment_line(tokenizer);
		break;
	case TOKEN_COMMENT_BLOCK:
		tokenizer_token_inc_comment_block(tokenizer);
		break;
	default:
		tokenizer_token_inc_default(tokenizer);
		break;
	}

	result.len = tokenizer->offset - prev_offset;

	// Rewinds the offset to prepare for a tokenizer increment

	// NOTE(sir->w): Is this really necessary though? Is the tokenizer
	// increment any more than syntactic sugar?
	tokenizer->offset--;

	return result;
}

struct tokenizer
tokenizer_file(struct memory_arena *allocator, struct str8 filename)
{
	struct tokenizer tokenizer = {0};
	tokenizer.file_data = read_file(allocator, filename);
	return tokenizer;
}

// TODO(sir->w7): When tokenizer->offset goes beyond the total length of
// the file, then we should have get_tokenizer_at return a null token.
struct token
get_tokenizer_at(struct tokenizer *tokenizer)
{
	struct token token = {0};
	//if (tokenizer->offset >= tokenizer->file_data.len) return token;

	switch (tokenizer->file_data.str[tokenizer->offset]) {
	case '\0':
		token.type = TOKEN_END_OF_FILE;
		// We don't need a string for this.
		break;
	case '@':
		token.type = TOKEN_TEMPLATE_DIRECTIVE;
		break;
	case ' ':
	case '\t':
	case '\r':
	case '\n':
		token.type = TOKEN_WHITESPACE;
		break;
	case '/':
		if (tokenizer->file_data.str[tokenizer->offset + 1] == '/') {
			token.type = TOKEN_COMMENT_LINE;
		} else if (tokenizer->file_data.str[tokenizer->offset + 1] == '*') {
			token.type = TOKEN_COMMENT_BLOCK;
		}
		break;
	case '{':
		token.type = TOKEN_BRACE_OPEN;
		break;
	case '}':
		token.type = TOKEN_BRACE_CLOSE;
		break;
	case '(':
		token.type = TOKEN_PARENTHETICAL_OPEN;
		break;
	case ')':
		token.type = TOKEN_PARENTHETICAL_CLOSE;
		break;
	default:
		token.type = TOKEN_IDENTIFIER;
		break;
	};
	
	token.str = str8_get_token(token.type, tokenizer);
	return token;
}

static inline struct token
tokenizer_inc_all(struct tokenizer *tokenizer)
{
	tokenizer->offset++;
	return get_tokenizer_at(tokenizer);
}

static struct token
tokenizer_inc_no_whitespace(struct tokenizer *tokenizer)
{
	struct token token = tokenizer_inc_all(tokenizer);
	while (token.type != TOKEN_END_OF_FILE) {
		if (token.type != TOKEN_WHITESPACE &&
		    token.type != TOKEN_COMMENT_LINE &&
		    token.type != TOKEN_COMMENT_BLOCK) {
			break;
		}
		token = tokenizer_inc_all(tokenizer);
	}
	return token;
}

static
void print_token(struct token token)
{
	println("token type: %.*s", str8_exp(str8_token_type(token.type)));

	printf("token_str: ");
	for (int i = 0; i < token.str.len; ++i) {
		if (token.str.str[i] == '\t') {
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
	for (struct token token = get_tokenizer_at(&tokenizer);
	     token.type != TOKEN_END_OF_FILE;
	     token = tokenizer_inc_no_whitespace(&tokenizer)) {
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
