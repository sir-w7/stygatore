#include "parser.h"

styx_function StyxSymbolTable
create_symbol_table(MemoryArena *arena)
{
	StyxSymbolTable table = {0};
	
	table.capacity = INITIAL_CAPACITY;
	table.syms = arena_push_struct_array(arena, StyxSymbol, table.capacity);
	
	return table;
}


styx_function void
symbol_table_push(StyxSymbolTable *table, 
                  MemoryArena *arena, StyxSymbol sym)
{
    if (sym.type == Symbol_Declaration) {
        u64 bucket = djb2_hash(sym.declaration.identifier);
        u64 idx = bucket % table->capacity;
        
        if (str8_is_nil(table->syms[idx].declaration.identifier)) {
            table->syms[idx] = sym;
            table->size++;
            return;
        }
        
        StyxSymbol *sym_new = arena_push_struct(arena, StyxSymbol);
        
        StyxSymbol *tail = &table->syms[idx];
        while (tail->next != NULL) tail = tail->next;
        
        tail->next = sym_new;
        table->size++;
    } else {
        if (table->references.head == nullptr) {
            table->references.head = arena_push_struct(arena, StyxSymbol);
            *table->references.head = sym;
            table->references.tail = table->references.head;
            return;
        }
        
        auto new_node = arena_push_struct(arena, StyxSymbol);
        *new_node = sym;
        
        table->references.tail->next = new_node;
        table->references.tail = new_node;
    }
}

styx_function StyxSymbol
symbol_table_lookup(StyxSymbolTable *table, Str8 identifier)
{
    auto bucket = djb2_hash(identifier);
    auto idx = bucket % table->capacity;
    
    // TODO(sir->w7): Account for collisions.
    return table->syms[idx];
}

// TODO(sir->w7): Parsing error processing.
styx_function StyxSymbol
parse_symbol_declaration(StyxTokenizer *tokens, MemoryArena *arena,
                         Str8 tok_identifier, u64 tok_line)
{
    StyxSymbol sym{};
    
    sym.type = Symbol_Declaration;
    sym.declaration.identifier = tok_identifier;
    sym.declaration.line = tok_line;
    
    auto tok = tokenizer_inc_no_whitespace(tokens);
    if (tok.type == Token_ParentheticalOpen) {
        while ((tok = tokenizer_inc_no_whitespace(tokens)).type != 
               Token_ParentheticalClose) {
            if (tok.type == Token_Comma) {
                continue;
            }
            str8list_push(&sym.declaration.params, arena, tok.str);
        }
    } else {
        str8list_push(&sym.declaration.params, arena, tok.str);
    }
    
    // NOTE(sir->w7): Eat all whitespace
    while ((tok = tokenizer_inc_all(tokens)).type == Token_Whitespace);
    
    // TODO(sir->w7): This is a very small penis expandable array implementation. Don't ever do this again, and don't leave this here when shipping.
    sym.declaration.definition = reinterpret_cast<StyxToken *>(arena->mem + arena->curr_offset);
    
    // TODO(sir->w7): Some decrement tokenizer feature so we don't need to do restore the tokenizer state to set it up for an increment;
    auto prev_state = store_tokenizer_state(tokens);
    for (; !known_styx_directive(tok) && tok.type != Token_EndOfFile;
         tok = tokenizer_inc_all(tokens)) {
        StyxToken *tok_ptr = static_cast<StyxToken *>(arena_push_pack(arena, sizeof(StyxToken)));
        *tok_ptr = tok;
        sym.declaration.tok_count++;
        prev_state = store_tokenizer_state(tokens);
    }
    
    // NOTE(sir->w7): Eat all whitespace at the end.
    for (auto i = sym.declaration.tok_count - 1;
         sym.declaration.definition[i].type == Token_Whitespace;
         --i) {
        sym.declaration.tok_count--;
    }
    
    restore_tokenizer_state(prev_state, tokens);
    
    return sym;
}

styx_function StyxSymbol
parse_symbol_reference(StyxTokenizer *tokens, MemoryArena *arena,
                       Str8 tok_identifier, u64 tok_line)
{
    StyxSymbol sym{};
    sym.type = Symbol_Reference;
    sym.reference.identifier = tok_identifier;
    sym.reference.line = tok_line;
    
    auto tok = tokenizer_inc_no_whitespace(tokens);
    while ((tok = tokenizer_inc_no_whitespace(tokens)).type != 
           Token_Colon) {
        if (tok.type == Token_Comma) {
            continue;
        }
        str8list_push(&sym.reference.args, arena, tok.str);
    }
    
    tok = tokenizer_inc_no_whitespace(tokens);
    sym.reference.gen_name = tok.str;
    
    return sym;
}

styx_function StyxSymbol
parse_symbol(StyxTokenizer *tokens, MemoryArena *arena)
{
    auto tok = tokenizer_inc_no_whitespace(tokens);
    
    auto tok_identifier = tok.str;
    auto tok_line = tok.line;
    
    tok = tokenizer_inc_no_whitespace(tokens);
    if (tok.type == Token_FeedLeft) {
        return parse_symbol_declaration(tokens, arena, tok_identifier, tok_line);
    } else {
        return parse_symbol_reference(tokens, arena, tok_identifier, tok_line);
    }
}

styx_function void
symbol_print(StyxSymbol sym)
{
    if (str8_is_nil(sym.declaration.identifier)) {
        println("sym: null");
        return;
    } 
    
    if (sym.type == Symbol_Declaration) {
        println("sym.type: Symbol_Declaration");
        println("sym.str: " str8_fmt, str8_exp(sym.declaration.identifier));
        println("sym.line: %d", sym.declaration.line);
        for (Str8Node *param = sym.declaration.params.head; param; param = param->next) {
            debugln_str8var(param->data);
        }
        
        println("sym.definition:");
        for (u64 i = 0; i < sym.declaration.tok_count; ++i) {
            token_print(sym.declaration.definition[i]);
        }
    } else {
        println("sym.type: Symbol_Reference");
        println("sym.str: " str8_fmt, str8_exp(sym.reference.identifier));
        println("sym.line: %d", sym.reference.line);
        for (Str8Node *arg = sym.reference.args.head; arg; arg = arg->next) {
            debugln_str8var(arg->data);
        }
        
        debugln_str8var(sym.reference.gen_name);
    }
}