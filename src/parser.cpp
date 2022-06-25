#include "common.h"
#include "tokenizer.h"
#include "parser.h"

void StyxSymbolTable::push(MemoryArena *allocator, StyxSymbol sym)
{
    if (sym.type == Symbol_Declaration) {
        auto bucket = djb2_hash(sym.declaration.identifier);
        auto idx = bucket % capacity;
        
        if (str8_is_nil(syms[idx].declaration.identifier)) {
            syms[idx] = sym;
            size++;
            return;
        }
        
        StyxSymbol *sym_new = (StyxSymbol *)allocator->push(sizeof(StyxSymbol));
        
        StyxSymbol *tail = &syms[idx];
        while (tail->next != NULL) tail = tail->next;
        
        tail->next = sym_new;
        size++;
    } else {
        if (references.head == nullptr) {
            references.head = (StyxSymbol *)allocator->push(sizeof(StyxSymbol));
            *references.head = sym;
            references.tail = references.head;
            return;
        }
        
        auto new_node = (StyxSymbol *)allocator->push(sizeof(StyxSymbol));
        *new_node = sym;
        
        references.tail->next = new_node;
        references.tail = new_node;
    }
}

StyxSymbol StyxSymbolTable::lookup(Str8 identifier)
{
    auto bucket = djb2_hash(identifier);
    auto idx = bucket % capacity;
    
    // TODO(sir->w7): Account for collisions.
    return syms[idx];
}

// TODO(sir->w7): Parsing error processing.
void StyxSymbol::parse_declaration(StyxTokenizer *tokens, MemoryArena *allocator,
                                   Str8 tok_identifier, u64 tok_line)
{
    this->type = Symbol_Declaration;
    this->declaration.identifier = tok_identifier;
    this->declaration.line = tok_line;
    
    auto tok = tokens->inc_no_whitespace();
    if (tok.type == Token_ParentheticalOpen) {
        while ((tok = tokens->inc_no_whitespace()).type != 
               Token_ParentheticalClose) {
            if (tok.type == Token_Comma) {
                continue;
            }
            this->declaration.params.push(allocator, tok.str);
        }
    } else {
        this->declaration.params.push(allocator, tok.str);
    }
    
    // NOTE(sir->w7): Eat all whitespace
    //while ((tok = tokenizer_inc_all(tokens)).type == Token_Whitespace);
    tok = tokens->inc_no_whitespace();
    // TODO(sir->w7): This is a very small penis expandable array implementation. Don't ever do this again, and don't leave this here when shipping.
    this->declaration.definition =
        reinterpret_cast<StyxToken *>(allocator->mem + allocator->offset);
    
    // TODO(sir->w7): Some decrement tokenizer feature so we don't need to do restore the tokenizer state to set it up for an increment;
    StyxTokenizerState prev_state(tokens);
    for (; !tok.known_styx_directive() && tok.type != Token_EndOfFile;
         tok = tokens->inc_no_comment()) {
        StyxToken *tok_ptr = (StyxToken *)allocator->push_pack(sizeof(StyxToken));
        *tok_ptr = tok;
        this->declaration.tok_count++;
        prev_state = StyxTokenizerState(tokens);
    }
    
    // NOTE(sir->w7): Eat all whitespace at the end.
    for (auto i = this->declaration.tok_count - 1;
         this->declaration.definition[i].type == Token_Whitespace;
         --i) {
        this->declaration.tok_count--;
    }
}

void StyxSymbol::parse_reference(StyxTokenizer *tokens, MemoryArena *arena,
                                 Str8 tok_identifier, u64 tok_line)
{
    this->type = Symbol_Reference;
    this->reference.identifier = tok_identifier;
    this->reference.line = tok_line;
    
    auto tok = tokens->inc_no_whitespace();
    do {
        if (tok.type == Token_Comma) {
            continue;
        } 
        this->reference.args.push(arena, tok.str);
    } while ((tok = tokens->inc_no_whitespace()).type != Token_Colon);
    
    tok = tokens->inc_no_whitespace();
    this->reference.gen_name = tok.str;
}

StyxSymbol::StyxSymbol(MemoryArena *allocator, StyxTokenizer *tokens)
{
    // How about let's just zero it from here and keep it simple?
    memory_set(this, 0, sizeof(StyxSymbol));

    auto tok = tokens->inc_no_whitespace();
    
    auto tok_identifier = tok.str;
    auto tok_line = tok.line;
    
    tok = tokens->inc_no_whitespace();
    if (tok.type == Token_FeedLeft) {
        parse_declaration(tokens, allocator, tok_identifier, tok_line);
    } else {
        parse_reference(tokens, allocator, tok_identifier, tok_line);
    }
}

void symbol_print(StyxSymbol sym)
{
    if (str8_is_nil(sym.declaration.identifier)) {
        println("sym: null");
        return;
    } 
    
    if (sym.type == Symbol_Declaration) {
        println("sym.type: Symbol_Declaration");
        println("sym.str: " str8_fmt, str8_exp(sym.declaration.identifier));
        println("sym.line: %llu", sym.declaration.line);
        for (Str8Node *param = sym.declaration.params.head; param; param = param->next) {
            debugln_str8var(param->data);
        }
        
        println("sym.definition:");
        for (u64 i = 0; i < sym.declaration.tok_count; ++i) {
            sym.declaration.definition[i].print();
        }
    } else {
        println("sym.type: Symbol_Reference");
        println("sym.str: " str8_fmt, str8_exp(sym.reference.identifier));
        println("sym.line: %llu", sym.reference.line);
        for (Str8Node *arg = sym.reference.args.head; arg; arg = arg->next) {
            debugln_str8var(arg->data);
        }
        
        debugln_str8var(sym.reference.gen_name);
    }
}