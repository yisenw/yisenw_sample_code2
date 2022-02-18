#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "vm_app.h"

using std::cout;
using std::endl;
using std::vector;

// swap back size 16

int main(){
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

    cout << "max virtual pages num: " << VM_ARENA_SIZE/VM_PAGESIZE << endl;
    cout << "up to now, we have used 18 virtual pages." << endl;

    char *many_virtual_pages[VM_ARENA_SIZE/VM_PAGESIZE];
    for (int i = 0; i < VM_ARENA_SIZE/VM_PAGESIZE; i++) {
        cout << i << endl;
        many_virtual_pages[i] = (char *) vm_map(nullptr, 0);
        many_virtual_pages[i][0] = '!';
    }
}