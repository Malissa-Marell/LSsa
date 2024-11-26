#ifndef INPUT_FUNCTIONS_H
#define INPUT_FUNCTIONS_H

#include <stdbool.h>
#include <stdio.h>

void read_string(const char * prompt, char * destination);
float read_float(const char * prompt);
int read_integer(const char * prompt);
int read_integer_in_range(const char * prompt, int min, int max);
bool read_boolean(const char * prompt);

#endif