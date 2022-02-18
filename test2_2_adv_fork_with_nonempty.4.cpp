#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
#include <cassert>

using std::cout;
using std::endl;

int main() { /* 4 pages of physical memory in the system */
    char *filename = (char *) vm_map(nullptr, 0);
    strcpy(filename, "data1.bin");
    char *fb_page_1 = (char *) vm_map(filename, 10);
    if (fork()) { // parent
        cout << "parent starts." << endl;
        char *fb_page = (char *) vm_map(filename, 0);
        fb_page[0] = 'B'; // should fault
        cout << "Parent prints shared page: " << fb_page_1[500] << endl;
        cout << "parent yields." << endl;
        vm_yield();
        assert(fb_page[0] == 'H'); // should not fault
        cout << "parent ends" << endl;
    } else { // child
        cout << "child starts." << endl;
        char *fb_page = (char *) vm_map(filename, 0);
        assert(fb_page[0] == 'B'); // should not fault
        fb_page[0] = 'H'; // should not fault
        cout << "Child prints shared page: " << fb_page_1[500] << endl;
        cout << "child ends." << endl;
    }
}

