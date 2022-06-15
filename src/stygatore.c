/* TODO(winston): error output */
/* TODO(winston): efficient memory usage */
/* TODO(winston): one-pass lexing */

/* TODO(sir->w7): A strong powerful string type for easier parsing. */
// TODO(sir->w7): Template concatenation for even more powerful templates.

// TODO(sir->w7): Unity/jumbo compilation?
#include "common.h"
#include "platform.h"
#include "tokenizer.h"
#include "parser.h"

#define STYX_EXT "styxgen"

struct compilation_settings
{
	Str8 output_name;
};

void parse_token_at(Tokenizer *tokens,
					SymbolTable *table,
					struct compilation_settings *settings,
					MemoryArena *temp_allocator)
{
	Token tok = tokenizer_get_at(tokens);
	print_token(tok);
	
#if 0
	if (tok.type == Token_TemplateDirective) {
		if (str8_compare(tok.str, str8_lit("@output"))) {
			Token next_tok = tokenizer_inc_no_whitespace(tokens);
			settings->output_name =
				push_str8_copy(temp_allocator, next_tok.str);
		} else if (str8_compare(tok.str, str8_lit("@template"))) {
			Symbol sym = {0};
		}
	}
#endif
}

void handle_file(MemoryArena *temp_allocator,
				 Str8 file)
{
	Str8 working_dir = file_working_dir(file);
	Str8 base_name = file_base_name(file);
	Str8 ext = file_ext(file);
	
	println("full path: %.*s", str8_exp(file));
	println("working_dir: %.*s", str8_exp(working_dir));
	println("base_name: %.*s", str8_exp(base_name));
	println("ext: %.*s", str8_exp(ext));
	
	struct compilation_settings settings = {0};
	SymbolTable table = create_symbol_table(temp_allocator);
	Tokenizer tokens = tokenizer_file(temp_allocator, file);
	
	do {
		parse_token_at(&tokens, &table, &settings, temp_allocator);
	} while (tokenizer_inc_no_whitespace(&tokens).type != Token_EndOfFile);
	
	println("Output file: %.*s", str8_exp(settings.output_name));
	printnl();
}

void handle_dir(MemoryArena *temp_allocator,
                Str8 dir)
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
