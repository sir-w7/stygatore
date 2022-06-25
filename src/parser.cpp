#include "common.h"
#include "tokenizer.h"
#include "parser.h"

StyxDeclaration::StyxDeclaration(StyxTokenizer *tokens, MemoryArena *allocator,
                                 Str8 tok_identifier, u64 tok_line)
{
    identifier = tok_identifier;
    line = tok_line;
    
    auto tok = tokens->inc_no_whitespace();
    if (tok.type == Token_ParentheticalOpen) {
        while ((tok = tokens->inc_no_whitespace()).type != Token_ParentheticalClose) {
            if (tok.type == Token_Comma) {
                continue;
            }
            params.push(allocator, tok.str);
        }
    } else {
        params.push(allocator, tok.str);
    }
    
    // NOTE(sir->w7): Eat all whitespace.
    tok = tokens->inc_no_whitespace();
    // TODO(sir->w7): This is a very small penis expandable array implementation. Don't ever do this again, and don't leave this here when shipping.
    definition = reinterpret_cast<StyxToken *>(allocator->mem + allocator->offset);
    
    // TODO(sir->w7): Some decrement tokenizer feature so we don't need to do restore the tokenizer state to set it up for an increment;
    StyxTokenizerState prev_state(tokens);
    for (; !tok.known_styx_directive() && tok.type != Token_EndOfFile;
         tok = tokens->inc_no_comment()) {
        auto tok_ptr = (StyxToken *)allocator->push_pack(sizeof(StyxToken));
        *tok_ptr = tok;
        tok_count++;
        prev_state = StyxTokenizerState(tokens);
    }
    
    // NOTE(sir->w7): Eat all whitespace at the end.
    for (auto i = tok_count - 1; definition[i].type == Token_Whitespace; --i) 
        tok_count--;
}

StyxReference::StyxReference(StyxTokenizer *tokens, MemoryArena *arena,
                             Str8 tok_identifier, u64 tok_line)
{
    identifier = tok_identifier;
    line = tok_line;

    auto tok = tokens->inc_no_whitespace();
    do {
        if (tok.type == Token_Comma)
            continue;
        args.push(arena, tok.str);
    } while ((tok = tokens->inc_no_whitespace()).type != Token_Colon);

    tok = tokens->inc_no_whitespace();
    gen_name = tok.str;
}

void StyxDeclaration::print()
{
    if (str8_is_nil(identifier)) {
        println("sym: (null)");
        return;
    }

    println("type: Symbol_Declaration");
    println("sym_str: " str8_fmt, str8_exp(identifier));
    println("sym_line: %llu", line);
    for (auto param = params.head; param; param = param->next) {
        debugln_str8var(param->data);
    }
    
    println("sym.definition:");
    for (u64 i = 0; i < tok_count; ++i) {
        definition[i].print();
    }
}

void StyxReference::print()
{
    if (str8_is_nil(identifier)) {
        println("sym: (null)");
        return;
    }

    println("type: Symbol_Reference");
    println("sym.str: " str8_fmt, str8_exp(identifier));
    println("sym.line: %llu", line);
    for (auto arg = args.head; arg; arg = arg->next) {
        debugln_str8var(arg->data);
    }
    
    debugln_str8var(gen_name);
}

void StyxSymbolTable::push(MemoryArena *allocator, StyxSymbol *symptr)
{
    auto declaration = dynamic_cast<StyxDeclaration *>(symptr);
    auto reference = dynamic_cast<StyxReference *>(symptr);

    if (declaration != nullptr) {
        auto bucket = djb2_hash(declaration->identifier);
        auto idx = bucket % capacity;

        syms[idx] = declaration;
        size++;
        // Account for hash collisions.
    } else {
        if (references.head == nullptr) {
            references.head = reference;
            references.tail = references.head;
            return;
        }
        
        references.tail->next = reference;
        references.tail = reference;
    }
}

StyxSymbol *StyxSymbolTable::lookup(Str8 identifier)
{
    auto bucket = djb2_hash(identifier);
    auto idx = bucket % capacity;
    
    // TODO(sir->w7): Account for collisions.
    return syms[idx];
}

StyxSymbol *parse_next(StyxTokenizer *tokens, MemoryArena *allocator)
{
    // How about let's just zero it from here and keep it simple?
    auto tok = tokens->inc_no_whitespace();
    
    auto tok_identifier = tok.str;
    auto tok_line = tok.line;
    
    tok = tokens->inc_no_whitespace();

    if (tok.type == Token_FeedLeft) {
        auto declaration = StyxDeclaration(tokens, allocator, tok_identifier, tok_line);
        auto sym = (StyxDeclaration *)allocator->push_initialize(sizeof(StyxDeclaration), &declaration);
        return sym;
    } else {
        auto reference = StyxReference(tokens, allocator, tok_identifier, tok_line);
        auto sym = (StyxReference *)allocator->push_initialize(sizeof(StyxReference), &reference);
        return sym;
    }
}