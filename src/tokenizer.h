#ifndef TOKENIZER_H
#define TOKENIZER_H

typedef enum TokenType TokenType;
enum TokenType
{
	Token_Unknown,
	Token_Identifier,
	
	Token_Semicolon,
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

typedef struct Token Token;
struct Token
{
	TokenType type;
	Str8 str;
};

typedef struct Tokenizer Tokenizer;
struct Tokenizer
{
	Str8 file_data;
	int offset;
};

Str8 str8_get_token(TokenType type, Tokenizer *tokens);
Tokenizer tokenizer_file(MemoryArena *allocator, Str8 filename);
Token tokenizer_get_at(Tokenizer *tokens);
void print_token(Token token);

Token tokenizer_inc_all(Tokenizer *tokens);
Token tokenizer_inc_no_whitespace(Tokenizer *tokens);

#endif
