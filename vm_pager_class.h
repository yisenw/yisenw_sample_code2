#ifndef _VM_PAGER_CLASS_H_
#define _VM_PAGER_CLASS_H_

//DEBUG:
#ifndef NDEBUG
#define NDEBUG
#endif

#include <limits.h>
#include <string.h>

#include <fstream>
#include <map>
#include <vector>
#include <list>

#include "vm_pager.h"

using std::map;
using std::pair;
using std::string;
using std::vector;
using std::list;

unsigned int get_vpn_index(const void* addr);
unsigned int get_vpn_offset(const void* addr);

class pager_class {
   private:
    unsigned int memory_pages = 0;
    unsigned int swap_blocks = 0;

    unsigned int resident_page_count =
        0;  // the number of valid pages in the physical memory
    unsigned int swap_page_count =
        0;  // the total number of swap-backed pages (cannot exceed swap_blocks)

    struct physical_page_entry_t {
        bool valid = false;  // 1: occupied, 0: available
        bool dirty = false;
        bool evict_ref = false;
        bool in_disk = false;
        string filename = "";
        unsigned int block_num = UINT_MAX;
        bool is_swap = true;
        vector<pair<pid_t, unsigned int>> referenced_vpages;
        // which virtual pages map to this page <pid, vpn>
        // to access the block_num:
        // (*(process_map[referenced_vpages[].first].os_virtual_page_table_ptr))[referenced_vpages[].second].block_num
    };

    struct virtual_page_entry_t {
        bool valid = false;
        bool resident = false;
        bool is_swap = true;
        string filename = "";
        unsigned int block_num =
            UINT_MAX;  // which block in the swap file it locates
    };
    typedef vector<virtual_page_entry_t> os_vpt_t;
    // os_virtual_page_table type

    // defined in pager.h
    // struct page_table_entry_t {
    //     unsigned int ppage : 20;       /* bit 0-19 */
    //     unsigned int read_enable : 1;  /* bit 20 */
    //     unsigned int write_enable : 1; /* bit 21 */
    // };
    // access through page_table_ptr->ptes[i]

    // both are idxed by VPN = (virtual_address >> 16 - (uintptr_t)
    // VM_ARENA_BASEADDR) / VM_PAGESIZE
    struct process_entry_t {
        os_vpt_t* os_virtual_page_table_ptr = nullptr;
        page_table_t* page_table_ptr = nullptr;
    };

    struct swap_slot_entry_t {
        bool valid = false;  // 1: occupied, 0: available
        vector<pair<pid_t, unsigned int>> referenced_vps;
        // should be equal to physical_page_entry_t.referenced_vps when evicted
    };

    struct fileback_entry_t {
        vector<pair<pid_t, unsigned int>> referenced_vps;
        unsigned int ppn = UINT_MAX;
    };

    // all pages in the physical memory, idxed by PPN (in page_table_t)
    std::vector<physical_page_entry_t> physical_page_table;

    map<pid_t, process_entry_t> process_map;

    vector<swap_slot_entry_t> swap_slot_table;

    // filename, block -> fileback_entry
    map<pair<string, unsigned int>, fileback_entry_t> fileback_map;

    // unsigned int evict_head = 0;  // idx of physical page table
    list<unsigned int> clock_queue;
    // clock queue stores the ppn

    ////////////// process methods ////////////
    void alloc_empty_process(pid_t);

    int alloc_non_empty_process(pid_t parent_pid, pid_t child_pid);

    // (if there is physical mem page invalid) set a physical memory page to
    // read-only & return PPN only be called when the pager first writes into
    // the swap page
    unsigned int gen_swap_page_phy();

    ///////////// page methods ///////////////
    // initialize the zero page; only execute once at init
    void alloc_zero_page();
    unsigned int get_free_phy_page();
    unsigned int get_free_swap_slot();

    // return new ppn, UINT_MAX if error occur
    unsigned int copy_and_change_pointer(unsigned int vpn);

    int restore(unsigned int vpn);

    unsigned int evict();              // return recently evicted ppn
    unsigned int find_evict_victim();  // return victim ppn
    unsigned int evict_swapback_page(unsigned int ppn);  // return block
    unsigned int evict_fileback_page(unsigned int ppn);

    /* decide read_enable write_enable ppage for ALL virtual pages pointing to ppn
     */
    // call this every time a physical page entry has been changed!
    void update_shared_vpt(unsigned int ppn);
    /* set to non resident for ALL vpages pointing to this evicted block */
    void update_shared_vpt_at_evict(unsigned int ppn, unsigned int block);

    ///////////// destroy methods /////////////////
    void destroy_resident_page(unsigned int ppn, unsigned int vpn);
    void destroy_swap_back_entry(unsigned int block, unsigned int vpn);
    void destroy_fileback_page_entry(string filename, unsigned int block,
                                     bool resident, unsigned int vpn=UINT_MAX);

    unsigned int map_fileback_page_entry(string filename, unsigned int block,
                                         pair<pid_t, unsigned int> entry);
    // return ppn if resident in memory, else UINT_MAX

   public:
    pid_t current_pid = UINT_MAX;

    pager_class(unsigned int mem_p, unsigned int swap_b)
        : memory_pages(mem_p),
          swap_blocks(swap_b),
          physical_page_table(mem_p),
          swap_slot_table(swap_b) {
        alloc_zero_page();
    }

    int fork(pid_t parent_pid, pid_t child_pid);  // 0: success; -1: fail
    void switch_process(pid_t target_pid);
    void destroy_current_process();
    int fault_handler(unsigned int vpn, bool write_flag);
    // return vpn. if there is no space, return UINT_MAX.
    unsigned int alloc_swap_page();
    bool arena_full();

    // return vpn. if there is no space, return UINT_MAX.
    unsigned int alloc_fileback_page(string filename, unsigned int block_num);

    // debug prints
#ifdef DEBUG
    void print_vpt(pid_t);
    void print_ppt();
    void print_sst();
    void print_fbm();
#endif
};

#ifdef DEBUG
extern std::ofstream log_st;
extern std::ofstream evict_log;
#endif

#endif