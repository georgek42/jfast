#include "jfast.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int compare(json_t *a, json_t *b) {
    if (a->kind != b->kind) return 0;
    switch (a->kind) {
        case Val: {
            json_val_t aa = a->val.val;
            json_val_t bb = b->val.val;
            if (aa.kind != bb.kind) return 0;
            switch (aa.kind) {
                case String:
                    return strcmp(aa.val.string, bb.val.string) == 0;
                break;
                case Int:
                    return aa.val.inum == bb.val.inum;
                break;
                case Float:
                    return aa.val.fnum == bb.val.fnum;
                break;
                case Bool:
                    return aa.val.bool == bb.val.bool;
                break;
                case Null:
                    return 1;
                break;
            }
        }
        break;
        case Obj: {
            json_obj_t aa = a->val.obj;
            json_obj_t bb = b->val.obj;
            if (aa.sz != bb.sz) return 0;
            uint32_t sz = aa.sz;
            while (sz != 0) {
                if (strcmp(aa.keys[sz-1], bb.keys[sz-1])) return 0;
                if (!compare(&aa.values[sz-1], &bb.values[sz-1])) return 0;
                sz--;
            }
        }
        break;
        case Arr: {
            json_arr_t aa = a->val.arr;
            json_arr_t bb = b->val.arr;
            if (aa.sz != bb.sz) return 0;
            uint32_t sz = aa.sz;
            while (sz != 0) {
                if (!compare(&aa.values[sz-1], &bb.values[sz-1])) return 0;
                sz--;
            }

        }
        break;
    }
    return 1;
}

int test(char *source, char *target) {
    json_t *parsed = json_parse(source);
    char *parsed_s = json_dumps(parsed);
    if(strcmp(parsed_s, target) == 0) {
        printf("OK!\n");
        return 1;
    } else {
        printf("FAIL!: expected %s, got %s\n", target, parsed_s);
        return 0;
    }
}

char *read_file(char *filename) {
    FILE *f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);
    uint64_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(fsize + 1);
    fread(buf, 1, fsize, f);
    fclose(f);
    return buf;
}

int main(int argc, char **argv) {
    int i = 1;
    char filename[256];
    int passed = 0;
    int failed = 0;
    for (; i < argc; i++) {
        printf("Testing %s: ", argv[i]);
        char *source = read_file(argv[i]);
        sprintf(filename, "%s.compact", argv[i]);
        char *target = read_file(filename);
        if (test(source, target)) passed++; else failed++;
    }
    printf("%d tests ran: %d passed: %d failed\n", passed + failed, passed, failed);
    exit(failed > 0);
}