// NOTE(sir->w7): We should dump all the code into one big file just to make everything easier  A big jumbo file.
#include "common.cpp"
#include "tokenizer.cpp"
#include "parser.cpp"

#define STYX_EXT "styxgen"

struct CompilationSettings
{
	Str8 output_name;
};

styx_function void
insert_comment(FILE *file, Str8 ext, Str8 comment)
{
    // Comments for C-based languages only.
    // TODO(sir->w7): Implement comments as a template directive (like @output).
    if (str8_compare(ext, str8_lit("h")) ||
        str8_compare(ext, str8_lit("c")) ||
        str8_compare(ext, str8_lit("cpp"))) {
        fprintf(file, "// ");
        fprintf(file, "%.*s", str8_exp(comment));
        fprintf(file, "\n");
    }
}

styx_function void
handle_file(MemoryArena *temp_allocator, Str8 file_relpath)
{
    Str8 file_abspath = get_file_abspath(temp_allocator, file_relpath);
    
    Str8 working_dir = file_working_dir(file_abspath);
	Str8 base_name = file_base_name(file_abspath);
    Str8 filename = file_name(file_abspath);
	Str8 ext = file_ext(file_abspath);
	
	CompilationSettings settings{};
	StyxSymbolTable table = create_symbol_table(temp_allocator);
	StyxTokenizer tokens = tokenizer_file(temp_allocator, file_abspath);
	
    auto time_start = get_time();
    defer {
        printf("%.*s -> %.*s",
               str8_exp(filename), str8_exp(settings.output_name));
        printf(" <- %.03f ms", get_time() - time_start);
        printnl();
    };
    
    for (StyxToken tok = tokenizer_get_at(&tokens);
         tok.type != Token_EndOfFile;
         tok = tokenizer_inc_no_whitespace(&tokens)) {
        if (tok.type == Token_StyxDirective) {
            if (str8_compare(tok.str, str8_lit("@output"))) {
                StyxToken next_tok = tokenizer_inc_no_whitespace(&tokens);
                settings.output_name = push_str8_copy(temp_allocator, next_tok.str);
            } else if (str8_compare(tok.str, str8_lit("@template"))) {
                StyxSymbol sym = parse_symbol(&tokens, temp_allocator);
                symbol_table_push(&table, temp_allocator, sym);
            }
        }
    }
    
    auto output_filename = push_str8_concat(temp_allocator, working_dir, settings.output_name);
    
    FILE *file{};
    fopen_s(&file, output_filename.str, "w");
    defer { fclose(file); };
    
    insert_comment(file, file_ext(settings.output_name), 
                   str8_lit("Courtesy of Stygatore."));
    fprintf(file, "\n");
    
    for (StyxSymbol *ref = table.references.head;
         ref;
         ref = ref->next) {
        StyxSymbol template_symbol = symbol_table_lookup(&table, ref->reference.identifier);
        
        auto param_count = template_symbol.declaration.params.count;
        auto arg_count = ref->reference.args.count;
        
        if (param_count != arg_count) {
            fprintln(stderr, "Incorrect number of arguments for parameters.");
            continue;
        }
        
        // TODO(sir->w7): Cache this for multiple references to the same template.
        auto params = arena_push_struct_array(temp_allocator, Str8, param_count);
        auto args = arena_push_struct_array(temp_allocator, Str8, arg_count);
        
        Str8Node *param_node = template_symbol.declaration.params.head;
        Str8Node *arg_node = ref->reference.args.head;
        for (u32 i = 0; i < param_count; ++i) {
            params[i] = param_node->data;
            args[i] = arg_node->data;
            
            param_node = param_node->next;
            arg_node = arg_node->next;
        }
        
        {
            TempArena scratch = begin_temp_arena(temp_allocator);
            defer { end_temp_arena(&scratch); };
            Str8 comment = push_str8_copy(temp_allocator, ref->reference.identifier);
            comment = push_str8_concat(temp_allocator, comment, str8_lit(" -> "));
            
            for (u32 i = 0; i < arg_count; ++i) {
                comment = push_str8_concat(temp_allocator, comment, args[i]);
                if (i < arg_count - 1) {
                    comment = push_str8_concat(temp_allocator, comment, str8_lit(", "));
                }
            }
            
            comment = push_str8_concat(temp_allocator, comment, str8_lit(": "));
            comment = push_str8_concat(temp_allocator, comment, ref->reference.gen_name);
            
            insert_comment(file, file_ext(settings.output_name), comment);
        }
        
        for (u64 i = 0; i < template_symbol.declaration.tok_count; ++i) {
            auto tok = template_symbol.declaration.definition[i];
            if (tok.type == Token_StyxDirective) {
                Str8 dir_str{tok.str.str + 1, tok.str.len - 1};
                if (str8_compare(dir_str, str8_lit("t_name"))) {
                    fprintf(file, "%.*s", str8_exp(ref->reference.gen_name));
                } else {
                    u32 idx = 0;
                    for (; idx < param_count; ++idx) {
                        if (str8_compare(params[idx], dir_str)) {
                            break;
                        }
                    }
                    fprintf(file, "%.*s", str8_exp(args[idx]));
                }
                continue;
            }
            fprintf(file, "%.*s", str8_exp(tok.str));
        }
        
        fprintf(file, "\n\n");
    }
}

styx_function void
handle_dir(MemoryArena *allocator, MemoryArena *temp_allocator,
           Str8 dir)
{
	Str8List files = get_dir_list_ext(allocator, dir, str8_lit(STYX_EXT));
	
	for (Str8Node *file = files.head; file; file = file->next) {
        arena_reset(temp_allocator);
		handle_file(temp_allocator, file->data);
	}
}

int main(int argc, char **argv)
{
	if (argc == 1) {
		println("stygatore is a sane, performant metaprogramming tool for language-agnostic generics with readable diagnostics for maximum developer productivity.");
		printnl();
		println("Usage: %s [files/directories]", argv[0]);
	}
	
	MemoryArena allocator = init_arena(megabytes(256));
    defer { free_arena(&allocator); };
	MemoryArena temp_allocator = init_arena(megabytes(512));
	defer { free_arena(&temp_allocator); };
    
	Str8List args = arg_list(&allocator, argc, argv);
	for (Str8Node *arg = args.head; arg; arg = arg->next) {
		arena_reset(&temp_allocator);
		if (is_file(arg->data)) {
			handle_file(&temp_allocator, arg->data);
		} else if (is_dir(arg->data)) {
			handle_dir(&allocator, &temp_allocator, arg->data);
		} else {
			fprintln(stderr, "Argument is neither a file nor a directory.");
		}
	}
}
