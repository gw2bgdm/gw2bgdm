#pragma once

#ifdef __cplusplus
extern "C" {
#endif

char *sprintf_num(double n, char *outstr, int outstrlen);
char* sprintf_num_commas(long long n, char *outstr, int outstrlen);

#ifdef __cplusplus
}
#endif