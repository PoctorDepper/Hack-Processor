#include "file_lines.h"
#include "translator.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#ifdef _WIN32
#define stat _stat
#endif

typedef struct vm_file {
        const char* file_name;
        file_lines* lines;
} vm_file;

// Modifies filepath to be the output path
void apply_write_path(char* filepath, const char delimiter)
{
        char* extension;
        if (delimiter == '/') {
                const char* file_ptr = extension = strrchr(filepath, delimiter);
                // Walk the file pointer backwards to get the folder name
                do
                        --file_ptr;
                while (*file_ptr && *file_ptr != '/');
                // Copy the folder name as the file name
                do {
                        *extension++ = *file_ptr++;
                }
                while (*file_ptr != '/');
        }
        else
        // Just append .asm to wherever the character is
                extension = strrchr(filepath, delimiter);
        // Apply the extension
        strncpy(extension, ".asm\0", 5);
}

// Reads a vm file to a file_lines output, free of comments or spaces
void read_file(char* filepath, file_lines* out, char* buffer, const unsigned short buffer_length)
{
        char* extension = filepath + strlen(filepath) - 3;
        if (!extension || strcmp(extension, ".vm")) {
                fprintf(stderr, "%s is not a VM file.\n", filepath);
                return;
        }
        FILE* in = fopen(filepath, "r");
        if (!in) {
                fprintf(stderr, "%s couldn't be opened.\n", filepath);
                return;
        }
        while (fgets(buffer, buffer_length, in)) {
                const char* line_ptr = buffer;
                while (isspace(*line_ptr))
                        line_ptr++;
                if (!*line_ptr || *line_ptr == '/')
                        continue;
                add_line(out, line_ptr);
        }
        fclose(in);
}

// Writes the assembly out to filepath
void write_assembly(const char* filepath, const file_lines* assembly)
{
        FILE* out = fopen(filepath, "w");
        if (!out) {
                fprintf(stderr, "%s couldn't be written to.\n", filepath);
                return;
        }
        char** out_ptr = assembly->line;
        char** out_end = assembly->line + assembly->length - 1;
        while (*out_ptr && out_ptr != out_end)
                fprintf(out, "%s\n", *out_ptr++);
        fprintf(out, "%s", *out_ptr);
        fclose(out);
}

int main(const int argc, const char* argv[])
{
        if (argc <= 1) {
                fprintf(stderr, "You must pass an argument.\n");
                return 1;
        }
        char filepath[512];
        char buffer[512];
        strcpy(filepath, argv[1]);
        file_lines* assembly = new_file_lines();
        struct stat buf;
        stat(argv[1], &buf);
        if (S_ISDIR(buf.st_mode)) {
                // Bootstrap and call Sys.init
                bootstrap(assembly, buffer);
                // Append a / to the end of filepath if not present
                if (*(strrchr(filepath, '/') + 1))
                        strcat(filepath, "/");
                // Grab a file from the directory
                struct dirent* entry;
                DIR* dir = opendir(filepath);

                while ((entry = readdir(dir))) {
                        file_lines* vm = new_file_lines();
                        sprintf(buffer, "%s%s", filepath, entry->d_name);
                        read_file(buffer, vm, buffer, sizeof(buffer));
                        translate(vm, assembly, entry->d_name, buffer);
                        free_file_lines(vm);
                }

                closedir(dir);
                apply_write_path(filepath, '/');
        }
        else {
                // Single file, no bootstrapping
                file_lines* vm = new_file_lines();
                read_file(filepath, vm, buffer, sizeof(buffer));
                translate(vm, assembly, filepath, buffer);
                free_file_lines(vm);
                apply_write_path(filepath, '.');
        }
        write_assembly(filepath, assembly);
        free_file_lines(assembly);
        return 0;
}
