#ifndef UTILS_C
#define UTILS_C

#include "utils.h"
#include <string.h>
#include <unistd.h>

// #include <regex.h>
// #define IPV4_ADDR_REGEXP "(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})"
// int parseIPV4(char* t){
//     return 0;
// }

void info(const char* msg){
    write(STDOUT_FILENO,msg,strlen(msg));
}

void warn(const char* msg){
    write(STDOUT_FILENO,msg,strlen(msg));
}

void err(const char* msg){
    write(STDERR_FILENO,msg,strlen(msg));
}

#endif