#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
#include <cassert>

using std::cout;
using std::endl;

int main() { /* 4 pages of physical memory in the system */

    
    if (fork()) { // parent
        char *filename = (char *) vm_map(nullptr, 0);
        strcpy(filename, "lampson83.txt");
        char *fb_page_1 = (char *) vm_map(filename, 1);
        cout << "parent starts." << endl;
        char *fb_page = (char *) vm_map(filename, 0);
        fb_page[VM_PAGESIZE - 1] = '\0';
        fb_page[0] = 'B'; // should fault
        cout << "Parent prints shared page: " << fb_page_1[0] << endl;
        cout << "parent yields." << endl;
        vm_yield();
        cout << "parent ends." << endl;
    } else { // 
        char *filename = (char *) vm_map(nullptr, 0);
        strcpy(filename, "lampson83.txt");
        char *fb_page_1 = (char *) vm_map(filename, 1);
        cout << "child starts." << endl;
        char *fb_page = (char *) vm_map(filename, 0);
        fb_page[VM_PAGESIZE - 1] = '\0';
        assert(fb_page[0] == 'B'); // should not fault
        fb_page[0] = 'H'; // should not fault
        cout << "Child prints shared page: " << fb_page_1[0] << endl;
        cout << "child ends." << endl;
    }
}

