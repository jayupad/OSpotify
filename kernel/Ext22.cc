#include "Ext22.h"


// Ext22

Ext22::Ext22(Shared<Ide> ide) {
    mySuperBlock2 = new SuperBlock2;
    ide->read_all(1024, 84, (char*) mySuperBlock2);
    auto num_groups = mySuperBlock2->number_of_blocks / mySuperBlock2->number_of_blocks_in_groups;
    auto mod = mySuperBlock2->number_of_blocks % mySuperBlock2->number_of_blocks_in_groups;
    if (mod > 0) {
        num_groups ++;
    }
    group_block_table = new GroupBlock[num_groups];
    auto group_table_size = num_groups * 32;
    auto block_size = get_block_size();
    auto group_block_table_start = 2048;
    if (block_size > 2048) {
        group_block_table_start = block_size;
    }
    // Debug::printf("%d\n", group_block_table_start);
    ide->read_all(group_block_table_start, group_table_size, (char*) group_block_table);
    bCache = new BlockCache(10, 5, group_block_table, mySuperBlock2, block_size, ide);
    nCache = new Node2Cache(9, 2, group_block_table, mySuperBlock2, block_size, ide, bCache);
    root = Shared<Node2>::make(block_size, 2, mySuperBlock2, ide, group_block_table, bCache);
}

Shared<Node2> Ext22::find(Shared<Node2> dir, const char* name) {
    uint32_t num = dir->arr->find_name(name);
    if (num != 0) {
        Shared<Node2> ret = nCache->get(num);
        return ret;
    }
    return Shared<Node2> {};
}