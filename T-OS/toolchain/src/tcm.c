#include <stdio.h>
#include <string.h>

void tcm_parse_file(const char* filename) {
    printf("TCM: Reading %s...
", filename);
    // Parse add_executable, target_link_libraries, etc.
}

int main(int argc, char** argv) {
    printf("T-OS CMake (TCM) v1.0
");
    tcm_parse_file("TcmLists.txt");
    return 0;
}
