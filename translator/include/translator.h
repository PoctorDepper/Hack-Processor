#pragma once

#include "file_lines.h"

/// Translate a VM buffer into an assembly buffer
/// @param vm the vm from the file
/// @param assembly the assembly to be written to
/// @param filename explicitly used for static calls
/// @param buffer a string buffer
void translate(const file_lines* vm, file_lines* assembly, const char* filename, char* buffer);

/// Bootstrap a multi-VM application
/// @param assembly the assembly to be written to
/// @param buffer a string buffer
void bootstrap(file_lines* assembly, char* buffer);