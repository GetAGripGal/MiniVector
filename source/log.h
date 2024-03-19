#pragma once
#include <stdio.h>

#pragma GCC diagnostic ignored "-Wreturn-type"

#ifdef DEBUG

#define MV_TRACE(...) printf("[\x1B[36mT\x1B[37m] " __VA_ARGS__);
#define MV_INFO(...) printf("[\x1B[32mI\x1B[37m] " __VA_ARGS__)
#define MV_WARN(...) printf("[\x1B[33mW\x1B[37m] " __VA_ARGS__)
#define MV_ERROR(...) printf("[\x1B[31mE\x1B[37m] " __VA_ARGS__)

#else
#define MV_TRACE(...)
#define MV_INFO(...) printf("[\x1B[32mI\x1B[37m] " __VA_ARGS__)
#define MV_WARN(...) printf("[\x1B[33mW\x1B[37m] " __VA_ARGS__)
#define MV_ERROR(...) printf("[\x1B[31mE\x1B[37m] " __VA_ARGS__)
#endif