#include "tokenizer.h"

styx_function Str8
str8_token_type(StyxTokenType type)
{
	// NOTE(sir->w7): Initializer is not a constant bullcrap, but this is really only a debug function, so it does not matter.
	Str8 token_type_str[] = {
		str8_lit("StyxToken_Unknown"),
		str8_lit("StyxToken_Identifier"),
		
		str8_lit("StyxToken_Semicolon"),
		str8_lit("StyxToken_Comma"),
		
		str8_lit("StyxToken_CommentLine"),
		str8_lit("StyxToken_CommentBlock"),
		str8_lit("StyxToken_Whitespace"),
		
		str8_lit("StyxToken_ParentheticalOpen"),
		str8_lit("StyxToken_ParentheticalClose"),
		str8_lit("StyxToken_BraceOpen"),
		str8_lit("StyxToken_BraceClose"),
		
		str8_lit("StyxToken_FeedRight"),
		str8_lit("StyxToken_FeedLeft"),
		
		str8_lit("StyxToken_TemplateDirective"),
		
		str8_lit("StyxToken_EndOfFile"),
	};
	
	return token_type_str[type];
}

styx_inline void
tokenizer_token_inc_comment_line(StyxTokenizer *tokens)
{
	while (tokens->offset++ < tokens->file_data.len &&
	       tokens->file_data.str[tokens->offset] != '\n');
}

styx_inline void
tokenizer_token_inc_comment_block(StyxTokenizer *tokens)
{
	// Increment the tokenizer past the block comment open.
	tokens->offset++;
	while ((tokens->offset++ < tokens->file_data.len)) {
        if (tokens->file_data.str[tokens->offset] == '\n') {
            tokens->line_at++;
        }
        
		if (tokens->file_data.str[tokens->offset] == '*' &&
		    tokens->file_data.str[++tokens->offset] == '/') {
			break;
        }
	}
	// NOTE(sir->w7): The tokenizer works by calculating to the string's end
	// to the character before the offset. Therefore, this is to maintain
	// compatibility with the other increment functions.
	tokens->offset++;
}

styx_inline void
tokenizer_token_inc_whitespace(StyxTokenizer *tokens)
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

styx_inline void
tokenizer_token_inc_def(StyxTokenizer *tokens)
{
	// Delimiter is likely not even the right term for this.
	static char delimiters[] = {
		' ', '\n', '\r', '\t',
		'(', ')', '{', '}', 
		';', ',',
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

styx_function Str8
str8_get_token(StyxTokenType type, StyxTokenizer *tokens)
{
	if (type == StyxToken_EndOfFile) return (Str8){0};
	
	Str8 result = {0};
	int prev_offset = tokens->offset;
	result.str = tokens->file_data.str + prev_offset;
	
	switch (type) {
		case StyxToken_Whitespace: { tokenizer_token_inc_whitespace(tokens); } break;
		case StyxToken_CommentLine: { tokenizer_token_inc_comment_line(tokens); } break;
		case StyxToken_CommentBlock: { tokenizer_token_inc_comment_block(tokens); } break;
		
		case StyxToken_BraceOpen:
		case StyxToken_BraceClose:
		case StyxToken_ParentheticalOpen:
		case StyxToken_ParentheticalClose: { tokens->offset++; } break;
		
		default: { tokenizer_token_inc_def(tokens); } break;
	}
	
	result.len = tokens->offset - prev_offset;
	
	// Rewinds the offset to prepare for a tokenizer increment
	
	// NOTE(sir->w): Is this really necessary though? Is the tokenizer
	// increment any more than syntactic sugar?
	//tokens->offset--;
	tokens->offset--;
	
	return result;
}

styx_function StyxTokenizer
tokenizer_file(MemoryArena *allocator, Str8 filename)
{
	StyxTokenizer tokens = {0};
	
	tokens.file_data = read_file(allocator, filename);
	tokens.line_at = 1;
	
	return tokens;
}

styx_inline char
tokenizer_peek_next(StyxTokenizer *tokens)
{
	return tokens->file_data.str[tokens->offset + 1];
}

// TODO(sir->w7): When tokenizer->offset goes beyond the total length of the file, then we should have tokenizer_get_at return a null token.
// NOTE(sir->w7): Moves the tokenizer offset forward in order to grab the token. Should this be done, or should all incrementing be done by the tokenizer_inc_* functions?
styx_function StyxToken
tokenizer_get_at(StyxTokenizer *tokens)
{
	StyxToken tok = {0};
	//if (tokenizer->offset >= tokenizer->file_data.len) return token;
	
	tok.line = tokens->line_at;
	switch (tokens->file_data.str[tokens->offset]) {
        case '\0': { tok.type = StyxToken_EndOfFile; } break;
        case '@': { tok.type = StyxToken_TemplateDirective; } break;
		
        case ' ':
        case '\t':
		case '\r': { tok.type = StyxToken_Whitespace; } break;
		
        case '\n': { 
			tok.type = StyxToken_Whitespace;
			tokens->line_at++;
		} break;
		
		case '/': {
			if (tokenizer_peek_next(tokens) == '/') {
				tok.type = StyxToken_CommentLine;
			} else if (tokenizer_peek_next(tokens) == '*') {
				tok.type = StyxToken_CommentBlock;
			}
		} break;
		
        case ';': { tok.type = StyxToken_Semicolon; } break;
		case ',': { tok.type = StyxToken_Comma; } break;
		
        case '{': { tok.type = StyxToken_BraceOpen; } break;
        case '}': { tok.type = StyxToken_BraceClose; } break;
        case '(': { tok.type = StyxToken_ParentheticalOpen; } break;
        case ')': { tok.type = StyxToken_ParentheticalClose; } break;
		
        case '<': {
			if (tokenizer_peek_next(tokens) == '-') {
				tok.type = StyxToken_FeedLeft;
			}
        } break;
		
        case '-': {
			if (tokenizer_peek_next(tokens) == '>') { 
				tok.type = StyxToken_FeedRight;
			}
        } break;
		
        default: { tok.type = StyxToken_Identifier; } break;
	};
	
	tok.str = str8_get_token(tok.type, tokens);
	
	return tok;
}

styx_function StyxToken
tokenizer_inc_all(StyxTokenizer *tokens)
{
	tokens->offset++;
	return tokenizer_get_at(tokens);
}

styx_function StyxToken 
tokenizer_inc_no_whitespace(StyxTokenizer *tokens)
{
	StyxToken tok = tokenizer_inc_all(tokens);
	while (tok.type != StyxToken_EndOfFile) {
		if (tok.type != StyxToken_Whitespace && tok.type != StyxToken_Semicolon &&
		    tok.type != StyxToken_CommentLine && tok.type != StyxToken_CommentBlock) {
			break;
		}
		tok = tokenizer_inc_all(tokens);
	}
	return tok;
}

styx_function void
print_token(StyxToken tok)
{
	printf("tok.type: %-25.*s\t", str8_exp(str8_token_type(tok.type)));
	printf("tok.str: %-16.*s\t", str8_exp(tok.str));
    printf("tok.line: %d", tok.line);
	printnl();
    
    // NOTE(sir->w7): Since we're not really going to debug whitespace as much at the moment.
#if 0
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
#endif 
}
