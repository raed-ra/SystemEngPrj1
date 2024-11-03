//  CITS2002 Project 1 2024
//  Student1:   21016839    Raed Rahmanseresht   
//  Platform:   Apple

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024
#define MAX_IDENTIFIER_LENGTH 12

// Function prototypes

void print_usage(const char *program_name); 
// Generate C code from ML file and write to the C file
void generate_c_code(FILE *ml_file, FILE *c_file, int arg_c, char *arg_v[]); 
// Generate C code for each line, determining its type (function, assignment, return, print, or other)
void generate_c_code_for_line(char *line, FILE *c_file, char *name_register, int *line_number, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int arg_c, char *arg_v[]); 

int compile_and_run_c_code(const char *c_filename, int argc, char *argv[]);

// Functions to check line type (function, assignment, return, print, or other)
int check_function(char *line, FILE *c_file, char *name_register, int *inside_function, FILE *ml_file, int *line_number);
int check_assignment(char *line, FILE *c_file, char *name_register, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]);
int check_return(char *line, FILE *c_file,char *name_register, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]);
int check_print(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number, int arg_c, char *arg_v[], char *name_register);
int check_other(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]);

// Utility functions
int is_valid_syntax_identifier(const char *str); // Check if a token is a valid identifier
char* trim_whitespace(char* str); // Remove leading and trailing spaces
char* read_line(FILE *ml_file, int *line_number, int *start_w_tab); // Read the next line from the .ml file
int starts_with_tab(const char *line); // Check if the line starts with a tab character
void ensure_main_function_printed(FILE *c_file, int *inside_function, int *main_printed, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]); // Ensure the main function is printed if no more functions are found
int find_word_in_lines(FILE *ml_file, int *line_number, int *start_w_tab, const char *target_word); // Find a word in the file lines
int identifier_exists(const char *name_register, const char *identifier); // Check if the identifier exists in the name register
int is_number(const char *token); // Check if a token is a number

// Main function to handle arguments, process files, and clean up
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
    printf("@ Generating c-file for %s file\n", ml_filename);
    
    // Generate the C code from ML file
    generate_c_code(ml_file, c_file, argc, argv);
    fclose(ml_file);
    fclose(c_file);

    printf("@ Compiling %s c-file\n", c_filename);
    
    // Compile and run the generated C code
    int result = compile_and_run_c_code(c_filename, argc, argv);

    return result;
} 

// Print usage instructions if arguments are missing
void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <filename.ml>\n", program_name);
}

// Generate C code for the ML file
void generate_c_code(FILE *ml_file, FILE *c_file, int arg_c, char *arg_v[]) {
    char name_register[1024] = "";  // Track declared variables and functions
    int line_number = 0;
    int inside_function = 0; // Flag for function context
    int main_printed = 0; // Flag for main function printing
    int start_w_tab = 0; // Flag for lines starting with a tab

    // Write C file header
    fprintf(c_file, "#include <stdio.h>\n");
    fprintf(c_file, "#include <stdlib.h>\n\n");

    // Suppress warnings for unused variables
    fprintf(c_file, "#pragma GCC diagnostic ignored \"-Wunused-variable\" \n"); 

    // Helper function to check if a number is an integer or double
    fprintf(c_file, "int is_integer(double num) {\n");
    fprintf(c_file, "    return num == (int)num;\n");
    fprintf(c_file, "}\n\n");

    while (1) {
        // Read the next line from ML file and trim comments/whitespace
        char *line = read_line(ml_file, &line_number, &start_w_tab);
        if (!line) {        
            fprintf(c_file, "};\n");
            return;
        }

        // Close the function if the line is not indented and we are inside a function
        if (inside_function && !start_w_tab) {  
            inside_function = 0;
            fprintf(c_file, "}\n");
        }

        // Process each line and generate C code
        generate_c_code_for_line(line, c_file, name_register, &line_number, &inside_function, &main_printed, &start_w_tab, ml_file, arg_c, arg_v);
    }
}

// Process each line and generate corresponding C code (function, assignment, return, print, or other)
void generate_c_code_for_line(char *line, FILE *c_file, char *name_register, int *line_number, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int arg_c, char *arg_v[]) {
    if (check_function(line, c_file, name_register, inside_function, ml_file, line_number)) {
        return;
    }
    if (check_assignment(line, c_file, name_register, inside_function, main_printed, start_w_tab, ml_file, line_number, arg_c, arg_v)) {
        return;
    }
    if (check_return(line, c_file, name_register, inside_function, main_printed, start_w_tab, ml_file, line_number, arg_c, arg_v)) {
        return;
    }
    if (check_print(line, c_file, inside_function, main_printed, start_w_tab, ml_file, line_number, arg_c, arg_v, name_register)) {
        return;
    }
    check_other(line, c_file, inside_function, main_printed, start_w_tab, ml_file, line_number, arg_c, arg_v);
}

// Check if the line is a function definition and process accordingly
int check_function(char *line, FILE *c_file, char *name_register, int *inside_function, FILE *ml_file, int *line_number) {
    if (strncmp(line, "function", 8) == 0 && isspace(line[8])) { 
        line += 8; // Skip "function" keyword
        line = trim_whitespace(line);

        // Extract function name and parameters
        char *func_name_token = strtok(line, " ");
        char *params = strtok(NULL, "");

        // Validate function name
        char function_name[MAX_IDENTIFIER_LENGTH + 1];
        if (func_name_token) {
            strncpy(function_name, func_name_token, MAX_IDENTIFIER_LENGTH);
            function_name[MAX_IDENTIFIER_LENGTH] = '\0';
        } else {
            fprintf(stderr, "!Error: Missing function name - ml-xxxx.c not deleted for diagnostics\n");
            exit(EXIT_FAILURE);
        }

        // Ensure function name is unique
        if (identifier_exists(name_register, function_name)) {
            fprintf(stderr, "!Error: Function or variable name '%s' already declared - ml-xxxx.c not deleted for diagnostics\n", function_name);
            exit(EXIT_FAILURE);
        }

        // Add function name to register
        strcat(name_register, function_name);
        strcat(name_register, " ");

        // Process all parameters in the function declaration
        char processed_params[MAX_LINE_LENGTH] = ""; 
        if (params) {
            char *param = strtok(params, " ");
            while (param) {
                if (!is_valid_syntax_identifier(param)) {
                    fprintf(stderr, "!Error: Invalid parameter name '%s' at line %d - ml-xxxx.c not deleted for diagnostics\n", param, *line_number);
                    exit(EXIT_FAILURE);
                }
                if (strlen(processed_params) > 0) {
                    strcat(processed_params, ", ");
                }
                strcat(processed_params, "double ");
                strcat(processed_params, param);

                strcat(name_register, param);
                strcat(name_register, " ");

                param = strtok(NULL, " "); 
            }
        }

        // Check if the function has a return statement
        long original_position = ftell(ml_file); 
        int temp_start_w_tab = 0;
        int found_return = find_word_in_lines(ml_file, line_number, &temp_start_w_tab, "return");

        // Return file pointer to original position
        fseek(ml_file, original_position, SEEK_SET);

        // Declare function return type based on presence of return statement
        if (found_return && temp_start_w_tab) { 
            fprintf(c_file, "double %s(%s);\n", function_name, processed_params);
            fprintf(c_file, "double %s(%s) {\n", function_name, processed_params);
        } else {  
            fprintf(c_file, "void %s(%s);\n", function_name, processed_params);
            fprintf(c_file, "void %s(%s) {\n", function_name, processed_params);
        }
        *inside_function = 1;
        return 1; 
    }
    return 0; 
} //end check_function

// Check if the line contains an assignment and process accordingly
int check_assignment(char *line, FILE *c_file, char *name_register, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]) {
    char *assignment_position = strstr(line, "<-");
    if (assignment_position) {
        *assignment_position = '\0';  

        char *identifier = trim_whitespace(line); // Left side of assignment
        char *expression = trim_whitespace(assignment_position + 2); // Right side of assignment

        // Validate identifier
        if (!is_valid_syntax_identifier(identifier)) {
            fprintf(stderr, "!Error: Invalid identifier '%s' at line %d - ml-xxxx.c not deleted for diagnostics\n", identifier, *line_number);
            exit(EXIT_FAILURE);
        }

        // Copy identifier and expression to avoid modifying original strings when we call strtok in the next step
        char identifier_copy[MAX_IDENTIFIER_LENGTH + 1];
        strncpy(identifier_copy, identifier, MAX_IDENTIFIER_LENGTH);
        identifier_copy[MAX_IDENTIFIER_LENGTH] = '\0';

        char expression_copy[MAX_LINE_LENGTH];
        strncpy(expression_copy, expression, MAX_LINE_LENGTH);
        expression_copy[MAX_LINE_LENGTH - 1] = '\0';

        // Ensure main function is printed if needed
        ensure_main_function_printed(c_file, inside_function, main_printed, ml_file, line_number, arg_c, arg_v);

        // Check for undeclared variables in the expression
        char expression_copy2[MAX_LINE_LENGTH];
        strncpy(expression_copy2, expression_copy, MAX_LINE_LENGTH);
        expression_copy2[MAX_LINE_LENGTH - 1] = '\0';

        char *token = strtok(expression_copy, " ()[]+-*/");
        while (token != NULL) {
            if (!is_number(token)) {
                if (strncmp(token, "arg", 3) == 0 && isdigit(token[3])) {
                    token = strtok(NULL, " ()[]+-*/");
                    continue;
                }
                if (!identifier_exists(name_register, token)) {
                    fprintf(c_file, "    double %s = 0;\n", token);
                    strcat(name_register, token);
                    strcat(name_register, " ");
                }
            }
            token = strtok(NULL, " ()[]+-*/");
        }

        // Generate assignment statement
        if (*start_w_tab || *inside_function) {
            if (!identifier_exists(name_register, identifier_copy)) {
                strcat(name_register, identifier_copy);
                strcat(name_register, " ");
                fprintf(c_file, "    double %s = %s;\n", identifier_copy, expression_copy2);
            } else {
                fprintf(c_file, "    %s = %s;\n", identifier_copy, expression_copy2);
            }
        } else {
            if (!identifier_exists(name_register, identifier_copy)) {
                strcat(name_register, identifier_copy);
                strcat(name_register, " ");
                fprintf(c_file, "double %s = %s;\n", identifier_copy, expression_copy2);
            } else {
                fprintf(c_file, "%s = %s;\n", identifier_copy, expression_copy2);
            }
        }
        return 1;
    }
    return 0;
} //end check_assignment

// Check if the line contains a return statement and process accordingly
int check_return(char *line, FILE *c_file, char *name_register, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]) {
    if (strncmp(line, "return", 6) == 0 && isspace(line[6])) {
        char *expression = trim_whitespace(line + 6);

        // Check for undeclared variables in the expression
        char expression_copy[MAX_LINE_LENGTH];
        strncpy(expression_copy, expression, MAX_LINE_LENGTH);
        expression_copy[MAX_LINE_LENGTH - 1] = '\0';

        char *token = strtok(expression_copy, " ()[]+-*/");
        while (token != NULL) {
            if (is_number(token)) {
                token = strtok(NULL, " ()[]+-*/");  
                continue;
            }
            if (!identifier_exists(name_register, token)) {
                fprintf(stderr, "!Error: Invalid or no parameter name '%s' at line %d Instead of assuming the identifier is zero its better to let you know that you have printed an undeclared identifier.- ml-xxxx.c not deleted for diagnostics\n", token, *line_number);
                exit(EXIT_FAILURE);
            }
            token = strtok(NULL, " ()[]+-*/");
        }

        // Print the return statement
        if (*start_w_tab || *inside_function) {
            fprintf(c_file, "    return %s;\n", expression);
        } else {
            fprintf(c_file, "return %s;\n", expression);
        }
        return 1;
    }
    return 0;
} //end check_return

// Utility function: Check if a token is a number
int is_number(const char *token) {
    char *endptr;
    strtod(token, &endptr); // Try to convert the token to a double
    return (*endptr == '\0'); // If endptr points to null, it's a valid number
} //end is_number

// Check if the line contains a print statement and process accordingly
int check_print(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number, int arg_c, char *arg_v[], char *name_register) {
    if (strncmp(line, "print", 5) == 0 && isspace(line[5])) {
        char *expression = trim_whitespace(line + 5);

        // Ensure main function is printed if needed
        ensure_main_function_printed(c_file, inside_function, main_printed, ml_file, line_number, arg_c, arg_v);

        // Copy expression to avoid modifying original string when calling strtok
        char expression_copy[MAX_LINE_LENGTH];
        strncpy(expression_copy, expression, MAX_LINE_LENGTH);
        expression_copy[MAX_LINE_LENGTH - 1] = '\0';

        // Check for undeclared variables in the expression that are in print statement and print an error message if there is any
        char *token = strtok(expression_copy, " ,()[]+-*/");
        while (token != NULL) {
            token = trim_whitespace(token);
            if (is_number(token)) {
                token = strtok(NULL, " ,()[]+-*/");
                continue;
            }
            if (strncmp(token, "arg", 3) == 0 && isdigit(token[3])) {
                token = strtok(NULL, " ,()[]+-*/");
                continue;
            }
            if (!identifier_exists(name_register, token)) {
                fprintf(stderr, "!Error: Invalid or no identifier '%s' at line %d. Instead of assuming the identifier is zero its better to let you know that you have printed an undeclared identifier- ml-xxxx.c not deleted for diagnostics\n", token, *line_number);
                exit(EXIT_FAILURE);
            }
            token = strtok(NULL, " ,()[]+-*/");
        }

        // Print logic for expression (0 decimal places if integer, 6 decimal places if double)
        if (*start_w_tab || *inside_function) {
            // is_integer function checks if the number is an integer or double and its already defined in the C file on line 27
            fprintf(c_file, "    if (is_integer(%s)) {\n", expression); 
            fprintf(c_file, "        printf(\"%%.0f\\n\", %s);\n", expression);
            fprintf(c_file, "    } else {\n");
            fprintf(c_file, "        printf(\"%%.6f\\n\", %s);\n", expression);
            fprintf(c_file, "    }\n");
        } else {
            fprintf(c_file, "if (is_integer(%s)) {\n", expression);
            fprintf(c_file, "    printf(\"%%.0f\\n\", %s);\n", expression);
            fprintf(c_file, "} else {\n");
            fprintf(c_file, "    printf(\"%%.6f\\n\", %s);\n", expression);
            fprintf(c_file, "}\n");
        }

        return 1;
    }
    return 0;
}  //end check_print

// Handle any other lines not covered by function, assignment, return, or print
int check_other(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]) {
    ensure_main_function_printed(c_file, inside_function, main_printed, ml_file, line_number, 0, NULL);
    if (*start_w_tab || *inside_function) {
        fprintf(c_file, "    %s;\n", line);
    } else {
        fprintf(c_file, "%s;\n", line);
    }
    return 1;
} //end check_other

// Compile and execute the generated C code, cleaning up files after execution
int compile_and_run_c_code(const char *c_filename, int argc, char *argv[]) {
    char binary_filename[64];
    snprintf(binary_filename, sizeof(binary_filename), "ml-%d", getpid()); // Create unique binary filename

    char compile_command[256];
    snprintf(compile_command, sizeof(compile_command), "cc -std=c11 -Wall -Werror -o %s %s", binary_filename, c_filename);
    if (system(compile_command) != 0) {
        fprintf(stderr, "!Error: Compilation failed\n");
        remove(binary_filename);
        remove(c_filename); 
        exit(EXIT_FAILURE);
    }

    // Validate command-line arguments
    for (int i = 2; i < argc; i++) {
        char *endptr;
        strtod(argv[i], &endptr); // Convert argument to double
        if (*endptr != '\0') { // If invalid characters remain, it's not a valid number
            fprintf(stderr, "!Error: Argument %d ('%s') is not a valid number.\n", i - 2, argv[i]);
            remove(binary_filename);
            remove(c_filename);
            return EXIT_FAILURE;
        }
    }

    char execute_command[256];
    snprintf(execute_command, sizeof(execute_command), "./%s", binary_filename);
    for (int i = 2; i < argc; i++) {
        strcat(execute_command, " ");
        strcat(execute_command, argv[i]);
    }
    printf("@ Executing: %s\n", execute_command);

    if (system(execute_command) != 0) {
        fprintf(stderr, "!Error: Execution failed\n");
        remove(binary_filename);
        remove(c_filename); 
        return EXIT_FAILURE;
    }
    remove(binary_filename);
    remove(c_filename);
    return EXIT_SUCCESS;
} //end compile_and_run_c_code

// Check if the identifier is alphanumeric or an underscore and meets length restrictions
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
} //end is_valid_syntax_identifier

// Utility function to remove leading and trailing whitespace
char* trim_whitespace(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    *(end + 1) = '\0';
    return str;
} //end trim_whitespace

// Utililty function that checks if the line starts with a tab character
int starts_with_tab(const char *line) {
    return line[0] == '\t';
} //end starts_with_tab

// Ensure the main function is printed if no more functions are found in the ML file
void ensure_main_function_printed(FILE *c_file, int *inside_function, int *main_printed, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]) {
    if (!*inside_function && !*main_printed) {

        // Save current file position before checking for upcoming function definitions
        long original_position = ftell(ml_file);

        int temp_start_w_tab = 0;
        int found_function = find_word_in_lines(ml_file, line_number, &temp_start_w_tab, "function");

        // Return to the original file position
        fseek(ml_file, original_position, SEEK_SET);

        if (!found_function) {
            fprintf(c_file, "\nint main(int argc, char *argv[]) {\n");
            if (arg_c > 1) {  // Convert arguments to double in the main function
                for (int i = 1; i < arg_c-1; ++i) {
                    fprintf(c_file, "    double arg%d = atof(argv[%d]);\n", i-1, i);
                }
            }
            *main_printed = 1;
        }
    }
} //end ensure_main_function_printed

// Utility function: Find a target word in the file's lines
int find_word_in_lines(FILE *ml_file, int *line_number, int *temp_start_w_tab, const char *target_word) {
    char *next_line;
    while ((next_line = read_line(ml_file, line_number, temp_start_w_tab)) != NULL) {
        if (strstr(next_line, target_word) != NULL) {
            return 1;
        }
    }
    return 0;
} //end find_word_in_lines

// Utility function to read a line, trim it, and remove comments
char* read_line(FILE *ml_file, int *line_number, int *start_w_tab) {
    static char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), ml_file) != NULL) {
        (*line_number)++;
        char *comment_start = strchr(line, '#'); // Remove comments
        if (comment_start) {
            *comment_start = '\0';
        }
        *start_w_tab = starts_with_tab(line); // Check if the line starts with a tab
        char *trimmed_line = trim_whitespace(line);
        if (trimmed_line && strlen(trimmed_line) > 0) {
            return trimmed_line;
        }
    }
    return NULL;
} //end read_line

// Utility function: Check if the identifier exists in the name register
int identifier_exists(const char *name_register, const char *identifier) {
    char padded_name_register[MAX_LINE_LENGTH];
    snprintf(padded_name_register, sizeof(padded_name_register), " %s ", name_register); // Add spaces around the name register

    char padded_identifier[MAX_IDENTIFIER_LENGTH + 3];
    snprintf(padded_identifier, sizeof(padded_identifier), " %s ", identifier); // Add spaces around the identifier

    return strstr(padded_name_register, padded_identifier) != NULL;
} //end identifier_exists