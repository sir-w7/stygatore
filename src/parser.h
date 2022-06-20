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
    StyxSymbol *next;
    
    union {
        struct {
            Str8 identifier;
            u64 line;
            
            Str8List args;
            
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
};

struct StyxSymbolTable
{
	StyxSymbol *syms;
    
	struct {
        StyxSymbol *head;
        StyxSymbol *tail;
    } references;
    
	u32 size;
	u32 capacity;
};

#define INITIAL_CAPACITY 32
#define GROWTH_RATE 2

styx_function StyxSymbolTable create_symbol_table(MemoryArena *arena);
styx_function void symbol_table_push(StyxSymbolTable *table, MemoryArena *arena, StyxSymbol sym);
styx_function StyxSymbol symbol_table_lookup(StyxSymbolTable *table, Str8 identifier);
styx_function StyxSymbol parse_symbol(StyxTokenizer *tokens, MemoryArena *arena);
styx_function void symbol_print(StyxSymbol sym);

#endif
