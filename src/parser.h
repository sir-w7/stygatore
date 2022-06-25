#ifndef PARSER_H
#define PARSER_H

// NOTE(sir->w7): Is runtime polymorphism really necessary here?
enum StyxSymbolType
{
    Symbol_Declaration,
    Symbol_Reference,
};

struct StyxSymbol
{
    StyxSymbolType type;

    Str8 identifier;
    u64 line;

    StyxSymbol *next;

    virtual void parse();
};

struct StyxDeclaration: public StyxSymbol
{
    Str8List params;

    StyxToken *definition;
    u64 tok_count;

    void parse();
};

struct StyxReference: public StyxSymbol
{
    Str8List args;
    Str8 gen_name;

    void parse();
};

struct StyxSymbol
{
    StyxSymbolType type;
    StyxSymbol *next;
    
    union {
        struct {
            Str8 identifier;
            u64 line;
            
            Str8List params;
            
            StyxToken *definition;
            u64 tok_count;
        } declaration;
        struct {
            Str8 identifier;
            u64 line;
            
            Str8List args;
            Str8 gen_name;
        } reference;
    };

    StyxSymbol() {}
    StyxSymbol(MemoryArena *allocator, StyxTokenizer *tokens);
private:
    void parse_declaration(StyxTokenizer *tokens, MemoryArena *allocator, Str8 tok_identifier, u64 tok_line);
    void parse_reference(StyxTokenizer *tokens, MemoryArena *allocator, Str8 tok_identifier, u64 tok_line);
};

#define INITIAL_CAPACITY 32
#define GROWTH_RATE 2

struct StyxSymbolTable
{
	StyxSymbol *syms = nullptr;
    
	struct {
        StyxSymbol *head = nullptr;
        StyxSymbol *tail = nullptr;
    } references;
    
	u32 size = 0;
	u32 capacity = INITIAL_CAPACITY;

    StyxSymbolTable(MemoryArena *allocator) {
        syms = (StyxSymbol *)allocator->push_array(sizeof(StyxSymbol), capacity);
    }
    void push(MemoryArena *allocator, StyxSymbol sym);
    StyxSymbol lookup(Str8 identifier);
};

void symbol_print(StyxSymbol sym);

#endif
