#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
#include <cassert>

using std::cout;
using std::endl;

int main() { /* 4 pages of physical memory in the system */

    char* swap[256];
    // leave only one empty swap slots
    cout << "Map 254 swap pages" << endl;
    for (int i = 0; i < 254; ++i) {
        swap[i] = (char *) vm_map(nullptr, 0);
        strcpy(swap[i], "data1.bin");
    }
    cout << "Allocation finished" << endl;

    if (fork()) { // parent
        
        swap[254] = (char *) vm_map(nullptr, 0);
        strcpy(swap[254], "data1.bin");
        swap[255] = (char *) vm_map(nullptr, 0);
        strcpy(swap[255], "data1.bin");
        // swap slot is now full
        cout << "Parent creates two more swap pages" << endl;
        cout << "swap slot is now full" << endl;

        for (int i = 0; i < 256; i++) {
            cout << swap[i] << endl;
        }

        char *fb_page1 = (char *) vm_map(swap[0], 0);
        char *fb_page2 = (char *) vm_map(swap[255], 1);

        vm_yield();

    } else { // child
        
        cout << "Child starts" << endl;
        // should not seg fault
        swap[254] = (char *) vm_map(nullptr, 0);
        strcpy(swap[254], "data2.bin");
        swap[255] = (char *) vm_map(nullptr, 0);
        strcpy(swap[255], "data2.bin");
        // swap slot is now full
        cout << "Child creates two more swap pages" << endl;
        cout << "swap slot is now full" << endl;

        for (int i = 0; i < 256; i++) {
            cout << swap[i] << endl;
        }

        char *fb_page1 = (char *) vm_map(swap[0], 0);
        char *fb_page2 = (char *) vm_map(swap[255], 1);

        cout << "Child ends" << endl;

    }
}
