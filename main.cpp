#include "dd_scanf.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    char buf[] = "My name is henry du and I'm 16 years old.";
    int x = 0;
    dd_sscanf(buf, "%*s%*s%*s%*s%*s%*s%*s%d", &x);
    printf("%d", x);
}