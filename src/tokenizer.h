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
	
	Token_StyxDirective,
	
	Token_EndOfFile,
};

struct StyxToken
{
	StyxTokenType type;
	Str8 str;
	
	u64 line;

	void print();
	bool known_styx_directive();
};

struct StyxTokenizer
{
	Str8 file_data;
	
	u32 offset = 0;
    u32 next_offset = 0;
    
	u32 line_at = 1;

	StyxTokenizer(MemoryArena *allocator, Str8 filename) {
		file_data = read_file(allocator, filename);
	}

	StyxToken get_at();

	StyxToken inc_all();
	StyxToken inc_no_whitespace();
	StyxToken inc_no_comment();

private:
	inline void token_inc_comment_line();
	inline void token_inc_comment_block();
	inline void token_inc_whitespace();
	inline void token_inc_def();

	Str8 get_token(StyxTokenType type);
};

struct StyxTokenizerState
{
    u32 offset;
    u32 next_offset;
    u32 line_at;
};

StyxTokenizerState store_tokenizer_state(StyxTokenizer *tokens);
void restore_tokenizer_state(StyxTokenizerState state, StyxTokenizer *tokens);

bool known_styx_directive(StyxToken token);

#endif
