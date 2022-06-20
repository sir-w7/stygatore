#include "tokenizer.h"

styx_inline void
tokenizer_token_inc_comment_line(StyxTokenizer *tokens)
{
	while (tokens->next_offset++ < tokens->file_data.len &&
	       tokens->file_data.str[tokens->next_offset] != '\n');
}

styx_inline void
tokenizer_token_inc_comment_block(StyxTokenizer *tokens)
{
	// Increment the tokenizer past the block comment open.
	tokens->next_offset++;
	while ((tokens->next_offset++ < tokens->file_data.len)) {
        if (tokens->file_data.str[tokens->next_offset] == '\n') {
            tokens->line_at++;
        }
        
		if (tokens->file_data.str[tokens->next_offset] == '*' &&
		    tokens->file_data.str[++tokens->next_offset] == '/') {
			break;
        }
	}
	// NOTE(sir->w7): The tokenizer works by calculating to the string's end to the character before the offset. Therefore, this is to maintain compatibility with the other increment functions.
	tokens->next_offset++;
}

styx_inline void
tokenizer_token_inc_whitespace(StyxTokenizer *tokens)
{
	static char whitespaces[] = {
		' ', '\n', '\r', '\t',
	};
    
    if (tokens->file_data.str[tokens->next_offset] == '\n') {
        tokens->line_at++;
    }
    
	// Used to do cool pointer increment while loop thing:
	// The classic sir->w7, but changed to other because more readable.
	while (tokens->next_offset++ < tokens->file_data.len) {
		for (int i = 0; i < array_count(whitespaces); ++i) {
			char whitespace = whitespaces[i];
			if (tokens->file_data.str[tokens->next_offset] != whitespace)
				return;
		}
	}
    
    if (tokens->file_data.str[tokens->next_offset] == '\n') {
        tokens->line_at++;
    }
}
// a b c
// 0 1 2

// len: 3
styx_inline void
tokenizer_token_inc_def(StyxTokenizer *tokens)
{
	// Delimiter is likely not even the right term for this.
	static char delimiters[] = {
		' ', '\n', '\r', '\t',
		'(', ')', '{', '}', 
		';', ':', ',',
	};
    
	while (tokens->next_offset++ < tokens->file_data.len) {
		for (int i = 0; i < array_count(delimiters); ++i) {
			char delimiter = delimiters[i];
			if (tokens->file_data.str[tokens->next_offset] == delimiter) {
				return;
			}
		}
	}
}

styx_function Str8
str8_get_token(StyxTokenType type, StyxTokenizer *tokens)
{
	if (type == Token_EndOfFile) return Str8{};
	
	Str8 result = {0};
	tokens->next_offset = tokens->offset;
	result.str = tokens->file_data.str + tokens->offset;
	
	switch (type) {
		case Token_Whitespace: { tokenizer_token_inc_whitespace(tokens); } break;
		case Token_CommentLine: { tokenizer_token_inc_comment_line(tokens); } break;
		case Token_CommentBlock: { tokenizer_token_inc_comment_block(tokens); } break;
		
		case Token_BraceOpen:
		case Token_BraceClose:
		case Token_ParentheticalOpen:
		case Token_ParentheticalClose: { tokens->next_offset++; } break;
		
		default: { tokenizer_token_inc_def(tokens); } break;
	}
	
	result.len = tokens->next_offset - tokens->offset;
	
	return result;
}

styx_function StyxTokenizer
tokenizer_file(MemoryArena *allocator, Str8 filename)
{
	StyxTokenizer tokens{};
	
	tokens.file_data = read_file(allocator, filename);
	tokens.line_at = 1;
	
	return tokens;
}

styx_inline char
tokenizer_peek_next_ch(StyxTokenizer *tokens)
{
	return tokens->file_data.str[tokens->offset + 1];
}

styx_function StyxToken
tokenizer_get_at(StyxTokenizer *tokens)
{
	StyxToken tok{};
	
    tok.line = tokens->line_at;
	switch (tokens->file_data.str[tokens->offset]) {
        case '\0': { tok.type = Token_EndOfFile; } break;
        case '@': { tok.type = Token_StyxDirective; } break;
		
        case ' ':
        case '\t':
		case '\r': 
        case '\n': { tok.type = Token_Whitespace; } break;
		
		case '/': {
			if (tokenizer_peek_next_ch(tokens) == '/') {
				tok.type = Token_CommentLine;
			} else if (tokenizer_peek_next_ch(tokens) == '*') {
				tok.type = Token_CommentBlock;
			}
		} break;
		
        case ';': { tok.type = Token_Semicolon; } break;
        case ':': { tok.type = Token_Colon; } break;
		case ',': { tok.type = Token_Comma; } break;
		
        case '{': { tok.type = Token_BraceOpen; } break;
        case '}': { tok.type = Token_BraceClose; } break;
        case '(': { tok.type = Token_ParentheticalOpen; } break;
        case ')': { tok.type = Token_ParentheticalClose; } break;
		
        case '<': {
			if (tokenizer_peek_next_ch(tokens) == '-') {
				tok.type = Token_FeedLeft;
			}
        } break;
        case '-': {
			if (tokenizer_peek_next_ch(tokens) == '>') { 
				tok.type = Token_FeedRight;
			}
        } break;
		
        default: { tok.type = Token_Identifier; } break;
	};
	
    if (tokens->next_offset > tokens->offset) {
        tok.str = Str8{
            tokens->file_data.str + tokens->offset,
            tokens->next_offset - tokens->offset,
        };
    } else {
        tok.str = str8_get_token(tok.type, tokens);
    }
    
	return tok;
}

styx_function StyxToken
tokenizer_inc_all(StyxTokenizer *tokens)
{
    StyxToken result{};
	tokens->offset = tokens->next_offset;
	return tokenizer_get_at(tokens);
}

styx_function StyxToken 
tokenizer_inc_no_whitespace(StyxTokenizer *tokens)
{
	StyxToken tok = tokenizer_inc_all(tokens);
	while (tok.type != Token_EndOfFile) {
		if (tok.type != Token_Whitespace && tok.type != Token_CommentLine && tok.type != Token_CommentBlock) {
			break;
		}
		tok = tokenizer_inc_all(tokens);
	}
	return tok;
}

styx_function Str8
str8_token_type(StyxTokenType type)
{
	// NOTE(sir->w7): Initializer is not a constant bullcrap, but this is really only a debug function, so it does not matter.
	Str8 token_type_str[] = {
		str8_lit("Token_Unknown"),
		str8_lit("Token_Identifier"),
		
		str8_lit("Token_Semicolon"),
        str8_lit("Token_Comma"),
		str8_lit("Token_Comma"),
		
		str8_lit("Token_CommentLine"),
		str8_lit("Token_CommentBlock"),
		str8_lit("Token_Whitespace"),
		
		str8_lit("Token_ParentheticalOpen"),
		str8_lit("Token_ParentheticalClose"),
		str8_lit("Token_BraceOpen"),
		str8_lit("Token_BraceClose"),
		
		str8_lit("Token_FeedRight"),
		str8_lit("Token_FeedLeft"),
		
		str8_lit("Token_StyxDirective"),
		
		str8_lit("Token_EndOfFile"),
	};
	
	return token_type_str[type];
}

styx_function void
token_print(StyxToken tok)
{
	printf("tok.type: %-24.*s  ", str8_exp(str8_token_type(tok.type)));
    
    printf("tok.str: ");
    
#define PRINT_MAX 32
    if (tok.str.len < PRINT_MAX) {
        u32 count = 0;
        for (u64 i = 0; i < tok.str.len; ++i) {
            if (tok.str.str[i] == '\t') {
                printf("\\t");
                count += 2;
            } else if (tok.str.str[i] == '\n') {
                printf("\\n");
                count += 2;
            } else {
                printf("%c", tok.str.str[i]);
                count++;
            }
        }
        
        for (u64 i = 0; i < PRINT_MAX - count; ++i) {
            printf(" ");
        }
    } else {
        for (u64 i = 0; i < PRINT_MAX - 3; ++i) {
            if (tok.str.str[i] == '\t') {
                printf("\\t");
            } else if (tok.str.str[i] == '\n') {
                printf("\\n");
            } else {
                printf("%c", tok.str.str[i]);
            }
        }
        
        printf("...");
    }
    
    printf("  ");
    
    printf("tok.line: %d", tok.line);
	printnl();
}

styx_function StyxTokenizerState
store_tokenizer_state(StyxTokenizer *tokens)
{
    return StyxTokenizerState{
        tokens->offset, tokens->next_offset, tokens->line_at,
    };
}

styx_function void
restore_tokenizer_state(StyxTokenizerState state, StyxTokenizer *tokens)
{
    tokens->offset = state.offset;
    tokens->next_offset = state.next_offset;
    tokens->line_at = state.line_at;
}

styx_function StyxToken 
tokenizer_peek_all(StyxTokenizer *tokens)
{
    auto state = store_tokenizer_state(tokens);
    defer { restore_tokenizer_state(state, tokens); };
    
    auto token = tokenizer_inc_all(tokens);
    return token;
}

styx_function StyxToken 
tokenizer_peek_no_whitespace(StyxTokenizer *tokens)
{
    auto state = store_tokenizer_state(tokens);
    defer { restore_tokenizer_state(state, tokens); };
    
    auto token = tokenizer_inc_no_whitespace(tokens);
    return token;
}

styx_function bool
known_styx_directive(StyxToken token)
{
    static const Str8 known_directives[] = {
        str8_lit("@output"),
        str8_lit("@template"),
    };
    
    for (auto str : known_directives) {
        if (str8_compare(str, token.str)) {
            return true;
        }
    }
    
    return false;
}
