#pragma once
#include <stdio.h>

#pragma GCC diagnostic ignored "-Wreturn-type"

#define TRACE(...) printf("[\x1B[36mT\x1B[37m] " __VA_ARGS__);
#define INFO(...) printf("[\x1B[32mI\x1B[37m] " __VA_ARGS__)
#define WARN(...) printf("[\x1B[33mW\x1B[37m] " __VA_ARGS__)
#define ERROR(...) printf("[\x1B[31mE\x1B[37m] " __VA_ARGS__)