#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <cassert>
#include "vm_app.h"

using std::cout;
using std::endl;


int main()
{
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

    // random write 10000 times
    for (int turn = 0; turn < 1000; turn++) {
        if (turn % 50 == 0) cout << turn << "/1000 completed." << endl;
        int block_num = rand() % 16;
        int offset =  rand() % VM_PAGESIZE;
        int change_to = rand() % 26;
        p0[block_num * VM_PAGESIZE + offset] = 'a' + change_to;
    }

    cout << "end 3_5" << endl;



}