#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
#include <cassert>

using std::cout;
using std::endl;

int main() { /* 4 pages of physical memory in the system */

    char* swap[127];
    // allocate 127 empty swap slots
    cout << "Map 127 swap pages" << endl;
    for (int i = 0; i < 127; ++i) {
        swap[i] = (char *) vm_map(nullptr, 0);
        strcpy(swap[i], "data1.bin");
    }
    cout << "Allocation finished" << endl;

    if (fork()) { // parent
        
        char *swap_local1 = (char *) vm_map(nullptr, 0);
        char *swap_local2 = (char *) vm_map(nullptr, 0);
        strcpy(swap_local1, "data2.bin");
        strcpy(swap_local2, "data3.bin");
        cout << "Parent creates two more swap page" << endl;
        cout << "Swap slot is now full" << endl;

        for (int i = 0; i < 127; i++) {
            cout << swap[i] << endl;
        }

        char *fb_page1 = (char *) vm_map(swap[0], 0);
        cout << "Parent creates one file page" << endl;

    } else { // child
        
        cout << "Child starts" << endl;
        char* swap_child[127];
        for (int i = 0; i < 127; ++i) {
            swap_child[i] = (char *) vm_map(nullptr, 0);
            strcpy(swap_child[i], "data1.bin");
        }
        
        // should not seg fault
        char *swap_local1 = (char *) vm_map(nullptr, 0);
        strcpy(swap_local1, "data2.bin");
        char *swap_local2 = (char *) vm_map(nullptr, 0);
        strcpy(swap_local2, "data3.bin");
        // swap slot is now full
        cout << "Child creates two more swap pages" << endl;
        cout << "swap slot is now full" << endl;

        for (int i = 0; i < 127; i++) {
            cout << swap[i] << endl;
        }
        for (int i = 0; i < 127; i++) {
            cout << swap_child[i] << endl;
        }
        cout << swap_local1 << swap_local2 << endl;

        cout << "Child ends" << endl;

    }
}
