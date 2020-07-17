#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 
// Tokenizer
//


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

// Input program
char *user_input;

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

// Reports an error location and exit.
void error_at(char *loc, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	int pos = loc - user_input;
	fprintf(stderr, "%s\n", user_input);
	fprintf(stderr, "%*s", pos, ""); // print pos spaces.
	fprintf(stderr, "^ ");
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
      		error_at(token->str, "expected a number");
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

// Tokenize 'user_input' and returns new tokens.
Token *tokenize() {
	char *p = user_input;
	Token head;
	head.next = NULL;
	Token *cur = &head;

	while (*p) {
		// skip empty
		if (isspace(*p)) {
			p++;
			continue;
		}
		
		// Punctuator
		if (strchr("+-+/()", *p)) {
			cur = new_token(TK_RESERVED, cur, p++);
			continue;
		}

		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p);
			cur->val = strtol(p, &p, 10);
			continue;
		}

		error_at(p, "invalid token");
	}

	new_token(TK_EOF, cur, p);
	return head.next;
}

// 
// Parser
//

typedef enum {
	ND_ADD, // +
	ND_SUB, // -
	ND_MUL, // *
	ND_DIV, // /
	ND_NUM, // Integer
} NodeKind;

// AST node type

typedef struct Node Node;
struct Node {
	NodeKind kind; // Node kind
	Node *lhs;     // Left-hand side
	Node *rhs;     // Right-hand side
	int val;       // Used if kind == ND_NUM
};

Node *new_node(NodeKind kind) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
	Node *node = new_node(kind);
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_num(int val) {
	Node *node = new_node(ND_NUM);
	node->val = val;
	return node;
}

Node *expr();
Node *mul();
Node *primary();

// expr = mul ("+" mul | "-" mul)*
Node *expr() {
	Node *node = mul();

	for (;;) {
		if (consume('+'))
			node = new_binary(ND_ADD, node, mul());
		else if (consume('-'))
			node = new_binary(ND_SUB, node, mul());
		else
			return node;
	}
}

// mul = primary ("*" primary | "/" parimary)*
Node *mul(){
	Node *node = primary();

	for (;;) {
		if (consume('*'))
			node = new_binary(ND_MUL, node, primary());
		else if (consume('/'))
			node = new_binary(ND_DIV, node, primary());
		else
			return node;
	}
}

// primary = "(" expr ")" | num
Node *primary() {
	if (consume('(')) {
		Node *node = expr();
		expect(')');
		return node;
	}

	return new_num(expect_number());
}

//
// Code generator
//

void gen(Node *node) {
	if (node->kind == ND_NUM) {
		printf("	push %d\n", node->val);
		return;
	}

	gen(node->lhs);
	gen(node->rhs);

	printf("	pop rdi\n");
	printf("	pop rax\n");

	switch (node->kind) {
	case ND_ADD:
		printf("	add rax, rdi\n");
		break;
	case ND_SUB:
		printf("	sub rax, rdi\n");
		break;
	case ND_MUL:
		printf("	imul rax, rdi\n");
		break;
	case ND_DIV:
		printf("	cqo\n");
		printf("	idiv rdi\n");
		break;
	}

	printf("	push rax\n");
}


int main(int argc, char **argv) {
	if (argc != 2)
		error("The number of parameters is not correct");
	
	// Tokenize and parse.
	user_input = argv[1];
	token = tokenize();
	Node *node = expr();

	// Print out the first half of assembly.
	printf(".intel_syntax noprefix\n");
	printf(".globl main\n");
	printf("main:\n");

	// Traverse the AST to emit assembly.
	gen(node);

	// A result must be at the top of the stack, so pop it
	// to RAX to make it a program exit code.
	printf("	pop rax\n");
	printf("	ret\n");
	return 0;
}
