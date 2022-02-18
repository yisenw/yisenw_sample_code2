#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
#include <cassert>

using std::cout;
using std::endl;

int main() { /* 4 pages of physical memory in the system */
    char *swap1 = (char *) vm_map(nullptr, 0);
    char *swap2 = (char *) vm_map(nullptr, 0);
    char *swap3 = (char *) vm_map(nullptr, 0);
    char *swap4 = (char *) vm_map(nullptr, 0);
    char *swap5 = (char *) vm_map(nullptr, 0);
    char *swap6 = (char *) vm_map(nullptr, 0);
    char *filename = (char *) vm_map(nullptr, 0);
    strcpy(filename, "data1.bin");
    // 1 file page
    char *fb_page_1 = (char *) vm_map(filename, 10);
    if (!fork()) { // child
        cout << "child starts." << endl;
        cout << "child does some reading" << endl;
        cout << *(swap1 + 1) << *(swap1 + 101) << *(swap3 + 2023) << endl;
        cout << filename << endl;

        char *fb_page = (char *) vm_map(filename, 0);
        fb_page[0] = 'H'; // should not fault
        cout << "Child prints shared page: " << fb_page_1[500] << endl;

        cout << "Child does some writing" << endl;
        strcpy(swap1, "data1.bin");
        strcpy(swap1 + 100, "data1.bin");
        strcpy(swap2, "data2.bin");
        strcpy(swap4 + 123, "data2.bin");
        strcpy(swap5 + 1234, "data2.bin");
        strcpy(swap6, "data3.bin");

        cout << "child ends." << endl;
    }
}
