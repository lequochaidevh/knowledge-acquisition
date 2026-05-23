#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int value;
    // with Basic Doubly LinkList will 2 ptr for 1 Node
    // save 50% with ptr
    uintptr_t xored;  // node prev
} Node;

Node *node_create(int value) {
    Node *node = malloc(sizeof(*node));
    memset(node, 0, sizeof(*node));
    node->value = value;
    return node;
}

typedef struct {
    Node *begin;
    Node *end;
} LinkList;

void linklist_append(LinkList *ll, int value) {
    // If begin and end are NULL
    if (ll->end == NULL) {
        assert(ll->begin == NULL);
        ll->end   = node_create(value);
        ll->begin = ll->end;
    } else {
        Node *node  = node_create(value);  // Nnext
        node->xored = (uintptr_t)ll->end;  // Nnext->xored = XSend = Nprev
        ll->end->xored ^= (uintptr_t)node;
        /**
         * N1.xor = xs.end = N0
         *
         * xs.end.xor(0) ^= N1
         * => N0.xor = 0 ^ N1 = N1
         * N0.xor = 00 ^ N1
         * same:
         * N1.xor = N0 ^ N2
         * N2.xor = N1 ^ N3
         * N3.xor = N2 ^ N4
         * ...
         */
        ll->end = node;
    }
}

Node *node_next(Node *node, uintptr_t *prev) {
    Node *next = (Node *)(node->xored ^ (*prev));
    *prev      = (uintptr_t)node;
    return next;
}

int main() {
    LinkList xs = {0};
    for (int x = 5; x <= 10; ++x) {
        linklist_append(&xs, x);
    }

    uintptr_t prev = 0;
    // node_next = xored ^ node_prev
    for (Node *iter = xs.begin; iter; iter = node_next(iter, &prev)) {
        printf("%d\n", iter->value);
    }

    printf("---------\n");

    prev = 0;  // NOTE: reset before loop linklist
    for (Node *iter = xs.end; iter; iter = node_next(iter, &prev)) {
        printf("%d\n", iter->value);
    }
    return 0;
}
