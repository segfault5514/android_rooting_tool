#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
//#include <error.h>    //for linux
#include <err.h>    //for mac os x

#include <sys/stat.h>
#include <sys/mman.h>

static const uint32_t pattern_0[] = {
    0x00000000, //  __vectors_start
    0x00001000, // __stubs_start
    0x00001004, // vector_rst
    0x00001020, // vector_irq
    0x000010a0, // vector_dabt
};

//static const uint32_t pattern_0[] = {
//    0xc0008000,
//    0xc0008000,
//    0xc000804c,
//    0xc0008100,
//    0xc000810c,
//};

static const uint32_t pattern_1[] = {
    0xc0008000, // __init_begin
    0xc0008000, // _sinittext
    0xc0008000, // stext
    0xc0008000, // _text
};

static const uint32_t pattern_2[] = {
    0xc0008000, // stext
    0xc0008000, // _text
};

static const uint32_t pattern_3[] = {
    0xc00081c0, // asm_do_IRQ
    0xc00081c0, // _stext
    0xc00081c0, // __exception_text_start
};

static const uint32_t pattern_4[] = {
    0xc0008180, // asm_do_IRQ
    0xc0008180, // _stext
    0xc0008180, // __exception_text_start
};

static const uint32_t pattern_5[] = {
    0xc0100000, // asm_do_IRQ
    0xc0100000, // _stext
    0xc0100000, // __exception_text_start
    0xc0100004, // do_undefinstr
};

static const uint32_t pattern_6[] = {
    0xc0108000, // T stext
    0xc0108000, // T _text
};

static const uint32_t pattern_7[] = {
    0xc4508000,// T stext
    0xc4508000,// T _sinittext
    0xc4508000,// T _stext
    0xc4508000,// T __init_begin
};

static const uint32_t pattern_8[] = {
    0xc0608000, // T stext
    0xc0608000, // T _text
    0xc0608084, // t __create_page_tables
};

static const uint32_t pattern_9[] = {
    0xc0108180, // T asm_do_IRQ
    0xc0108180, // T _stext
    0xc0108180, // T __exception_text_start
};


static const uint32_t pattern_10[] = {
    0x00000000, //__vectors_start
    0x00001000, //__stubs_start
    0x00001004, //vector_rst
    0x00001020, //vector_irq
    0x000010a0, //vector_dabt
    0x00001120, //vector_pabt
    0x000011a0, //vector_und
    0x00001220, //vector_addrexcptn
    0x00001224, //vector_fiq
    0x00001224, //vector_fiq_offset
    0x00001228, //.krait_fixup
    0xc0008000, //stext
    0xc0008000, //_text
};


//static const uint32_t pattern_10[] = {
//    0xc0008000,
//    0xc0008000,
//    0xc000804c,
//    0xc0008100,
//    0xc000810c,
//};

static const uint32_t pattern_11[] = {
    0xc01081c0, //asm_do_IRQ
    0xc01081c0, //_stext
    0xc01081c0, //__exception_text_start
};


static const uint32_t pattern_12[] = {
    0x80008000, //stext
    0x80008000, //_sinittext
    0x80008000, //_stext
    0x80008000, //__init_begin
};

static const uint32_t pattern_13[] = {
    0xc4508000, //stext
    0xc4508000, //_sinittext
    0xc4508000, //_stext
};

static const uint32_t pattern_14[] = {
    0xc0308000, //stext
    0xc0308000, //_text
    0xc0308068, //__create_page_tables
    0xc030806c, //__turn_mmu_on_loc
};

static const uint32_t pattern_15[] = {
    0xc0408000,
    0xc0408000,
    0xc0408080,
    0xc0408160,
    0xc040816c,
    0xc04081d8,
    0xc04081e4,
    0xc04081f0,
    0xc0408220,
    0xc0408280,
};

static const uint32_t pattern_16[] = {
    0xc00081c0,
    0xc00081c0,
};

static const uint32_t pattern_17[] = {
    0xc1200000,
    0xc1200000,
    0xc1201000,
    0xc1201000,
};

static const uint32_t pattern_18[] = {
    0xc0008160,
    0xc0008160,
    0xc0008160,
};

static struct {
    size_t size;
    const uint32_t* pattern;
} patterns[] = {
    { sizeof(pattern_0), pattern_0 },
    { sizeof(pattern_1), pattern_1 },
    { sizeof(pattern_2), pattern_2 },
    { sizeof(pattern_3), pattern_3 },
    { sizeof(pattern_4), pattern_4 },
    { sizeof(pattern_5), pattern_5 },
    { sizeof(pattern_6), pattern_6 },
    { sizeof(pattern_7), pattern_7 },
    { sizeof(pattern_8), pattern_8 },
    { sizeof(pattern_9), pattern_9 },
    { sizeof(pattern_10), pattern_10 },
    { sizeof(pattern_11), pattern_11 },
    { sizeof(pattern_12), pattern_12 },
    { sizeof(pattern_13), pattern_13 },
    { sizeof(pattern_14), pattern_14 },
    { sizeof(pattern_15), pattern_15 },
    { sizeof(pattern_16), pattern_16 },
    { sizeof(pattern_17), pattern_17 },
    { sizeof(pattern_18), pattern_18 },
};

static size_t get_file_length(const char *file_name)
{
  struct stat st;

  if (stat(file_name, &st) < 0) {
    return 0;
  }

  return st.st_size;
}


static void get_symbol(uint32_t *ptr)
{
    uint32_t *symbol_start, *symbol_end;
    uint32_t num_of_symbol;
    unsigned char *compressed_token_start, *token_start, *ch_ptr;
    int i;
    int token_count, token_ptr;
    char** token;

    /* start of the address */
    symbol_start = ptr;
    ptr += 100;
//  while ( *ptr > 0xc0000000 )
    while ( *ptr > 0x80008000 )
        ptr++;
    symbol_end = ptr-1;
    while ( *ptr == 0 )
        ptr++;
    num_of_symbol = *ptr;
//  ptr+=4;
    ptr+=1;

    while ( *ptr == 0 )
        ptr++;

    /* compressed symbol strings */
    compressed_token_start = ch_ptr = (unsigned char*)ptr;
    while ( ch_ptr[0] != 0 )
        ch_ptr += (unsigned long)(ch_ptr[0]) + 1;
    ptr = (uint32_t*)((((unsigned long)ch_ptr+4) >> 2) << 2);

    /* unknown area */
    while ( *ptr == 0 )
        ptr++;
    if ( *ptr > 0x1000000 ){
        while ( *ptr > 0 )
            ptr++;
        while ( *ptr == 0 )
            ptr++;
        while ( *ptr < 0x1000000 ){
            if (*ptr == 0)
                break;
            ptr++;
        }
    }
    else{
//      while ( *ptr < 0x1000000 )
        while ( *ptr < 0x1000000 ){
            if (*ptr == 0)
                break;
            ptr++;
        }
    }
    while ( *ptr == 0 )
        ptr++;
    ptr = (uint32_t*)(((unsigned long)ptr >> 4) << 4); //alignment

    /* token table parse 1 */
    token_start = ch_ptr = (unsigned char*)ptr;
    token_count = 1;
    while ( !(ch_ptr[0] == 0 && ch_ptr[1] == 0) ) {
        if ( ch_ptr[0] == 0 )
            token_count++;
        ch_ptr++;
    }
    token = (char**)malloc(sizeof(char*)*token_count);

    /* token table parse 2 */
    token[0] = (char*)(token_start);
    for( ch_ptr = token_start, token_ptr = 1 ; token_ptr < token_count ; ch_ptr++ ) {
        if ( ch_ptr[0] == 0 )
            token[token_ptr++] = (char*)(ch_ptr+1);
    }

#if 0 //debug
    printf("start %p: %08x\n", symbol_start, *symbol_start);
    printf("end   %p: %08x\n", symbol_end, *symbol_end);
    printf("num_of_symbol %08x\n", num_of_symbol);
    printf("compressed_token_start %p: %08x\n",
            compressed_token_start, *compressed_token_start);
    printf("token_start %p: %08x\n",
            token_start, *token_start);
    for ( i=0 ; i<token_count ; i++ )
        printf("token[%d] %s\n", i, token[i]);
#else
    /* parse all together */
    for( ptr = symbol_start, ch_ptr = compressed_token_start ; ptr <= symbol_end ; ptr++ ) {
        char buf[128];
        int len = 0;

        printf("%08x ", *ptr);
        for( i=1 ; i <= (int)(ch_ptr[0]) ; i++ ) {
            if ( ch_ptr[i] < token_count )
                len += snprintf(buf+len, 128, "%s", token[ch_ptr[i]]);
            else
                len += snprintf(buf+len, 128, "%c", ch_ptr[i]);
        }
        printf("%c %s\n", buf[0], &(buf[1]));
//      printf("%s\n", &(buf[0]));
        ch_ptr += ch_ptr[0]+1;
    }
#endif
    free(token);
}

int main(int argc, char** argv)
{
    struct stat st;
    unsigned long len;
    int fd;
    uint32_t *mem, *ptr;
    int i;

    if ( argc != 2 ) {
        fprintf(stderr, "Usage: %s <kernel binary file>\n", argv[0]);
        return -1;
    }

    if ( stat(argv[1], &st) < 0 ) {
        fprintf(stderr, "stat error for %s\n", argv[1]);
        return -2;
    }
    len = st.st_size;

    if ( (fd = open(argv[1], O_RDONLY)) < 0 ) {
        fprintf(stderr, "can't open kernel binary file\n");
        return -3;
    }

    if ( (mem = (uint32_t*)mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED ) {
        fprintf(stderr, "mmap failed\n");
        return -4;
    }

    for ( i=0 ; i < sizeof(patterns)/sizeof(patterns[0]) ; i++ ) {
        for( ptr = mem ; ((unsigned long)ptr-(unsigned long)mem) < (len-patterns[i].size) ; ptr++ ) {
            if ( memcmp(patterns[i].pattern, ptr, patterns[i].size) == 0 ) {
                //printf("we got pattern %d\n", i);
                get_symbol(ptr);
                goto out;
            }
        }
    }

    printf("[-] pattern not found\n");

out:
    munmap(mem, len);
    close(fd);

    return 0;
}
