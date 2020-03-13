#include"str.h"
#include"common.h"

void str_trim_crlf(char* str){
    for(int i=strlen(str)-1;i>=0;i--)
        if(str[i]=='\r'||str[i]=='\n')str[i]=0;
}

void str_split(const char* str,char *a,char *b,char c){
    char* p=(char*)strchr(str,c);
    if(p==NULL)strcpy(a,str);
    else {
        strncpy(a,str,p-str);
        a[p-str+1]=0;
        strcpy(b,p+1);
    }
}

int str_all_space(const char* str){
    for(;*str;++str)if((*str)!=' ')
        return 0;
    return 1;
}

void str_upper(char* str){
    for(;*str;++str)*str=toupper(*str);
}

long long str_to_longlong(const char* str){
    long long ret=0;
    for(;*str;++str){
        if(!isdigit(*str))return 0;
        ret=ret*10+((*str)-'0');
    }
    return ret;
}

///8进制
unsigned str_octal_to_uint(const char* str){
    unsigned ret=0;
    for(;*str;++str){
        if(!(*str>='0'&&*str<='7'))break;
        ret=(ret<<3)+(*str-'0');
    }
    return ret;
}




