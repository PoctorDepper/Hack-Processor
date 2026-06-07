#include "translator.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// Evaluate the top of the stack with two operands and push the result
/// @param out the assembly to be written to
/// @param operation the operation to use
/// @param buffer a string buffer
void double_arithmetic(file_lines* out, const char operation, char* buffer)
{
        // decrement and grab the stack pointer
        add_line(out, "@SP");
        add_line(out, "AM=M-1");
        // grab 'y'
        add_line(out, "D=M");
        // go to 'x' and overwrite with 'x[op]y'
        add_line(out, "A=A-1");
        sprintf(buffer, "MD=M%cD", operation);
        add_line(out, buffer);
}

/// Evaluate the top of the stack with a single operand and push the result
/// @param out the assembly to be written to
/// @param operation the operation to use
/// @param buffer a string buffer
void single_arithmetic(file_lines* out, const char operation, char* buffer)
{
        // grab stack pointer - 1
        add_line(out, "@SP");
        add_line(out, "A=M-1");
        // replace 'x' with '[op]x'
        sprintf(buffer, "M=%cM", operation);
        add_line(out, buffer);
}

/// Evaluate the top of the stack and push the result to the stack
/// @param out the assembly to be written to
/// @param jump the type of jump to use, inverse of the check
/// @param i the label offset
/// @param buffer a string buffer
void stack_evaluation(file_lines* out, const char* jump, const unsigned short i, char* buffer)
{
        // subtraction is great for a difference check
        double_arithmetic(out, '-', buffer);
        // D and A come primed from double_arithmetic
        add_line(out, "M=0");
        sprintf(buffer, "@%s$%d", jump, i);
        add_line(out, buffer);
        sprintf(buffer, "D;%s", jump);
        add_line(out, buffer);
        // True
        add_line(out, "@SP");
        add_line(out, "A=M-1");
        add_line(out, "M=-1");
        // False
        sprintf(buffer, "(%s$%d)", jump, i);
        add_line(out, buffer);
}

/// Push a constant number or table value to the stack
/// @param out the assembly to be written to
/// @param value the value to push
/// @param buffer a string buffer
void push_constant(file_lines* out, const char* value, char* buffer)
{
        sprintf(buffer, "@%s", value);
        add_line(out, buffer);
        add_line(out, "D=A");
        add_line(out, "@SP");
        add_line(out, "M=M+1");
        add_line(out, "A=M-1");
        add_line(out, "M=D");
}

/// Push RAM[segment + offset] to stack
/// @param out the assembly to be written to
/// @param segment the segment pointer in RAM
/// @param offset how much to offset the segment pointer by
/// @param buffer a string buffer
void push_segment(file_lines* out, const char* segment, const char* offset, char* buffer)
{
        sprintf(buffer, "@%s", segment);
        add_line(out, buffer);
        // Slight optimization on segment 0 calls
        if (strcmp(offset, "0")) {
                add_line(out, "D=M");
                sprintf(buffer, "@%s", offset);
                add_line(out, buffer);
                add_line(out, "A=D+A");
        }
        else
                add_line(out, "A=M");
        // Grab RAM[segment + i]
        add_line(out, "D=M");
        // Go to stack while incrementing pointer
        add_line(out, "@SP");
        add_line(out, "M=M+1");
        add_line(out, "A=M-1");
        // Put our value on top
        add_line(out, "M=D");
}

/// Push RAM[location] to stack
/// @param out the assembly to be written to
/// @param location the location in RAM
/// @param buffer a string buffer
void push_static(file_lines* out, const char* location, char* buffer)
{
        // Grab value from RAM[location]
        sprintf(buffer, "@%s", location);
        add_line(out, buffer);
        add_line(out, "D=M");
        // Go to stack whilst incrementing pointer
        add_line(out, "@SP");
        add_line(out, "M=M+1");
        add_line(out, "A=M-1");
        // Put out value on top
        add_line(out, "M=D");
}

/// Push to the stack based on arguments wrapper
/// @param out the assembly to write to
/// @param arguments the arguments of the line
/// @param buffer a string buffer
void push(file_lines* out, const file_lines* arguments, const char* filename, char* buffer)
{
        char statics[128];
        if (!strcmp("local", arguments->line[1]))
                push_segment(out, "LCL", arguments->line[2], buffer);
        else if (!strcmp("argument", arguments->line[1]))
                push_segment(out, "ARG", arguments->line[2], buffer);
        else if (!strcmp("this", arguments->line[1]))
                push_segment(out, "THIS", arguments->line[2], buffer);
        else if (!strcmp("that", arguments->line[1]))
                push_segment(out, "THAT", arguments->line[2], buffer);
        else if (!strcmp("static", arguments->line[1])) {
                sprintf(statics, "%s.%s", filename, arguments->line[2]);
                push_static(out, statics, buffer);
        }
        else if (!strcmp("temp", arguments->line[1])) {
                sprintf(statics, "%d", atoi(arguments->line[2]) + 5);
                push_static(out, statics, buffer);
        }
        else if (!strcmp("pointer", arguments->line[1])) {
                sprintf(statics, "%d", atoi(arguments->line[2]) + 3);
                push_static(out, statics, buffer);
        }
        else
                push_constant(out, arguments->line[2], buffer);
}

/// Pop stack to RAM[segment + i]
/// @param out the assembly to be written to
/// @param segment the segment pointer in RAM
/// @param offset how much to offset the segment pointer by
/// @param buffer a string buffer
void pop_segment(file_lines* out, const char* segment, const char* offset, char* buffer)
{
        if (strcmp(offset, "0")) {
                sprintf(buffer, "@%s", segment);
                add_line(out, buffer);
                add_line(out, "D=M");
                // Add the offset
                sprintf(buffer, "@%s", offset);
                add_line(out, buffer);
                add_line(out, "D=D+A");
                // go to stack whilst decrementing pointer
                add_line(out, "@SP");
                add_line(out, "AM=M-1");
                // cycle A, D, and M left, leaving the value where we need it
                add_line(out, "D=D+M");
                add_line(out, "A=D-M");
                add_line(out, "M=D-A");
        }
        else {
                add_line(out, "@SP");
                add_line(out, "AM=M-1");
                add_line(out, "D=M");
                sprintf(buffer, "@%s", segment);
                add_line(out, buffer);
                add_line(out, "A=M");
                add_line(out, "M=D");
        }
}

/// Pop RAM[pointer] to RAM[location]
/// @param out the assembly to be written to
/// @param pointer the pointer location in RAM
/// @param location the location in RAM
/// @param buffer a string buffer
void pop_pointer_static(file_lines* out, const char* pointer, const char* location, char* buffer)
{
        // go to pointer address whilst decrementing pointer
        sprintf(buffer, "@%s", pointer);
        add_line(out, buffer);
        add_line(out, "AM=M-1");
        add_line(out, "D=M");
        // apply it to the memory location
        sprintf(buffer, "@%s", location);
        add_line(out, buffer);
        add_line(out, "M=D");
}

/// Pop from stack based on arguments wrapper
/// @param out the assembly to be written to
/// @param arguments the arguments of the line
/// @param filename explicitly used for static calls
/// @param buffer a string buffer
void pop(file_lines* out, const file_lines* arguments, const char* filename, char* buffer)
{
        char statics[128];
        if (!strcmp("local", arguments->line[1]))
                pop_segment(out, "LCL", arguments->line[2], buffer);
        else if (!strcmp("argument", arguments->line[1]))
                pop_segment(out, "ARG", arguments->line[2], buffer);
        else if (!strcmp("this", arguments->line[1]))
                pop_segment(out, "THIS", arguments->line[2], buffer);
        else if (!strcmp("that", arguments->line[1]))
                pop_segment(out, "THAT", arguments->line[2], buffer);
        else if (!strcmp("static", arguments->line[1])) {
                sprintf(statics, "%s.%s", filename, arguments->line[2]);
                pop_pointer_static(out, "SP", statics, buffer);
        }
        else if (!strcmp("temp", arguments->line[1])) {
                sprintf(statics, "%d", atoi(arguments->line[2]) + 5);
                pop_pointer_static(out, "SP", statics, buffer);
        }
        else if (!strcmp("pointer", arguments->line[1])) {
                sprintf(statics, "%d", atoi(arguments->line[2]) + 3);
                pop_pointer_static(out,"SP", statics, buffer);
        }
}

/// Writes a label
/// @param out the assembly to be written to
/// @param label the label to be written
/// @param buffer a string buffer
void label(file_lines* out, const char* label, char* buffer)
{
        sprintf(buffer, "(%s)", label);
        add_line(out, buffer);
}

/// Goto a label unconditionally
/// @param out the assembly to be written to
/// @param destination the label to jump to
/// @param buffer a string buffer
void goto_label(file_lines* out, const char* destination, char* buffer)
{
        sprintf(buffer, "@%s", destination);
        add_line(out, buffer);
        add_line(out, "0;JMP");
}

/// Goto a label if the top of the stack isn't zero
/// @param out the assembly to be written to
/// @param destination the label to jump to
/// @param buffer a string buffer
void if_goto(file_lines* out, const char* destination, char* buffer)
{
        add_line(out, "@SP");
        add_line(out, "AM=M-1");
        add_line(out, "D=M");
        sprintf(buffer, "@%s", destination);
        add_line(out, buffer);
        add_line(out, "D;JNE");
}

/// Dictates a function header, setting the locals up for the function
/// @param out the assembly to be written to
/// @param name the name of the function
/// @param locals the number of parameters
/// @param buffer a string buffer
void function(file_lines* out, const char* name, const char* locals, char* buffer)
{
        label(out, name, buffer);
        if (!strcmp(locals, "0"))
                return;
        // How many locals
        sprintf(buffer, "@%s", locals);
        add_line(out, buffer);
        add_line(out, "D=A");
        // Set SP after parameters
        add_line(out, "@SP");
        add_line(out, "M=M+D");
        // Loop over parameters setting to 0
        add_line(out, "D=D-1");
        sprintf(buffer, "(%s$locals)", name);
        add_line(out, buffer);
        add_line(out, "@LCL");
        add_line(out, "A=M+D");
        add_line(out, "M=0");
        sprintf(buffer, "@%s$locals", name);
        add_line(out, buffer);
        add_line(out, "D=D-1;JGE");
}

/// Calls the function, storing the state to return to
/// @param out the assembly to be written to
/// @param function the name of the function being called
/// @param args the number of arguments being passed
/// @param offset the offset used for the return address label
/// @param buffer a string buffer
void call(file_lines* out, const char* function, const char* args, const unsigned short offset, char* buffer)
{
        // Push our return address
        char return_label[64];
        sprintf(return_label, "%s$ret.%d", function, offset);
        push_constant(out, return_label, buffer);
        // Push our state
        push_static(out, "LCL", buffer);
        push_static(out, "ARG", buffer);
        push_static(out, "THIS", buffer);
        push_static(out, "THAT", buffer);
        // Align SP and LCL
        add_line(out, "@SP");
        add_line(out, "D=M");
        add_line(out, "@LCL");
        add_line(out, "M=D");
        // Align ARG to SP-args-5
        sprintf(buffer, "@%d", atoi(args) + 5);
        add_line(out, buffer);
        add_line(out, "D=D-A");
        add_line(out, "@ARG");
        add_line(out, "M=D");
        // Add function goto and return label
        goto_label(out, function, buffer);
        label(out, return_label, buffer);
}

/// Returns the state to before the function call
/// @param out the assembly to be written to
/// @param buffer a string buffer
void cleanup(file_lines* out, char* buffer)
{
        // @R15 for return address
        add_line(out, "@5");
        add_line(out, "D=A");
        add_line(out, "@LCL");
        add_line(out, "A=M-D");
        add_line(out, "D=M");
        add_line(out, "@R15");
        add_line(out, "M=D");
        // Pop the return value into ARG
        pop_segment(out, "ARG", "0", buffer);
        // Align the SP (A comes primed from pop)
        add_line(out, "D=A+1");
        add_line(out, "@SP");
        add_line(out, "M=D");
        // Return pointer frame, LCL works as a stack
        pop_pointer_static(out, "LCL", "THAT", buffer);
        pop_pointer_static(out, "LCL", "THIS", buffer);
        pop_pointer_static(out, "LCL", "ARG", buffer);
        pop_pointer_static(out, "LCL", "LCL", buffer);
        // Return
        add_line(out, "@R15");
        add_line(out, "A=M");
        add_line(out, "0;JMP");
}

void translate(const file_lines* vm, file_lines* assembly, const char* filename, char* buffer)
{
        for (unsigned short i = 0; i < vm->length; ++i) {
                // Get all line arguments
                file_lines* arguments = new_file_lines();
                const char* argument_ptr = strtok(vm->line[i], " \t\r\n");
                while (argument_ptr && *argument_ptr && *argument_ptr != '/') {
                        add_line(arguments, argument_ptr);
                        argument_ptr = strtok(NULL, " \t\r\n");
                }
                if (!strcmp(arguments->line[0], "push"))
                        push(assembly, arguments, filename, buffer);
                else if (!strcmp(arguments->line[0], "pop"))
                        pop(assembly, arguments, filename, buffer);
                else if (!strcmp(arguments->line[0], "add"))
                        double_arithmetic(assembly, '+', buffer);
                else if (!strcmp(arguments->line[0], "sub"))
                        double_arithmetic(assembly, '-', buffer);
                else if (!strcmp(arguments->line[0], "neg"))
                        single_arithmetic(assembly, '-', buffer);
                else if (!strcmp(arguments->line[0], "eq"))
                        stack_evaluation(assembly, "JNE", i, buffer);
                else if (!strcmp(arguments->line[0], "gt"))
                        stack_evaluation(assembly, "JLE", i, buffer);
                else if (!strcmp(arguments->line[0], "lt"))
                        stack_evaluation(assembly, "JGE", i, buffer);
                else if (!strcmp(arguments->line[0], "and"))
                        double_arithmetic(assembly, '&', buffer);
                else if (!strcmp(arguments->line[0], "or"))
                        double_arithmetic(assembly, '|', buffer);
                else if (!strcmp(arguments->line[0], "not"))
                        single_arithmetic(assembly, '!', buffer);
                else if (!strcmp(arguments->line[0], "label"))
                        label(assembly, arguments->line[1], buffer);
                else if (!strcmp(arguments->line[0], "goto"))
                        goto_label(assembly, arguments->line[1], buffer);
                else if (!strcmp(arguments->line[0], "if-goto"))
                        if_goto(assembly, arguments->line[1], buffer);
                else if (!strcmp(arguments->line[0], "function"))
                        function(assembly, arguments->line[1], arguments->line[2], buffer);
                else if (!strcmp(arguments->line[0], "call"))
                        call(assembly, arguments->line[1], arguments->line[2], i, buffer);
                else if (!strcmp(arguments->line[0], "return"))
                        cleanup(assembly, buffer);
                free_file_lines(arguments);
        }
}

void bootstrap(file_lines* assembly, char* buffer)
{
        // Initialize SP
        add_line(assembly, "@256");
        add_line(assembly, "D=A");
        add_line(assembly, "@SP");
        add_line(assembly, "M=D");
        // Call Sys.init
        call(assembly, "Sys.init", "0", 0, buffer);
}
