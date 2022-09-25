#include "common.h"
#include "tokenizer.h"

static Str8
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

void StyxToken::print()
{
	printf("type: %-24.*s  ", str8_exp(str8_token_type(type)));
    
    printf("str: ");
    
#define PRINT_MAX 32
    if (str.len < PRINT_MAX) {
        u32 count = 0;
        for (u64 i = 0; i < str.len; ++i) {
            if (str.str[i] == '\t') {
                printf("\\t");
                count += 2;
            } else if (str.str[i] == '\n') {
                printf("\\n");
                count += 2;
            } else {
                printf("%c", str.str[i]);
                count++;
            }
        }
        
        for (u64 i = 0; i < PRINT_MAX - count; ++i) {
            printf(" ");
        }
    } else {
        for (u64 i = 0; i < PRINT_MAX - 3; ++i) {
            if (str.str[i] == '\t') {
                printf("\\t");
            } else if (str.str[i] == '\n') {
                printf("\\n");
            } else {
                printf("%c", str.str[i]);
            }
        }
        
        printf("...");
    }
    
    printf("  ");
    
    printf("line: %llu", line);
	printnl();
}

bool StyxToken::known_styx_directive()
{
    static const Str8 known_directives[] = {
        str8_lit("@output"),
        str8_lit("@template"),
    };
    
    for (auto str : known_directives) {
        if (str8_compare(str, this->str))
            return true;
    }
    
    return false;
}

inline void
StyxTokenizer::token_inc_comment_line()
{
	while (next_offset++ < file_data.len && 
	       file_data.str[next_offset] != '\n');
}

inline void
StyxTokenizer::token_inc_comment_block()
{
	// Increment the tokenizer past the block comment open.
	next_offset++;
	while (next_offset++ < file_data.len) {
        if (file_data.str[next_offset] == '\n') {
            line_at++;
        }
        
		if (file_data.str[next_offset] == '*' &&
		    file_data.str[++next_offset] == '/') {
			break;
        }
	}
	// NOTE(sir->w7): The tokenizer works by calculating to the string's end to the character before the offset. Therefore, this is to maintain compatibility with the other increment functions.
	next_offset++;
}

inline void
StyxTokenizer::token_inc_whitespace()
{
	static char whitespaces[] = {
		' ', '\n', '\r', '\t',
	};
    
    if (file_data.str[next_offset] == '\n') {
        line_at++;
    }
    
	// Used to do cool pointer increment while loop thing:
	// The classic sir->w7, but changed to other because more readable.
	while (next_offset++ < file_data.len) {
		for (int i = 0; i < array_count(whitespaces); ++i) {
			char whitespace = whitespaces[i];
			if (file_data.str[next_offset] != whitespace)
				return;
		}
	}
    
    if (file_data.str[next_offset] == '\n')
        line_at++;
}

inline void
StyxTokenizer::token_inc_def()
{
	// Delimiter is likely not even the right term for this.
	static char delimiters[] = {
		' ', '\n', '\r', '\t',
		'(', ')', '{', '}', 
		';', ':', ',',
	};
    
	while (next_offset++ < file_data.len) {
		for (int i = 0; i < array_count(delimiters); ++i) {
			char delimiter = delimiters[i];
			if (file_data.str[next_offset] == delimiter) {
				return;
			}
		}
	}
}

Str8 StyxTokenizer::get_token(StyxTokenType type)
{
	Str8 result{};

	if (type == Token_EndOfFile) return result;
	
	next_offset = offset;
	result.str = file_data.str + offset;
	
	switch (type) {
		case Token_Whitespace: { token_inc_whitespace(); } break;
		case Token_CommentLine: { token_inc_comment_line(); } break;
		case Token_CommentBlock: { token_inc_comment_block(); } break;
		
		case Token_BraceOpen:
		case Token_BraceClose:
		case Token_ParentheticalOpen:
		case Token_ParentheticalClose: { next_offset++; } break;
		
		default: { token_inc_def(); } break;
	}
	
	result.len = next_offset - offset;
	
	return result;
}

StyxToken StyxTokenizer::get_at()
{
	StyxToken tok{};
	
    tok.line = line_at;
	switch (file_data.str[offset]) {
        case '\0': { tok.type = Token_EndOfFile; } break;
        case '@': { tok.type = Token_StyxDirective; } break;
		
        case ' ':
        case '\t':
		case '\r': 
        case '\n': { tok.type = Token_Whitespace; } break;
		
		case '/': {
			if (file_data.str[offset + 1] == '/') {
				tok.type = Token_CommentLine;
			} else if (file_data.str[offset + 1] == '*') {
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
			if (file_data.str[offset + 1] == '-') {
				tok.type = Token_FeedLeft;
			}
        } break;
        case '-': {
			if (file_data.str[offset + 1] == '>') { 
				tok.type = Token_FeedRight;
			}
        } break;
		
        default: { tok.type = Token_Identifier; } break;
	};
	
    if (next_offset > offset) {
        tok.str = Str8(file_data.str + offset,
                       next_offset - offset);
    } else {
        tok.str = get_token(tok.type);
    }
    
	return tok;
}

StyxToken StyxTokenizer::inc_all()
{
    StyxToken result{};
	offset = next_offset;
	return get_at();
}

StyxToken StyxTokenizer::inc_no_whitespace()
{
	auto tok = inc_all();
	while (tok.type != Token_EndOfFile) {
		if (tok.type != Token_Whitespace && tok.type != Token_CommentLine && tok.type != Token_CommentBlock) {
			break;
		}
		tok = inc_all();
	}
	return tok;
}

StyxToken StyxTokenizer::inc_no_comment()
{
    auto tok = inc_all();
	while (tok.type != Token_EndOfFile) {
		if (tok.type != Token_CommentLine && tok.type != Token_CommentBlock) {
			break;
		}
		tok = inc_all();
	}
	return tok;
}