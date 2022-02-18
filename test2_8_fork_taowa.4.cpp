#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
#include <cassert>
#include <vector>

using std::cout;
using std::endl;

int main() { /* 4 pages of physical memory in the system */
    /* Allocate swap-backed page from the arena */
    char *page0 = (char *) vm_map(nullptr, 0);
    char *page0_5 = (char *) vm_map(nullptr, 0);

    char *filename = page0 + VM_PAGESIZE - 2;

    /* Write the name of the file that will be mapped */
    strcpy(filename, "data2.bin");
    cout << "file name received: " << filename << endl;

    /* Map a page from the specified file */
    std::vector<char *> p;
    for (int i = 0; i < 16; i++) p.push_back((char *) vm_map (filename, i));

    cout << "read file successfull." << endl;

    /* Print the first part of the paper */
    char* p0 = p[0];

    if (fork()) {
        cout << "parent_1 begins" << endl;
        p0[rand() % 16 * VM_PAGESIZE + rand() % VM_PAGESIZE] = 'a' + rand() % 26;
        if (fork()) {
            cout << "parent_2 begins" << endl;
            p0[rand() % 16 * VM_PAGESIZE + rand() % VM_PAGESIZE] = 'a' + rand() % 26;
            if (fork()) {
                cout << "parent_3 begins" << endl;
                p0[rand() % 16 * VM_PAGESIZE + rand() % VM_PAGESIZE] = 'a' + rand() % 26;
                if (fork()) {
                    cout << "parent_4 begins" << endl;
                    p0[rand() % 16 * VM_PAGESIZE + rand() % VM_PAGESIZE] = 'a' + rand() % 26;
                    
                }
                else {
                    cout << "child_4" << endl;
                    p0[rand() % 16 * VM_PAGESIZE + rand() % VM_PAGESIZE] = 'a' + rand() % 26;
                }
            }
            else {
                cout << "child_3" << endl;
                p0[rand() % 16 * VM_PAGESIZE + rand() % VM_PAGESIZE] = 'a' + rand() % 26;
            }
            
        }
        else {
            cout << "child_2" << endl;
            p0[rand() % 16 * VM_PAGESIZE + rand() % VM_PAGESIZE] = 'a' + rand() % 26;
        }
    }
    else {
        cout << "child_1" << endl;
        p0[rand() % 16 * VM_PAGESIZE + rand() % VM_PAGESIZE] = 'a' + rand() % 26;
    }


}

