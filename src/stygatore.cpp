// NOTE(sir->w7): We should dump all the code into one big file just to make everything easier  A big jumbo file.
#include "common.h"
#include "tokenizer.h"
#include "parser.h"
#include "platform.h"

#define STYX_EXT "styxgen"

struct CompilationSettings
{
	Str8 output_name;
    Str8 comment_keyword;
};

void insert_comment(FILE *file, Str8 ext, Str8 comment)
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

void handle_file(MemoryArena *temp_allocator, Str8 file_relpath)
{
    auto file_abspath = get_file_abspath(temp_allocator, file_relpath);
    
    auto working_dir = file_working_dir(file_abspath);
	auto base_name = file_base_name(file_abspath);
    auto filename = file_name(file_abspath);
	auto ext = file_ext(file_abspath);
	
	CompilationSettings settings{};
	StyxSymbolTable table(temp_allocator);
	StyxTokenizer tokens(temp_allocator, file_abspath);
	
    auto time_start = get_time();
    
    for (auto tok = tokens.get_at();
         tok.type != Token_EndOfFile;
         tok = tokens.inc_no_whitespace()) {
        if (tok.type == Token_StyxDirective) {
            if (str8_compare(tok.str, str8_lit("@output"))) {
                auto next_tok = tokens.inc_no_whitespace();
                settings.output_name = Str8(temp_allocator, next_tok.str);
            } else if (str8_compare(tok.str, str8_lit("@template"))) {
                StyxSymbol sym(temp_allocator, &tokens);
                table.push(temp_allocator, sym);
            }
        }
    }
    
    auto output_filename = push_str8_concat(temp_allocator, working_dir, settings.output_name);
    
    auto file = fopen(output_filename.str, "w");
    defer { fclose(file); };
    
    insert_comment(file, file_ext(settings.output_name), 
                   str8_lit("Courtesy of Stygatore."));
    fprintf(file, "\n");
    
    for (auto ref = table.references.head;
         ref;
         ref = ref->next) {
        auto template_symbol = table.lookup(ref->reference.identifier);
        
        auto param_count = template_symbol.declaration.params.count;
        auto arg_count = ref->reference.args.count;
        
        if (param_count != arg_count) {
            fprintln(stderr, "Incorrect number of arguments for parameters.");
            continue;
        }
        
        // TODO(sir->w7): Cache this for multiple references to the same template.
        auto params = (Str8 *)temp_allocator->push_array(sizeof(Str8), param_count);
        auto args = (Str8 *)temp_allocator->push_array(sizeof(Str8), arg_count);

        auto param_node = template_symbol.declaration.params.head;
        auto arg_node = ref->reference.args.head;
        for (u32 i = 0; i < param_count; ++i) {
            params[i] = param_node->data;
            args[i] = arg_node->data;
            
            param_node = param_node->next;
            arg_node = arg_node->next;
        }
        
        {
            TempArena scratch(temp_allocator);
            auto comment = Str8(temp_allocator, ref->reference.identifier);
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
    printf("%.*s -> %.*s",
           str8_exp(filename), str8_exp(settings.output_name));
    printf(" <- %.03f ms", get_time() - time_start);
    printnl();
}

void handle_dir(MemoryArena *allocator, MemoryArena *temp_allocator,
                Str8 dir)
{
	auto files = get_dir_list_ext(allocator, dir, str8_lit(STYX_EXT));
	
	str8list_it(file, files) {
        temp_allocator->reset();
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
	
	MemoryArena allocator(megabytes(256));
	MemoryArena temp_allocator(megabytes(512));
    
	auto args = arg_list(&allocator, argc, argv);
	str8list_it(arg, args) {
		temp_allocator.reset();
		if (is_file(arg->data)) {
			handle_file(&temp_allocator, arg->data);
		} else if (is_dir(arg->data)) {
			handle_dir(&allocator, &temp_allocator, arg->data);
		} else {
			fprintln(stderr, "Argument is neither a file nor a directory.");
		}
	}

    return 0;
}