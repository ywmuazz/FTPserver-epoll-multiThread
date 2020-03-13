#ifndef STR_H_INCLUDED
#define STR_H_INCLUDED

void str_trim_crlf(char* str);
void str_split(const char* str,char *left,char* right,char c);
int  str_all_space(const char* str);
void str_upper(char* str);
long long str_to_longlong(const char* str);
unsigned str_octal_to_uint(const char* str);

#endif // STR_H_INCLUDED
