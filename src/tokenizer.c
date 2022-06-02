#include "common.h"
#include "tokenizer.h"

static struct str8
str8_token_type(enum token_type type)
{
	static struct str8 token_type_str[] = {
#define token_type_decl(enum_val) str8_lit(#enum_val),
		#include "token_type.h"
#undef token_type_decl
	};

	return token_type_str[type];
}

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
tokenizer_token_inc_def(struct tokenizer *tokenizer)
{
	// Delimiter is likely not even the right term for this.
	static char delimiters[] = {
		' ', '\n', '\r', '\t',
		'(', ')', '{', '}', ';',
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
	case TOKEN_BRACE_OPEN:
	case TOKEN_BRACE_CLOSE:
	case TOKEN_PARENTHETICAL_OPEN:
	case TOKEN_PARENTHETICAL_CLOSE:
		// We basically only want to grab the one character.
		tokenizer->offset++;
		break;
	default:
		tokenizer_token_inc_def(tokenizer);
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

static enum token_type
lex_token_str(struct token token)
{
	enum token_type type = TOKEN_UNKNOWN;
	if (token.type == TOKEN_TEMPLATE_DIRECTIVE) {
		if (str8_compare(token.str, str8_lit("@output"))) {
			type = TOKEN_TEMPLATE_DIRECTIVE_OUTPUT;
		} else {
			type = TOKEN_TEMPLATE_DIRECTIVE;
		}
	} else {
		type = token.type;
	}

	return type;
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
	case ';':
		token.type = TOKEN_SEMICOLON;
		break;
	default:
		token.type = TOKEN_IDENTIFIER;
		break;
	};

	token.str = str8_get_token(token.type, tokenizer);
	token.type = lex_token_str(token);

	return token;
}

struct token
tokenizer_inc_all(struct tokenizer *tokenizer)
{
	tokenizer->offset++;
	return get_tokenizer_at(tokenizer);
}

struct token
tokenizer_inc_no_whitespace(struct tokenizer *tokenizer)
{
	struct token token = tokenizer_inc_all(tokenizer);
	while (token.type != TOKEN_END_OF_FILE) {
		if (token.type != TOKEN_WHITESPACE &&
		    token.type != TOKEN_COMMENT_LINE &&
		    token.type != TOKEN_COMMENT_BLOCK &&
		    token.type != TOKEN_SEMICOLON) {
			break;
		}
		token = tokenizer_inc_all(tokenizer);
	}
	return token;
}

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


