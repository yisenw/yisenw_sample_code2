#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
#include <cassert>
#include <vector>

using std::cout;
using std::endl;

int main() { /* 4 pages of physical memory in the system */
    char *filename = (char *) vm_map(nullptr, 0);
    strcpy(filename, "lampson83.txt");
    char *fb_page = (char *) vm_map(filename, 0);
    
    if (fork()) { // parent
        cout << "parent starts." << endl;
        fb_page[VM_PAGESIZE - 1] = '\0';
        fb_page[0] = 'B'; // should fault
        cout << "Parent prints shared page: " << fb_page[10] << endl;

        char *page0 = (char *) vm_map(nullptr, 0);
        char *page0_5 = (char *) vm_map(nullptr, 0);
        char *fn_parent = page0 + VM_PAGESIZE - 2;
        strcpy(fn_parent, "data4.bin");
        std::vector<char *> p;
        for (int i = 0; i < 16; i++) p.push_back((char *) vm_map (fn_parent, i));
        char* p0 = p[0];
        cout << "parent read in a large file" << endl;

        if (fork()) {
            cout << "parent fork another parent_1, checking fileback pages" << endl;
            p0[VM_PAGESIZE - 10] = '!';
            // cout << "?" << endl;
            p0[4 * VM_PAGESIZE - 10] = '!';
            // cout << "??" << endl;
            p0[10 * VM_PAGESIZE - 10] = '!';
            cout << p0[VM_PAGESIZE - 10] <<
                " parent_1 change and yield" << endl;
            vm_yield();
            assert(p0[13 * VM_PAGESIZE - 10] == '!');
            cout << "parent_1 end" << endl;
        }
        else {
            cout << "child_1 starts" << endl;
            assert(p0[VM_PAGESIZE - 10] == p0[10 * VM_PAGESIZE - 10]);
            assert(p0[4 * VM_PAGESIZE - 10] == p0[10 * VM_PAGESIZE - 10]);
            p0[13 * VM_PAGESIZE - 10] = '!';
            cout << "child_1 ends." << endl;
        }


        cout << "parent yields." << endl;
        vm_yield();
        cout << "parent ends." << endl;
    } else { // 
        cout << "child starts." << endl;
        char *fb_page = (char *) vm_map(filename, 0);
        fb_page[VM_PAGESIZE - 1] = '\0';
        assert(fb_page[0] == 'B'); // should not fault
        fb_page[0] = 'H'; // should not fault
        cout << "Child prints shared page: " << fb_page[10] << endl;

        char* page2 =  (char *) vm_map(nullptr, 0);
        char* page2_5 =  (char *) vm_map(nullptr, 0);
        char* sb_page_ptr = page2 + VM_PAGESIZE - 4;
        strcpy(sb_page_ptr, "12345678");
        if (fork()) {
            cout << "child fork another parent_2, checking sb pages" << endl;
            sb_page_ptr[3] = '?';
            sb_page_ptr[6] = '?';
            cout << sb_page_ptr << " parent_2 yields" << endl;
            vm_yield();
            // assert(sb_page_ptr[4] == '?');
            cout << "parent_2 ends." << endl;
        }
        else {
            cout << "child_2 begins " << sb_page_ptr << endl;
            // assert(sb_page_ptr[3] == sb_page_ptr[6]);
            sb_page_ptr[4] = '?';
            cout << sb_page_ptr << " child_2 ends" << endl;
        }

        cout << "child ends." << endl;
    }
}

