#include <stdio.h>
#include <string.h>

int main()
{
    static char buffer[64];
    
    for (int i = 0; i < 100; ++i) {
        snprintf(buffer, 64, "generated/gen%d.styxgen", i);
        FILE *file = fopen(buffer, "w");
        
        snprintf(buffer, 64, "gen%d.h", i);
        fprintf(file, "@output %s\n\n", buffer);
        fprintf(file, 
                "// Is the <- feed operator even necessary, or just syntactic sugar?\n"
                "@template Vec4 <- T\n"
                "// Do we want to output comments like this into the output file?\n"
                "// No, the template file is meant for reading, while the\n"
                "// output file is mostly for compiler powerlifting.\n"
                "\n"
                "/* This is a block comment to test out my\n"
                "pro skills block comment tokenizing ability. */\n"
                "\n"
                "typedef struct @t_name @t_name;\n"
                "struct @t_name\n"
                "{\n"
                "\t@T x;\n"
                "\t@T y;\n"
                "\t@T z;\n"
                "\t@T w;\n"
                "};\n"
                "\n"
                "@template Vec3 <- (T, new, norm)\n"
                "typedef struct @t_name @t_name;\n"
                "struct @t_name\n"
                "{\n"
                "\t@T x;\n"
                "\t@T y;\n"
                "\t@T z;\n"
                "};\n"
                "\n"
                "static inline @t_name @new(@T x, @T y, @T z)\n"
                "{\n"
                "\treturn (@t_name){.x = x, .y = y, .z = z};\n"
                "}\n"
                "\n"
                "static inline @t_name @norm(@t_name vector)\n"
                "{\n"
                "\t@t_name result = {0};\n"
                "\n"
                "\tf32 len = sqrt((f32)(vector.x ^ 2) + (f32)(vector.y ^ 2) + (f32)(vector.z ^ 2));\n"
                "\tvector.x = (@T)((f32)vector.x / len);\n"
                "\tvector.y = (@T)((f32)vector.y / len);\n"
                "\tvector.z = (@T)((f32)vector.z / len);\n"
                "\n"
                "\treturn result;\n"
                "}\n"
                "\n"
                "@template Vec2 <- T\n"
                "typedef struct @t_name @t_name;\n"
                "struct @t_name\n"
                "{\n"
                "\t@T x;\n"
                "\t@T y;\n"
                "};\n"
                "\n"
                "@template Vec4 -> f32: Vec4f\n"
                "@template Vec4 -> u8: Vec4c\n"
                "\n"
                "@template Vec3 -> f32, new_vec3f, vec3f_norm: Vec3f\n"
                "@template Vec3 -> u8, new_vec3c, vec3c_norm: Vec3c\n"
                "\n"
                "@template Vec2 -> f32: Vec2f\n"
                "@template Vec2 -> u16: Vec2Coords\n"
                );
        
        fclose(file);
    }
    return 0;
}