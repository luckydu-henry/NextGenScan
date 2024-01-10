#define _CRT_SECURE_NO_WARNINGS
#include <string>

#include "dd_scanf.h"
#include <tuple>
#include <format>
#include <iostream>


int main(int argc, char* argv[]) {
    constexpr char buf[] = R"(
#version 330 core
uniform vec4 aPos;
void main() {
    gl_Position = aPos * (1.0, 1.0, 1.0, 1.0);
}
)";

    char q[1 << 8] = "";

    dds::format_from(buf, "{:[^\n\0]}", q);

    std::cout << q << std::endl;
}