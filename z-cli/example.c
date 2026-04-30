#define Z_CLI_IMPLEMENTATION
#include "z-cli.h"
#include <stdio.h>

int main(int argc, char **argv) {
    bool verbose = false;
    bool help = false;
    int port = 0;
    float ratio = 0.0f;
    const char *output = NULL;
    const char *input = NULL;

    z_cli_t cli;
    z_cli_init(&cli, "myapp", "1.0.0");
    z_cli_set_description(&cli, "A simple example application demonstrating z-cli.");

    z_cli_add_flag(&cli, 'v', "verbose", "Enable verbose output", &verbose);
    z_cli_add_flag(&cli, 'h', "help", "Show this help message", &help);
    z_cli_add_option(&cli, 'p', "port", "Port number (default: 8080)", Z_CLI_INT, &port);
    z_cli_add_option(&cli, 'o', "output", "Output file path", Z_CLI_STRING, &output);
    z_cli_add_option(&cli, 'r', "ratio", "Ratio value", Z_CLI_FLOAT, &ratio);
    z_cli_add_positional(&cli, "input", "Input file", Z_CLI_STRING, &input);

    if (z_cli_parse(&cli, argc, argv) != 0) {
        return 1;
    }

    if (help) {
        z_cli_print_help(&cli);
        z_cli_destroy(&cli);
        return 0;
    }

    printf("Parsed arguments:\n");
    printf("  verbose: %s\n", verbose ? "true" : "false");
    printf("  port: %d\n", port);
    printf("  ratio: %.2f\n", ratio);
    printf("  output: %s\n", output ? output : "(not set)");
    printf("  input: %s\n", input ? input : "(not set)");

    if (cli.remaining_count > 0) {
        printf("  remaining args:\n");
        for (int i = 0; i < cli.remaining_count; i++) {
            printf("    [%d]: %s\n", i, cli.remaining_args[i]);
        }
    }

    z_cli_destroy(&cli);
    return 0;
}
