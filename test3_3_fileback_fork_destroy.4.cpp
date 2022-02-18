#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
#include <cassert>

using std::cout;
using std::endl;

int main() { /* 4 pages of physical memory in the system */
    char *filename = (char *) vm_map(nullptr, 0);
    strcpy(filename, "lampson83.txt");
    char *fb_page_1 = (char *) vm_map(filename, 0);
    if (fork()) { // parent
        cout << "parent starts." << endl;
        char *fb_page = (char *) vm_map(filename, 0);
        fb_page[0] = 'B'; // should fault
        cout << "Parent prints shared page: " << fb_page_1[0] << endl;
        cout << "parent yields." << endl;
        vm_yield();
    } else { // child
        cout << "child starts." << endl;
        char *fb_page = (char *) vm_map(filename, 0);
        assert(fb_page[0] == 'B'); // should not fault
        fb_page[0] = 'H'; // should not fault
        cout << "Child prints shared page: " << fb_page_1[0] << endl;
        cout << "child ends." << endl;
    }

    cout << "\nAnother parent_child pair: " << endl;
    if (fork()) {
        cout << "parent starts." << endl;
        char *page0 = (char *) vm_map(nullptr, 0);
        char *page1 = (char *) vm_map(nullptr, 0);
        char *page2 = (char *) vm_map(nullptr, 0);
        page0[0] = page1[0] = page2[0] = 'a';
        cout << "parent ends directly." << endl;
        // vm_yield();
    }
    else {
        cout << "child starts" << endl;
        char *page0 = (char *) vm_map(nullptr, 0);
        strcpy(page0, "Hello, world!");
        cout << "Child says: " << page0 << endl;
        cout << "Child ends" << endl;
    }

    cout << "3_3 ends." << endl; 
    return 0;
}

