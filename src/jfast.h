#ifndef JFAST_H
#define JFAST_H

#include <stdint.h>

struct json_t;

struct json_t *json_parse(char *buf);
char *json_dumps(struct json_t *json);

typedef struct json_obj_t {
    uint32_t sz; // Number of keys
    struct json_t* values;
    char** keys;
} json_obj_t;

typedef struct json_arr_t {
    uint32_t sz; // Number of values
    struct json_t* values;
} json_arr_t;

typedef enum json_bool_t {
    True,
    False
} json_bool_t;

typedef enum json_val_kind {
    String,
    Int,
    Float,
    Bool,
    Null
} json_val_kind;

typedef struct json_val_t {
    json_val_kind kind;
    union _json_val_t {
        char* string;
        int64_t inum;
        double fnum;
        json_bool_t bool;
    } val;
} json_val_t;

typedef enum json_kind {
    Obj,
    Arr,
    Val
} json_kind;

typedef struct json_t {
    json_kind kind;
    union _json_t {
        json_obj_t obj;
        json_arr_t arr;
        json_val_t val;
    } val;
} json_t;

#endif