#include "lisper.h"
#include  <stdio.h>

const char *
parse_list(exp_t * out, const char ** in, int *len) {
  const char * err = NULL;

  if(!(*len) || (**in) != '(') {
    return "List does not begin with '('";
  }

  out->type = EXP_VEC;
  out->val.vec.children = NULL;
  out->val.vec.cap = 0;
  out->val.vec.len = 0;

  for((*in)++, (*len)--;**in && *len; (*in)++, (*len)--) {
    char c = **in;
    switch(c) {
      case ')':
        return err;
      case '(': {
        exp_t sublist = {0};
        if(err = parse_list(&sublist, in, len)) {
          return err;
        }
        if(err = vec_push_copy(out, &sublist)) {
          return err;
        }
       } break;
      case ' ':
      case '\t':
        continue;
      case '"': {
        exp_t str = {0};
        if(err = parse_string(&str, in, len)) {
          return err;
        }
        if(err = vec_push_copy(out, &str)) {
          return err;
        }
       } break;
      case 'x': {
        exp_t str = {0};
        if(err = parse_hex_string(&str, in, len)) {
          return err;
        }
        if(err = vec_push_copy(out, &str)) {
          return err;
        }
       } break;
      case '-':
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
        exp_t str = {0};
        if(err = parse_number(&str, in, len)) {
          return err;
        }
        if(err = vec_push_copy(out, &str)) {
          return err;
        }
       } break;
      default:
        return "Unexpected character when parsing start of list element.";
    }
  }
  return "Reached end of input without list being closed.";
}

const char *
parse_string(exp_t * out, const char ** in, int *len) {
  const char * err = NULL;

  if(!(*len) || (**in) != '"') {
    return "String does not begin with '\"'";
  }

  out->type = EXP_BYTES;
  out->val.bytes.data = NULL;
  out->val.bytes.cap = 0;
  out->val.bytes.len = 0;

  int escaped = 0;

  for((*in)++, (*len)--;**in && *len; (*in)++, (*len)--) {
    char c = **in;
    switch(c) {
      case '"': {
        if(escaped) {
          if(err = bytes_push_byte(out, c)) {
            return err;
          }
          escaped = 0;
          continue;
        }
      } return err;
      case '\\': {
        if(escaped) {
          if(err = bytes_push_byte(out, c)) {
            return err;
          }
          escaped = 0;
          continue;
        }
        escaped = 1;
      } break;
      case 't': {
        if(escaped) {
          if(err = bytes_push_byte(out, '\t')) {
            return err;
          }
          escaped = 0;
          continue;
        }
        if(err = bytes_push_byte(out, c)) {
          return err;
        }
      } break;
      case 'n': {
        if(escaped) {
          if(err = bytes_push_byte(out, '\n')) {
            return err;
          }
          escaped = 0;
          continue;
        }
        if(err = bytes_push_byte(out, c)) {
          return err;
        }
      } break;
      case 'r': {
        if(escaped) {
          if(err = bytes_push_byte(out, '\r')) {
            return err;
          }
          escaped = 0;
          continue;
        }
        if(err = bytes_push_byte(out, c)) {
          return err;
        }
      } break;
      default: {
        if(escaped) {
          return "Unrecognized escape sequence.";
        }
        if(err = bytes_push_byte(out, c)) {
          return err;
        }
      } break;
    }
  }
  return "Reached end of input without string being closed.";
}

const char *
parse_hex_string(exp_t * out, const char ** in, int *len) {
  const char * err = NULL;

  if(!(*len) || (**in) != 'x') {
    return "Hex string does not begin with 'x\"'";
  }
  (*in)++;
  (*len)--;
  if(!(*len) || (**in) != '"') {
    return "Hex string does not begin with 'x\"'";
  }

  out->type = EXP_BYTES;
  out->val.bytes.data = NULL;
  out->val.bytes.cap = 0;
  out->val.bytes.len = 0;

  uint8_t cur = 0;
  uint8_t even = 1;
  for((*in)++, (*len)--;**in && *len; (*in)++, (*len)--) {
    char c = **in;
    cur = cur << 4;
    even = !even;
    
    if(c >= '0' && c <= '9') {
      cur = cur | (c - '0');
    } else if(c >= 'A' && c <= 'F') {
      cur = cur | (c - 'A' + 10);
    } else if(c >= 'a' && c <= 'f') {
      cur = cur | (c - 'a' + 10);
    } else if(c == '"') {
      if(even) {
        return "Hex string does not end on byte boundary";
      }
      return NULL;
    }

    if (even) {
        if(err = bytes_push_byte(out, cur)) {
          return err;
        }
    }
  }
  return "Reached end of input without hex string being closed.";
}

const char *
parse_number(exp_t * out, const char ** in, int *len) {
}

const char *
vec_push_copy(exp_t * out, exp_t *in) {
  if(out->val.vec.len >= out->val.vec.cap) {
    int new_cap = out->val.vec.cap * 2;
    new_cap = new_cap ? new_cap : 128;
    exp_t * new_children = realloc(out->val.vec.children, new_cap * sizeof(exp_t));
    if(!new_children) {
      return "Allocating more space for list vec failed.";
    }
    out->val.vec.children = new_children;
    out->val.vec.cap = new_cap;
  }
  out->val.vec.children[out->val.vec.len++] = *in;
  return NULL;
}

const char *
bytes_push_byte(exp_t * out, uint8_t in) {
  if(out->val.bytes.len >= out->val.bytes.cap) {
    int new_cap = out->val.bytes.cap * 2;
    new_cap = new_cap ? new_cap : 128;
    uint8_t * new_data = realloc(out->val.bytes.data, new_cap);
    if(!new_data) {
      return "Allocating more space for byte buffer failed.";
    }
    out->val.bytes.data = new_data;
    out->val.bytes.cap = new_cap;
  }
  out->val.bytes.data[out->val.bytes.len++] = in;
  return NULL;
}

const char *
print_exp(exp_t * out, int indent) {
  for(int i = 0; i < indent; i++) {
    printf("  ");
  }

  switch(out->type) {
    case EXP_INT64:
      printf("EXP_INT64   %ld\n", (long)out->val.i64);
      break;
    case EXP_FLOAT64:
      printf("EXP_FLOAT64 %lf\n", out->val.f64);
      break;
    case EXP_BYTES:
      printf("EXP_BYTES   %.*s\n", (int)out->val.bytes.len, out->val.bytes.data);
      break;
    case EXP_VEC:
      printf("EXP_VEC     (\n");
      for(int i = 0; i < out->val.vec.len; i++) {
        print_exp(&out->val.vec.children[i], indent+1);
      }
      for(int i = 0; i < indent; i++) {
        printf("  ");
      }
      printf(")\n");
    }
  return NULL;
}
