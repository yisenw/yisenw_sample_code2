#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
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
    strcpy(filename, "data4.bin");
    cout << "file name received: " << filename << endl;

    /* Map a page from the specified file */
    std::vector<char *> p;
    for (int i = 0; i < 16; i++) p.push_back((char *) vm_map (filename, i));

    cout << "read file successfull." << endl;

    /* Print the first part of the paper */
    char* p0 = p[0];

    for (unsigned int i = 0; i < 16 * VM_PAGESIZE; i++) { // read in 4 pages, evict 4 pages
        int block_num = i / VM_PAGESIZE;
        // cout << i << p0[0] << endl;

        if (i % VM_PAGESIZE == 0) {p0[i] = '!'; cout << "Block " << block_num << " starts!" << endl;}
        // if (i % VM_PAGESIZE == 0) {p[block_num][i] = '!'; cout << "Block " << i % VM_PAGESIZE << " starts!" << endl;}
        // cout << p[block_num][i];
	    // cout << p0[i];
    }
    cout << endl;
    p0[1] = '?'; // read in 1 page; evict 1 (not dirty)

    cout << "begin to read another file." << endl;

    filename[3] = '*'; // read in a page; evict 1 (dirty)
    cout << "print file name0: " << filename << endl;

    char *page1 = (char *) vm_map(nullptr, 0);
    strcpy(page1, "data3.bin"); // read in 1 page; evict 1 (dirty)
    cout << "file name received: " << page1 << endl;

    std::vector<char *> pp;
    for (int i = 0; i < 16; i++) pp.push_back((char *) vm_map (page1, i));

    cout << "read file successfull." << endl;

    char* pp0 = p[0];

    pp0[7 * VM_PAGESIZE] = '!'; // read in 1 page; evict 1 (dirty)
    cout << pp0[7 * VM_PAGESIZE - 1] << pp0[7 * VM_PAGESIZE] << pp0[7 * VM_PAGESIZE + 1] << endl; // read in another page; evict 1 (dirty)
    // pp[7][0] = '!'; // read in 1 page; evict 1 (dirty)
    // cout << pp[6][VM_PAGESIZE - 1] << pp[7][0] << pp[7][1] << endl; // read in another page; evict 1 (dirty)
}