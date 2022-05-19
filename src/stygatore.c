/* TODO(winston): error output */
/* TODO(winston): efficient memory usage */
/* TODO(winston): one-pass lexing */

/* TODO(sir->w7): A strong powerful string type for easier parsing. */
// TODO(sir->w7): Template concatenation for even more powerful templates.

#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "common.h"
#include "platform.h"

//--------------------------------------------------------------------
//-----------------------------Stygatore------------------------------
//--------------------------------------------------------------------
enum token_type
{
	token_unknown,

	token_code,
	token_comment,
	token_feed_right,
	token_feed_left,

	token_template_directive,
	token_template_definition,
	token_template_type_identifier,
	token_template_type,

	token_template_name,
	token_template_gen_name,

	token_template_statement_name,

	token_end_of_file,
};

struct token
{
	enum token_type type;
	struct str8 data;

	struct token *next;
};

struct token_list
{
	struct token *head, *tail;
	struct token *at;
};

void token_list_push(struct token_list *list, 
                     struct memory_arena *allocator, struct token token) 
{
	if (list->head == NULL) {
		list->head = arena_push_struct(allocator, struct token);
		memory_copy(list->head, &token, sizeof(struct token));

		list->tail = list->head;
		return;
	}

	struct token *new_node = arena_push_struct(allocator, struct token);
	memory_copy(new_node, &token, sizeof(struct token));

	list->tail->next = new_node;
	list->tail = new_node;
}

struct token_list
index_file(struct memory_arena *allocator, struct str8 filename)
{
	struct token_list tokenizer = {0};

	struct str8 file_data = read_file(allocator, filename);
	if (file_data.len == 0) return tokenizer;

	for (int i = 0; i < file_data.len; ++i) {
		//struct token token = {0};
		char ch = file_data.str[i];

		switch (ch) {
		case '@':
			break;
		default:
			break;
		};
	}

	return tokenizer;
}

void handle_file(struct memory_arena *allocator,
		 struct memory_arena *temp_allocator,
		 struct str8 file)
{
	struct str8 working_dir = file_working_dir(file);
	struct str8 base_name = file_base_name(file);
	struct str8 ext = file_ext(file);

	println("working_dir: %.*s", str8_exp(working_dir));
	println("base_name: %.*s", str8_exp(base_name));
	println("ext: %.*s", str8_exp(ext));

	printnl();
}

void handle_dir(struct memory_arena *allocator,
		struct memory_arena *temp_allocator,
		struct str8 dir)
{
	struct str8list files = get_dir_list_ext(allocator, dir, str8_lit("styxgen"));
	for (struct str8node *file = files.head; file; file = file->next) {
		handle_file(allocator, temp_allocator, file->data);
	}
}

int main(int argc, char **argv)
{
	struct memory_arena allocator = init_arena(megabytes(256));
	struct memory_arena temp_allocator = init_arena(megabytes(512));

	struct str8list args = arg_list(&allocator, argc, argv);
	for (struct str8node *arg = args.head; arg; arg = arg->next) {
		arena_reset(&temp_allocator);
		if (is_file(arg->data)) {
			handle_file(&allocator, &temp_allocator, arg->data);
		} else if (is_dir(arg->data)) {
			handle_dir(&allocator, &temp_allocator, arg->data);	
		}
	}

	free_arena(&temp_allocator);
	free_arena(&allocator);
}
