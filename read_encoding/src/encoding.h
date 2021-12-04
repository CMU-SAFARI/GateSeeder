#ifndef ENCODING_H
#define ENCODING_H

void encode_const_len(int fd, unsigned l, char *name);
void encode_var_len(int fd, unsigned l, char *name);

#endif
