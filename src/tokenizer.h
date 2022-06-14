#ifndef TOKENIZER_H
#define TOKENIZER_H

enum token_type
{
	Token_Unknown,
	Token_Identifier,
	Token_Semicolon,

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

struct str8 str8_get_token(enum token_type type, struct tokenizer *tokens);
struct tokenizer tokenizer_file(struct memory_arena *allocator, struct str8 filename);
struct token tokenizer_get_at(struct tokenizer *tokens);
void print_token(struct token token);

struct token tokenizer_inc_all(struct tokenizer *tokens);
struct token tokenizer_inc_no_whitespace(struct tokenizer *tokens);

#endif
