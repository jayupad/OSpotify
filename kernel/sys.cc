#include "sys.h"
#include "stdint.h"
#include "idt.h"
#include "debug.h"
#include "threads.h"
#include "process.h"
#include "machine.h"
#include "ext2.h"
#include "elf.h"
#include "libk.h"
#include "file.h"
#include "heap.h"
#include "shared.h"
#include "kernel.h"

int SYS::exec(const char* path,
              int argc,
              const char* argv[]
) {
    using namespace gheith;
    auto file = root_fs->find(root_fs->root,path); 
    if (file == nullptr) {
        return -1;
    }
    if (!file->is_file()) {
        return -1;
    }

    uint32_t sp = 0xefffe000;

    // Copy arguments
    // Clear the address space
    // MISSING();
    uint32_t total_size = 4 + 4; // argc + char** 
    for (int i = 0; i < argc; i ++) {
        total_size += 4;
        uint32_t len = K::strlen(argv[i]);
        total_size += len;
        total_size = total_size +  4 - (total_size % 4);
    }
    total_size += 4; // CHECK HERE
    total_size = total_size +  4 - (total_size % 4);
    sp -= total_size / 4;
    uint32_t* sp_pointer =  (uint32_t*) sp;
    *sp_pointer = argc;
    sp_pointer ++;
    *sp_pointer = (uint32_t) (sp_pointer + 1);
    sp_pointer ++;

    uint32_t* string_pointers = sp_pointer;
    char* strings = (char*) (sp_pointer + argc);
    for (int i = 0; i < argc; i ++) {
        *string_pointers = (uint32_t) strings;
        // uint32_t len = K::strlen(argv[i]) + 4 - (K::strlen(argv[i]) % 4);
        uint32_t len = K::strlen(argv[i]);
        len += 4 - (len % 4);
        memcpy(strings, argv[i], len);
        strings += len;
        string_pointers ++;
    }
    string_pointers = 0x00000000; // CHECK HERE
    


    uint32_t e = ELF::load(file);
    file == nullptr;

    switchToUser(e,sp,0);
    Debug::panic("*** implement switchToUser");
    return -1;
}

extern "C" int sysHandler(uint32_t eax, uint32_t *frame) {
    using namespace gheith;

    uint32_t *userEsp = (uint32_t*)frame[3];
    uint32_t userPC = frame[0];

    // Debug::printf("*** syscall #%d\n",eax);

    switch (eax) {
    case 0:
        {
            auto status = userEsp[1];
            current()->process->clear_private();
	    // MISSING();
            current()->process->output->set(status);
            stop();
        }
        return 0;
    case 1: /* write */
        {
            int fd = (int) userEsp[1];
            char* buf = (char*) userEsp[2];
            size_t nbyte = (size_t) userEsp[3];
	    // MISSING();
            if (fd < 0 || fd > 2) return -1;
            if ((uint32_t) buf < 0x80000000) return -1;
            auto file = current()->process->getFile(fd);
            if (file == nullptr) return -1;
            if (file->isMine()) return -1;
            return file->write(buf,nbyte);
        }
    case 2: /* fork */
    	{
		    // MISSING();
            int index = -1;
            Shared<Process> parent_process = current()->process;
            Shared<Process> child_process = parent_process->fork(index);
            // Debug::printf("index %x\n", index);
            if (child_process == nullptr) return -1;
            thread(child_process, [userPC, userEsp, eax] {
                switchToUser(userPC, (uint32_t) userEsp, 0);
            });
            
    		return index;
    	}
    case 3: /* sem */
        {
		    // MISSING();
            return current()->process->newSemaphore(userEsp[1]);
    		// return -1;
        }

    case 4: /* up */
    	{
		    // MISSING();
            Shared<Semaphore> sem = current()->process->getSemaphore(userEsp[1]);
            if (sem == nullptr) return -1;
            sem->up();
    		return 0;
    	}
    case 5: /* down */
      	{
		    // MISSING();
            Shared<Semaphore> sem = current()->process->getSemaphore(userEsp[1]);
            if (sem == nullptr) return -1;
            sem->down();
    		return 0;
       	}
    case 6: /* close */
        // MISSING();
        // Shared<Process> p = current()->process;
        return current()->process->close(userEsp[1]);
        // return -1;

    case 7: /* shutdown */
		Debug::shutdown();
        return -1;

    case 8: /* wait */
        // MISSING();
        if (current()->process->children[userEsp[1] & 0x0FFFFFFF] == nullptr) return -1;
        if (userEsp[2] < 0x80000000) return -1;
        return current()->process->wait(userEsp[1], (uint32_t*) userEsp[2]);
        // return -1;

    case 9: /* execl */
        // MISSING();
        {

                /*
                Shared<Node> inode = root_fs->find(root_fs->root, (char*) userEsp[1]);
                if (inode == nullptr) return -1;
                int count = 0;
                for (int i = 2; userEsp[i] != '\0'; i ++) {
                    count ++;
                }
                int* str_count = new int[count];
                for (int i = 0; i < count; i ++) {
                    str_count[i] = K::strlen((char*) userEsp[2 + i]);
                }
                char** argv = new char*[count];
                for (int i = 0; i < count; i ++) {
                    memcpy(argv[i], (char*) userEsp[i + 2], str_count[i]);
                    // create strings on the heap and then delete them in exec
                }

                delete str_count
                */
        // Shared<Node> inode = root_fs->find(root_fs->root, (char*) userEsp[1]);
        // if (inode == nullptr) return -1;
        // int count = 0;
        // for (int i = 2; userEsp[i] != '\0'; i ++) {
        //     count ++;
        // }
        
        // char** argv = new char*[count];
        // for (int i = 0; i < count; i ++) {
        //     argv[i] = (char*) userEsp[i + 2];
        //     // create strings on the heap and then delete them in exec
        // }

        // TO DO: INVALID ELF FILE, PATH OR ARGS NOT IN USER SPACE // 
        if (userEsp[1] < 0x80000000) return -1;
        Shared<Node> inode = root_fs->find(root_fs->root, (char*) userEsp[1]);
        if (inode == nullptr) return -1;
        uint32_t sym_count = 1;
        while (inode->is_symlink()) {
            if (sym_count >= 10) {
                return -1;
            }
            sym_count ++;
            char* buf = new char[inode->size_in_bytes() + 1];
            inode->get_symbol(buf);
            buf[inode->size_in_bytes()] = '\0';
            inode = root_fs->find(root_fs->root, buf);
            delete buf;
        }
        ElfHeader* elf = new ElfHeader;
        inode->read_all(0, sizeof(ElfHeader), (char*) elf);
        if (elf->maigc0 != 0x7F || elf->magic1 != 'E' || elf->magic2 != 'L' || elf->magic3 != 'F' || elf->entry < 0x80000000) return -1;
        for (int i = 0; i < elf->phnum; i ++) {
            ProgramHeader p;
            inode->read(elf->phoff, p);
            
            if (p.vaddr + p.memsz < 0x80000000) {
                delete elf;
                return -1;
            }
        }
        delete elf;
        int count = 0;
        for (int i = 2; userEsp[i] != '\0'; i ++) {
            if (userEsp[i] < 0x80000000) return -1;
            count ++;
        }
        int* str_count = new int[count];
        for (int i = 0; i < count; i ++) {
            str_count[i] = K::strlen((char*) userEsp[2 + i]) + 1;
        }
        char** argv = new char*[count];
        for (int i = 0; i < count; i ++) {
            argv[i] = new char[str_count[i]];
            memcpy(argv[i], (char*) userEsp[i + 2], str_count[i] - 1);
            argv[i][str_count[i] - 1] = '\0';
            // create strings on the heap and then delete them in exec
        }
        // DO I NEED TO CLEAR PRIVATE SPACE?
        int len = K::strlen((char*) userEsp[1]);
        char* path = new char[len + 1];
        memcpy(path, (char*) userEsp[1], len);
        path[len] = '\0';
        current()->process->clear_private();
        for (int i = 0; i < 10; i ++) {
            current()->process->sems[i] = nullptr;
            current()->process->children[i] = nullptr;
        }
        return SYS::exec((const char*) path, count, (const char**) argv);
        }
		

    case 10: /* open */
    {
        // MISSING();
        char* path_to = (char*) userEsp[1];
        if (path_to == nullptr || (uint32_t) path_to < 0x80000000) return -1;
        Shared<Node> inode_ret = root_fs->find(root_fs->root, path_to);
        if (inode_ret == nullptr) return -1;
        uint32_t count = 1;
        while (inode_ret->is_symlink()) {
            if (count >= 10) {
                return -1;
            }
            count ++;
            uint32_t len = K::strlen(inode_ret->my_path) + 1;
            char* buf = new char[inode_ret->size_in_bytes() + len];
            inode_ret->get_symbol(buf);
            memcpy(buf + inode_ret->size_in_bytes(), inode_ret->my_path, len); // CHECK THIS SHIT
            Debug::printf("%s\n", buf);
            buf[inode_ret->size_in_bytes() + len - 1] = '\0';
            inode_ret = root_fs->find(root_fs->root, buf);
            delete buf;
        }
        Shared<Process> p = current()->process;
        ShobhitFile* shob = new ShobhitFile(inode_ret);
        Shared<File> file { shob };
        return p->setFile(file);
    }
    case 11: /* len */
    {
        Shared<Process> p = current()->process;
        if (userEsp[1] == 0 || userEsp[1] == 1 || userEsp[1] == 2 ) {
            Shared<File> file = p->getFile(userEsp[1]);
            if (!file->isMine()) {
                return 0;
            }
        }
        Shared<File> file = p->getFile(userEsp[1]);
        if (file == nullptr) {
            return -1;
        }
        return file->size();
        
        // return -1;

    }
    case 12: /* read */
    {
        // MISSING();
        Shared<Process> p = current()->process;
        if (userEsp[2] < 0x80000000) return -1;
        if (userEsp[1] == 0 || userEsp[1] == 1 || userEsp[1] == 2) {
            Shared<File> file = p->getFile(userEsp[1]);
            if (file == nullptr || !file->isMine()) {
                return -1;
            }
        }
        Shared<File> f = p->getFile(userEsp[1]);
        if (f == nullptr) return -1;
        return f->read((void*) userEsp[2], userEsp[3]);

        // return -1;

    }
    case 13: /* seek */
    {
        // MISSING();
        Shared<Process> p = current()->process;
        if (userEsp[1] == 0 || userEsp[1] == 1 || userEsp[1] == 2) {
            Shared<File> file = p->getFile(userEsp[1]);
            if (file == nullptr || !file->isMine()) {
                return -1;
            }
        }

        Shared<File> f = p->getFile(userEsp[1]);
        if (f == nullptr) return -1;
        return f->seek(userEsp[2]);
    }

    default:
        Debug::printf("*** 1000000000 unknown system call %d\n",eax);
        return -1;
    }
    
}   

void SYS::init(void) {
    IDT::trap(48,(uint32_t)sysHandler_,3);
}
