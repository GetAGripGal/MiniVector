#include "config.h"
#include "log.h"
#include "color.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Read the configuration from the command line arguments
 */
mv_config mv_read_config(int32_t argc, char *argv[])
{
    static const char *usage = "usage: %s [options]\n"
                               "options:\n"
                               "    window: \n"
                               "      -w,  --window <width> <height>     Set the window size\n"
                               "      -f   --fullscreen                  Set the window to fullscreen\n"
                               "    display:\n"
                               "      -r,  --resolution <width> <height> Set the resolution\n"
                               "      -p,  --primary <color_hex>         Set the primary color\n"
                               "      -s,  --secondary <color_hex>       Set the secondary color\n"
                               "    executor:\n"
                               "      -i,  --pipe <pipe>                 Set the pipe to read the instructions\n"
                               "      -e,  --instruction-per-frame <n>   Set the number of instructions per frame\n"
                               "    legacy:\n"
                               "      -le, --legacy                      Use the legacy renderer\n"
                               "      -l,  --line-width <width>          Set the line width\n"
                               "      -h,  --help                        Show this help message\n";
    // The default configuration
    mv_config config = {
        .window = {
            .width = DEFAILT_WINDOW_WIDTH,
            .height = DEFAILT_WINDOW_HEIGHT,
            .fullscreen = 0,
        },
        .resolution = {
            .width = DEFAULT_RESOLUTION_WIDTH,
            .height = DEFAULT_RESOLUTION_HEIGHT,
        },
        .palette = {
            .primary = DEFAULT_PRIMARY_COLOR,
            .secondary = DEFAULT_SECONDARY_COLOR,
        },
        .executor = {
            .instruction_per_frame = DEFAULT_INSTRUCTION_PER_FRAME,
        },
        .line_width = DEFAULT_LINE_WIDTH,
        .pipe = DEFAULT_PIPE,
        .legacy = 0,
    };
    for (int32_t i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--window") == 0)
        {
            if (i + 2 >= argc)
            {
                ERROR("Expected two arguments after '%s'\n", argv[i]);
                printf(usage, argv[0]);
                exit(1);
            }
            config.window.width = atoi(argv[i + 1]);
            config.window.height = atoi(argv[i + 2]);
            i += 2;
        }
        else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--resolution") == 0)
        {
            if (i + 2 >= argc)
            {
                ERROR("Expected two arguments after '%s'\n", argv[i]);
                printf(usage, argv[0]);
                exit(1);
            }
            config.resolution.width = atoi(argv[i + 1]);
            config.resolution.height = atoi(argv[i + 2]);
            i += 2;
        }
        else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--primary") == 0)
        {
            if (i + 1 >= argc)
            {
                ERROR("Expected one argument after '%s'\n", argv[i]);
                printf(usage, argv[0]);
                exit(1);
            }
            config.palette.primary = mv_create_color_from_hex_str(argv[i + 1]);
            i += 1;
        }
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--secondary") == 0)
        {
            if (i + 1 >= argc)
            {
                ERROR("Expected one argument after '%s'\n", argv[i]);
                printf(usage, argv[0]);
                exit(1);
            }
            config.palette.secondary = mv_create_color_from_hex_str(argv[i + 1]);
            i += 1;
        }
        else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--line-width") == 0)
        {
            if (i + 1 >= argc)
            {
                ERROR("Expected one argument after '%s'\n", argv[i]);
                printf(usage, argv[0]);
                exit(1);
            }
            config.line_width = atoi(argv[i + 1]);
            i += 1;
        }
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--pipe") == 0)
        {
            if (i + 1 >= argc)
            {
                ERROR("Expected one argument after '%s'\n", argv[i]);
                printf(usage, argv[0]);
                exit(1);
            }
            config.pipe = argv[i + 1];
            i += 1;
        }
        else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--instruction-per-frame") == 0)
        {
            if (i + 1 >= argc)
            {
                ERROR("Expected one argument after '%s'\n", argv[i]);
                printf(usage, argv[0]);
                exit(1);
            }
            config.executor.instruction_per_frame = atoi(argv[i + 1]);
            i += 1;
        }
        else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--fullscreen") == 0)
        {
            config.window.fullscreen = 1;
        }
        else if (strcmp(argv[i], "-le") == 0 || strcmp(argv[i], "--legacy") == 0)
        {
            config.legacy = 1;
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            printf(usage, argv[0]);
            exit(0);
        }
        else
        {
            ERROR("Unknown option '%s'\n", argv[i]);
            printf(usage, argv[0]);
            exit(1);
        }
    }
    return config;
}