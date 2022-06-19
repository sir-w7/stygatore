// TODO(sir->w7): Fix this implementation because it's so idiotic. Like bruh, why does tokenizer_get_at move the tokenizer? It's only supposed to read where the tokenizer is at.
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
	// Used to do cool pointer increment while loop thing:
	// The classic sir->w7; but changed to other because more readable.
	while (tokens->next_offset++ < tokens->file_data.len) {
		for (int i = 0; i < array_count(whitespaces); ++i) {
			char whitespace = whitespaces[i];
			if (tokens->file_data.str[tokens->next_offset] != whitespace)
				return;
		}
	}
}

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
tokenizer_peek_next(StyxTokenizer *tokens)
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
        case '@': { tok.type = Token_TemplateDirective; } break;
		
        case ' ':
        case '\t':
		case '\r': { tok.type = Token_Whitespace; } break;
		
        case '\n': {
			tok.type = Token_Whitespace;
            tok.line--;
            
            if (!(tokens->next_offset > tokens->offset)) {
                tokens->line_at++;
            }
		} break;
		
		case '/': {
			if (tokenizer_peek_next(tokens) == '/') {
				tok.type = Token_CommentLine;
			} else if (tokenizer_peek_next(tokens) == '*') {
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
			if (tokenizer_peek_next(tokens) == '-') {
				tok.type = Token_FeedLeft;
			}
        } break;
        case '-': {
			if (tokenizer_peek_next(tokens) == '>') { 
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
		if (tok.type != Token_Whitespace && tok.type != Token_Semicolon &&
		    tok.type != Token_CommentLine && tok.type != Token_CommentBlock) {
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
		
		str8_lit("Token_TemplateDirective"),
		
		str8_lit("Token_EndOfFile"),
	};
	
	return token_type_str[type];
}

styx_function void
print_token(StyxToken tok)
{
	printf("tok.type: %-24.*s  ", str8_exp(str8_token_type(tok.type)));
    
    if (tok.type == Token_Whitespace) {
        printf("tok.str: ");
        u32 count = 0;
        
        for (u64 i = 0; i < tok.str.len; ++i) {
            if (tok.str.str[i] == '\t') {
                printf("\\t");
                count += 2;
            } else if (tok.str.str[i] == '\n') {
                printf("\\n");
                count += 2;
            } else if (tok.str.str[i] == ' ') {
                printf("<spc>");
                count += 5;
            } else {
                printf("%c", tok.str.str[i]);
                count++;
            }
        }
        
        for (u64 i = 0; i < 16 - count; ++i) {
            printf(" ");
        }
        printf("  ");
    } else {
        printf("tok.str: %-16.*s  ", str8_exp(tok.str));
    }
    
    printf("tok.line: %d", tok.line);
	printnl();
    
    // NOTE(sir->w7): Since we're not really going to debug whitespace as much at the moment.
}
