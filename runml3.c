#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>  // Needed for isalnum

#define MAX_LINE_LENGTH 1024
#define MAX_IDENTIFIER_LENGTH 12  // Maximum number of characters in an identifier

// Function prototypes
void print_usage(const char *program_name);
void generate_c_code(FILE *ml_file, FILE *c_file, int arg_c, char *arg_v[]);
void generate_c_code_for_line(char *line, FILE *c_file, char *variables, int *line_number, int *inside_function, int *main_printed, int *start_w_tab);
void compile_and_run_c_code(const char *c_filename, int argc, char *argv[]);
int is_valid_syntax_identifier(const char *str);
char* trim_whitespace(char* str);

char* read_line(FILE *ml_file, int *line_number, int *start_w_tab);
int check_function(char *line, FILE *c_file, char *variables, int *inside_function);
int check_assignment(char *line, FILE *c_file, char *variables, int *inside_function, int *main_printed, int *start_w_tab);
int check_return(char *line, FILE *c_file, int *inside_function,int *main_printed, int *start_w_tab);
int check_print(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab);
int check_other(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab);
int starts_with_tab(const char *line);
void ensure_main_function_printed(FILE *c_file, int *inside_function, int *main_printed);

// Global flag to track if we're inside a function definition


int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *ml_filename = argv[1];
    FILE *ml_file = fopen(ml_filename, "r");
    if (ml_file == NULL) {
        fprintf(stderr, "!Error: Could not open file %s\n", ml_filename);
        return EXIT_FAILURE;
    }

    char c_filename[64];
    snprintf(c_filename, sizeof(c_filename), "ml-%d.c", getpid());
    FILE *c_file = fopen(c_filename, "w");
    if (c_file == NULL) {
        fprintf(stderr, "!Error: Could not create file %s\n", c_filename);
        fclose(ml_file);
        return EXIT_FAILURE;
    }

    generate_c_code(ml_file, c_file, argc, argv);
    fclose(ml_file);
    fclose(c_file);

    compile_and_run_c_code(c_filename, argc, argv);

    return EXIT_SUCCESS;
}

void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <filename.ml>\n", program_name);
}

void generate_c_code(FILE *ml_file, FILE *c_file, int arg_c, char *arg_v[]) {
    char variables[1024] = "";  // To store declared variables
    int line_number = 0;
    int inside_function = 0;  // Local int variable
    int main_printed = 0;     // Local int variable
    int start_w_tab = 0;      // Local int variable

    // Write C file header
    fprintf(c_file, "#include <stdio.h>\n");
    fprintf(c_file, "#include <stdlib.h>\n\n");

    // Process each line
    while (1) {
        char *line = read_line(ml_file, &line_number, &start_w_tab);
        if (!line) {
            fprintf(c_file, "};\n");
            return;
        }
        printf("1. line: %s, variables: %s, line numbe:%d, inside_function: %d main_printed: %d start_w_tab %d\n",line, variables, line_number, inside_function, main_printed, start_w_tab);

        if (inside_function && !start_w_tab) {
            // Check if the line does NOT start with a tab and we're inside a function
            inside_function = 0;
            fprintf(c_file, "}\n"); // close function after no tab post function
        }
        printf("2. line: %s, variables: %s, line numbe:%d, inside_function: %d main_printed: %d start_w_tab %d\n",line, variables, line_number, inside_function, main_printed, start_w_tab);

        generate_c_code_for_line(line, c_file, variables, &line_number, &inside_function, &main_printed, &start_w_tab);
    }
}

void generate_c_code_for_line(char *line, FILE *c_file, char *variables, int *line_number, int *inside_function, int *main_printed, int *start_w_tab) {


    // Check if the line starts a function definition
    if (check_function(line, c_file, variables, inside_function)) {
        return;
    }

    // Check if the line is an assignment operation
    if (check_assignment(line, c_file, variables, inside_function, main_printed, start_w_tab)) {
        return;
    }

    // Check if the line is a return statement
    if (check_return(line, c_file, inside_function, main_printed, start_w_tab)) {
        return;
    }

    // Check if the line is a print statement
    if (check_print(line, c_file, inside_function, main_printed, start_w_tab)) {
        return;
    }

    // If none of the above, print the line as-is
    check_other(line, c_file, inside_function, main_printed, start_w_tab);
}




char* read_line(FILE *ml_file, int *line_number, int *start_w_tab) {
    static char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), ml_file) != NULL) {
        (*line_number)++;

        char *comment_start = strchr(line, '#');
        if (comment_start) {
            *comment_start = '\0';  // Remove comments
        }
        *start_w_tab = starts_with_tab(line);

        char *trimmed_line = trim_whitespace(line);
        if (trimmed_line && strlen(trimmed_line) > 0) {
            return trimmed_line;
        } else {
            printf("Empty line, reading next...\n");
        }
    }

    return NULL;
}

int check_function(char *line, FILE *c_file, char *variables, int *inside_function) {
    if (strncmp(line, "function", 8) == 0 && isspace(line[8])) {
        line += 8;
        line = trim_whitespace(line);

        // Extract the function name
        char *function_name = strtok(line, " ");
        char *params = strtok(NULL, "");


        // Validate the function name
        if (!is_valid_syntax_identifier(function_name)) {
            fprintf(stderr, "!Error: Invalid function name '%s'\n", function_name);
            exit(EXIT_FAILURE);
        }

        // Check if the function name is already used as a variable
        if (strstr(variables, function_name)) {
            fprintf(stderr, "!Error: Function name '%s' already used as a variable\n", function_name);
            exit(EXIT_FAILURE);
        }

        // Add the function name to the variables list
        strcat(variables, function_name);
        printf("variables: %s\n", variables);
        printf("params: %s\n", params);
        strcat(variables, " ");

        // Prepare a buffer to store the processed parameters
        char processed_params[MAX_LINE_LENGTH] = "";
        if (params) {
            char *param = strtok(params, " ");
            while (param) {
                if (!is_valid_syntax_identifier(param)) {
                    fprintf(stderr, "!Error: Invalid parameter name '%s'\n", param);
                    exit(EXIT_FAILURE);
                }

                // Add the parameter as a double to the processed params
                if (strlen(processed_params) > 0) {
                    strcat(processed_params, ", ");
                }
                strcat(processed_params, "double ");
                strcat(processed_params, param);

                // Add the parameter to the variables list
                strcat(variables, param);
                strcat(variables, " ");

                param = strtok(NULL, " ");
            }
        }

        // Declare the function
        fprintf(c_file, "void %s(%s);\n", function_name, processed_params);

        // Start the function definition
        fprintf(c_file, "void %s(%s) {\n", function_name, processed_params);

        *inside_function = 1;

        return 1;
    }
    return 0;
}



int check_assignment(char *line, FILE *c_file, char *variables, int *inside_function, int *main_printed, int *start_w_tab) {
    char *assignment_position = strstr(line, "<-");
    if (assignment_position) {
        *assignment_position = '\0';  // Terminate to get the identifier
        char *identifier = trim_whitespace(line);  // Left side is the identifier
        char *expression = trim_whitespace(assignment_position + 2);  // Right side is the expression

        if (!is_valid_syntax_identifier(identifier)) {
            fprintf(stderr, "!Error: Invalid identifier '%s'\n", identifier);
            exit(EXIT_FAILURE);
        }

        // Ensure the main function is printed before anything else without a tab
        ensure_main_function_printed(c_file, inside_function, main_printed);

        // Always print a tab if the line starts with a tab, otherwise check inside_function
        if (*start_w_tab || *inside_function) {
            if (!strstr(variables, identifier)) {
                strcat(variables, identifier);
                strcat(variables, " ");
                fprintf(c_file, "    double %s = %s;\n", identifier, expression);  // Declare and assign the variable
            } else {
                fprintf(c_file, "    %s = %s;\n", identifier, expression);
            }
        } else {
            if (!strstr(variables, identifier)) {
                strcat(variables, identifier);
                strcat(variables, " ");
                fprintf(c_file, "double %s = %s;\n", identifier, expression);  // Declare and assign the variable
            } else {
                fprintf(c_file, "%s = %s;\n", identifier, expression);
            }
        }

        return 1;
    }
    return 0;
}



int check_return(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab) {
    // Ensure "return" is a standalone word followed by a space
    if (strncmp(line, "return", 6) == 0 && isspace(line[6])) {
        char *expression = trim_whitespace(line + 6);

        // Ensure the main function is printed before anything else without a tab
        ensure_main_function_printed(c_file, inside_function, main_printed);

        // Always print a tab if the line starts with a tab, otherwise check inside_function
        if (*start_w_tab || *inside_function) {
            fprintf(c_file, "    return %s;\n", expression);
        } else {
            fprintf(c_file, "return %s;\n", expression);
        }

        *inside_function = 0;
        return 1;
    }
    return 0;
}



int check_print(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab) {
    // Ensure "print" is a standalone word followed by a space
    if (strncmp(line, "print", 5) == 0 && isspace(line[5])) {
        char *expression = trim_whitespace(line + 5);

        // Ensure the main function is printed before anything else without a tab
        ensure_main_function_printed(c_file, inside_function, main_printed);
        
        // Always print a tab if the line starts with a tab, otherwise check inside_function
        if (*start_w_tab || *inside_function) {
            fprintf(c_file, "    printf(\"%%.6f\\n\", %s);\n", expression);
        } else {
            fprintf(c_file, "printf(\"%%.6f\\n\", %s);\n", expression);
        }
        return 1;
    }
    return 0;
}

int check_other(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab) {
    printf("check other: inside function is:  %d  main printed is: %d \n", *inside_function , *main_printed);
    // Ensure the main function is printed before anything else without a tab
    ensure_main_function_printed(c_file, inside_function, main_printed);

    // Print the line exactly as it appears in the .ml file
    // Add a tab if the line starts with a tab or if inside_function is true
    if (*start_w_tab || *inside_function) {
        fprintf(c_file, "    %s;\n", line);
    } else {
        fprintf(c_file, "%s;\n", line);
    }
    return 1;
}

void compile_and_run_c_code(const char *c_filename, int argc, char *argv[]) {
    char binary_filename[64];
    snprintf(binary_filename, sizeof(binary_filename), "ml-%d", getpid());

    // Compile the C code
    char compile_command[256];
    snprintf(compile_command, sizeof(compile_command), "cc -std=c11 -Wall -Werror -o %s %s", binary_filename, c_filename);
    if (system(compile_command) != 0) {
        fprintf(stderr, "!Error: Compilation failed\n");
        remove(binary_filename);
        exit(EXIT_FAILURE);
    }

    // Execute the compiled program
    char execute_command[256];
    snprintf(execute_command, sizeof(execute_command), "./%s", binary_filename);
    for (int i = 2; i < argc; i++) {
        strcat(execute_command, " ");
        strcat(execute_command, argv[i]);
    }
    if (system(execute_command) != 0) {
        fprintf(stderr, "!Error: Execution failed\n");
    }

    // Clean up the compiled binary
    remove(binary_filename);
}

int is_valid_syntax_identifier(const char *str) {
    if (str == NULL || strlen(str) < 1 || strlen(str) > MAX_IDENTIFIER_LENGTH) {
        return 0;
    }
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isalnum(str[i]) && str[i] != '_') {
            return 0;
        }
    }
    return 1;
}

// Utility function to trim leading and trailing whitespace
char* trim_whitespace(char* str) {
    // Trim leading whitespace
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0)  // All spaces?
        return str;

    // Trim trailing whitespace
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = '\0';

    return str;
}

int starts_with_tab(const char *line) {
    printf("Is there a tab in line '%s': %d\n", line, line[0] == '\t');
    return line[0] == '\t';
}


void ensure_main_function_printed(FILE *c_file, int *inside_function, int *main_printed) {
    if (!*inside_function && !*main_printed) {
        fprintf(c_file, "\nint main(int argc, char *argv[]) {\n");
        *main_printed = 1;
    }
}