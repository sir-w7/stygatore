#ifndef TOKENIZER_H
#define TOKENIZER_H

typedef enum StyxTokenType StyxTokenType;
enum StyxTokenType
{
	StyxToken_Unknown,
	StyxToken_Identifier,
	
	StyxToken_Semicolon,
	StyxToken_Comma,
	
	StyxToken_CommentLine,
	StyxToken_CommentBlock,
	StyxToken_Whitespace,
	
	StyxToken_ParentheticalOpen,
	StyxToken_ParentheticalClose,
	StyxToken_BraceOpen,
	StyxToken_BraceClose,
	
	StyxToken_FeedRight,
	StyxToken_FeedLeft,
	
	StyxToken_TemplateDirective,
	
	StyxToken_EndOfFile,
};

typedef struct StyxToken StyxToken;
struct StyxToken
{
	StyxTokenType type;
	Str8 str;
	
	u32 line;
};

typedef struct StyxTokenizer StyxTokenizer;
struct StyxTokenizer
{
	Str8 file_data;
	
	u32 offset;
	u32 line_at;
};

styx_function Str8 str8_get_token(StyxTokenType type, StyxTokenizer *tokens);
styx_function StyxTokenizer tokenizer_file(MemoryArena *allocator, Str8 filename);
styx_function StyxToken tokenizer_get_at(StyxTokenizer *tokens);
styx_function void print_token(StyxToken token);

styx_function StyxToken tokenizer_inc_all(StyxTokenizer *tokens);
styx_function StyxToken tokenizer_inc_no_whitespace(StyxTokenizer *tokens);

#endif
