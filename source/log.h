#pragma once
#include <stdio.h>

#pragma GCC diagnostic ignored "-Wreturn-type"

#ifdef DEBUG

#if defined(_WIN96)
#define MV_TRACE(...) printf("[minivector][T] " __VA_ARGS__);
#define MV_INFO(...) printf("[minivector][I] " __VA_ARGS__)
#define MV_WARN(...) printf("[minivector][W] " __VA_ARGS__)
#define MV_ERROR(...) printf("[minivector][E] " __VA_ARGS__)

#else
#define MV_TRACE(...) printf("[\x1B[36mT\x1B[37m] " __VA_ARGS__)
#define MV_INFO(...) printf("[\x1B[32mI\x1B[37m] " __VA_ARGS__)
#define MV_WARN(...) printf("[\x1B[33mW\x1B[37m] " __VA_ARGS__)
#define MV_ERROR(...) printf("[\x1B[31mE\x1B[37m] " __VA_ARGS__)

#endif

#else

#if defined(_WIN96)
#define MV_TRACE(...)
#define MV_INFO(...) printf("[minivector][I] " __VA_ARGS__)
#define MV_WARN(...) printf("[minivector][W] " __VA_ARGS__)
#define MV_ERROR(...) printf("[minivector][E] " __VA_ARGS__)

#else
#define MV_TRACE(...)
#define MV_INFO(...) printf("[\x1B[32mI\x1B[37m] " __VA_ARGS__)
#define MV_WARN(...) printf("[\x1B[33mW\x1B[37m] " __VA_ARGS__)
#define MV_ERROR(...) printf("[\x1B[31mE\x1B[37m] " __VA_ARGS__)
#endif

#endif