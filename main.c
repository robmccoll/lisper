#include "lisper.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

const char *
read_file(const char * path, uint8_t ** data_out, size_t *len_out)
{
  const char * err = NULL;
  *data_out = NULL;

  struct stat st;
  if(0 != stat(path, &st)) {
    err = "could not stat file";
    goto read_file_exit;
  }

  FILE * fp = fopen(path, "rb");
  if(!fp) {
    err = "could not open file";
    goto read_file_exit;
  }

  if(NULL == (*data_out = calloc(st.st_size, 1))) {
    err = "failed to allocate buffer";
    goto read_file_exit;
  }

  if(st.st_size != fread(*data_out, 1, st.st_size, fp)) {
    err = "could not read file";
    goto read_file_exit;
  }

  *len_out = st.st_size;

read_file_exit:
  if((*data_out) && err) free(*data_out);
  if(fp) fclose(fp);
  return err;
}
  
int
main(int argc, char *argv[])
{
  if(argc < 2) {
    printf("usage: %s <filename>\n", argv[0]);
    return -1;
  }

  uint8_t * list = NULL;
  size_t len = 0;

  const char * err = NULL;

  if(err = read_file(argv[1], &list, &len)) {
    printf("reading %s failed: %s", argv[1], err);
    return -1;
  }

  const char * cur = list;
  int cur_len = len;

  printf("read in:\n%.*s\n\n", len, list);

  exp_t parsed;
  if(err = parse_list(&parsed, &cur, &cur_len)) {
    printf("parsing err: %s\n%s\n", err, cur);
  }
  print_exp(&parsed, 0);

  free_exp(&parsed);
  free(list);
  return 0;
}
