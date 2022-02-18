#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
#include <cassert>

using std::cout;
using std::endl;

int main() { /* 4 pages of physical memory in the system */
    // 7 swap pages
    // 6 maps to zero page, 1 maps to another place
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
    if (fork()) { // parent
        cout << "parent starts." << endl;
        cout << "parent does some reading" << endl;
        cout << *(swap1 + 2021) << *(swap2 + 2022) << *(swap3 + 2023) << endl;
        cout << filename << endl;
        char *fb_page = (char *) vm_map(filename, 0);
        fb_page[0] = 'B'; // should fault
        cout << "Parent prints shared page: " << fb_page_1[500] << endl;
        
        cout << "Parent does some writing" << endl;
        strcpy(swap1, "data1.bin");
        strcpy(swap1 + 100, "data1.bin");
        strcpy(swap2, "data2.bin");
        strcpy(swap4 + 123, "data2.bin");
        strcpy(swap5 + 1234, "data2.bin");
        strcpy(swap6, "data3.bin");

        cout << "parent yields." << endl;
        vm_yield();

        cout << "parent continues" << endl;

        cout << "parent does some reading" << endl;
        cout << *(swap1 + 1) << *(swap1 + 101) << *(swap3 + 2023) << endl;

        cout << fb_page[0] << endl;
        assert(fb_page[0] == 'H'); // should not fault
        cout << "parent ends" << endl;
    } else { // child
        cout << "child starts." << endl;
        cout << "child does some reading" << endl;
        cout << *(swap1 + 1) << *(swap1 + 101) << *(swap3 + 2023) << endl;
        cout << filename << endl;

        char *fb_page = (char *) vm_map(filename, 0);
        assert(fb_page[0] == 'B'); // should not fault
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
