#ifndef PARSER_H
#define PARSER_H

struct StyxSymbol
{
    Str8 identifier;
    u64 line = 0;

    StyxSymbol *next = nullptr;

    virtual void print() {}
};

struct StyxDeclaration: public StyxSymbol
{
    Str8List params;

    StyxToken *definition = nullptr;
    u64 tok_count = 0;

    StyxDeclaration(StyxTokenizer *tokens, MemoryArena *allocator, Str8 tok_identifier, u64 tok_line);
    void print() override;
};

struct StyxReference: public StyxSymbol
{
    Str8List args;
    Str8 gen_name;

    StyxReference(StyxTokenizer *tokens, MemoryArena *allocator, Str8 tok_identifier, u64 tok_line);
    void print() override;
};

#define INITIAL_CAPACITY 32
#define GROWTH_RATE 2

struct StyxSymbolTable
{
	StyxDeclaration **syms = nullptr;
    
	struct {
        StyxReference *head = nullptr;
        StyxReference *tail = nullptr;
    } references;
    
	u32 size = 0;
	u32 capacity = INITIAL_CAPACITY;

    StyxSymbolTable(MemoryArena *allocator) {
        syms = (StyxDeclaration **)allocator->push_array(sizeof(StyxDeclaration *), capacity);
    }

    void push(MemoryArena *allocator, StyxSymbol *sym);
    StyxSymbol *lookup(Str8 identifier);
};

StyxSymbol *parse_next(StyxTokenizer *tokenizer, MemoryArena *allocator);

#endif