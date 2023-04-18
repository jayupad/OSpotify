#include "ide.h"
#include "ext2.h"
#include "Ext22.h"
#include "libk.h"
#include "threads.h"
#include "semaphore.h"
#include "future.h"
#include "pit.h"
#include "physmem.h"

struct Header {
    char fmt_chunk_marker[4];          // fmt string with trailing null char
    uint32_t length_of_fmt;                 // length of the format data
    uint16_t format_type;                   // format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
    uint16_t channels;                      // no.of channels
    uint32_t sample_rate;                   // sampling rate (blocks per second)
    uint32_t byte_rate;                      // SampleRate * NumChannels * BitsPerSample/8
    uint16_t bit_channels;                   // NumChannels * BitsPerSample/8
    uint16_t bits_per_sample;               // bits per sample, 8- 8bits, 16- 16 bits etc
};


class WaveParser_list {

    public:
    Header * fmt;
    char * b_entries; 
    uint64_t offset; 
    uint32_t size_of_the_junk;
    Shared<Node2> overallFile; 
    uint32_t size_of_the_whole_file;

    uint32_t bit_divsor; 
    uint32_t bit_per_sample; 

    WaveParser_list(Shared<Node2> file) {
        overallFile = file; 
        offset = 0; 
        fmt = new Header();
        b_entries = (char *) PhysMem::alloc_frame();
        size_of_the_whole_file = 0;

    // RIFF CHECK 
    char* riff = new char[5];
    riff[4] = '\0';
    file->read_all(0, 4, riff);
    Debug::printf("WOOF WOOF: %s\n", riff);
    delete riff; 

    // - Size of File 

    char * size_of_file = new char[5];
    file->read_all(4, 4, size_of_file);
    size_of_the_whole_file = *(uint32_t *)size_of_file;
    Debug::printf("Size Of File: %d\n", *(uint32_t *)size_of_file);
    delete size_of_file; 

    // WAVE CHECK 
    char* wave = new char[5];
    wave[4] = '\0';
    file->read_all(8, 4, wave);
    Debug::printf("Is it wave: %s\n", wave);
    delete wave; 

    Header* fmt_stuff = fmt; 

    Debug::printf("Size Of fmt: %d\n", sizeof(Header));
    file->read_all(12, sizeof(Header), (char*)fmt_stuff);
    Debug::printf("fmt_chunk_marker: %s\n", fmt_stuff->fmt_chunk_marker);
    Debug::printf("length_of_fmt: %d\n", fmt_stuff->length_of_fmt);
    Debug::printf("format_type: %d\n", fmt_stuff->format_type);
    Debug::printf("channels: %d\n", fmt_stuff->channels);
    Debug::printf("sample_rate: %d\n", fmt_stuff->sample_rate);
    Debug::printf("byte_rate: %d\n", fmt_stuff->byte_rate);
    Debug::printf("bit_channels: %d\n", fmt_stuff->bit_channels);
    Debug::printf("bits_per_sample: %d\n", fmt_stuff->bits_per_sample);

    if(fmt_stuff->sample_rate == 16000) {
        bit_divsor = 0x200; 
    } else {
        bit_divsor = 0x500; 
    }

    if(fmt_stuff->bits_per_sample == 16) {
        bit_per_sample = 0x10; 
    } else {
        bit_per_sample = 0; 
    }

    // Junk Check 
    char* junk = new char[5];
    junk[4] = '\0';
    file->read_all(12 + sizeof(Header), 4, junk);
    Debug::printf("Is it LIST: %s\n", junk);
    delete junk; 

    // Size of junk
    char * size_of_junk = new char[5];
    file->read_all(12 + sizeof(Header) + 4, 4, size_of_junk);
    size_of_the_junk = 12 + sizeof(Header) + 4 + *(uint32_t *) size_of_junk + 4;
    Debug::printf("Size of List: %d\n", size_of_junk);
    delete size_of_junk;

    // Data Check 
    char * data_chunk_header = new char[5];               // DATA string or FLLR string
    data_chunk_header[4] = '\0';
    file->read_all(size_of_the_junk, 4, data_chunk_header);
    Debug::printf("Is it Data: %s\n", data_chunk_header);
    size_of_the_junk += 4; 
    delete data_chunk_header; 
                
    // NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
    char * data_size = new char[5];
    file->read_all(size_of_the_junk, 4, data_size);
    Debug::printf("Size Of junk: %d\n", *(uint32_t*)data_size);
    size_of_the_junk += 4;
    delete data_size;

    // Make 16 Pages of data 

    for(int i = 0; i < 16; i++) {
        char * current_entry = (b_entries + (i * 16));
        *(uint64_t *) current_entry = PhysMem::alloc_frame();
       
        uint64_t current_addy = *(uint64_t *) current_entry; 

        ASSERT(current_addy == *(uint64_t *) current_entry);



        *(uint32_t *) (current_entry + 8) = 4096; 
        *(uint32_t *) (current_entry + 12) = 0;
        file->read_all(size_of_the_junk+ (4096 * i), 4096, (char*) (uint64_t*)current_addy);
        offset = size_of_the_junk + (4096 * i) + 4096; 
        Debug::printf("Offset: %d\n", offset);
    }

    char * first_few = new char[10];
    file->read_all(size_of_the_junk, 8, (char*) (uint32_t*)first_few);
    Debug::printf("First few bytes from the wav file: %x\n", *(uint32_t *)first_few);
    Debug::printf("First few bytes from the wav file ~ 2: %x\n", *((uint32_t *)first_few + 1));
    delete first_few; 

    for(int x = 0; x < 100; x++) {
        char * current_entry = (b_entries);
        Debug::printf("%x\n", *(((uint32_t *)(*((uint64_t *) current_entry))) + x));
    }


    // Set the Addy 

    // Set Registers with the right values (, bit_channels, )

    // make RUN into 1 

}

    void rebuildData(uint32_t index) {

        char * current_entry = (b_entries + (index * 16));
        uint64_t current_addy = *(uint64_t *) current_entry; 

        char * first_few = new char[10];
        overallFile->read_all(offset, 8, (char*) (uint32_t*)first_few);
        Debug::printf("First few bytes from the wav file: %x\n", *(uint32_t *)first_few);
        Debug::printf("First few bytes from the wav file ~ 2: %x\n", *((uint32_t *)first_few + 1));
        delete first_few; 

        overallFile->read_all(offset, 4096, (char*) (uint64_t*)current_addy);
        offset+=4096; 

        for(int x = 0; x < 5; x++) {
            char * current_entry = (b_entries + (index * 16));
            Debug::printf("%x\n", *(((uint32_t *)(*((uint64_t *) current_entry))) + x));
        }

    }

};


