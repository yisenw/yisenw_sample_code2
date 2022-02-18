#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;
using std::endl;

int main() { 
    /* 4 pages of physical memory in the system */
    if (fork()) { // parent
        cout << "parent starts." << endl;
        char *page0 = (char *) vm_map(nullptr, 0);
        char *page1 = (char *) vm_map(nullptr, 0);
        char *page2 = (char *) vm_map(nullptr, 0);
        page0[0] = page1[0] = page2[0] = 'a';
        cout << "parent yields." << endl;
        vm_yield();
        cout << "parent ends." << endl;
    } 
    else { // child
        cout << "child starts." << endl;
        char *page0 = (char *) vm_map(nullptr, 0);
        strcpy(page0, "Hello, world!"); // should evict
        cout << page0 << endl;
        cout << "child ends." << endl;
    }
}
