#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;
using std::endl;

int main()
{
    /* Allocate swap-backed page from the arena */
    char *filename = (char *) vm_map(nullptr, 0);

    /* Write the name of the file that will be mapped */
    strcpy(filename, "lampson83.txt");
    cout << "file name received." << endl;

    /* Map a page from the specified file */
    char *p = (char *) vm_map (filename, 0);
    cout << "read file successfull." << endl;
    // cout << "strlen is " << strlen(p) << endl;
    /* Print the first part of the paper */
    for (unsigned int i=0; i<1937; i++) {
	    cout << p[i];
    }
}