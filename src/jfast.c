#include "jfast.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Dynamic string buffer
#define INIT_BUF_SIZE 1024
char *str_init() {
    // [size (8) | index (8) | rest]
    char *buf = malloc(INIT_BUF_SIZE);
    buf[INIT_BUF_SIZE-1] = '\0';
    *(uint64_t *)buf = (uint64_t)1024;
    *((uint64_t *)buf + 1) = 16;
    return buf;
}
char *str_write(char *buf, char *data, uint64_t len) {
    uint64_t size = *(uint64_t *)buf;
    uint64_t pos = *((uint64_t *)buf + 1);
    if (pos + len > size-1) {
        char *new_buf = malloc(size*2);
        strncpy(new_buf, buf, size);
        *(uint64_t *)buf = size*2;
        *((uint64_t *)buf + 1) = pos;
        free(buf);
        buf = new_buf;
    }
    strncpy(buf + pos, data, len);
    *((uint64_t *)buf + 1) = pos + len;
    return buf;
}
char *strip_header(char *str) {
    uint64_t pos = *((uint64_t *)str + 1);
    str[pos] = '\0';
    return (char *)((uint64_t *)str + 2);
}

// Dynamic stack
#define INIT_STACK_SIZE 100
uint64_t *stack_init() {
    // [size (8) | top (8) | rest]
    uint64_t *buf = malloc(8 * INIT_STACK_SIZE);
    buf[0] = 10;
    buf[1] = 2;
    return buf;
}
uint64_t *stack_pop(uint64_t *stack, uint64_t *into) {
    uint64_t top = stack[1];
    if (top < 3) return stack;
    *into = stack[top-1];
    stack[1] = top - 1;
    return stack;
}
uint64_t *stack_push(uint64_t *stack, uint64_t item) {
    uint64_t top = stack[1];
    uint64_t size = *stack;
    if (top == size) {
        uint64_t *new_stack = malloc(8 * size * 2);
        memcpy(new_stack, stack, 8 * size);
        new_stack[0] = size * 2;
        free(stack);
        stack = new_stack;
    }
    stack[top] = item;
    stack[1] = top + 1;
    return stack;
}

char *json_dumps(json_t *json) {
    enum state {
        init,
        obj,
        obj_items,
        arr,
        arr_items
    };
    enum state __attribute__((aligned(64))) state = init;

    json_t *prev = (json_t *)stack_init();
    enum state *states = (enum state *)stack_init();

    uint64_t arr_index = 0;
    uint64_t *arr_indices = stack_init();

    char *buf = str_init();
    while(1) {
    switch(json->kind) {
        case Obj: {
            json_obj_t json_obj = json->val.obj;
            switch(state) {
                case init:
                    states = (enum state *)stack_push((uint64_t *)states, (uint64_t)state);
                    state = obj;
                break;
                case obj: {
                    str_write(buf, "{", 1);
                    uint64_t sz = json_obj.sz;
                    if (sz > 0) {
                        str_write(buf, "\"", 1);
                        str_write(buf, json_obj.keys[sz-1], strlen(json_obj.keys[sz-1]));
                        str_write(buf, "\":", 2);
                        state = obj_items;
                        json->val.obj.sz--;
                        prev = (json_t *)stack_push((uint64_t *)prev, (uint64_t)json);
                        states = (enum state *)stack_push((uint64_t *)states, (uint64_t)state);
                        json = &json->val.obj.values[sz-1];
                        state = obj;
                    } else {
                        state = obj_items;
                    }
                }
                break;
                case arr: {
                    str_write(buf, "{", 1);
                    uint64_t sz = json_obj.sz;
                    if (sz > 0) {
                        str_write(buf, "\"", 1);
                        str_write(buf, json_obj.keys[sz-1], strlen(json_obj.keys[sz-1]));
                        str_write(buf, "\":", 2);
                        state = arr_items;
                        json->val.arr.sz--;
                        prev = (json_t *)stack_push((uint64_t *)prev, (uint64_t)json);
                        states = (enum state *)stack_push((uint64_t *)states, (uint64_t)state);
                        json = &json->val.arr.values[sz-1];
                        state = arr;
                    } else {
                        state = arr_items;
                    }
                }
                break;
                case obj_items: {
                    uint64_t sz = json_obj.sz;
                    if (sz > 0) {
                        str_write(buf, ",", 1);
                        str_write(buf, "\"", 1);
                        str_write(buf, json_obj.keys[sz-1], strlen(json_obj.keys[sz-1]));
                        str_write(buf, "\":", 2);
                        state = obj_items;
                        json->val.obj.sz--;
                        prev = (json_t *)stack_push((uint64_t *)prev, (uint64_t)json);
                        states = (enum state *)stack_push((uint64_t *)states, (uint64_t)state);
                        json = &json->val.obj.values[sz-1];
                        state = obj;
                    } else {
                        str_write(buf, "}", 1);
                        prev = (json_t *)stack_pop((uint64_t *)prev, (uint64_t*)&json);
                        states = (enum state *)stack_pop((uint64_t *)states, (uint64_t*)&state);
                        if (state == init) goto done;
                    }
                }
                break;
                case arr_items: {
                    uint64_t sz = json_obj.sz;
                    if (sz > 0) {
                        str_write(buf, ",", 1);
                        str_write(buf, "\"", 1);
                        str_write(buf, json_obj.keys[sz-1], strlen(json_obj.keys[sz-1]));
                        str_write(buf, "\":", 2);
                        state = arr_items;
                        json->val.arr.sz--;
                        prev = (json_t *)stack_push((uint64_t *)prev, (uint64_t)json);
                        states = (enum state *)stack_push((uint64_t *)states, (uint64_t)state);
                        json = &json->val.arr.values[sz-1];
                        state = arr;
                    } else {
                        str_write(buf, "}", 1);
                        prev = (json_t *)stack_pop((uint64_t *)prev, (uint64_t*)&json);
                        states = (enum state *)stack_pop((uint64_t *)states, (uint64_t*)&state);
                        if (state == init) goto done;
                    }
                }
                break;
            }
        }
        break;
        case Arr: {
            json_arr_t json_arr = json->val.arr;
            switch(state) {
                case init:
                    states = (enum state *)stack_push((uint64_t *)states, (uint64_t)state);
                    state = arr;
                break;
                case obj:
                case arr: {
                    arr_indices = stack_push(arr_indices, arr_index);
                    arr_index = 0;
                    str_write(buf, "[", 1);
                    uint64_t sz = json_arr.sz;
                    if (sz > 0 && arr_index < sz) {
                        state = arr_items;
                        prev = (json_t *)stack_push((uint64_t *)prev, (uint64_t)json);
                        states = (enum state *)stack_push((uint64_t *)states, (uint64_t)state);
                        json = &json->val.arr.values[arr_index];
                        state = arr;
                    } else {
                        state = arr_items;
                    }
                }
                break;
                case arr_items: {
                    uint64_t sz = json_arr.sz;
                    if (sz > 0 && arr_index + 1 < sz) {
                        str_write(buf, ",", 1);
                        state = arr_items;
                        arr_index++;
                        prev = (json_t *)stack_push((uint64_t *)prev, (uint64_t)json);
                        states = (enum state *)stack_push((uint64_t *)states, (uint64_t)state);
                        json = &json->val.obj.values[arr_index];
                        state = arr;
                    } else {
                        str_write(buf, "]", 1);
                        prev = (json_t *)stack_pop((uint64_t *)prev, (uint64_t*)&json);
                        states = (enum state *)stack_pop((uint64_t *)states, (uint64_t*)&state);
                        arr_indices = stack_pop(arr_indices, &arr_index);
                        if (state == init) goto done;
                    }
                }
                break;
            }
        }
        break;
        case Val: {
            json_val_t json_val = json->val.val;
            switch(json_val.kind) {
                case Int: {
                    char snum[256];
                    sprintf(snum, "%lld", json_val.val.inum);
                    str_write(buf, snum, strlen(snum));
                }
                break;
                case Float: {
                    char snum[256];
                    sprintf(snum, "%g", json_val.val.fnum);
                    str_write(buf, snum, strlen(snum));
                }
                break;
                case Bool: {
                    if (json_val.val.bool == True)
                        str_write(buf, "true", 4);
                    else
                        str_write(buf, "false", 5);
                }
                break;
                case Null:
                    str_write(buf, "null", 4);
                break;
                case String:
                    str_write(buf, "\"", 1);
                    str_write(buf, json_val.val.string, strlen(json_val.val.string));
                    str_write(buf, "\"", 1);
                break;
            }
            switch(state) {
                case init:
                    goto done;
                case obj:
                    prev = (json_t *)stack_pop((uint64_t *)prev, (uint64_t*)&json);
                    states = (enum state *)stack_pop((uint64_t *)states, (uint64_t*)&state);
                break;
                case arr:
                    prev = (json_t *)stack_pop((uint64_t *)prev, (uint64_t*)&json);
                    states = (enum state *)stack_pop((uint64_t *)states, (uint64_t*)&state);
                break;
            }
        }
        break;
    }}
    done:
        return strip_header(buf);
}

#define END '\0'
#define OBJ_START '{'
#define OBJ_END '}'
#define COMMA ','
#define COLON ':'
#define ARR_START '['
#define ARR_END ']'
#define QUOTE '"'
#define ESCAPE '\\'

char *eat_ws(char * buf) {
    while(1) {
    switch(*buf) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            buf++;
        break;
        default:
            return buf;
    }}
}

void json_parse_val(char *buf, uint64_t* off) {
}

char *json_parse_string(char *buf, char* string) {
    enum state {
        init,
        in_quote
    } state = init;
    uint64_t i = 0;

    while(1) {
    switch(*buf) {
        case QUOTE:
            if (state == in_quote)
                return buf;
            else
                state = in_quote;
        break;
        // TODO escapes
        default:
            if (state != in_quote) {
                fprintf(stderr, "Unexpected character: %c\n", *buf);
                exit(1);
            }
            string[i] = *buf;
            i++;
    }
    buf++;
    }
    return buf;
}

char *json_parse_null(char *buf, json_t *json) {
    if (buf[0] == 'n' &&
        buf[1] == 'u' &&
        buf[2] == 'l' &&
        buf[3] == 'l') {
        json->kind = Val;
        json->val.val.kind = Null;
        return buf + 4;
    }
    fprintf(stderr, "Unexpected character: %c\n", *buf);
    exit(1);
}

char *json_parse_bool(char *buf, json_t *json) {
    if (buf[0] == 't' &&
        buf[1] == 'r' &&
        buf[2] == 'u' &&
        buf[3] == 'e') {
        json->kind = Val;
        json->val.val.kind = Bool;
        json->val.val.val.bool = True;
        return buf + 4;
    } else if (buf[0] == 'f' &&
               buf[1] == 'a' &&
               buf[2] == 'l' &&
               buf[3] == 's' &&
               buf[4] == 'e') {
        json->kind = Val;
        json->val.val.kind = Bool;
        json->val.val.val.bool = False;
        return buf + 5;
    }
    fprintf(stderr, "Unexpected character: %c\n", *buf);
    exit(1);
}

#define MAX_DIGIT_C 256
char *json_parse_num(char *buf, json_t *json) {
    enum state {
        acc_int,
        acc_frac
    } state = acc_int;
    char accum_int[MAX_DIGIT_C];
    int accum_int_c = 0;
    char accum_frac[MAX_DIGIT_C];
    int accum_frac_c = 0;
    while(1) {
    switch(*buf) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if(state == acc_int) {
                accum_int[accum_int_c] = *buf;
                accum_int_c++;
            } else {
                accum_frac[accum_frac_c] = *buf;
                accum_frac_c++;
            }
            buf++;
            continue;
        case '.':  
            state = acc_frac;
            buf++;
        break;
        default: {
            accum_int[accum_int_c] = '\0';
            accum_frac[accum_frac_c] = '\0';
            json->kind = Val;
            if(state == acc_int) {
                uint64_t num = atoi(accum_int);
                json->val.val.kind = Int;
                json->val.val.val.inum = num;
            } else {
                char fbuf[accum_int_c+accum_frac_c+1];
                sprintf(fbuf, "%s.%s", accum_int, accum_frac);
                double num = strtod(fbuf, NULL);
                json->val.val.kind = Float;
                json->val.val.val.fnum = num;
            }
            return buf;
        }
    }}
}

json_t *json_parse(char *buf) {
    enum state {
        init,
        in_object,
        read_key,
        read_key_colon,
        read_key_colon_val,
        in_array,
        array_item
    };
    enum state __attribute__((aligned(64))) state = init;

    enum state *states = (enum state *)stack_init();
    json_t *prev = (json_t *)stack_init();
    int prev_json_c = 0;

    json_t *json = malloc(sizeof(json_t));

    while(1) {
    no_increment:
    buf = eat_ws(buf);
    switch(*buf) {
        case ARR_START: {
            switch(state) {
                case init: {
                    json->kind = Arr;
                    json->val.arr.sz = 0;
                    json->val.arr.values = malloc(sizeof(json_t) * 10); // TODO fixme
                    states = (enum state *)stack_push((uint64_t *)states, (uint64_t)state);
                    state = in_array;
                }
                break;
                case in_array: {
                    state = array_item;
                    uint64_t index = json->val.arr.sz;
                    json->val.arr.sz++;
                    json->val.arr.values[index].kind = Arr;
                    json->val.arr.values[index].val.arr.sz = 0;
                    json->val.arr.values[index].val.arr.values = malloc(sizeof(json_t) * 10); // TODO fixme
                    prev = (json_t *)stack_push((uint64_t *)prev, (uint64_t)json);
                    prev_json_c++;
                    states = (enum state *)stack_push((uint64_t *)states, (uint64_t)state);
                    json = &json->val.arr.values[index];
                    state = in_array;
                }
                break;
                case read_key_colon: {
                    prev = (json_t *)stack_push((uint64_t *)prev, (uint64_t)json);
                    state = read_key_colon_val;
                    states = (json_t *)stack_push((uint64_t *)states, (uint64_t)state);
                    prev_json_c++;
                    uint64_t sz = json->val.obj.sz;
                    json->val.obj.sz++;
                    json = &json->val.obj.values[sz];
                    json->kind = Arr;
                    json->val.arr.sz = 0;
                    json->val.arr.values = malloc(sizeof(json_t) * 10); // TODO fixme
                    state = in_array;
                }
                break;
                default:
                    goto fail;
            }
        }
        break;
        case ARR_END: {
            switch(state) {
                case in_array: {
                    if (prev_json_c <= 0) return json;
                    prev = (json_t *)stack_pop((uint64_t *)prev, (uint64_t*)&json);
                    states = (json_t *)stack_pop((uint64_t *)states, (uint64_t*)&state);
                    prev_json_c--; // TODO maybe fix
                }
                break;
                case array_item: {
                    if (prev_json_c <= 0) return json;
                    prev = (json_t *)stack_pop((uint64_t *)prev, (uint64_t*)&json);
                    states = (json_t *)stack_pop((uint64_t *)states, (uint64_t*)&state);
                    prev_json_c--;
                }
                break;
                default:
                    goto fail;
            }
        }
        break;
        case OBJ_START:
            switch(state) {
                case init:
                    json->kind = Obj;
                    json->val.obj.sz = 0;
                    json->val.obj.keys = malloc(80); // TODO fixme
                    json->val.obj.values = malloc(sizeof(json_t) * 10); // TODO fixme
                    state = in_object;
                break;
                case read_key_colon: {
                    prev = (json_t *)stack_push((uint64_t *)prev, (uint64_t)json);
                    prev_json_c++;
                    uint64_t sz = json->val.obj.sz;
                    json->val.obj.sz++;
                    json = &json->val.obj.values[sz];
                    json->kind = Obj;
                    json->val.obj.sz = 0;
                    json->val.obj.keys = malloc(80); // TODO fixme
                    json->val.obj.values = malloc(sizeof(json_t) * 10); // TODO fixme
                    state = in_object;
                }
                break;
                case in_array: {
                    prev = (json_t *)stack_push((uint64_t *)prev, (uint64_t)json);
                    states = (enum state *)stack_push((uint64_t *)states, (uint64_t)state);
                    prev_json_c++;
                    uint64_t sz = json->val.arr.sz;
                    json->val.arr.sz++;
                    json = &json->val.arr.values[sz];
                    json->kind = Obj;
                    json->val.obj.sz = 0;
                    json->val.obj.keys = malloc(80); // TODO fixme
                    json->val.obj.values = malloc(sizeof(json_t) * 10); // TODO fixme
                    state = in_object;
                }
                break;
                default:
                    goto fail;
            }
        break;
        case OBJ_END:
            switch(state) {
                case read_key_colon_val:
                    if (prev_json_c <= 0) return json;
                    prev = (json_t *)stack_pop((uint64_t *)prev, (uint64_t*)&json);
                    states = (enum state *)stack_pop((uint64_t *)states, (uint64_t*)&state);
                    prev_json_c--;
                break;
                case in_object:
                    if (prev_json_c <= 0) return json;
                    prev = (json_t *)stack_pop((uint64_t *)prev, (uint64_t*)&json);
                    prev_json_c--; // TODO maybe fix
                    states = (enum state *)stack_pop((uint64_t *)states, (uint64_t*)&state);
                break;
                case in_array:
                    if (prev_json_c <= 0) return json;
                    prev = (json_t *)stack_pop((uint64_t *)prev, (uint64_t*)&json);
                    prev_json_c--; // TODO maybe fix
                    states = (enum state *)stack_pop((uint64_t *)states, (uint64_t*)&state);
                break;
                default:
                    goto fail;
            }
        break;
        case QUOTE:
            switch(state) {
                case in_object: {
                    char *string = malloc(1024); // TODO fixme
                    json->val.obj.keys[json->val.obj.sz] = string;
                    buf = json_parse_string(buf, string);
                    state = read_key;
                }
                break;
                case init: {
                    char *string = malloc(1024); // TODO fixme
                    json->kind = Val;
                    json->val.val.kind = String;
                    json->val.val.val.string = string;
                    buf = json_parse_string(buf, string);
                    return json;
                }
                case read_key_colon: {
                    json_t *json_string = malloc(sizeof(json_t));
                    char *string = malloc(1024); // TODO fixme
                    json_string->kind = Val;
                    json_string->val.val.kind = String;
                    json_string->val.val.val.string = string;
                    buf = json_parse_string(buf, string);
                    uint64_t sz = json->val.obj.sz;
                    json->val.obj.sz++;
                    json->val.obj.values[sz] = *json_string;
                    state = read_key_colon_val;
                }
                break;
                case in_array: {
                    json_t *json_string = malloc(sizeof(json_t));
                    char *string = malloc(1024); // TODO fixme
                    json_string->kind = Val;
                    json_string->val.val.kind = String;
                    json_string->val.val.val.string = string;
                    buf = json_parse_string(buf, string);
                    uint64_t sz = json->val.obj.sz;
                    json->val.arr.sz++;
                    json->val.arr.values[sz] = *json_string;
                }
                break;
                default:
                    goto fail;
            }
        break;
        case COLON:
            if (state != read_key) goto fail;
            state = read_key_colon;
        break;
        case COMMA:
            switch(state) {
                case read_key_colon_val:
                    state = in_object;
                break;
                case array_item:
                    state = in_array;
                break;
                case in_array:
                case in_object:
                break;
                default:
                    goto fail;
            }
        break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            json_t *json_num = malloc(sizeof(json_t));
            buf = json_parse_num(buf, json_num);
            switch(state) {
                case init:
                    return json_num; // optimistic
                break;
                case in_object:
                    goto fail;
                break;
                case read_key:
                    goto fail;
                break;
                case read_key_colon: {
                    uint64_t sz = json->val.obj.sz;
                    json->val.obj.sz++;
                    json->val.obj.values[sz] = *json_num;
                    state = read_key_colon_val;
                    goto no_increment;
                }
                break;
                case in_array: {
                    uint64_t sz = json->val.obj.sz;
                    json->val.arr.sz++;
                    json->val.arr.values[sz] = *json_num;
                    goto no_increment;
                }
                break;
                case read_key_colon_val:
                    goto fail;
                break;
                default:
                    goto fail;
            }
        }
        break;
        case 't':
        case 'f': {
            json_t *json_bool = malloc(sizeof(json_t));
            buf = json_parse_bool(buf, json_bool);
            switch(state) {
                case init:
                    return json_bool; // optimistic
                break;
                case read_key_colon: {
                    uint64_t sz = json->val.obj.sz;
                    json->val.obj.sz++;
                    json->val.obj.values[sz] = *json_bool;
                    state = read_key_colon_val;
                    goto no_increment;
                }
                break;
                case in_array: {
                    uint64_t sz = json->val.arr.sz;
                    json->val.arr.sz++;
                    json->val.arr.values[sz] = *json_bool;
                    goto no_increment;
                }
                break;
                default:
                    goto fail;
            }
        }
        break;
        case 'n': {
            json_t *json_null = malloc(sizeof(json_t));
            buf = json_parse_null(buf, json_null);
            switch(state) {
                case init:
                    return json_null; // optimistic
                break;
                case read_key_colon: {
                    uint64_t sz = json->val.obj.sz;
                    json->val.obj.sz++;
                    json->val.obj.values[sz] = *json_null;
                    state = read_key_colon_val;
                    goto no_increment;
                }
                break;
                case in_array: {
                    uint64_t sz = json->val.arr.sz;
                    json->val.arr.sz++;
                    json->val.arr.values[sz] = *json_null;
                    goto no_increment;
                }
                break;
                default:
                    goto fail;
            }
        }
        break;
        default:
            goto fail;
    }
    buf++;
    }

    return json;

    fail:
        fprintf(stderr, "Unexpected character: %c\n", *buf);
        exit(1);
}