#include "lisper.h"
#include <stdio.h>
#include <math.h>

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

  exp_t symbol = {0};
  if(err = parse_symbol(&symbol, in, len)) {
    return err;
  }

  if(symbol.val.bytes.len == 1 && symbol.val.bytes.data[0] == '.') {
    free_exp(&symbol);
  } else {
    out->type = EXP_CALL;
    if(err = vec_push_copy(out, &symbol)) {
      return err;
    }
  }

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
      case '\n':
      case '\r':
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
      case '$': {
        exp_t symbol = {0};
        if(err = parse_symbol(&symbol, in, len)) {
          return err;
        }
        if(err = vec_push_copy(out, &symbol)) {
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
parse_symbol(exp_t * out, const char ** in, int *len) {
  const char * err = NULL;

  out->type = EXP_SYMBOL;
  out->val.bytes.data = NULL;
  out->val.bytes.cap = 0;
  out->val.bytes.len = 0;

  for((*in)++, (*len)--;**in && *len; (*in)++, (*len)--) {
    char c = **in;
    if(!(c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
      break;
    }
  }

  for( ;**in && *len; (*in)++, (*len)--) {
    char c = **in;
    if(c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ')' || c == '(') {
      --(*in);
      ++(*len);
      break;
    }
    if(err = bytes_push_byte(out, c)) {
      return err;
    }
  }

  if(out->val.bytes.len == 0) {
    return "no symbol found";
  }
  return NULL;
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
  const char * err = NULL;

  out->type = EXP_INT64;
  out->val.i64 = 0;

  uint8_t is_float = 0;
  uint8_t past_decimal = 0;
  uint8_t past_e = 0;
  uint8_t is_negative = 0;
  uint8_t is_eng_negative = 0;
  int64_t num = 0;
  int64_t eng = 0;
  int64_t dec = 0;
  int64_t n = 0;
  for(;**in && *len; (*in)++, (*len)--) {
    n++;
    char c = **in;
    switch(c) {
      case '-': {
        if(n != 1) {
          return "Negative was not first character in number";
        }
        if(past_e) {
          is_eng_negative = 1;
        } else {
          is_negative = 1;
        }
      } break;
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
        if(past_e) {
          eng *= 10;
          eng += c - '0';
        } else if(past_decimal) {
          dec -= 1;
          num *= 10;
          num += c - '0';
        } else {
          num *= 10;
          num += c - '0';
        }
      } break;
      case '.': {
        if(past_decimal) {
          return "Multiple decimals in number";
        }
        is_float = 1;
        past_decimal = 1;
      } break;
      case 'e':
      case 'E': {
        if(past_e) {
          return "Multiple 'e' or 'E' in number";
        }
        past_e = 1;
        is_float = 1;
        n = 0;
      } break;
      default: {
        --(*in);
        ++(*len);
        if(is_negative) {
          num *= -1;
        }
        if(!is_float) {
          out->val.i64 = num;
          return NULL;
        }
        out->type = EXP_FLOAT64;
        if(is_eng_negative) {
          eng *= -1;
        }
        eng += dec;
        out->val.f64 = (double)num;
        out->val.f64 *= pow((double)10.0, (double)eng);
        return NULL;
      }
    }
  }
  return NULL;
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
      printf("EXP_FLOAT64 %lg\n", out->val.f64);
      break;
    case EXP_BYTES:
      printf("EXP_BYTES   %.*s\n", (int)out->val.bytes.len, out->val.bytes.data);
      break;
    case EXP_SYMBOL:
      printf("EXP_SYMBOL %.*s\n", (int)out->val.bytes.len, out->val.bytes.data);
      break;
    case EXP_VEC: {
      printf("EXP_VEC     (\n");
      for(int i = 0; i < out->val.vec.len; i++) {
        print_exp(&out->val.vec.children[i], indent+1);
      }
      for(int i = 0; i < indent; i++) {
        printf("  ");
      }
      printf(")\n");
    } break;
    case EXP_CALL: {
      printf("EXP_CALL     (\n");
      for(int i = 0; i < out->val.vec.len; i++) {
        print_exp(&out->val.vec.children[i], indent+1);
      }
      for(int i = 0; i < indent; i++) {
        printf("  ");
      }
      printf(")\n");
    } break;
  }
  return NULL;
}

const char *
free_exp(exp_t * out) {
  switch(out->type) {
    case EXP_INT64:
    case EXP_FLOAT64:
      break;
    case EXP_BYTES:
    case EXP_SYMBOL:
      if(out->val.bytes.data) free(out->val.bytes.data);
      out->val.bytes.data = NULL;
      out->val.bytes.len = 0;
      break;
    case EXP_VEC:
    case EXP_CALL:
      for(int i = 0; i < out->val.vec.len; i++) {
        free_exp(&out->val.vec.children[i]);
      }
      if(out->val.vec.children) free(out->val.vec.children);
      out->val.vec.children = NULL;
      out->val.vec.len = 0;
      break;
    }
  return NULL;
}
