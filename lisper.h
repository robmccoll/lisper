#include  <stdint.h>
#include  <stdlib.h>

typedef enum exptype {
  EXP_INT64,
  EXP_FLOAT64,
  EXP_BYTES,
  EXP_VEC,
  EXP_CALL,
  EXP_SYMBOL,
} exptype_t;

typedef struct exp {
  exptype_t type;
  union {
    int64_t i64;
    double  f64;
    struct {
      uint8_t * data;
      size_t  len;
      size_t  cap;
    } bytes;
    struct {
      struct exp * children;
      size_t  len;
      size_t  cap;
    } vec;
  } val;
} exp_t;

const char *
print_exp(exp_t * out, int indent);

const char *
parse_list(exp_t * out, const char ** in, int * len);

const char *
parse_symbol(exp_t * out, const char ** in, int *len);

const char *
parse_string(exp_t * out, const char ** in, int *len);

const char *
parse_hex_string(exp_t * out, const char ** in, int *len);

const char *
parse_number(exp_t * out, const char ** in, int *len);

const char *
vec_push_copy(exp_t * out, exp_t *in);

const char *
bytes_push_byte(exp_t * out, uint8_t in);

const char *
free_exp(exp_t * out);
