#include "common.h"
#include "tokenizer.h"

static inline struct str8
str8_token_type(enum token_type type)
{
    // NOTE(sir->w7): Initializer is not a constant bullcrap, but this is really only a debug function, so it does not matter.
    struct str8 token_type_str[] = {
#define token_type_decl(enum_val) str8_lit(#enum_val),
# include "token_type.h"
#undef token_type_decl
    };
    
	return token_type_str[type];
}

static inline void
tokenizer_token_inc_comment_line(struct tokenizer *tokens)
{
	while (tokens->offset++ < tokens->file_data.len &&
	       tokens->file_data.str[tokens->offset] != '\n');
}

static inline void
tokenizer_token_inc_comment_block(struct tokenizer *tokens)
{
	// Increment the tokenizer past the block comment open.
	tokens->offset++;
	while ((tokens->offset++ < tokens->file_data.len)) {
		if (tokens->file_data.str[tokens->offset] == '*' &&
		    tokens->file_data.str[++tokens->offset] == '/')
			break;
	}
	// NOTE(sir->w7): The tokenizer works by calculating to the string's end
	// to the character before the offset. Therefore, this is to maintain
	// compatibility with the other increment functions.
	tokens->offset++;
}

static inline void
tokenizer_token_inc_whitespace(struct tokenizer *tokens)
{
	static char whitespaces[] = {
		' ', '\n', '\r', '\t',
	};
	// Used to do cool pointer increment while loop thing:
	// The classic sir->w7; but changed to other because more readable.
	while (tokens->offset++ < tokens->file_data.len) {
		for (int i = 0; i < array_count(whitespaces); ++i) {
			char whitespace = whitespaces[i];
			if (tokens->file_data.str[tokens->offset] != whitespace)
				return;
		}
	}
}

static inline void
tokenizer_token_inc_def(struct tokenizer *tokens)
{
	// Delimiter is likely not even the right term for this.
	static char delimiters[] = {
		' ', '\n', '\r', '\t',
		'(', ')', '{', '}', ';',
	};
    
	while (tokens->offset++ < tokens->file_data.len) {
		for (int i = 0; i < array_count(delimiters); ++i) {
			char delimiter = delimiters[i];
			if (tokens->file_data.str[tokens->offset] == delimiter) {
				return;
			}
		}
	}
}

struct str8
str8_get_token(enum token_type type,
               struct tokenizer *tokens)
{
	if (type == TOKEN_END_OF_FILE) return (struct str8){0};
    
	struct str8 result = {0};
	int prev_offset = tokens->offset;
	result.str = tokens->file_data.str + prev_offset;
    
	switch (type) {
        case TOKEN_WHITESPACE: {
            tokenizer_token_inc_whitespace(tokens);
		} break;
        case TOKEN_COMMENT_LINE: {
            tokenizer_token_inc_comment_line(tokens);
		} break;
        case TOKEN_COMMENT_BLOCK: {
            tokenizer_token_inc_comment_block(tokens);
		} break;
        case TOKEN_BRACE_OPEN:
        case TOKEN_BRACE_CLOSE:
        case TOKEN_PARENTHETICAL_OPEN:
        case TOKEN_PARENTHETICAL_CLOSE: {
            // We basically only want to grab the one character.
            tokens->offset++;
		} break;
        default: {
            tokenizer_token_inc_def(tokens);
		} break;
	}
    
	result.len = tokens->offset - prev_offset;
    
	// Rewinds the offset to prepare for a tokenizer increment
    
	// NOTE(sir->w): Is this really necessary though? Is the tokenizer
	// increment any more than syntactic sugar?
	tokens->offset--;
    
	return result;
}

struct tokenizer
tokenizer_file(struct memory_arena *allocator, struct str8 filename)
{
	struct tokenizer tokens = {0};
	tokens.file_data = read_file(allocator, filename);
	return tokens;
}

static enum token_type
lex_token_str(struct token tok)
{
	enum token_type type = TOKEN_UNKNOWN;
	if (tok.type == TOKEN_TEMPLATE_DIRECTIVE) {
		if (str8_compare(tok.str, str8_lit("@output"))) {
			type = TOKEN_TEMPLATE_DIRECTIVE_OUTPUT;
		} else {
			type = TOKEN_TEMPLATE_DIRECTIVE;
		}
	} else {
		type = tok.type;
	}
    
	return type;
}

// TODO(sir->w7): When tokenizer->offset goes beyond the total length of
// the file, then we should have get_tokenizer_at return a null token.
struct token
get_tokenizer_at(struct tokenizer *tokens)
{
	struct token tok = {0};
	//if (tokenizer->offset >= tokenizer->file_data.len) return token;
    
	switch (tokens->file_data.str[tokens->offset]) {
        case '\0': {
            tok.type = TOKEN_END_OF_FILE;
            // We don't need a string for this.
		} break;
        case '@': {
            tok.type = TOKEN_TEMPLATE_DIRECTIVE;
		} break;
        case ' ':
        case '\t':
        case '\r':
        case '\n': {
            tok.type = TOKEN_WHITESPACE;
		} break;
        case '/': {
            if (tokens->file_data.str[tokens->offset + 1] == '/') {
                tok.type = TOKEN_COMMENT_LINE;
            } else if (tokens->file_data.str[tokens->offset + 1] == '*') {
                tok.type = TOKEN_COMMENT_BLOCK;
            }
		} break;
        case '{': {
            tok.type = TOKEN_BRACE_OPEN;
		} break;
        case '}': {
            tok.type = TOKEN_BRACE_CLOSE;
		} break;
        case '(': {
            tok.type = TOKEN_PARENTHETICAL_OPEN;
		} break;
        case ')': {
            tok.type = TOKEN_PARENTHETICAL_CLOSE;
		} break;
        case ';': {
            tok.type = TOKEN_SEMICOLON;
		} break;
        default: {
            tok.type = TOKEN_IDENTIFIER;
		} break;
	};
    
	tok.str = str8_get_token(tok.type, tokens);
	tok.type = lex_token_str(tok);
    
	return tok;
}

struct token
tokenizer_inc_all(struct tokenizer *tokens)
{
	tokens->offset++;
	return get_tokenizer_at(tokens);
}

struct token
tokenizer_inc_no_whitespace(struct tokenizer *tokens)
{
	struct token tok = tokenizer_inc_all(tokens);
	while (tok.type != TOKEN_END_OF_FILE) {
		if (tok.type != TOKEN_WHITESPACE &&
		    tok.type != TOKEN_COMMENT_LINE &&
		    tok.type != TOKEN_COMMENT_BLOCK &&
		    tok.type != TOKEN_SEMICOLON) {
			break;
		}
		tok = tokenizer_inc_all(tokens);
	}
	return tok;
}

void print_token(struct token tok)
{
	println("token type: %.*s", str8_exp(str8_token_type(tok.type)));
    
	printf("token_str: ");
	for (int i = 0; i < tok.str.len; ++i) {
		if (tok.str.str[i] == '\t') {
			printf("\\t");
		} else if (tok.str.str[i] == '\r') {
			printf("\\r");
		} else if (tok.str.str[i] == '\n') {
			printf("\\n");
		} else {
			printf("%c", tok.str.str[i]);
		}
	}
	printnl();
}


