#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Token Types
typedef enum {
	TK_RESERVED, // Symbol
	TK_NUM,      // Integer Token
	TK_EOF,      // End of Words Token
} TokenKind;

typedef struct Token Token;

// Token Structs
struct Token {
	TokenKind kind; // Token Type
	Token *next;    // Token Next Input
	int val;        // Integer, If kind is TK_NUM
	char *str;      // Token String
};

// Forcas Token
Token *token;

// Report error function
// Takes the same parameters as printf
void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

// If the next token is the expecting symbol, read one token forward
// and return true. Otherwise, return false.
bool consume(char op) {
	if (token->kind != TK_RESERVED || token->str[0] != op)
		return false;
	token = token->next;
	return true;
}

// If the next token is the expecting symbol, read one token forward,
// otherwise report error.
void expect(char op) {
       if (token->kind != TK_RESERVED || token->str[0] != op)
      		error("It is not '%c'", op);
       token = token->next;
}

// If the next token is the number, read and response one token forward,
// otherwise report error.
int expect_number() {
	if (token->kind != TK_NUM)
		error("Not are Numbers");
	int val = token->val;
	token = token->next;
	return val;
}

bool at_eof() {
	return token->kind == TK_EOF;
}

// Create a new token and connect it to cur
Token *new_token(TokenKind kind, Token *cur, char *str) {
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	cur->next = tok;
	return tok;
}

// Tokenize the input string p and return it
Token *tokenize(char *p) {
	Token head;
	head.next = NULL;
	Token *cur = &head;

	while (*p) {
		// skip empty
		if (isspace(*p)) {
			p++;
			continue;
		}

		if (*p == '+' || *p == '-') {
			cur = new_token(TK_RESERVED, cur, p++);
			continue;
		}

		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p);
			cur->val = strtol(p, &p, 10);
			continue;
		}

		error("Can't Tokenize!!");
	}

	new_token(TK_EOF, cur, p);
	return head.next;
}


int main(int argc, char **argv) {
	if (argc != 2) {
		error("The number of parameters is not correct");
		return 1;
	}
	
	// Tokenize
	token = tokenize(argv[1]);

	// Output the first half of the assembly
	printf(".intel_syntax noprefix\n");
	printf(".globl main\n");
	printf("main:\n");

	// Check first part of the expression has to be a number
	// and output mov instruction.
	printf("	mov rax, %d\n", expect_number());

	// Outputs the assembly, consuming the sequence of tokens
	// '+ <number>' or '- <number>'.
	while (!at_eof()) {
		if (consume('+')) {
			printf("	add rax, %d\n", expect_number());
			continue;
		}

		expect('-');
		printf("	sub rax, %d\n", expect_number());
	}

	printf("	ret\n");
	return 0;
}
