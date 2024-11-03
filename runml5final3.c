#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024
#define MAX_IDENTIFIER_LENGTH 12

// Function prototypes
void print_usage(const char *program_name);
void generate_c_code(FILE *ml_file, FILE *c_file, int arg_c, char *arg_v[]);
void generate_c_code_for_line(char *line, FILE *c_file, char *name_register, int *line_number, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file);
void compile_and_run_c_code(const char *c_filename, int argc, char *argv[]);
int is_valid_syntax_identifier(const char *str);
char* trim_whitespace(char* str);
char* read_line(FILE *ml_file, int *line_number, int *start_w_tab);
int check_function(char *line, FILE *c_file, char *name_register, int *inside_function, FILE *ml_file, int *line_number);
int check_assignment(char *line, FILE *c_file, char *name_register, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number);
int check_return(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number);
int check_print(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number);
int check_other(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number);
int starts_with_tab(const char *line);
void ensure_main_function_printed(FILE *c_file, int *inside_function, int *main_printed, FILE *ml_file, int *line_number);
int find_word_in_lines(FILE *ml_file, int *line_number, int *start_w_tab, const char *target_word);
int identifier_exists(const char *name_register, const char *identifier);

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
    char name_register[1024] = "";  // To store both declared variables and functions
    int line_number = 0;
    int inside_function = 0; // Flag to track if we are inside a function
    int main_printed = 0; // Flag to track if the main function has been printed
    int start_w_tab = 0; // Flag to track if the line starts with a tab

    // Write C file header
    fprintf(c_file, "#include <stdio.h>\n");
    fprintf(c_file, "#include <stdlib.h>\n\n");

    while (1) {
        // Read the next line from the .ml file
        char *line = read_line(ml_file, &line_number, &start_w_tab); 
        // Stop if there are no more lines
        if (!line) {        
            fprintf(c_file, "};\n");
            return;
        }
        // Close the function if it's not indented
        if (inside_function && !start_w_tab) {  
            inside_function = 0;
            fprintf(c_file, "}\n");
        }

        generate_c_code_for_line(line, c_file, name_register, &line_number, &inside_function, &main_printed, &start_w_tab, ml_file);
    }
}

void generate_c_code_for_line(char *line, FILE *c_file, char *name_register, int *line_number, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file) {
    // Check if the line starts a function definition
    if (check_function(line, c_file, name_register, inside_function, ml_file, line_number)) {
        return;
    }

    // Check if the line is an assignment operation
    if (check_assignment(line, c_file, name_register, inside_function, main_printed, start_w_tab, ml_file, line_number)) {
        return;
    }

    // Check if the line is a return statement
    if (check_return(line, c_file, inside_function, main_printed, start_w_tab, ml_file, line_number)) {
        return;
    }

    // Check if the line is a print statement
    if (check_print(line, c_file, inside_function, main_printed, start_w_tab, ml_file, line_number)) {
        return;
    }

    // If none of the above, print the line as-is
    check_other(line, c_file, inside_function, main_printed, start_w_tab, ml_file, line_number);
}


int check_function(char *line, FILE *c_file, char *name_register, int *inside_function, FILE *ml_file, int *line_number) {
    // Check if the line starts with "function" ensuring it's a standalone word
    if (strncmp(line, "function", 8) == 0 && isspace(line[8])) { 
        line += 8; // Skip the "function" keyword and put point at the next character
        line = trim_whitespace(line); //ensure no leading whitespace

        // Extract the function name
        char *func_name_token = strtok(line, " "); // Extract the function name
        char *params = strtok(NULL, ""); // Extract the parameters

        // Duplicate the function name to avoid referencing the static buffer
        char function_name[MAX_IDENTIFIER_LENGTH + 1];
        if (func_name_token) {
            strncpy(function_name, func_name_token, MAX_IDENTIFIER_LENGTH);
            function_name[MAX_IDENTIFIER_LENGTH] = '\0'; // Ensure null-termination
        } else {
            fprintf(stderr, "!Error: Missing function name\n");
            exit(EXIT_FAILURE);
        }

        // Check if the function name is already used as a variable or function
        if (identifier_exists(name_register, function_name)) {
            fprintf(stderr, "!Error: Function or variable name '%s' already declared\n", function_name);
            exit(EXIT_FAILURE);
        }

        // Add the function name to the name register
        strcat(name_register, function_name);
        strcat(name_register, " ");

        // Prepare a buffer to store the processed parameters
        char processed_params[MAX_LINE_LENGTH] = ""; 
        if (params) {
            char *param = strtok(params, " ");  // Extract the parameters
            while (param) {
                if (!is_valid_syntax_identifier(param)) { // Check if the parameter is a valid identifier and less than 12 characters
                    fprintf(stderr, "!Error: Invalid parameter name '%s'\n", param);
                    exit(EXIT_FAILURE);
                }

                // Add the parameter as a double to the processed params
                if (strlen(processed_params) > 0) {
                    strcat(processed_params, ", "); //
                }
                strcat(processed_params, "double ");
                strcat(processed_params, param);

                // Add the parameter to the name register
                strcat(name_register, param);
                strcat(name_register, " ");

                param = strtok(NULL, " "); // will now return the next token, "b", and so on, until all tokens (a, b, c, d, e) are processed.
            }
        }

        // Remember the current file position
        long original_position = ftell(ml_file);

        // Check if the function has a return statement or its a void function
        int temp_start_w_tab = 0;
        int found_return = find_word_in_lines(ml_file, line_number, &temp_start_w_tab, "return"); 

        // Rewind the file to the original position
        fseek(ml_file, original_position, SEEK_SET);

        // If the function has a return statement the function type is double since it returns a double value
        if (found_return && temp_start_w_tab) { 
            fprintf(c_file, "double %s(%s);\n", function_name, processed_params);
            fprintf(c_file, "double %s(%s) {\n", function_name, processed_params);
        } else {  
            // If the function does not have a return statement the function type is void
            fprintf(c_file, "void %s(%s);\n", function_name, processed_params);
            fprintf(c_file, "void %s(%s) {\n", function_name, processed_params);
        }
        // Set the inside_function flag to true since we have found a function word and we are inside this if block
        *inside_function = 1;
        
        // Return 1 to indicate that we have found a function and when it goes back to the main function it will skip the other checks
        return 1; 
    }
    // Return 0 if the line does not start with "function" and let other checks handle this line
    return 0; 
}

int check_assignment(char *line, FILE *c_file, char *name_register, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number) {
    
    // Check if the line contains an assignment operator "<-"
    char *assignment_position = strstr(line, "<-");
    if (assignment_position) {

        // Terminate to get the identifier
        *assignment_position = '\0';  

        // Left side is the identifier
        char *identifier = trim_whitespace(line);  
        // Right side is the expression
        char *expression = trim_whitespace(assignment_position + 2);  

        // Check if the identifier is a valid syntax identifier by checking if it is alphanumeric or underscore and less than 12 characters
        if (!is_valid_syntax_identifier(identifier)) {
            fprintf(stderr, "!Error: Invalid identifier '%s'\n", identifier);
            exit(EXIT_FAILURE);
        }

        // Create local copies of identifier and expression to prevent modification - since when ensure_main_function_printed is called it will modify the identifier and expression by calling find_word_in_lines and read_line
        char identifier_copy[MAX_IDENTIFIER_LENGTH + 1];
        strncpy(identifier_copy, identifier, MAX_IDENTIFIER_LENGTH);
        identifier_copy[MAX_IDENTIFIER_LENGTH] = '\0';

        char expression_copy[MAX_LINE_LENGTH];
        strncpy(expression_copy, expression, MAX_LINE_LENGTH);
        expression_copy[MAX_LINE_LENGTH - 1] = '\0';

        // Ensure the main function is printed before anything else without a tab, this will only print the main function if it has not been printed yet or if we are not inside a function. It also ensrures  that there are no functions in the following lines before the main function is printed
        ensure_main_function_printed(c_file, inside_function, main_printed, ml_file, line_number);

        // Always print a tab if the line starts with a tab or if we are inside_function
        if (*start_w_tab || *inside_function) {
            if (!identifier_exists(name_register, identifier_copy)) {
                strcat(name_register, identifier_copy);
                strcat(name_register, " ");
                fprintf(c_file, "    double %s = %s;\n", identifier_copy, expression_copy);  // Declare and assign the variable
            } else {
                fprintf(c_file, "    %s = %s;\n", identifier_copy, expression_copy);
            }
        } else { //otherwise print the line without a tab
            if (!identifier_exists(name_register, identifier_copy)) {
                strcat(name_register, identifier_copy);
                strcat(name_register, " ");
                fprintf(c_file, "double %s = %s;\n", identifier_copy, expression_copy);  // Declare and assign the variable
            } else {
                fprintf(c_file, "%s = %s;\n", identifier_copy, expression_copy);
            }
        }
        return 1;
    }
    return 0;
}


// Check if the line is a return statement
int check_return(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number) {
    if (strncmp(line, "return", 6) == 0 && isspace(line[6])) {
        char *expression = trim_whitespace(line + 6);

        // Ensure the main function is printed before anything else without a tab
        ensure_main_function_printed(c_file, inside_function, main_printed, ml_file, line_number);
        
        if (*start_w_tab || *inside_function) {
            fprintf(c_file, "    return %s;\n", expression);
        } else {
            fprintf(c_file, "return %s;\n", expression);
        }
        return 1;
    }
    return 0;
}

int check_print(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number) {
    if (strncmp(line, "print", 5) == 0 && isspace(line[5])) {
        char *expression = trim_whitespace(line + 5);

        // Ensure the main function is printed before anything else without a tab
        ensure_main_function_printed(c_file, inside_function, main_printed, ml_file, line_number);

        if (*start_w_tab || *inside_function) {
            fprintf(c_file, "    printf(\"%%.6f\\n\", %s);\n", expression);
        } else {
            fprintf(c_file, "printf(\"%%.6f\\n\", %s);\n", expression);
        }
        return 1;
    }
    return 0;
}

int check_other(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number) {
    // Ensure the main function is printed before anything else without a tab
    ensure_main_function_printed(c_file, inside_function, main_printed, ml_file, line_number);

    // Print the line exactly as it appears in the .ml file
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

int is_valid_syntax_identifier(const char *str) {   // Check if the identifier is alphanumeric or underscore and less than 12 characters
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
    return line[0] == '\t';
}

void ensure_main_function_printed(FILE *c_file, int *inside_function, int *main_printed, FILE *ml_file, int *line_number) {
    if (!*inside_function && !*main_printed) {
        long original_position = ftell(ml_file);

        int temp_start_w_tab = 0;
        int found_function = find_word_in_lines(ml_file, line_number, &temp_start_w_tab, "function");

        fseek(ml_file, original_position, SEEK_SET);

        if (!found_function) {
            fprintf(c_file, "\nint main(int argc, char *argv[]) {\n");
            *main_printed = 1;
        }
    }
}

int find_word_in_lines(FILE *ml_file, int *line_number, int *temp_start_w_tab, const char *target_word) {
    char *next_line;
    while ((next_line = read_line(ml_file, line_number, temp_start_w_tab)) != NULL) {
        if (strstr(next_line, target_word) != NULL) {
            return 1;
        }
    }
    return 0;
}

char* read_line(FILE *ml_file, int *line_number, int *start_w_tab) {
    static char line[MAX_LINE_LENGTH];

    // Read the next line from the file
    while (fgets(line, sizeof(line), ml_file) != NULL) {
        (*line_number)++;  // Increment the line number for tracking

        // Check if the line contains a comment (assuming comments start with '#')
        char *comment_start = strchr(line, '#');
        if (comment_start) {
            *comment_start = '\0';  // Remove the comment part by terminating the line early
        }

        // Determine if the line starts with a tab character
        *start_w_tab = starts_with_tab(line);

        // Trim leading and trailing whitespace from the line
        char *trimmed_line = trim_whitespace(line);

        // If the line is not empty, return it
        if (trimmed_line && strlen(trimmed_line) > 0) {
            return trimmed_line;
        }
    }

    // Return NULL if no more lines are available
    return NULL;
}

// Function to check if the identifier exists as a full word in name_register
int identifier_exists(const char *name_register, const char *identifier) {
    // Create a copy of name_register with space-padding
    char padded_name_register[MAX_LINE_LENGTH];
    snprintf(padded_name_register, sizeof(padded_name_register), " %s ", name_register);

    // Create a copy of identifier with space-padding
    char padded_identifier[MAX_IDENTIFIER_LENGTH + 3]; // +3 for space at start, end, and null terminator
    snprintf(padded_identifier, sizeof(padded_identifier), " %s ", identifier);

    // Check for the exact match with space padding
    return strstr(padded_name_register, padded_identifier) != NULL;
}