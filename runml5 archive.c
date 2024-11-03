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

// Enum to represent types
typedef enum {
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING
} ArgType;

// Function prototypes
void print_usage(const char *program_name); // Print the usage of the program
void generate_c_code(FILE *ml_file, FILE *c_file, int arg_c, char *arg_v[]); //generate C code for the ML file and write it to the C file and its calls generate_c_code_for_line
void generate_c_code_for_line(char *line, FILE *c_file, char *name_register, int *line_number, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int arg_c, char *arg_v[]); //generate C code for each line in the ML file and its different from generate_c_code because it will check if the line is a function, assignment, return, print, or other
void compile_and_run_c_code(const char *c_filename, int argc, char *argv[]);
int is_valid_syntax_identifier(const char *str);
char* trim_whitespace(char* str);
char* read_line(FILE *ml_file, int *line_number, int *start_w_tab);
int check_function(char *line, FILE *c_file, char *name_register, int *inside_function, FILE *ml_file, int *line_number);
int check_assignment(char *line, FILE *c_file, char *name_register, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]);
int check_return(char *line, FILE *c_file,char *name_register, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]);
int check_print(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]);
int check_other(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]);
int starts_with_tab(const char *line);
void ensure_main_function_printed(FILE *c_file, int *inside_function, int *main_printed, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]);
int find_word_in_lines(FILE *ml_file, int *line_number, int *start_w_tab, const char *target_word); // Find a word in the lines of a file for example "function" or "return" or "print"...etc
int identifier_exists(const char *name_register, const char *identifier);
ArgType detect_type(const char *s); // Detect the type of the argument

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

    // This function checks if a number is an integer and if not, the program will print with 6 decimal places 
    fprintf(c_file, "int is_integer(double num) {\n");
    fprintf(c_file, "    return num == (int)num;\n");
    fprintf(c_file, "}\n\n");
 
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

        generate_c_code_for_line(line, c_file, name_register, &line_number, &inside_function, &main_printed, &start_w_tab, ml_file, arg_c, arg_v);
    }
}

void generate_c_code_for_line(char *line, FILE *c_file, char *name_register, int *line_number, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int arg_c, char *arg_v[]) {
    // Check if the line starts a function definition
    if (check_function(line, c_file, name_register, inside_function, ml_file, line_number)) {
        return;
    }

    // Check if the line is an assignment operation
    if (check_assignment(line, c_file, name_register, inside_function, main_printed, start_w_tab, ml_file, line_number, arg_c, arg_v)) {
        return;
    }

    // Check if the line is a return statement
    if (check_return(line, c_file, name_register, inside_function, main_printed, start_w_tab, ml_file, line_number, arg_c, arg_v)) {
        return;
    }

    // Check if the line is a print statement
    if (check_print(line, c_file, inside_function, main_printed, start_w_tab, ml_file, line_number, arg_c, arg_v)) {
        return;
    }

    // If none of the above, print the line as-is
    check_other(line, c_file, inside_function, main_printed, start_w_tab, ml_file, line_number, arg_c, arg_v);

}

//check_function function will check if the line starts with "function" and if it does, it will print the function declaration and definition. It will also check if the function name is valid and if it is not, it will print an error message and exit. It will also check if the function name already exists in the name register and if it does, it will print an error message and exit. It will also check if the function has a return statement and if it does, it will print the function with a return type of double, otherwise, it will print the function with a return type of void.
int check_function(char *line, FILE *c_file, char *name_register, int *inside_function, FILE *ml_file, int *line_number) {
    if (strncmp(line, "function", 8) == 0 && isspace(line[8])) { 
        line += 8; // Skip the "function" keyword
        line = trim_whitespace(line);

        char *func_name_token = strtok(line, " ");
        char *params = strtok(NULL, "");

        char function_name[MAX_IDENTIFIER_LENGTH + 1];
        if (func_name_token) {
            strncpy(function_name, func_name_token, MAX_IDENTIFIER_LENGTH);
            function_name[MAX_IDENTIFIER_LENGTH] = '\0';
        } else {
            fprintf(stderr, "!Error: Missing function name\n");
            exit(EXIT_FAILURE);
        }

        if (identifier_exists(name_register, function_name)) {
            fprintf(stderr, "!Error: Function or variable name '%s' already declared\n", function_name);
            exit(EXIT_FAILURE);
        }

        strcat(name_register, function_name);
        strcat(name_register, " ");

        char processed_params[MAX_LINE_LENGTH] = ""; 
        if (params) {
            char *param = strtok(params, " ");
            while (param) {
                if (!is_valid_syntax_identifier(param)) {
                  fprintf(stderr, "!Error: Invalid parameter name '%s' at line %d\n", param, *line_number);
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

        long original_position = ftell(ml_file); //this is because find_word_in_lines will change the position of the file pointer so we need to store the original position to return to it later

        int temp_start_w_tab = 0;
        int found_return = find_word_in_lines(ml_file, line_number, &temp_start_w_tab, "return");

        fseek(ml_file, original_position, SEEK_SET);

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
}

//check_assignment function will check if the line contains an assignment operation and if it does, it will print the assignment but before that, it will check if the identifier is valid and if it is not, it will print an error message and exit. It also checks if the identifier exists in the name register and if it does not, it will add it to the name register and print the assignment with a tab if the line starts with a tab or if we are inside a function. Before printing it will ensure that the main function is printed if there are no more functions in the ML file or if the main function has not been printed yet.
int check_assignment(char *line, FILE *c_file, char *name_register, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]) {
    char *assignment_position = strstr(line, "<-");
    if (assignment_position) {
        *assignment_position = '\0';  

        char *identifier = trim_whitespace(line);
        char *expression = trim_whitespace(assignment_position + 2);

        if (!is_valid_syntax_identifier(identifier)) {
            fprintf(stderr, "!Error: Invalid identifier '%s' at line %d\n", identifier, *line_number);
            exit(EXIT_FAILURE);
        }

        char identifier_copy[MAX_IDENTIFIER_LENGTH + 1];
        strncpy(identifier_copy, identifier, MAX_IDENTIFIER_LENGTH);
        identifier_copy[MAX_IDENTIFIER_LENGTH] = '\0';

        char expression_copy[MAX_LINE_LENGTH];
        strncpy(expression_copy, expression, MAX_LINE_LENGTH);
        expression_copy[MAX_LINE_LENGTH - 1] = '\0';

        ensure_main_function_printed(c_file, inside_function, main_printed, ml_file, line_number, arg_c, arg_v);

        if (*start_w_tab || *inside_function) {
            if (!identifier_exists(name_register, identifier_copy)) {
                strcat(name_register, identifier_copy);
                strcat(name_register, " ");
                fprintf(c_file, "    double %s = %s;\n", identifier_copy, expression_copy);
            } else {
                fprintf(c_file, "    %s = %s;\n", identifier_copy, expression_copy);
            }
        } else {
            if (!identifier_exists(name_register, identifier_copy)) {
                strcat(name_register, identifier_copy);
                strcat(name_register, " ");
                fprintf(c_file, "double %s = %s;\n", identifier_copy, expression_copy);
            } else {
                fprintf(c_file, "%s = %s;\n", identifier_copy, expression_copy);
            }
        }
        return 1;
    }
    return 0;
}

//check_return function will check if the line starts with "return" and if it does, it will print the expression
int check_return(char *line, FILE *c_file, char *name_register, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]) {
    if (strncmp(line, "return", 6) == 0 && isspace(line[6])) {
        char *expression = trim_whitespace(line + 6);

        // Tokenize the expression, extract the first token
        char *first_token = strtok(expression, " "); // Get the first token

        if (!identifier_exists(name_register, first_token)) {
            fprintf(stderr, "!Error: Invalid or no parameter name '%s' at line %d\n", first_token, *line_number);
            exit(EXIT_FAILURE);
            }

        if (*start_w_tab || *inside_function) {
            fprintf(c_file, "    return %s;\n", expression);
        } else {
            fprintf(c_file, "return %s;\n", expression);
        }
        return 1;
    }
    return 0;
}

//check_print function will check if the line starts with "print" and if it does, it will print the expression with 0 decimal places if it is an integer and 6 decimal places if it is a double
// Check if the line is a print statement, and print it with the appropriate format
// Check if the line is a print statement, and print it with the appropriate format
int check_print(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]) {
    if (strncmp(line, "print", 5) == 0 && isspace(line[5])) {
        char *expression = trim_whitespace(line + 5);
        ensure_main_function_printed(c_file, inside_function, main_printed, ml_file, line_number, 0, NULL);

        // Detect the type of the expression and print accordingly
        ArgType arg_type = detect_type(expression);

        if (*start_w_tab || *inside_function) {
            if (arg_type == TYPE_INT) {
                fprintf(c_file, "    printf(\"%%d\\n\", %s);\n", expression);
            } else if (arg_type == TYPE_DOUBLE) {
                fprintf(c_file, "    printf(\"%%.6f\\n\", %s);\n", expression);
            }
        } else {
            if (arg_type == TYPE_INT) {
                fprintf(c_file, "printf(\"%%d\\n\", %s);\n", expression);
            } else if (arg_type == TYPE_DOUBLE) {
                fprintf(c_file, "printf(\"%%.6f\\n\", %s);\n", expression);
            }
        }
        return 1;
    }
    return 0;
}


// Check if the line is anything other than function, assignment, return, or print and print it as-is
int check_other(char *line, FILE *c_file, int *inside_function, int *main_printed, int *start_w_tab, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]) {
    ensure_main_function_printed(c_file, inside_function, main_printed, ml_file, line_number, 0, NULL);
    if (*start_w_tab || *inside_function) {
        fprintf(c_file, "    %s;\n", line);
    } else {
        fprintf(c_file, "%s;\n", line);
    }
    return 1;
}

// Compile and run the C code by creating a unique binary filename and then removing it after execution
void compile_and_run_c_code(const char *c_filename, int argc, char *argv[]) {
    char binary_filename[64];
    snprintf(binary_filename, sizeof(binary_filename), "ml-%d", getpid());// Create a unique binary filename

    char compile_command[256];
    snprintf(compile_command, sizeof(compile_command), "cc -std=c11 -Wall -Werror -o %s %s", binary_filename, c_filename);
    if (system(compile_command) != 0) {
        fprintf(stderr, "!Error: Compilation failed\n");
        remove(binary_filename);
        exit(EXIT_FAILURE);
    }

    char execute_command[256];
    snprintf(execute_command, sizeof(execute_command), "./%s", binary_filename);
    for (int i = 2; i < argc; i++) {
        strcat(execute_command, " ");
        strcat(execute_command, argv[i]);
    }

    if (system(execute_command) != 0) {
        fprintf(stderr, "!Error: Execution failed\n");
    }

    remove(binary_filename);
}

// Check if the identifier is alphanumeric or underscore and less than 12 characters
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
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    *(end + 1) = '\0';
    return str;
}

// Check if the line starts with a tab character
int starts_with_tab(const char *line) {
    return line[0] == '\t';
}

// Function to check if a string is an integer, double, or string
ArgType detect_type(const char *s) {
    char *endptr;
    
    // First, try to convert it to a double
    double val = strtod(s, &endptr);
    
    // If it cannot be converted to a double, it's a string
    if (*endptr != '\0') {
        return TYPE_STRING;
    }
    
    // If the double value has no fractional part, it's an integer
    if (val == (int)val) {
        return TYPE_INT;
    }
    
    // Otherwise, it's a double
    return TYPE_DOUBLE;
}


// Ensure the main function is printed in the C file when there is no more function in the ML file, if there are more than 2 arguments, the main function will also convert the arguments to double
void ensure_main_function_printed(FILE *c_file, int *inside_function, int *main_printed, FILE *ml_file, int *line_number, int arg_c, char *arg_v[]) {
    if (!*inside_function && !*main_printed) {
        long original_position = ftell(ml_file);
        int temp_start_w_tab = 0;
        int found_function = find_word_in_lines(ml_file, line_number, &temp_start_w_tab, "function");

        fseek(ml_file, original_position, SEEK_SET);

        if (!found_function) {
            fprintf(c_file, "\nint main(int argc, char *argv[]) {\n");

            // Declare arg variables based on their detected types
            for (int i = 1; i < arg_c; ++i) {
                // Determine the type of the argument
                ArgType arg_type = detect_type(arg_v[i]);
                
                // Declare and print based on the detected type
                if (arg_type == TYPE_INT) {
                    fprintf(c_file, "        int arg%d = atoi(argv[%d]);\n", i - 1, i);
                } else if (arg_type == TYPE_DOUBLE) {
                    fprintf(c_file, "        double arg%d = atof(argv[%d]);\n", i - 1, i);
                } else {
                    fprintf(c_file, "        char arg%d = *argv[%d];\n", i - 1, i);
                }
            }
            *main_printed = 1;
        }
    }
}


// Find a word in the lines of a file
int find_word_in_lines(FILE *ml_file, int *line_number, int *temp_start_w_tab, const char *target_word) {
    char *next_line;
    while ((next_line = read_line(ml_file, line_number, temp_start_w_tab)) != NULL) {
        if (strstr(next_line, target_word) != NULL) {
            return 1;
        }
    }
    return 0;
}

// Utility function to read a line from a file and return a trimmed version and it 
// also returns the line number and if the line starts with a tab
char* read_line(FILE *ml_file, int *line_number, int *start_w_tab) {
    static char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), ml_file) != NULL) {
        (*line_number)++;
        char *comment_start = strchr(line, '#'); // Check for comments and remove them
        if (comment_start) {
            *comment_start = '\0';
        }
        *start_w_tab = starts_with_tab(line); // Check if the line starts with a tab and set the flag
        char *trimmed_line = trim_whitespace(line);
        if (trimmed_line && strlen(trimmed_line) > 0) {
            return trimmed_line;
        }
    }
    return NULL;
}

// Check if the identifier exists (already defined) in the name register
int identifier_exists(const char *name_register, const char *identifier) {
    char padded_name_register[MAX_LINE_LENGTH];
    snprintf(padded_name_register, sizeof(padded_name_register), " %s ", name_register);

    char padded_identifier[MAX_IDENTIFIER_LENGTH + 3];
    snprintf(padded_identifier, sizeof(padded_identifier), " %s ", identifier);

    return strstr(padded_name_register, padded_identifier) != NULL;
}
