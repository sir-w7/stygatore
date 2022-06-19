#ifndef TOKENIZER_H
#define TOKENIZER_H

enum StyxTokenType
{
	Token_Unknown,
	Token_Identifier,
	
	Token_Semicolon,
    Token_Colon,
	Token_Comma,
	
	Token_CommentLine,
	Token_CommentBlock,
	Token_Whitespace,
	
	Token_ParentheticalOpen,
	Token_ParentheticalClose,
	Token_BraceOpen,
	Token_BraceClose,
	
	Token_FeedRight,
	Token_FeedLeft,
	
	Token_TemplateDirective,
	
	Token_EndOfFile,
};

struct StyxToken
{
	StyxTokenType type;
	Str8 str;
	
	u64 line;
};

struct StyxTokenizer
{
	Str8 file_data;
	
	u32 offset;
    u32 next_offset;
    
	u32 line_at;
};

styx_function Str8 str8_get_token(StyxTokenType type, StyxTokenizer *tokens);
styx_function StyxTokenizer tokenizer_file(MemoryArena *allocator, Str8 filename);
styx_function StyxToken tokenizer_get_at(StyxTokenizer *tokens);
styx_function void print_token(StyxToken token);

styx_function StyxToken tokenizer_inc_all(StyxTokenizer *tokens);
styx_function StyxToken tokenizer_inc_no_whitespace(StyxTokenizer *tokens);

#endif
