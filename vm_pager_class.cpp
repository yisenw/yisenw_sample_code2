#include "vm_pager_class.h"
// #include "vm_arena.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
using std::cerr;
using std::cout;
using std::endl;

unsigned int get_vpn_index(const void *addr) {
    if ((uintptr_t)addr < (uintptr_t)VM_ARENA_BASEADDR) return UINT_MAX;
    if ((uintptr_t)addr >= (uintptr_t)VM_ARENA_BASEADDR + VM_ARENA_SIZE)
        return UINT_MAX;
    return ((uintptr_t)addr - (uintptr_t)VM_ARENA_BASEADDR) / VM_PAGESIZE;
}

unsigned int get_vpn_offset(const void *addr) {
    return (uintptr_t)addr & 65535;
    // return (uintptr_t)addr - ((uintptr_t)addr >> 16) << 16;
}

bool pager_class::arena_full(){
    os_vpt_t &os_vpt(*process_map[current_pid].os_virtual_page_table_ptr);
    unsigned int vpn = 0;
    for (vpn = 0; vpn < VM_ARENA_SIZE / VM_PAGESIZE; vpn++) {
        if (!os_vpt[vpn].valid) break;
    }
    if (vpn >= VM_ARENA_SIZE / VM_PAGESIZE) return true;
    return false;
}

////////////// process methods //////////////////
void pager_class::alloc_empty_process(pid_t pid) {
    if (process_map.find(pid) != process_map.end()) {
        cerr << "ERROR: cannot create new process with pid " << pid
             << ": pid already exist" << endl;
        exit(1);
    }
    process_entry_t entry;

    entry.os_virtual_page_table_ptr =
        new vector<virtual_page_entry_t>(VM_ARENA_SIZE / VM_PAGESIZE);

    entry.page_table_ptr = new page_table_t;
    for (auto &pte : entry.page_table_ptr->ptes) {
        pte.ppage = 0;
        pte.read_enable = 0;
        pte.write_enable = 0;
    }

    process_map[pid] = entry;
}

int pager_class::alloc_non_empty_process(pid_t parent_pid, pid_t child_pid) {
    assert(process_map.find(child_pid) == process_map.end());
    assert(process_map.find(parent_pid) != process_map.end());
#ifdef DEBUG
    log_st << "alloc_non_emtpty_process: parent " << parent_pid << " child "
           << child_pid << endl;
    log_st << "parent vpt" << endl;
    print_vpt(parent_pid);
#endif

    unsigned int new_swap_page_count = swap_page_count;

    process_entry_t entry_child;
    process_entry_t &entry_parent = process_map[parent_pid];
    page_table_t &parent_pt(*entry_parent.page_table_ptr);
    os_vpt_t &parent_vpt(*entry_parent.os_virtual_page_table_ptr);

    /* count parent swap back page */
    for (auto &elt : parent_vpt) {
        if (elt.valid && elt.is_swap) new_swap_page_count++;
    }
    if (new_swap_page_count > swap_blocks) return -1;
    swap_page_count = new_swap_page_count;

    /* set parent swap back pages write_enable to 0 */
    for (unsigned int i = 0; i < VM_ARENA_SIZE / VM_PAGESIZE; i++) {
        if (!parent_vpt[i].valid) continue;
        if (parent_vpt[i].is_swap)  // is swap back page
            parent_pt.ptes[i].write_enable = 0;
    }

    /* deep copy virtual page table & page table */
    entry_child.os_virtual_page_table_ptr =
        new vector<virtual_page_entry_t>(VM_ARENA_SIZE / VM_PAGESIZE);
    *entry_child.os_virtual_page_table_ptr =
        *entry_parent
             .os_virtual_page_table_ptr;  // copy the entire vector
                                          // (auto deep copy) 个屁 现在是了

    entry_child.page_table_ptr = new page_table_t;  // deep copy
    for (unsigned int i = 0; i < VM_ARENA_SIZE / VM_PAGESIZE; i++) {
        entry_child.page_table_ptr->ptes[i] =
            entry_parent.page_table_ptr->ptes[i];
    }

    process_map[child_pid] = entry_child;

    /* add new process to physical page table/swap back slot / fileback map
     * entry */
    for (unsigned int vpn = 0; vpn < VM_ARENA_SIZE / VM_PAGESIZE; vpn++) {
        if (!parent_vpt[vpn].valid) continue;

        if (!parent_vpt[vpn].is_swap) {  // fileback page
#ifdef DEBUG
            log_st << "filename: " << parent_vpt[vpn].filename << " block "
                   << parent_vpt[vpn].block_num << endl;
#endif
            assert(fileback_map.find({string(parent_vpt[vpn].filename),
                                      parent_vpt[vpn].block_num}) !=
                   fileback_map.end());
            fileback_map[{string(parent_vpt[vpn].filename),
                          parent_vpt[vpn].block_num}]
                .referenced_vps.push_back({child_pid, vpn});
        }
        // resident pages
        if (parent_vpt[vpn].resident) {
            physical_page_entry_t &ppe(
                physical_page_table[parent_pt.ptes[vpn].ppage]);
            ppe.referenced_vpages.push_back({child_pid, vpn});
            update_shared_vpt(parent_pt.ptes[vpn].ppage);
            continue;
        }

        // non resident & swap back pages
        if (parent_vpt[vpn].is_swap) {
            swap_slot_entry_t &sse(swap_slot_table[parent_vpt[vpn].block_num]);
            sse.referenced_vps.push_back({child_pid, vpn});
        } else {  // no resident & fileback page
        }
    }

#ifdef DEBUG
    log_st << "after fork: " << endl;
    print_vpt(parent_pid);
    print_vpt(child_pid);
    print_ppt();
    print_sst();
    print_fbm();
#endif
    return 0;
}

////////////////// page methods ////////////////

void pager_class::alloc_zero_page() {
    physical_page_entry_t &zero_page(physical_page_table[0]);
    zero_page.valid = true;
    zero_page.dirty = false;
    zero_page.evict_ref = true;
    zero_page.in_disk = false;
    zero_page.is_swap = true;
    zero_page.referenced_vpages.push_back({UINT_MAX, UINT_MAX});
    // zero_page.referenced_vpages

    ++resident_page_count;
    // initialize the zero page
    for (size_t i = 0; i < VM_PAGESIZE; ++i) {
        ((char *)vm_physmem)[i] = 0;
    }
}

int pager_class::restore(unsigned int vpn) {
#ifdef DEBUG
    log_st << "RESTORE: vpn" << vpn << endl;
#endif
    auto process_it = process_map.find(current_pid);
    virtual_page_entry_t &os_vpt_entry(
        (*(process_it->second.os_virtual_page_table_ptr))[vpn]);

    unsigned int new_ppn = get_free_phy_page();
    assert(new_ppn != 0);

#ifdef DEBUG
    log_st << "alloc physical page " << new_ppn << " in mem" << endl;
    log_st << "read from disk " << os_vpt_entry.filename;
    log_st << " block " << os_vpt_entry.block_num << endl;
#endif

    /* copy page from disk to memory */
    if (os_vpt_entry.is_swap) {
        if (file_read(nullptr, os_vpt_entry.block_num,
                      (void *)vm_physmem + new_ppn * VM_PAGESIZE) == -1) {
#ifdef DEBUG
            evict_log << "Read swap backed file failed" << endl;
#endif
            return UINT_MAX;
        }
    }

    else {
#ifdef DEBUG
        log_st << "restore " << os_vpt_entry.filename << " block "
               << os_vpt_entry.block_num << endl;
#endif
        if (file_read(os_vpt_entry.filename.c_str(), os_vpt_entry.block_num,
                      (void *)vm_physmem + new_ppn * VM_PAGESIZE) == -1) {
#ifdef DEBUG
            evict_log << "Read file backed file failed" << endl;
#endif
            return UINT_MAX;
        }
        fileback_map[{os_vpt_entry.filename, os_vpt_entry.block_num}].ppn =
            new_ppn;
    }

// update the clock queue
#ifdef DEBUG
    auto clock_it = clock_queue.begin();
    while (clock_it != clock_queue.end()) {
        if (*clock_it == new_ppn) break;
        clock_it++;
    }
    assert(clock_it == clock_queue.end());
#endif
    clock_queue.push_back(new_ppn);

    /* construct physical page table */
    physical_page_table[new_ppn].valid = true;
    physical_page_table[new_ppn].dirty = false;
    physical_page_table[new_ppn].evict_ref = true;
    physical_page_table[new_ppn].in_disk = true;
    physical_page_table[new_ppn].filename = os_vpt_entry.filename;
    physical_page_table[new_ppn].block_num = os_vpt_entry.block_num;
    physical_page_table[new_ppn].is_swap = os_vpt_entry.is_swap;

    if (os_vpt_entry.is_swap)
        physical_page_table[new_ppn].referenced_vpages =
            swap_slot_table[os_vpt_entry.block_num].referenced_vps;
    else
        physical_page_table[new_ppn].referenced_vpages =
            fileback_map[{string(os_vpt_entry.filename),
                          os_vpt_entry.block_num}]
                .referenced_vps;

    /* update virtual pages in other process sharing this page */
    update_shared_vpt(new_ppn);

    resident_page_count++;
#ifdef DEBUG
    print_vpt(current_pid);
    print_ppt();
    print_sst();
#endif
    return 0;
}

unsigned int pager_class::find_evict_victim() {
    assert(!clock_queue.empty());
    while (physical_page_table[clock_queue.front()].evict_ref) {
        physical_page_table[clock_queue.front()].evict_ref = false;
        unsigned int temp_ppn = clock_queue.front();
        update_shared_vpt(temp_ppn);
        clock_queue.pop_front();
        clock_queue.push_back(temp_ppn);
    }

    unsigned int victim = clock_queue.front();
    clock_queue.pop_front();
    assert(victim != 0);
    return victim;
}

unsigned int pager_class::evict_swapback_page(unsigned int ppn) {
    unsigned int block_num = physical_page_table[ppn].block_num;

    assert(physical_page_table[ppn].in_disk | physical_page_table[ppn].dirty);

    // // allocate new swap_back_slot if not in disk
    // if (!physical_page_table[ppn].in_disk) {
    //     for (block_num = 0; block_num < swap_blocks; block_num++) {
    //         if (!swap_slot_table[block_num].valid) break;
    //     }
    //     swap_slot_table[block_num].valid = true;
    // }

    // now allocate swap back slot at copy on write

    swap_slot_table[block_num].referenced_vps =
        physical_page_table[ppn].referenced_vpages;

    /* do not write to disk if not dirty */
    if (!physical_page_table[ppn].dirty) return block_num;

    /* write swap back page to disk*/
    if (file_write((char *)nullptr, block_num,
                   (void *)((char *)vm_physmem + ppn * VM_PAGESIZE)) == -1)
        return UINT_MAX;
    return block_num;
}

unsigned int pager_class::evict_fileback_page(unsigned int ppn) {
    physical_page_entry_t &ppe(physical_page_table[ppn]);
    unsigned int block_num = ppe.block_num;
    assert(ppe.in_disk);

    if (ppe.referenced_vpages.size() == 0) {
        destroy_fileback_page_entry(ppe.filename, block_num, false);
    } else {
        fileback_entry_t &fbe(fileback_map[{ppe.filename, ppe.block_num}]);
        fbe.referenced_vps = ppe.referenced_vpages;
        fbe.ppn = UINT_MAX;
    }

    if (!ppe.dirty) return block_num;
    if (file_write(ppe.filename.c_str(), block_num,
                   (void *)((char *)vm_physmem + ppn * VM_PAGESIZE)) == -1)
        return UINT_MAX;

    return block_num;
}

unsigned int pager_class::evict() {
#ifdef DEBUG
    log_st << "Evict" << endl;
#endif
    assert(resident_page_count == memory_pages);

    unsigned int victim_ppn = find_evict_victim();

#ifdef DEBUG
    log_st << "victim: ppn=" << victim_ppn << endl;
    for (auto vpage : physical_page_table[victim_ppn].referenced_vpages)
        log_st << "<" << vpage.first << "," << vpage.second << "> ";
    log_st << endl;
#endif

    // file-back: must be in_disk
    // swap-back: in_disk, or is dirty, otherwise
    assert(physical_page_table[victim_ppn].in_disk |
           physical_page_table[victim_ppn].dirty);

    unsigned int block_num;

    if (physical_page_table[victim_ppn].is_swap) {
        block_num = evict_swapback_page(victim_ppn);
        if (block_num == UINT_MAX) return UINT_MAX;
    } else {
        block_num = evict_fileback_page(victim_ppn);
        if (block_num == UINT_MAX) return UINT_MAX;
    }

    /* update shared vpages, clean physical page table entry, decrease
        resident page count */
    update_shared_vpt_at_evict(victim_ppn, block_num);
    resident_page_count--;
    physical_page_table[victim_ppn] = physical_page_entry_t();
    return victim_ppn;
}

unsigned int pager_class::get_free_phy_page() {
    unsigned int new_ppn;
    if (memory_pages == resident_page_count) {
        new_ppn = evict();
    } else {
        // TODO: we might need a more efficient way to traverse
        for (new_ppn = 1; new_ppn < memory_pages; new_ppn++)
            if (!physical_page_table[new_ppn].valid) break;
    }
    physical_page_table[new_ppn] = physical_page_entry_t();
    return new_ppn;
}

unsigned int pager_class::get_free_swap_slot() {
    unsigned int block_num = UINT_MAX;
    // allocate new swap_back_slot if not in disk
    for (unsigned int i = 0; i < swap_blocks; i++) {
        if (!swap_slot_table[i].valid) {
            block_num = i;
            break;
        }
    }
    assert(block_num != UINT_MAX);
    swap_slot_table[block_num].valid = true;
    swap_slot_table[block_num].referenced_vps.clear();
    return block_num;
}

unsigned int pager_class ::copy_and_change_pointer(unsigned int vpn) {
#ifdef DEBUG
    log_st << "copy_and_change_pointer called with vpn:" << vpn << endl;
#endif

    page_table_entry_t &pte(process_map[current_pid].page_table_ptr->ptes[vpn]);
    // os_vpt_t &os_vpt(*process_map[current_pid].os_virtual_page_table_ptr);
    unsigned int current_ppn = pte.ppage;

    physical_page_table[current_ppn].evict_ref = true;

    // first copy into the buf
    char *old_page = (char *)vm_physmem + current_ppn * VM_PAGESIZE;
    char buffer[VM_PAGESIZE];  // somewhere in this kernel
    memcpy(buffer, old_page, VM_PAGESIZE * sizeof(char));
    physical_page_entry_t buffer_ppe = physical_page_table[current_ppn];
    // dirty, evict_ref will be overwritten by fault_handler
    buffer_ppe.in_disk = false;
    buffer_ppe.block_num = get_free_swap_slot();
    buffer_ppe.referenced_vpages.clear();
    buffer_ppe.referenced_vpages.push_back({current_pid, vpn});

    // remove the vpn from the referenced pages of the old physical page
    for (auto it = physical_page_table[current_ppn].referenced_vpages.begin();
         it != physical_page_table[current_ppn].referenced_vpages.end(); it++) {
        if (it->first == current_pid && it->second == vpn) {
            physical_page_table[current_ppn].referenced_vpages.erase(it);
            break;
        }
    }

    update_shared_vpt(current_ppn);

    // then try to get a new page
    // might evict the original page
    unsigned int new_ppn = get_free_phy_page();
    assert(new_ppn != 0);
    assert(new_ppn != UINT_MAX);

#ifdef DEBUG
    log_st << "remap vpn=" << vpn << " from ppn=" << current_ppn
           << " to ppn=" << new_ppn << endl;
#endif

// update the clock queue
#ifdef DEBUG
    auto clock_it = clock_queue.begin();
    while (clock_it != clock_queue.end()) {
        if (*clock_it == new_ppn) break;
        clock_it++;
    }
    assert(clock_it == clock_queue.end());
#endif
    clock_queue.push_back(new_ppn);

    pte.ppage = new_ppn;
    // pte.read_enable = 1; // overwrite by fault_handler
    // pte.write_enable = 1; // overwrite by fault_handler
    char *new_page = (char *)vm_physmem + new_ppn * VM_PAGESIZE;
    memcpy(new_page, buffer, VM_PAGESIZE * sizeof(char));
    // directly copy from the buffer
    physical_page_table[new_ppn] = buffer_ppe;
    resident_page_count++;
    update_shared_vpt(new_ppn);
    return new_ppn;
}

void pager_class::update_shared_vpt(unsigned int ppn) {
    physical_page_entry_t &phy_entry(physical_page_table[ppn]);

    for (auto vpage : phy_entry.referenced_vpages) {
        if (vpage.second == UINT_MAX)
            continue;  // if this is the pinned vpage for zero_page
        virtual_page_entry_t &vpt(
            process_map[vpage.first].os_virtual_page_table_ptr->at(
                vpage.second));
        vpt.valid = true;
        vpt.resident = true;
        vpt.block_num = physical_page_table[ppn].block_num;
        page_table_entry_t &pte(
            process_map[vpage.first].page_table_ptr->ptes[vpage.second]);
        pte.ppage = ppn;
        if (!phy_entry.evict_ref) {
            // turn off both read & write enable if no evict ref
            pte.read_enable = 0;
            pte.write_enable = 0;
            continue;
        }
        pte.read_enable = 1;
        pte.write_enable = 1;
        if (!phy_entry.dirty && phy_entry.in_disk) {
            // turn off write enable for in-disk but not dirty pages
            pte.write_enable = 0;
        }
        if (phy_entry.is_swap && phy_entry.referenced_vpages.size() > 1) {
            // turn off write en for shared swap back page
            pte.write_enable = 0;
        }
    }

    if (phy_entry.is_swap && ppn != 0) {
        swap_slot_table[phy_entry.block_num].valid = true;
        swap_slot_table[phy_entry.block_num].referenced_vps =
            phy_entry.referenced_vpages;
    }

    if (!phy_entry.is_swap) {
        fileback_map[{phy_entry.filename, phy_entry.block_num}].ppn = ppn;
        fileback_map[{phy_entry.filename, phy_entry.block_num}].referenced_vps =
            phy_entry.referenced_vpages;
    }
}

void pager_class::update_shared_vpt_at_evict(unsigned int ppn,
                                             unsigned int block) {
    for (auto &vpage : physical_page_table[ppn].referenced_vpages) {
        page_table_entry_t &pte(
            process_map[vpage.first].page_table_ptr->ptes[vpage.second]);
        virtual_page_entry_t &vpte(
            process_map[vpage.first].os_virtual_page_table_ptr->at(
                vpage.second));
        vpte.resident = false;
        vpte.block_num = block;
        pte.ppage = 0;
        pte.read_enable = 0;
        pte.write_enable = 0;
    }
}

///////////// destory methods //////////////
void pager_class::destroy_fileback_page_entry(string filename,
                                              unsigned int block, bool resident,
                                              unsigned int vpn) {
#ifdef DEBUG
    log_st << "destroy_fileback_page_entry " << filename << " block " << block
           << " vpn " << vpn  << " resident? " << resident << endl;
    log_st << "----------- fileback map -------------" << endl;
    log_st << "filename\tblock\tppn\treference_vpts" << endl;
    auto entry_it = fileback_map.find({filename, block});
    auto entry = *entry_it;
    log_st << entry.first.first << "\t" << entry.first.second << "\t";
    log_st << entry.second.ppn << "\t";
    for (auto vpage : entry.second.referenced_vps) {
        log_st << "<" << vpage.first << "," << vpage.second << ">";
    }
    log_st << endl;

#endif
    auto fbe_it = fileback_map.find({filename, block});
    fileback_entry_t &fbe(fbe_it->second);

    vector<pair<pid_t, unsigned int>> &referenced_vps(fbe.referenced_vps);
    for (auto vpage_it = referenced_vps.begin();
            vpage_it != referenced_vps.end(); vpage_it++) {
        if (vpage_it->first == current_pid && vpage_it->second == vpn) {
            referenced_vps.erase(vpage_it);
            break;
        }
    }
    /* this block is shared with other processes/this block is in memory*/
    if (referenced_vps.size() > 0 || resident)
        return;
#ifdef DEBUG
    log_st << "not share/resident. fbe_it found? "
           << !(fbe_it == fileback_map.end()) << endl;
#endif
    /* this block is not shared with other processes */
    fileback_map.erase(fbe_it);
#ifdef DEBUG
    log_st << "erase successfully " << filename << " block " << block << endl;
#endif
}

void pager_class::destroy_resident_page(unsigned int ppn, unsigned int vpn) {
#ifdef DEBUG
    log_st << "destroy_resident_page " << ppn << endl;
    log_st << "######## physical page table #############"
           << "resident_page_count " << resident_page_count << endl;
    log_st << "ppn\tdirty\tevict_ref\tin_disk\tfilename\tblock_num\tis_"
              "swap\treferenced_"
              "pages"
           << endl;
    log_st << ppn << "\t" << physical_page_table[ppn].dirty << "\t"
           << physical_page_table[ppn].evict_ref << "\t\t"
           << physical_page_table[ppn].in_disk << "\t";
    log_st << physical_page_table[ppn].filename << "\t"
           << physical_page_table[ppn].block_num << "\t"
           << physical_page_table[ppn].is_swap << "\t";
    for (auto elt : physical_page_table[ppn].referenced_vpages)
        log_st << "<" << elt.first << "," << elt.second << "> ";
    log_st << endl;
#endif
    physical_page_entry_t &ppe(physical_page_table[ppn]);

    /* leave shared pages & fileback pages*/
    if (ppe.referenced_vpages.size() > 1 || !ppe.is_swap) {
        for (auto it = ppe.referenced_vpages.begin();
             it != ppe.referenced_vpages.end(); it++) {
            if (it->first == current_pid && it->second == vpn) {
                ppe.referenced_vpages.erase(it);
                update_shared_vpt(ppn);
                return;
            }
        }
    }

    // update the clock queue
    auto clock_it = clock_queue.begin();
    while (clock_it != clock_queue.end()) {
        if (*clock_it == ppn) break;
        clock_it++;
    }
    assert(clock_it != clock_queue.end());
    clock_queue.erase(clock_it);

#ifdef DEBUG
    evict_log << "Destroy ppn: " << ppn << endl;
    for (auto clock_it = clock_queue.begin(); clock_it != clock_queue.end();
         clock_it++) {
        evict_log << *clock_it << " ("
                  << physical_page_table[*clock_it].evict_ref << ")\t";
    }
    evict_log << endl;
    evict_log << "============================" << endl;
#endif

    /* clean memory */
    ppe = physical_page_entry_t();
    resident_page_count--;
}

void pager_class::destroy_swap_back_entry(unsigned int block,
                                          unsigned int vpn) {
    swap_slot_entry_t &sse(swap_slot_table[block]);


#ifdef DEBUG
    log_st << "destroy_swap_back_page " << block  << " vpn " << vpn << endl;
    log_st << "------------ swap slot table -------------"
           << "swap page count " << swap_page_count << endl;
    log_st << "block\treferenced_pages" << endl;
        log_st << block << "\t";
        for (auto elt : swap_slot_table[block].referenced_vps)
            log_st << "<" << elt.first << "," << elt.second << ">";
        log_st << endl;
#endif

    for (auto it = sse.referenced_vps.begin();
            it != sse.referenced_vps.end(); it++) {
        if (it->first == current_pid && it->second == vpn) {
            sse.referenced_vps.erase(it);
            break;
        }
    }
#ifdef DEBUG
    log_st << "------------ swap slot table -------------"
           << "swap page count " << swap_page_count << endl;
    log_st << "block\treferenced_pages" << endl;
        log_st << block << "\t";
        for (auto elt : swap_slot_table[block].referenced_vps)
            log_st << "<" << elt.first << "," << elt.second << ">";
        log_st << endl;
#endif
    /* leave shared page along */
    if (sse.referenced_vps.size() > 0) return;
    /* clean disk swap block */
    sse = swap_slot_entry_t();
}

unsigned int pager_class::map_fileback_page_entry(
    string filename_str, unsigned int block, pair<pid_t, unsigned int> entry) {
#ifdef DEBUG
    log_st << "map fileback page " << filename_str << " block " << block
           << endl;
#endif
    unsigned int ppn = UINT_MAX;

    auto fbe_it = fileback_map.find({filename_str, block});
    if (fbe_it == fileback_map.end()) {
#ifdef DEBUG
        log_st << "never seen block " << block << "in file " << filename_str
               << "before" << endl;
#endif
        fileback_map[{filename_str, block}] = fileback_entry_t();
    } else {
        if (fbe_it->second.ppn != UINT_MAX) {
            ppn = fbe_it->second.ppn;
        }
    }

    fileback_map[{filename_str, block}].referenced_vps.push_back(entry);
    return ppn;
}
/////////// public methods //////////////

int pager_class::fork(pid_t parent_pid, pid_t child_pid) {
    auto parent_it = process_map.find(parent_pid);
    if (parent_it != process_map.end()) {
        /*TODO: advanced */
        if (alloc_non_empty_process(parent_pid, child_pid) == -1) return -1;
        ;
    } else {
        alloc_empty_process(child_pid);
    }
    return 0;
}
void pager_class::switch_process(pid_t target_pid) {
    auto p_it = process_map.find(target_pid);
    if (p_it == process_map.end()) return;  // unknown process
#ifdef DEBUG
    log_st << "switch from " << current_pid << endl;
    if (current_pid != UINT_MAX && current_pid != 0) print_vpt(current_pid);
    log_st << "switch to " << target_pid << endl;
    print_vpt(target_pid);
    print_ppt();
    print_sst();
#endif
    current_pid = target_pid;
    page_table_base_register = process_map[target_pid].page_table_ptr;
}

void pager_class::destroy_current_process() {
#ifdef DEBUG
    log_st << "destroy_current_process curret_id=" << current_pid << endl;
    print_vpt(current_pid);
    print_ppt();
    print_sst();
    print_fbm();
#endif
    auto process_it = process_map.find(current_pid);
    os_vpt_t &os_vpt(*process_it->second.os_virtual_page_table_ptr);
    page_table_entry_t *pt(process_it->second.page_table_ptr->ptes);

    /* clean pages */
    for (unsigned int vpn = 0; vpn < VM_ARENA_SIZE / VM_PAGESIZE; ++vpn) {
        if (!os_vpt[vpn].valid) continue;
        bool resident = os_vpt[vpn].resident;
        if (os_vpt[vpn].resident) destroy_resident_page(pt[vpn].ppage, vpn);

        if (os_vpt[vpn].is_swap) {  // swap back page
            if (!resident ||
                pt[vpn].ppage != 0)  // not paged to zero page
                destroy_swap_back_entry(os_vpt[vpn].block_num, vpn);
            swap_page_count--;
        } else {  // fileback page
            destroy_fileback_page_entry(os_vpt[vpn].filename,
                                        os_vpt[vpn].block_num,
                                        resident, vpn);
        }
    }

    /* delete virtual tables */
    delete process_it->second.os_virtual_page_table_ptr;
    delete process_it->second.page_table_ptr;

    /* delete process */
    process_map.erase(process_it);
    current_pid = 0;
#ifdef DEBUG
    log_st << "Successfully destroyed process " << current_pid << endl;
    print_ppt();
    print_sst();
    // log_st << "什么鬼" << endl;
#endif
}

int pager_class::fault_handler(unsigned int vpn, bool write_flag) {
#ifdef DEBUG
    log_st << "fault_handler "
           << "vpn:" << vpn << " write? " << write_flag << endl;

    evict_log << "fault_handler "
              << "vpn:" << vpn << " write? " << write_flag << endl;
#endif
    if (vpn > VM_ARENA_SIZE / VM_PAGESIZE) return -1;
    auto process_it = process_map.find(current_pid);
    virtual_page_entry_t &os_vpt_entry(
        (*process_it->second.os_virtual_page_table_ptr)[vpn]);
    page_table_entry_t &page_table_entry(
        process_it->second.page_table_ptr->ptes[vpn]);
    /* check if current vpn is valid */
    if (!os_vpt_entry.valid) return -1;
    /* check if is non-resident page */
    if (!os_vpt_entry.resident) {
#ifdef DEBUG
        evict_log << "Originally not resident" << endl;
#endif
        if (restore(vpn) == UINT_MAX) {
#ifdef DEBUG
            print_vpt(current_pid);
            print_ppt();
            print_sst();
            for (auto clock_it = clock_queue.begin();
                 clock_it != clock_queue.end(); clock_it++) {
                evict_log << *clock_it << " ("
                          << physical_page_table[*clock_it].evict_ref << ")\t";
            }
            evict_log << endl;
            evict_log << "============================" << endl;
#endif
            return -1;
        }
    }

    ////// assumption: page is valid in memory ///////
    unsigned int ppn = page_table_entry.ppage;
    physical_page_entry_t &old_phy_entry(physical_page_table[ppn]);

    // map vpn to new ppn if nessessary
    if (write_flag && old_phy_entry.referenced_vpages.size() > 1 &&
        old_phy_entry.is_swap)
        ppn = copy_and_change_pointer(vpn);

    assert(ppn != UINT_MAX);
    physical_page_entry_t &phy_entry(physical_page_table[ppn]);

    /* add evict reference */
#ifdef DEBUG
    log_st << "set ppn " << ppn << " evict ref to true" << endl;
#endif
    phy_entry.evict_ref = true;

    /* set physical page dirty bit if write */
    if (write_flag) {
        phy_entry.dirty = true;
        page_table_entry.write_enable = 1;
    }

    update_shared_vpt(ppn);
#ifdef DEBUG
    print_vpt(current_pid);
    print_ppt();
    print_sst();
    print_fbm();
    evict_log << "Now maps to " << ppn << endl;
    for (auto clock_it = clock_queue.begin(); clock_it != clock_queue.end();
         clock_it++) {
        evict_log << *clock_it << " ("
                  << physical_page_table[*clock_it].evict_ref << ")\t";
    }
    evict_log << endl;
    evict_log << "============================" << endl;
#endif
    return 0;
}

unsigned int pager_class::alloc_swap_page() {
#ifdef DEBUG
    log_st << "alloc_swap_page called! swap page count " << swap_page_count
           << " out of " << swap_blocks << endl;
#endif
    /* check if swap slots are full */
    if (swap_page_count >= swap_blocks) return UINT_MAX;
    unsigned int vpn;
    // reference to the virtual page table (os_vpt is a vector) (for
    // convenience)
    os_vpt_t &os_vpt(*process_map[current_pid].os_virtual_page_table_ptr);
    // pointer to the page table (page_table is an array pointer)
    page_table_entry_t *page_table(
        process_map[current_pid].page_table_ptr->ptes);

    // find first invalid virtual page number
    for (vpn = 0; vpn < VM_ARENA_SIZE / VM_PAGESIZE; vpn++) {
        if (!os_vpt[vpn].valid) break;
    }

    // fail if the arena is full
    if (vpn >= VM_ARENA_SIZE / VM_PAGESIZE) return UINT_MAX;

    os_vpt[vpn].valid = true;
    os_vpt[vpn].resident = true;

    page_table[vpn].ppage = 0;  // map to pinning zero page
    page_table[vpn].read_enable = 1;
    page_table[vpn].write_enable = 0;

    swap_page_count++;
    physical_page_table[0].referenced_vpages.push_back({current_pid, vpn});

    return vpn;
}

unsigned int pager_class::alloc_fileback_page(string filename_str,
                                              unsigned int block_num) {
#ifdef DEBUG
    log_st << "alloc_fileback_page for " << filename_str << " block "
           << block_num << endl;
#endif
    auto &os_vpt(*process_map[current_pid].os_virtual_page_table_ptr);
    auto &pt(process_map[current_pid].page_table_ptr->ptes);
    /* find available vpn for file */
    unsigned int vpn;
    for (vpn = 0; vpn < VM_ARENA_SIZE / VM_PAGESIZE; vpn++) {
        if (!os_vpt[vpn].valid) break;
    }
    if (vpn >= VM_ARENA_SIZE / VM_PAGESIZE) return UINT_MAX;
#ifdef DEBUG
    log_st << "map to vpn=" << vpn << endl;
#endif
    unsigned int ppn =
        map_fileback_page_entry(filename_str, block_num, {current_pid, vpn});

    os_vpt[vpn].valid = true;
    os_vpt[vpn].resident = false;
    os_vpt[vpn].is_swap = false;
    os_vpt[vpn].filename = filename_str;
    os_vpt[vpn].block_num = block_num;
    pt[vpn].ppage = 0;
    pt[vpn].read_enable = 0;
    pt[vpn].write_enable = 0;

#ifdef DEBUG
    log_st << "fileback page is resident at " << ppn << endl;
#endif
    /* if the page is resident */
    if (ppn != UINT_MAX) {
        physical_page_table[ppn].referenced_vpages.push_back(
            {current_pid, vpn});
        update_shared_vpt(ppn);
    }

#ifdef DEBUG
    print_vpt(current_pid);
    print_ppt();
    print_sst();
#endif
    return vpn;
}

///////////// DEBUG PRINTS ////////////////////
#ifdef DEBUG
std::ofstream log_st("debug.log");
std::ofstream evict_log("evict.log");

void pager_class::print_vpt(pid_t pid) {
    log_st << "====== virtual page table for " << pid << "===========" << endl;
    log_st << "vpn\tppage\tread_enable\twrite_"
              "enable\tvalid\tresident\tswap?\tfilename\tblock_num"
           << endl;
    os_vpt_t &os_vpt(*process_map[pid].os_virtual_page_table_ptr);
    page_table_entry_t *pt(process_map[pid].page_table_ptr->ptes);
    for (unsigned int vpn = 0; vpn < VM_ARENA_SIZE / VM_PAGESIZE; vpn++) {
        if (!os_vpt[vpn].valid) continue;
        log_st << vpn << "\t" << pt[vpn].ppage << "\t" << pt[vpn].read_enable
               << "\t\t" << pt[vpn].write_enable << "\t\t";
        log_st << os_vpt[vpn].valid << "\t" << os_vpt[vpn].resident << "\t\t"
               << os_vpt[vpn].is_swap << "\t";
        if (os_vpt[vpn].filename != "")
            log_st << string(os_vpt[vpn].filename);
        else
            log_st << "nullptr ";
        log_st << "\t" << os_vpt[vpn].block_num << endl;

        /* check for physical page table, virtual page table coherence */
        if (os_vpt[vpn].resident && pt[vpn].ppage != 0) {
            assert(physical_page_table[pt[vpn].ppage].filename ==
                   os_vpt[vpn].filename);
            assert(physical_page_table[pt[vpn].ppage].block_num ==
                   os_vpt[vpn].block_num);
            auto it =
                physical_page_table[pt[vpn].ppage].referenced_vpages.begin();
            for (; it !=
                   physical_page_table[pt[vpn].ppage].referenced_vpages.end();
                 it++) {
                if (it->first == pid && it->second == vpn) break;
            }
            assert(it !=
                   physical_page_table[pt[vpn].ppage].referenced_vpages.end());
        }
    }
}

void pager_class::print_ppt() {
    unsigned int count = 0;
    log_st << "######## physical page table #############"
           << "resident_page_count " << resident_page_count << endl;
    log_st << "ppn\tdirty\tevict_ref\tin_disk\tfilename\tblock_num\tis_"
              "swap\treferenced_"
              "pages"
           << endl;
    for (unsigned int ppn = 0; ppn < memory_pages; ppn++) {
        if (!physical_page_table[ppn].valid) continue;
        count++;
        log_st << ppn << "\t" << physical_page_table[ppn].dirty << "\t"
               << physical_page_table[ppn].evict_ref << "\t\t"
               << physical_page_table[ppn].in_disk << "\t";
        log_st << physical_page_table[ppn].filename << "\t"
               << physical_page_table[ppn].block_num << "\t"
               << physical_page_table[ppn].is_swap << "\t";
        for (auto elt : physical_page_table[ppn].referenced_vpages)
            log_st << "<" << elt.first << "," << elt.second << "> ";
        log_st << endl;

        if (ppn == 0) continue;

        /* check for fileback_map/ swap_slot_table coherrence */
        if (physical_page_table[ppn].is_swap) {
            assert(swap_slot_table[physical_page_table[ppn].block_num].valid);
            assert(swap_slot_table[physical_page_table[ppn].block_num]
                       .referenced_vps ==
                   physical_page_table[ppn].referenced_vpages);
        } else {
            assert(fileback_map.find({physical_page_table[ppn].filename,
                                      physical_page_table[ppn].block_num}) !=
                   fileback_map.end());
            auto &fbe(fileback_map[{physical_page_table[ppn].filename,
                                    physical_page_table[ppn].block_num}]);
            assert(fbe.referenced_vps ==
                   physical_page_table[ppn].referenced_vpages);
            assert(fbe.ppn == ppn);
        }
    }
    assert(resident_page_count == count);
}

void pager_class::print_sst() {
    log_st << "------------ swap slot table -------------"
           << "swap page count " << swap_page_count << endl;
    log_st << "block\treferenced_pages" << endl;
    for (unsigned int i = 0; i < swap_blocks; i++) {
        if (!swap_slot_table[i].valid) continue;
        log_st << i << "\t";
        for (auto elt : swap_slot_table[i].referenced_vps)
            log_st << "<" << elt.first << "," << elt.second << ">";
        log_st << endl;
    }
    // log_st << "------------ swap slot table ends -------------" << endl;
}

void pager_class::print_fbm() {
    log_st << "----------- fileback map -------------" << endl;
    log_st << "filename\tblock\tppn\treference_vpts" << endl;
    for (auto &entry : fileback_map) {
        log_st << entry.first.first << "\t" << entry.first.second << "\t";
        log_st << entry.second.ppn << "\t";
        for (auto vpage : entry.second.referenced_vps) {
            log_st << "<" << vpage.first << "," << vpage.second << ">";
        }
        log_st << endl;
    }
}
#endif