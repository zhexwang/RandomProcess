#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <stdio.h>
#include <stdlib.h>

#define COLOR_RED "\033[01;31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_HIGH_GREEN "\033[01;32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[01;36m"
#define COLOR_END "\033[0m"

#define PRINT(format, ...) do{ fprintf(stderr, format, ##__VA_ARGS__);}while(0)
#define INFO(format, ...) do{ fprintf(stderr,COLOR_GREEN format COLOR_END, ##__VA_ARGS__);}while(0)
#define BLUE(format, ...) do{ fprintf(stderr,COLOR_BLUE format COLOR_END, ##__VA_ARGS__);}while(0)
#define YELLOW(format, ...) do{ fprintf(stderr,COLOR_YELLOW format COLOR_END, ##__VA_ARGS__);}while(0)
#define ERR(format, ...) do{ fprintf(stderr,COLOR_RED format COLOR_END, ##__VA_ARGS__);}while(0)

#ifdef _DEBUG
#define PERROR(cond, format) do{if(!(cond)){perror(format); abort();}}while(0)
#define FATAL(cond, ...) do{if(cond) {ERR("FATAL: %s:%-4d "__VA_ARGS__, __FILE__, __LINE__); abort();}}while(0)
#define ASSERT(cond)       do{if(!(cond)) {ERR("ASSERT failed: %s:%-4d ", __FILE__, __LINE__); abort();}}while(0)
#define ASSERTM(cond, format, ...) do{if(!(cond)) {ERR("ASSERT failed: %s:%-4d "format, __FILE__, __LINE__, ## __VA_ARGS__); abort();}}while(0)
#define NOT_IMPLEMENTED(who)        do{ERR("%s() in %s is not implemented by %s\n", __FUNCTION__, __FILE__, #who); abort();} while (0)
#else
#define FATAL(cond, ...) do{if(cond) {ERR("FATAL: %s:%-4d "__VA_ARGS__, __FILE__, __LINE__); abort();}}while(0)
#define ASSERT(cond)       
#define PERROR(cond, format)
#define ASSERTM(cond, format, ...) 
#define NOT_IMPLEMENTED(who)       
#endif

#define btoa(x) ((x)?"true":"false")

#endif
