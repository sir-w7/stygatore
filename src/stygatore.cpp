// NOTE(sir->w7): We should dump all the code into one big file just to make everything easier  A big jumbo file.
#include "common.cpp"
#include "tokenizer.cpp"
#include "parser.cpp"

#define STYX_EXT "styxgen"

typedef struct CompilationSettings CompilationSettings;
struct CompilationSettings
{
	Str8 output_name;
};

styx_function void
handle_file(MemoryArena *temp_allocator, Str8 file_relpath)
{
    Str8 file_abspath = get_file_abspath(temp_allocator, file_relpath);
    
    Str8 working_dir = file_working_dir(file_abspath);
	Str8 base_name = file_base_name(file_abspath);
	Str8 ext = file_ext(file_abspath);
	
    debugln_str8var(file_abspath);
    debugln_str8var(working_dir);
    debugln_str8var(base_name);
    debugln_str8var(ext);
    
	CompilationSettings settings{};
	StyxSymbolTable table = create_symbol_table(temp_allocator);
	StyxTokenizer tokens = tokenizer_file(temp_allocator, file_abspath);
	
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
    
    debugln_str8var(settings.output_name);
    
    for (StyxSymbol *ref = table.references.head; ref; ref = ref->next) {
        printnl();
        symbol_print(*ref);
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
        printnl();
        printnl();
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
