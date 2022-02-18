#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <cassert>
#include "vm_app.h"

using std::cout;
using std::endl;
using std::vector;

int main(){
    vector<char*> page(50, nullptr);
    vector<char*>ssh(4, nullptr);
    for (int i = 0; i < 50; i++) page[i] = (char *) vm_map(nullptr, 0);

    ssh[0] = page[0] + VM_PAGESIZE - 4;
    strcpy(ssh[0], "lampson83.txt");
    ssh[1] = page[1] + VM_PAGESIZE - 1;
    strcpy(ssh[1], "data1.bin");
    ssh[2] = page[2] + VM_PAGESIZE / 3;
    strcpy(ssh[2], "data2.bin");
    ssh[3] = page[3] + VM_PAGESIZE - 5;
    strcpy(ssh[3], "data3.bin");
    ssh[4] = page[4] + VM_PAGESIZE - 100;
    strcpy(ssh[4], "data4.bin");
    cout << "shuo sao hua!" << endl;
    for (int i = 0; i < 5; i++) {
        cout << ssh[i] << endl;
        page[i + 5] = (char *) vm_map(ssh[i], i);
    }

    page[10] = (char *) vm_map(ssh[4], 4);



    // cout << page[5][0] << endl;
    
    for (int i = 5; i <= 10; i++) {
        cout << "iteration " << i - 5 << "/6" << endl;
        for (int j = 0; j <= VM_PAGESIZE / 2; j++) {
            if (j % 3 == 0) {
                char a = page[i][j];
                // cout << page[i][j] << endl; 
            }
            else if (j % 3 == 1) page[i][j] = 'A' + rand() % 26;
            else {
                // cout << "copy!" << endl; 
                memcpy(page[i + 10], page[i], VM_PAGESIZE);
            }
        }
    }
}