
#include "isa.h"
#include "parser.h"

extern int yylex_direct(void);

typedef struct token_data_t
{
	int type;
	YYSTYPE value;
} token_data_t;

typedef struct token_sequence_t
{
	size_t capacity;
	size_t count;
	token_data_t * buffer;
} token_sequence_t;

void token_sequence_init(token_sequence_t * sequence)
{
	memset(sequence, 0, sizeof(token_sequence_t));
}

void token_sequence_free(token_sequence_t * sequence)
{
	free(sequence->buffer);
}

void token_sequence_append(token_sequence_t * sequence, int type, YYSTYPE value)
{
	if(sequence->count == sequence->capacity)
	{
		sequence->capacity += 16;
		sequence->buffer = sequence->buffer ? realloc(sequence->buffer, sequence->capacity * sizeof(token_data_t)) : malloc(sequence->capacity * sizeof(token_data_t));
	}
	sequence->buffer[sequence->count].type = type;
	sequence->buffer[sequence->count].value = value;
	sequence->count ++;
}

typedef struct macro_definition_t macro_definition_t;
struct macro_definition_t
{
	char * name;
	size_t argument_count;
	char ** argument_names;
	token_sequence_t definition;
	macro_definition_t * next;
};

macro_definition_t * macros;

void begin_macro_definition(char * name)
{
	macro_definition_t * macro = malloc(sizeof(macro_definition_t));
	memset(macro, 0, sizeof(macro_definition_t));
	macro->name = name;
	macro->next = macros;
	macros = macro;
}

void append_macro_definition_parameter(char * name)
{
	macros->argument_count++;
	macros->argument_names = macros->argument_names ? realloc(macros->argument_names, macros->argument_count * sizeof(char *)) : malloc(macros->argument_count * sizeof(char *));
	macros->argument_names[macros->argument_count - 1] = name;
}

bool is_replacement;

void advance_line(void)
{
	if(!is_replacement)
		current_parser_state->line_number++;
}

void parse_macro_definition(void)
{
	int token_type;
	if(!is_replacement) // one newline gets swallowed due to look ahead
		current_parser_state->line_number++;
	while(true)
	{
		token_type = yylex_direct();
		if(token_type == 0)
			break;
		if(token_type == KWD_ENDMACRO)
		{
			yylex_direct();
			break;
		}
		if(token_type == '\n' && !is_replacement)
		{
			current_parser_state->line_number++;
		}
		token_sequence_append(&macros->definition, token_type, yylval);
	}
}

struct replacement_t
{
	// the following fields are only set for macro replacement
	macro_definition_t * definition;
	size_t parameter_count;
	token_sequence_t * parameters;

	// which context should the tokens be evaluated in?
	// points to itself unless it is a parameter
	replacement_t * context;

	token_sequence_t * token_sequence;
	size_t current_token_index;
	replacement_t * next;
};

replacement_t * current_replacement;

int fetch_next_token(void)
{
restart_replacement:
	while(current_replacement != NULL)
	{
		if(current_replacement->current_token_index >= current_replacement->token_sequence->count)
		{
			replacement_t * current = current_replacement;
			current_replacement = current->next;
			if(current->parameters != NULL)
			{
				for(size_t i = 0; i < current->parameter_count; i++)
					token_sequence_free(&current->parameters[i]);
				free(current->parameters);
			}
			free(current);
		}
		else
		{
			int token_type = current_replacement->token_sequence->buffer[current_replacement->current_token_index].type;
			yylval = current_replacement->token_sequence->buffer[current_replacement->current_token_index].value;
			current_replacement->current_token_index ++;

			if(token_type == TOK_IDENTIFIER)
			{
				for(size_t argument_index = 0; argument_index < current_replacement->context->definition->argument_count; argument_index++)
				{
					if(strcmp(current_replacement->context->definition->argument_names[argument_index], yylval.s) == 0)
					{
						replacement_t * replacement = malloc(sizeof(replacement_t));
						memset(replacement, 0, sizeof(replacement_t));
						replacement->token_sequence = &current_replacement->context->parameters[argument_index];
						replacement->context = current_replacement->context;
						replacement->next = current_replacement;
						current_replacement = replacement;
						goto restart_replacement;
					}
				}
			}

			is_replacement = true;
			return token_type;
		}
	}

	is_replacement = false;
	return yylex_direct();
}

void invoke_macro(replacement_t * next_replacement)
{
	// read macro arguments
	int paren_depth = 0;
	int token_type = fetch_next_token();
	if(token_type != '\n' && token_type != 0)
	{
		next_replacement->parameter_count ++;
		next_replacement->parameters = malloc(sizeof(token_sequence_t));
		token_sequence_init(&next_replacement->parameters[0]);
		while(token_type != '\n' && token_type != 0)
		{
			if(token_type == '(' || token_type == '[' || token_type == '{')
			{
				paren_depth ++;
			}
			else if(token_type == ')' || token_type == ']' || token_type == '}')
			{
				// for simplicity, we do not care whether the parenthesis types are matched
				if(paren_depth > 0)
					paren_depth --;
			}
			else if(paren_depth == 0 && token_type == ',')
			{
				next_replacement->parameter_count ++;
				next_replacement->parameters = realloc(next_replacement->parameters, next_replacement->parameter_count * sizeof(token_sequence_t));
				token_sequence_init(&next_replacement->parameters[next_replacement->parameter_count - 1]);
			}
			else
			{
				token_sequence_append(&next_replacement->parameters[next_replacement->parameter_count - 1], token_type, yylval);
			}

			token_type = fetch_next_token();
		}
	}

	current_replacement = next_replacement;
}

static bool is_conditional_statement(int token_type)
{
	switch(token_type)
	{
	case KWD_IF:
	case KWD_ELSE_IF:
	case KWD_ELSE:
	case KWD_ENDIF:
		return true;
	default:
		return false;
	}
}

void preprocessor_define(const char * name, expression_t * exp)
{
	reference_t ref[1];
	evaluate_expression(exp, ref, 0);
	label_define(name, ref);
	int_clear(ref->value);
}

static unsigned int false_if_level = 0;
static bool past_true_if_clause = false;
static bool parsing_conditional = false;
static bool is_line_start = false;

void handle_if_directive(expression_t * condition)
{
	if(false_if_level > 0 || past_true_if_clause)
	{
		false_if_level ++;
	}
	else
	{
		reference_t condition_result[1];
		evaluate_expression(condition, condition_result, 0); // TODO: make strict
		if(is_scalar(condition_result) && int_is_zero(condition_result->value))
			false_if_level = 1;
		int_clear(condition_result->value);
	}
}

void handle_else_if_directive(expression_t * condition)
{
	if(!past_true_if_clause)
	{
		if(false_if_level == 0)
		{
			past_true_if_clause = true;
		}
		else if(false_if_level == 1)
		{
			reference_t condition_result[1];
			evaluate_expression(condition, condition_result, 0); // TODO: make strict
			if(is_scalar(condition_result) && int_is_zero(condition_result->value))
			{
				false_if_level = 1;
			}
			else
			{
				false_if_level = 0;
			}
			int_clear(condition_result->value);
		}
	}
}

void handle_else_directive(void)
{
	if(!past_true_if_clause)
	{
		if(false_if_level == 0)
		{
			past_true_if_clause = true;
		}
		else if(false_if_level == 1)
		{
			false_if_level = 0;
		}
	}
}

void handle_end_if_directive(void)
{
	if(false_if_level >= 1)
	{
		false_if_level --;
	}
	if(false_if_level == 0)
		past_true_if_clause = false;
}

int yylex(void)
{
	for(;;)
	{
		int token_type = fetch_next_token();

		if(!(false_if_level == 0 && !past_true_if_clause))
		{
//printf("Line %ld, (%d, %d), %d, %d\n", current_parser_state->line_number, false_if_level, past_true_if_clause, is_line_start, is_conditional_statement(token_type));
			if(is_line_start && is_conditional_statement(token_type))
			{
				parsing_conditional = true;
			}
			else if(!parsing_conditional)
			{
				if(token_type == '\n')
				{
					is_line_start = true;
					advance_line();
				}
				if(token_type != 0)
					continue;
			}
		}

		if(token_type == TOK_IDENTIFIER)
		{
			for(macro_definition_t * current_macro = macros; current_macro != NULL; current_macro = current_macro->next)
			{
				if(strcmp(current_macro->name, yylval.s) == 0)
				{
					replacement_t * replacement = malloc(sizeof(replacement_t));
					memset(replacement, 0, sizeof(replacement_t));
					replacement->token_sequence = &current_macro->definition;
					replacement->context = replacement;
					replacement->definition = current_macro;
					replacement->next = current_replacement;
					free(yylval.s);
					yylval.macro = replacement;
					return TOK_MACRONAME;
				}
			}
		}

		is_line_start = token_type == '\n';

		if(is_line_start)
			parsing_conditional = false;

		return token_type;
	}
}

