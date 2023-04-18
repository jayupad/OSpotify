#ifndef _FILE_H_
#define _FILE_H_

#include "atomic.h"
#include "stdint.h"
#include "shared.h"
#include "ext2.h"

class File {
    Atomic<uint32_t> ref_count;
public:
    Shared<Node> inode {};
    File(): ref_count(0) {}
    virtual ~File() {}
    virtual bool isFile() = 0;
    virtual bool isDirectory() = 0;
    virtual off_t size() = 0;
    virtual off_t seek(off_t offset) = 0;
    virtual ssize_t read(void* buf, size_t size) = 0;
    virtual ssize_t write(void* buf, size_t size) = 0;
    virtual bool isMine() { return false; }

    friend class Shared<File>;
};

class ShobhitFile : public File {
    Atomic<uint32_t> ref_count;
    
    public:
    bool isU2;
    friend class Shared<ShobhitFile>;
    Shared<Node> inode {};
    Atomic<off_t> offset {0};

    ShobhitFile() : ref_count(0), isU2(false) {

    }

    ShobhitFile(Shared<Node> f) : ref_count(0) {
        inode = f;
        offset = 0;
    }

    ~ShobhitFile() {
        inode = nullptr;
    }

    bool isFile() {
        return inode->is_file();
    }

    bool isDirectory() {
        return inode->is_dir();
    }

    off_t size() {
        return inode->size_in_bytes();
    }

    off_t seek(off_t offset) {
        this->offset.set(offset);
        return offset;
    }

    ssize_t read(void* buf, size_t size) {
        if (offset.get() >= this->size()) {
            return 0;
        }
        // Debug::printf("size of file %d\n", this->size());
        if (size > this->size() - offset.get()) {
            size = this->size() - offset.get();
        }
        inode->read_all(offset, size, (char*) buf);
        offset.add_fetch(size);
        // Debug::printf("\n\n\n\noffbn: %d\n\n\n\n\n", offset.get());
        return size;
    }

    ssize_t write(void* buf, size_t size) {
        return size;
    }

    bool isMine() {
        return true;
    }

};

#endif
