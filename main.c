#include <stdio.h>
#include <stdlib.h>
#include "bt.c"

int main() {
    short root = NIL;

    if (!btopen()) {
        btree = fopen("btree.dat","w");
        fclose(btree);

        btopen();
        putroot(root);
    }

    root = getroot();
    BTKEY key;

    while (1) {
        int op;
        printf("DESEJA FAZER UMA NOVA INCLUSAO [0|1]: ");
        scanf("%d", &op);

        if (!op) break;

        printf("id: ");
        scanf("%d", &(key.id));
        key.rrn = 1;

        insertnode(getroot(), key);
    }

    btclose();
}
