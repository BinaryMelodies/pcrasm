#ifndef _PREPROCESS_H
#define _PREPROCESS_H

typedef struct replacement_t replacement_t;

extern bool is_replacement;

void advance_line(void);

void begin_macro_definition(char * name);
void append_macro_definition_parameter(char * name);
void parse_macro_definition(void); // reads definition from input stream

void invoke_macro(replacement_t * next_replacement);

void handle_if_directive(expression_t * condition);
void handle_else_if_directive(expression_t * condition);
void handle_else_directive(void);
void handle_end_if_directive(void);

void preprocessor_define(const char * name, expression_t * exp);

#endif // _PREPROCESS_H
