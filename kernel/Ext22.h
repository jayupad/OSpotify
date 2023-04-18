#ifndef _Ext22_h_
#define _Ext22_h_

#include "ide.h"
#include "atomic.h"
#include "debug.h"
#include "shared.h"
#include "libk.h"
#include "blocking_lock.h"

struct Pixel {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct DirectoryEntry {
    uint32_t iNode2;
    uint16_t total_size;
    uint8_t name_length;
    char type_indicator;
    char name[256];
};

struct INode2Stuff {
    uint16_t type_permissions;
    uint16_t user_id;
    uint32_t lower32_size;
    uint32_t last_access_time;
    uint32_t creation_time;
    uint32_t last_mod_time;
    uint32_t deletion_time;
    uint16_t group_id;
    uint16_t hard_link_count;
    uint32_t disk_sectors;
    uint32_t flags;
    uint32_t operator_specific_value;

    uint32_t direct_pointers[12];
    uint32_t single_Node2;
    uint32_t double_Node2;
    uint32_t triple_Node2;
    
    uint32_t generation_number;
    uint32_t some_val;
    uint32_t some_val2;
    uint32_t block_address_fragment;
    char OSSV2[12];
};

struct SuperBlock2 {
    uint32_t number_of_iNode2s;
    uint32_t number_of_blocks;
    uint32_t number_of_blocks_for_superuser;
    uint32_t number_of_unallocated;
    uint32_t number_of_unallocated_iNode2s;
    uint32_t block_of_SuperBlock2;
    uint32_t log2_blocksize_minus_10;
    uint32_t log2_fragmentsize_minus_10;
    uint32_t number_of_blocks_in_groups;
    uint32_t number_of_fragments_in_groups;
    uint32_t number_of_iNode2s_in_groups;
    uint32_t last_mount_time;
    uint32_t last_written_time;
    uint16_t number_of_times_the_volume_has_been_mounted;
    uint16_t number_of_mounts_allowed_before_consistency_check;
    uint16_t Ext22_signature;
    uint16_t file_system_state;
    uint16_t what_to_do_with_error;
    uint16_t minor_portion_of_version;
    uint32_t posix_time;
    uint32_t interval_between_forced_consistency_checks;
    uint32_t operating_system_id;
    uint32_t major_portion_of_version;
    uint16_t user_id_that_can_use;
    uint16_t group_id_that_can_use;
};

struct GroupBlock {
    uint32_t block_address_of_block;
    uint32_t block_address_of_iNode2;
    uint32_t starting_block_address_of_table;
    uint16_t number_of_unallocated_blocks;
    uint16_t number_unallocated_iNode2s;
    uint16_t number_of_directories;
    char unused[14];
};


struct CacheBlockBlock {
    uint32_t number;
    uint32_t block_num;
    char valid;
    uint16_t lru;
    char alinger;
    char* hddBlock;
};

class BlockCache {
    BlockingLock lock {};
    public:
    uint16_t lru;
    uint32_t s;
    uint32_t b;
    CacheBlockBlock*** cache;
    GroupBlock* block_group_table;
    SuperBlock2* mySuperBlock2;
    uint32_t block_size;
    Shared<Ide> ide;

    BlockCache(uint32_t s, uint32_t b, GroupBlock* bgdt, SuperBlock2* sb, uint32_t block_size, Shared<Ide> ide) : s(s), b(b), block_group_table(bgdt), mySuperBlock2(sb), block_size(block_size), ide(ide) {
        uint32_t S = 1 << s;
        uint32_t B = 1 << b;
        lru = 0;
        cache = new CacheBlockBlock**[S];
        for (uint32_t i = 0; i < S; i ++) {
            cache[i] = new CacheBlockBlock*[B];
            for (uint32_t j = 0; j < B; j ++) {
                cache[i][j] = new CacheBlockBlock;
                cache[i][j]->valid = '0';
                cache[i][j]->lru = 0;
                cache[i][j]->hddBlock = nullptr;
            }
        }
    }

    void print() {
        uint32_t S = 1 << s;
        uint32_t B = 1 << b;
        Debug::printf("cache:\n");
        for (uint32_t j = 0; j < S; j ++) {
            CacheBlockBlock** line = cache[j];
            Debug::printf("set %d: \n", j);
            for (uint32_t i = 0; i < B; i ++) {
                Debug::printf("room %d: %d", i, line[i]->number);
            }
            Debug::printf("\n");
        }
    }

    

    void get(uint32_t numbers, uint32_t number, char* buffer, INode2Stuff* caller) {
        lock.lock();
        lru += 1;
        uint32_t S = 1 << s;
        uint32_t B = 1 << b;
        uint32_t set = numbers % S;
        CacheBlockBlock** line = cache[set];
        // Debug::printf("S: %d\n", S);
        // print();
        for (uint32_t i = 0; i < B; i ++) {
            CacheBlockBlock* curr = line[i];
            // Debug::printf("given_block_num: %d, block_num: %d, valid: %c, iNode2_given: %d, curr_inoce: %d\n", numbers, curr->block_num, curr->valid, number, curr->number);
            if (curr->valid == '1') {
                if (curr->number == number && curr->block_num == numbers) {
                    // Debug::printf("hit\n");
                    memcpy(buffer, curr->hddBlock, block_size);
                    curr->lru = lru;
                    lock.unlock();
                    return;
                }
            }
        }
        miss(line, number, numbers, buffer, caller);
        lock.unlock();
    }

    void miss(CacheBlockBlock** line, uint32_t number, uint32_t numbers, char* buffer, INode2Stuff* caller) {
        // Debug::printf("in miss\n");
        uint32_t B = 1 << b;
        uint32_t min_index = 0;
        bool found_one = false;
        for (uint32_t i = 0; i < B; i ++) {
            if (line[i]->valid == '0') {
                min_index = i;
                found_one = true;
                break;
            }
        }
        if (!found_one) {
            for (uint32_t i = 1; i < B; i ++) {
                if (line[i]->lru < line[min_index]->lru) {
                    min_index = i;
                }
            }
        }

        CacheBlockBlock* curr = line[min_index];
        // Debug::printf("%d : %d\n", curr->number, min_index);
        curr->lru = lru;
        curr->number = number;
        curr->valid = '1';
        // Debug::printf("set : \n");
        // for (uint32_t i = 0; i < B; i ++) {
        //     Debug::printf("room %d: %d || \n", i, curr->number);
        // }
        // Debug::printf("lru: %d\n", lru);
        if (curr->hddBlock != nullptr) {
            delete curr->hddBlock;
        }
        curr->hddBlock = new char[block_size];
        curr->block_num = numbers;
        read_block(caller, numbers, buffer);
        memcpy(curr->hddBlock, buffer, block_size);
        // for (uint32_t i = 1; i < B; i ++)  {
        //     line[i]->number = number;
        //     line[i]->valid = '1';
        //     if (line[i]->hddBlock != nullptr) {
        //         delete line[i]->hddBlock;
        //     }
        //     line[i]->hddBlock = new char[block_size];
        //     line[i]->block_num = numbers + i;
        //     read_block(caller, numbers + i, line[i]->hddBlock);
        // }
        // Debug::printf("number: %d\ncurr_num: %d\n", number, curr->number);
        // Debug::printf("adress: %x\n", curr);
    }

    void read_block(INode2Stuff* caller, uint32_t number, char* buffer) {
        if (number < 12) {
            ide->read_all(caller->direct_pointers[number] * block_size, block_size, buffer);
        } else if (number < ((block_size / 4) + 12)) {
            uint32_t* arr = new uint32_t[block_size / 4];
            ide->read_all(caller->single_Node2 * block_size, block_size, (char *) arr);
            ide->read_all((arr[number - 12]) * block_size, block_size, buffer);
            delete arr;
        } else if (number < (((block_size * block_size) / 16) + ((block_size / 4) + 12))) {
            uint32_t* arr = new uint32_t[block_size / 4];
            uint32_t num_indexes = (block_size / 4);
            ide->read_all(caller->double_Node2 * block_size, block_size, (char*) arr);
            uint32_t* next_arr = new uint32_t[block_size / 4];
            uint32_t next_index = number - 12 -  num_indexes;
            ide->read_all(arr[next_index / (num_indexes)] * block_size, block_size, (char*) next_arr);
            delete arr;
            uint32_t next_next_index = next_index % num_indexes;
            ide->read_all(next_arr[next_next_index] * block_size, block_size, buffer);
            delete next_arr;
        } else if (number < (((block_size * block_size * block_size) / 64) + 13)) {
            uint32_t num_indexes = block_size / 4;
            uint32_t* first_arr = new uint32_t[block_size / 4];
            ide->read_all(caller->triple_Node2, block_size, (char*) first_arr);
            uint32_t* second_arr = new uint32_t[block_size / 4];
            uint32_t second_index = ((number - 14) / (num_indexes)) / (num_indexes);
            ide->read_all(first_arr[second_index], block_size, (char*) second_arr);
            uint32_t* third_arr = new uint32_t[block_size /4];
            uint32_t third_index = ((number - 14) / num_indexes) % num_indexes;
            ide->read_all(second_arr[third_index], block_size, (char*) third_arr);
            uint32_t fourth_index = (number - 14) % num_indexes;
            ide->read_all(third_arr[fourth_index], block_size, (char*) buffer);
            delete first_arr;
            delete second_arr;
            delete third_arr;
        }
    }
};

struct StackNode2 {
    uint32_t iNode2_number;
    // StackNode2* next;
    char* name;
};


class DirectoryArray {
    public:
        StackNode2** arr;
        uint32_t num;
        uint32_t curr;

        DirectoryArray(uint32_t num_dirs) {
            arr = new StackNode2*[num_dirs];
            num = num_dirs;
            curr = 0;
        }

        uint32_t find_name(const char* name) {
            for (uint32_t i = 0; i < num; i ++) {
                StackNode2* curr = arr[i];
                if (K::streq(curr->name, name)) {
                    return curr->iNode2_number;
                }
            }

            return 0;
        }

        void put(DirectoryEntry* dir) {
            ASSERT(curr < num);
            arr[curr] = new StackNode2;
            StackNode2* current = arr[curr];
            current->iNode2_number = dir->iNode2;
            current->name = new char[256];
            memcpy(current->name, dir->name, 256);
            curr++;
        }
        
};


// A wrapper around an i-Node2
class Node2 : public BlockIO { // we implement BlockIO because we
                              // represent data

uint32_t block_group;
uint32_t index;
uint32_t containing_block;
bool get_symbol_called;
char* symbol;

    void set_array() {
        ASSERT(is_dir());
        arr = new DirectoryArray(num_directs);
        uint32_t num_blocks = iNode2->lower32_size / block_size;
        uint32_t index = 0;
        while (num_blocks > 0) {
            char* buffer = new char[block_size];
            read_block(index, buffer);
            uint32_t current_start = 0;
            while (current_start < block_size) {
                DirectoryEntry* dir = new DirectoryEntry;
                dir->iNode2 = *(uint32_t*) (buffer + current_start);
                dir->total_size = *(uint16_t*) (buffer + 4 + current_start);
                dir->name_length = *(uint8_t*) (buffer + 6 + current_start);
                dir->type_indicator = *(uint8_t*) (buffer + 7 + current_start);
                memcpy(dir->name, (buffer + 8 + current_start), dir->name_length);
                current_start += dir->total_size;
                dir->name[dir->name_length] = '\0';
                arr->put(dir);
                delete dir;
            }
            index ++;
            num_blocks --;
            delete buffer;
        }
    }

public:
    Shared<Ide> ide_n;
    INode2Stuff *iNode2;
    uint32_t num_directs;
    // i-number of this Node2
    const uint32_t number;
    Atomic<int> ref_count{0};
    BlockCache* bCache;
    DirectoryArray* arr;
    // Node2Cache* nCache;


    Node2(uint32_t block_size, uint32_t number, SuperBlock2* mySuperBlock2, Shared<Ide> ide, GroupBlock* group_block_table, BlockCache* bCache) : BlockIO(block_size), number(number), bCache(bCache) {
        // MISSING();
        block_group = (number - 1) / mySuperBlock2->number_of_iNode2s_in_groups;
        index = (number - 1) % mySuperBlock2->number_of_iNode2s_in_groups;
        containing_block = (index * sizeof(INode2Stuff)) / block_size;
        iNode2 = new INode2Stuff;
        uint32_t iNode2_start = group_block_table[block_group].starting_block_address_of_table * block_size + (index * 128);
        ide->read_all(iNode2_start, 128, (char*)iNode2); // might need to change to read
        ide_n = ide;
        if (is_dir()) {
            num_directs = 0;
            entry_count();
            set_array();
        }
        if (is_symlink()) {
            get_symbol_called = false;
            char* buf = new char[block_size];
            get_symbol(buf);
            delete buf;
        }
    }

    virtual ~Node2() {
        delete iNode2;
        delete symbol;
        delete arr;
    }

    char* read_bmp(Shared<Node2> bmp) {
        // for (int y = 0; y < 70; ++y) {
        //     for (int x = 0; x < 70; ++x) {
        //         uint8_t pixel[3];
        //         ide_n->read_all(54 + y * 70 + x, 3, (char*) pixel);
        //         (*pixels)[(70 * (69 - y)) + x].r = pixel[2];
        //         (*pixels)[(70 * (69 - y)) + x].g = pixel[1];
        //         (*pixels)[(70 * (69 - y)) + x].b = pixel[0]; // endianess
        //     }
        // }

        // char * con = new char[2];
        // bmp->read_all(0, 2, con);
        // Debug::printf("inside read_bmp Value of the first two: %s, Hex for Kavya: %x\n",con, *con);

        char* file_header = new char[14];
        char* info_header = new char[40];
        // BMPFileHeader* fh = new BMPFileHeader();
        // BMPInfoHeader* ih = new BMPInfoHeader();
        bmp->read_all(0, 14, (char*) file_header);
        bmp->read_all(14, 40, (char*) info_header);

        // uint32_t off = *((uint32_t*) (file_header + 10));
        // uint32_t off = 138;
        // uint16_t bit_count = *((uint16_t*) (info_header + 14));
        // uint32_t size = *((uint32_t*) (file_header + 2));
        // Debug::printf("type: %c", file_header[0]);
        // Debug::printf("%c\n", file_header[1]);
        // Debug::printf("size: %d\n", size);
        // Debug::printf("offset: %x\n", off);
        // Debug::printf("bit count: %d\n", bit_count);
        // if (bit_count < 24) {
        //     Debug::panic("Not enough bits!");
        // } else if (bit_count > 32) {
        //     Debug::panic("Too many bits!!");
        // } 
        // uint32_t width = *((uint32_t*) (info_header + 24));
        // uint32_t height = *((uint32_t*) (info_header + 28));
        // Debug::printf("width: %d, offset: %d\n", width, off);
        // uint32_t width = 70;
        // uint32_t height = 70;
        
        uint8_t* buf = new uint8_t[19738-138];
        bmp->read_all(138, 19738-138, (char*) buf);

        char* ret_shob = new char[((19600) / 4) * 3];

        for (int i = 0, rIdx = 0; i < 19600; i += 4, rIdx += 3) {
            ret_shob[rIdx] = buf[i + 2]; // red
            ret_shob[rIdx + 1] = buf[i + 1]; // green
            ret_shob[rIdx + 2] = buf[i]; // blue
        // Debug::printf("RED: %x\n", (uint8_t)buf[i + 2]);
        // Debug::printf("GREEN: %x\n", (uint8_t)buf[i + 1]);
        // Debug::printf("BLUE: %x\n", (uint8_t)buf[i]);
        // Debug::printf("ALPHA: %x\n---------\n", (uint8_t)buf[i + 3]);

        // Debug::printf("RED: %x\n", (uint8_t)ret_shob[rIdx]);
        // Debug::printf("GREEN: %x\n", (uint8_t)ret_shob[rIdx + 1]);
        // Debug::printf("BLUE: %x\n---------\n", (uint8_t)ret_shob[rIdx + 2]);
        }
        return ret_shob;
    }

    // How many bytes does this i-Node2 represent
    //    - for a file, the size of the file
    //    - for a directory, implementation dependent
    //    - for a symbolic link, the length of the name
    uint32_t size_in_bytes() override {
        // MISSING();
        if (is_file()) {
            return iNode2->lower32_size;
        } else if (is_dir()) {
            return iNode2->lower32_size;
        } else if (is_symlink()) {
            return iNode2->lower32_size; // fix this
        }
        return 17154;
    }

    // read the given block (panics if the block number is not valid)
    // remember that block size is defined by the file system not the device
    void read_block(uint32_t numbers, char* buffer) {
        bCache->get(numbers, number, buffer, iNode2);
    }


    // returns the Ext22 type of the Node2
    uint32_t get_type() {
        // MISSING();
        if (is_dir()) {
            return 2;
        }
        if (is_file()) {
            return 1;
        }
        if (is_symlink()) {
            return 7;
        }
        return 0;
    }

    // true if this Node2 is a directory
    bool is_dir() {
        // MISSING();
        return (iNode2->type_permissions & 0xF000) == 0x4000;
    }

    // true if this Node2 is a file
    bool is_file() {
        // MISSING();
        return (iNode2->type_permissions & 0xF000) == 0x8000;
    }

    // true if this Node2 is a symbolic link
    bool is_symlink() {
        // MISSING();
        return (iNode2->type_permissions & 0xF000) == 0xA000;
    }

    // If this Node2 is a symbolic link, fill the buffer with
    // the name the link referes to.
    //
    // Panics if the Node2 is not a symbolic link
    //
    // The buffer needs to be at least as big as the the value
    // returned by size_in_byte()
    void get_symbol(char* buffer) {
        // MISSING();
        if (!is_symlink()) {
            Debug::panic("is not symbol link\n");
        }
        if (size_in_bytes() < 60) {
            memcpy(buffer, iNode2->direct_pointers, size_in_bytes());
        } else {
            // memcpy(buffer, (char*) (iNode2->direct_pointers[0] * block_size), size_in_bytes());
            if (get_symbol_called) {
                memcpy(buffer, symbol, size_in_bytes());
            } else {
                ide_n->read_all(iNode2->direct_pointers[0] * block_size, size_in_bytes(), buffer); // maybe wrong
                symbol = new char[size_in_bytes()];
                memcpy(symbol, buffer, size_in_bytes());
                get_symbol_called = true;
            }
        }
    }

    // Returns the number of hard links to this Node2
    uint32_t n_links() {
        // MISSING();
        return iNode2->hard_link_count;
    }

    // Returns the number of entries in a directory Node2
    //
    // Panics if not a directory
    uint32_t entry_count() {
        
        if (!is_dir()) {
            Debug::panic("not a directory\n");
        }
        if (num_directs != 0) {
            return num_directs;
        }
        uint32_t num_blocks = iNode2->lower32_size / block_size;
        uint32_t index = 0;
        uint32_t num_entries = 0;
        while (num_blocks > 0) {
            char* buffer = new char[block_size];
            read_block(index, buffer);
            uint32_t current_start = 0;
            while (current_start < block_size) {
                DirectoryEntry* dir = new DirectoryEntry;
                dir->iNode2 = *(uint32_t*) (buffer + current_start);
                dir->total_size = *(uint16_t*) (buffer + 4 + current_start);
                current_start += dir->total_size;
                if (dir->iNode2 != 0) num_entries ++;
                delete dir;
            }
            index ++;
            num_blocks --;
            delete buffer;
        }
        num_directs = num_entries;
        return num_entries;
    }
};

struct CacheBlockINode2 {
    uint32_t number;
    char valid; // stops me from setting everything to null
    uint8_t lru;
    char buffer[2];
    Shared<Node2> me;
    char name[256];
};

class Node2Cache {
    BlockingLock lock {};

    public:
    uint8_t lru;
    uint32_t s;
    uint32_t b;
    CacheBlockINode2*** cache;
    GroupBlock* block_group_table;
    SuperBlock2* mySuperBlock2;
    uint32_t block_size;
    Shared<Ide> ide;
    BlockCache* bCache;

    Node2Cache(uint32_t s, uint32_t b, GroupBlock* bgdt, SuperBlock2* sb, uint32_t block_size, Shared<Ide> ide, BlockCache* bc) : s(s), b(b), block_group_table(bgdt), mySuperBlock2(sb), block_size(block_size), ide(ide), bCache(bc) {
        uint32_t S = 1 << s;
        uint32_t B = 1 << b;
        lru = 0;
        cache = new CacheBlockINode2**[S];
        for (uint32_t i = 0; i < S; i ++) {
            cache[i] = new CacheBlockINode2*[B];
            for (uint32_t j = 0; j < B; j ++) {
                cache[i][j] = new CacheBlockINode2;
                cache[i][j]->valid = '0';
                //  Debug::printf("%c\n", cache[i][j]->valid);
                cache[i][j]->lru = 0;
                // cache[i][j]->me = nullptr;
            }
        }
    }

    Shared<Node2> get(uint32_t number) {
        lock.lock();
        lru ++;
        uint32_t S = 1 << s;
        uint32_t B = 1 << b;
        uint32_t set = number % S;
        CacheBlockINode2** line = cache[set];
        for (uint32_t i = 0; i < B; i ++) {
            CacheBlockINode2* curr = line[i];
            if (curr->valid == '1') {
                if (curr->number == number) {
                    curr->lru = lru;
                    lock.unlock();
                    return curr->me;
                }
            }
        }
        lock.unlock();
        return miss(line, number);
    }


    Shared<Node2> miss(CacheBlockINode2** line, uint32_t number) {
        lock.lock();
        uint32_t min_index = 0;
        uint32_t B = 1 << b;
        for (uint32_t i = 1; i < B; i ++) {
            if (line[i]->lru < line[min_index]->lru) {
                min_index = i;
            }
        }
        CacheBlockINode2* curr = line[min_index];
        if (curr->me != nullptr) {
            curr->me = nullptr;
        }
        curr->me = Shared<Node2>::make(block_size, number, mySuperBlock2, ide, block_group_table, bCache);
        curr->valid = '1';
        curr->lru = lru;
        curr->number = number;
        lock.unlock();
        return curr->me;
    }
};


// This class encapsulates the implementation of the Ext22 file system
class Ext22 {
    SuperBlock2* mySuperBlock2;
    GroupBlock* group_block_table;
    BlockCache* bCache;
    Node2Cache* nCache;
public:
    Shared<Node2> root;
    Atomic<int> ref_count{0};
public:
    // Mount an existing file system residing on the given device
    // Panics if the file system is invalid
    Ext22(Shared<Ide> ide);

    // Returns the block size of the file system. Doesn't have
    // to match that of the underlying device
    uint32_t get_block_size() {
        // MISSING();
        uint32_t pow = mySuperBlock2->log2_blocksize_minus_10 + 10;
        uint32_t block_size = 1 << pow;
        return block_size;
    }

    // Returns the actual size of an i-Node2. Ext22 specifies that
    // an i-Node2 will have a minimum size of 128B but could have
    // more bytes for extended attributes
    uint32_t get_iNode2_size() {
        // MISSING();
        uint32_t size = sizeof(INode2Stuff);
        return size;
    }

    // If the given Node2 is a directory, return a reference to the
    // Node2 linked to that name in the directory.
    //
    // Returns a null reference if "name" doesn't exist in the directory
    //
    // Panics if "dir" is not a directory
    Shared<Node2> find(Shared<Node2> dir, const char* name);
};

#endif
