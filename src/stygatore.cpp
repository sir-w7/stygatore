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
	
	println("full path: " str8_fmt, str8_exp(file_abspath));
	println("working_dir: " str8_fmt, str8_exp(working_dir));
	println("base_name: " str8_fmt, str8_exp(base_name));
	println("ext: " str8_fmt, str8_exp(ext));
	
	CompilationSettings settings = {0};
	SymbolTable table = create_symbol_table(temp_allocator);
	StyxTokenizer tokens = tokenizer_file(temp_allocator, file_abspath);
	
	for (StyxToken tok = tokenizer_get_at(&tokens);
		 tok.type != StyxToken_EndOfFile;
		 tok = tokenizer_inc_no_whitespace(&tokens)) {
		print_token(tok);
        
		if (tok.type == StyxToken_TemplateDirective) {
			if (str8_compare(tok.str, str8_lit("@output"))) {
				// NOTE(sir->w7): If it is output directive.
				StyxToken next_tok = tokenizer_inc_no_whitespace(&tokens);
				settings.output_name = push_str8_copy(temp_allocator, next_tok.str);
			} else if (str8_compare(tok.str, str8_lit("@template"))) {
				// NOTE(sir->w7): If it is a template declaration symbol to be hashed.
				Symbol sym = {0};
			}
		}
	}
	
	println("Output file: " str8_fmt, str8_exp(settings.output_name));
	printnl();
}

styx_function void
handle_dir(MemoryArena *temp_allocator, Str8 dir)
{
	Str8List files = get_dir_list_ext(temp_allocator, dir, str8_lit(STYX_EXT));
	
	for (Str8Node *file = files.head; file; file = file->next) {
		handle_file(temp_allocator, file->data);
	}
}

int main(int argc, char **argv)
{
	if (argc == 1) {
		println("stygatore is a sane, performant metaprogramming tool for\n"
				"language-agnostic generics with readable diagnostics for maximum\n"
				"developer productivity.");
		printnl();
		println("Usage: %s [files/directories]", argv[0]);
	}
	
	MemoryArena allocator = init_arena(megabytes(256));
	MemoryArena temp_allocator = init_arena(megabytes(512));
	
	Str8List args = arg_list(&allocator, argc, argv);
	for (Str8Node *arg = args.head; arg; arg = arg->next) {
		arena_reset(&temp_allocator);
		if (is_file(arg->data)) {
			handle_file(&temp_allocator, arg->data);
		} else if (is_dir(arg->data)) {
			handle_dir(&temp_allocator, arg->data);
		} else {
			fprintln(stderr, "Argument is neither a file nor a directory.");
		}
	}
	
	free_arena(&temp_allocator);
	free_arena(&allocator);
}
