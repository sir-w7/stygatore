/* TODO(winston): error output */
/* TODO(winston): efficient memory usage */
/* TODO(winston): one-pass lexing */

/* TODO(sir->w7): A strong powerful string type for easier parsing. */
// TODO(sir->w7): Template concatenation for even more powerful templates.

// TODO(sir->w7): Unity/jumbo compilation?
#include "common.h"
#include "platform.h"
#include "tokenizer.h"

#define STYX_EXT "styxgen"

//--------------------------------------------------------------------
//-----------------------------Stygatore------------------------------
//--------------------------------------------------------------------

void handle_file(struct memory_arena *allocator,
                 struct memory_arena *temp_allocator,
                 struct str8 file)
{
	struct str8 working_dir = file_working_dir(file);
	struct str8 base_name = file_base_name(file);
	struct str8 ext = file_ext(file);
    
	println("full path: %.*s", str8_exp(file));
	println("working_dir: %.*s", str8_exp(working_dir));
	println("base_name: %.*s", str8_exp(base_name));
	println("ext: %.*s", str8_exp(ext));
    
#if 0
	struct tokenizer tokenizer = tokenizer_file(allocator, file);
	for (struct token token = get_tokenizer_at(&tokenizer);
	     token.type != TOKEN_END_OF_FILE;
	     token = tokenizer_inc_no_whitespace(&tokenizer)) {
		print_token(token);
	}
#endif
    
	printnl();
}

void handle_dir(struct memory_arena *allocator,
                struct memory_arena *temp_allocator,
                struct str8 dir)
{
	struct str8list files = 
		get_dir_list_ext(allocator, dir, str8_lit(STYX_EXT));
	//struct str8list files = get_dir_list_ext(allocator, dir, str8_lit("c"));
	for (struct str8node *file = files.head; file; file = file->next) {
		handle_file(allocator, temp_allocator, file->data);
	}
}

int main(int argc, char **argv)
{
	if (argc == 1) {
		println("stygatore is a sane, performant metaprogramming tool for language-agnostic generics\n"
                "with readable diagnostics and viewable output free from compiler/vendor control\n"
                "for maximum developer productivity.");
		printnl();
		println("Usage: %s [files/directories]", argv[0]);
	}
    
	struct memory_arena allocator = init_arena(megabytes(256));
	struct memory_arena temp_allocator = init_arena(megabytes(512));
    
	struct str8list args = arg_list(&allocator, argc, argv);
	for (struct str8node *arg = args.head; arg; arg = arg->next) {
		arena_reset(&temp_allocator);
		if (is_file(arg->data)) {
			handle_file(&allocator, &temp_allocator, arg->data);
		} else if (is_dir(arg->data)) {
			handle_dir(&allocator, &temp_allocator, arg->data);
		} else {
			fprintln(stderr, "Argument is neither a file nor a directory.");
		}
	}
    
	free_arena(&temp_allocator);
	free_arena(&allocator);
}
