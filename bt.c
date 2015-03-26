#define MAXKEYS 4
#define MINKEYS MAXKEYS/2
#define NIL (-1)
#define NOKEY '@'
#define NO 0
#define YES 1

typedef struct {
    int id;
    short rrn;
} BTKEY;

typedef struct {
    short keycount;
    BTKEY key[MAXKEYS];
    short child[MAXKEYS+1];
} BTPAGE;

#define PAGESIZE sizeof(BTPAGE)

FILE *btree;

int btopen() {
    btree = fopen("btree.dat", "r+");
    return btree != NULL;
}

void btclose() {
    fclose(btree);
}

short getroot() {
    short root;

    fseek(btree, 0L, 0);
    if (!fread(&root, sizeof(short), 1, btree)) {
        printf("Error: Unable to get root. \007\n");
        exit(1);
    }

    return root;
}

void putroot(short root) {
    fseek(btree, 0, 0);
    fwrite(&root, sizeof(short), 1, btree);
}

int btread(short rrn, BTPAGE *page) {
    long addr;

    addr = (long) rrn * (long)PAGESIZE + 2L;
    fseek(btree, addr, 0);

    return (fread(page, PAGESIZE, 1, btree));
}

int searchnode(int id, BTPAGE *page, short *pos) {
    int i;
    for (i = 0; i < page->keycount && (id > page->key[i].id); i++);

    *pos = i;

    return *pos < page->keycount && (id == page->key[*pos].id) ? YES : NO;
}

void insertpage(BTKEY key, short r_child, BTPAGE *page) {
    int i;
    for (i = page-> keycount; key.id < page->key[i-1].id && i > 0; i--) {
        page->key[i] = page->key[i-1];
        page->child[i+1] = page->child[i];
    }

    page->keycount++;
    page->key[i] = key;
    page->child[i+1] = r_child;
}

int btwrite(short rrn, BTPAGE *page){
    long addr;

    addr = (long)rrn * (long) PAGESIZE + 2L;
    fseek(btree, addr, 0);

    return (fwrite(page, PAGESIZE, 1, btree));
}

short newpage() {
    long addr;

    fseek(btree, 0, 2);
    addr = ftell(btree) - 2L;

    return ((short) addr / PAGESIZE);
}

void initpage(BTPAGE *page){
    int i;

    for (i = 0; i < MAXKEYS; i++) {
        page->key[i].id = NOKEY;
        page->key[i].rrn = NIL;
        page->child[i] = NIL;
    }

    page->child[MAXKEYS] = NIL; //pois o vetor child tem maxkeys + 1
}

short createroot(BTKEY key, short left, short right) {
    BTPAGE page;
    short rrn;

    rrn = newpage();
    initpage(&page);

    page.key[0] = key;
    page.child[0] = left;
    page.child[1] = right;
    page.keycount = 1;

    btwrite(rrn, &page);
    putroot(rrn);

    return rrn;
}

short createtree() {
    BTKEY key;
    btree = fopen("btree.dat","w");
    fclose(btree);

    btopen();

    key.id = 1;
    key.rrn = 1;

    return createroot(key, NIL, NIL);
}

void split(BTKEY key, short r_child, BTPAGE *p_oldpage, BTKEY *promo_key, short *promo_r_child, BTPAGE *p_newpage){
    int j;
    BTKEY workkeys[MAXKEYS+1];
    short workchil[MAXKEYS+2];

    for (j = 0; j < MAXKEYS; j++) {
        workkeys[j] = p_oldpage->key[j];
        workchil[j] = p_oldpage->child[j];
    }

    workchil[j] = p_oldpage->child[j];

    for (j = MAXKEYS; key.id < workkeys[j-1].id && j > 0; j--) {
        workkeys[j] = workkeys[j-1];
        workchil[j+1] = workchil[j];
    }

    workkeys[j] = key;
    workchil[j+1] = r_child;

    *promo_r_child = newpage();
    initpage(p_newpage);

    for (j = 0; j < MINKEYS; j++){
        p_oldpage->key[j] = workkeys[j];
        p_oldpage->child[j] = workchil[j];

        p_newpage->key[j] = workkeys[j+1+MINKEYS];
        p_newpage->child[j] = workchil[j+1+MINKEYS];

        p_oldpage->key[j+MINKEYS].id = NOKEY;
        p_oldpage->key[j+MINKEYS].rrn = NIL;

        p_oldpage->child[j+1+MINKEYS] = NIL;
    }

    p_oldpage->child[MINKEYS] = workchil[MINKEYS];
    p_newpage->child[MINKEYS] = workchil[j+1+MINKEYS];
    p_newpage->keycount = MAXKEYS - MINKEYS;
    p_oldpage->keycount = MINKEYS;

    *promo_key = workkeys[MINKEYS];
}

int insert(short rrn, BTKEY key, short *promo_r_child, BTKEY *promo_key, int *found) {
    int promoted;
    short pos, p_b_rrn;
    BTPAGE page, newpage;
    BTKEY p_b_key;

    if (rrn == NIL) {
        *promo_key = key;
        *promo_r_child = NIL;
        return(YES);
    }

    btread(rrn, &page);

    *found = searchnode(key.id, &page, &pos);
    if (*found) {
        printf("Error: attempt to insert duplicate key: %c \n\007", key.id);
        return(NO);
    }

    promoted = insert(page.child[pos], key, &p_b_rrn, &p_b_key, found);
    if (!promoted) {
        return(NO);
    }

    if (page.keycount < MAXKEYS) {
        insertpage(p_b_key, p_b_rrn, &page);
        btwrite(rrn, &page);
        return(NO);
    } else {
        printf("Divisao de no.\n");
        split(p_b_key, p_b_rrn, &page, promo_key, promo_r_child, &newpage);
        printf("Chave %d promovida.\n", promo_key->id);

        btwrite(rrn, &page);
        btwrite(*promo_r_child, &newpage);

        return(YES);
    }
}

void insertnode(short root, BTKEY key) {
    int promoted, found;
    short promo_rrn;

    BTKEY promo_key;

    if (root == NIL) {
        root = createroot(key, NIL, NIL);
        printf("Chave %d inserida com sucesso\n", key.id);
        return;
    }

    promoted = insert(root, key, &promo_rrn, &promo_key, &found);

    if (promoted)
        root = createroot(promo_key, root, promo_rrn);

    if (!found)
        printf("Chave %d inserida com sucesso\n", key.id);
}
