#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
//#include <error.h>     //for linux
#include <err.h>     //for mac os x

#include <sys/stat.h>
#include <sys/mman.h>

static const uint64_t pattern_0[] = {
    0xffffffc000680000,
    0xffffffc000680000,
};

static const uint64_t pattern_1[] = {
    0xffffffc000080000,
    0xffffffc000080040,
};

static const uint64_t pattern_2[] = {
    0xffffffc000206000,
    0xffffffc000206000,
    0xffffffc000206000,
};

static const uint64_t pattern_3[] = {
    0xffffffc000081000,
    0xffffffc000081000,
    0xffffffc000081000,
};

static struct {
    size_t size;
    const uint64_t* pattern;
} patterns[] = {
    { sizeof(pattern_0), pattern_0 },
    { sizeof(pattern_1), pattern_1 },
    { sizeof(pattern_2), pattern_2 },
    { sizeof(pattern_3), pattern_3 },
};

static void get_symbol(uint64_t *ptr)
{
    uint64_t *symbol_start, *symbol_end;
    uint64_t num_of_symbol;
    unsigned char *compressed_token_start, *token_start, *ch_ptr;
    int i;
    int token_count, token_ptr;
    char** token;

    /* start of the address */
    symbol_start = ptr;
    ptr += 100;
    while ( *ptr > 0xffffffc000000000 )
        ptr++;
    symbol_end = ptr-1;
    while ( *ptr == 0 )
        ptr++;
    num_of_symbol = *ptr;
    ptr++;
    while ( *ptr == 0 )
        ptr++;

    /* compressed symbol strings */
    compressed_token_start = ch_ptr = (unsigned char*)ptr;
    //while ( ch_ptr[0] != 0 )
        //ch_ptr += (unsigned long)(ch_ptr[0]) + 1;
    for(i=0;i<num_of_symbol;++i){
        ch_ptr += (unsigned long)(ch_ptr[0]) + 1;
    }

    if((uint64_t)ch_ptr%8){
        ch_ptr += (8-(uint64_t)ch_ptr%8);
    }

    ptr=(uint64_t)ch_ptr;

    /* unknown area */
    while ( *ptr == 0 )
        ptr++;
    while ( *ptr < 0x00000000ffffffff )
        ptr++;

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
    printf("compressed_token_start %p: %02x\n",
            compressed_token_start, *compressed_token_start);
    printf("token_start %p: %02x\n",
            token_start, *token_start);
    for ( i=0 ; i<token_count ; i++ )
        printf("token[%d] %s\n", i, token[i]);
#else
    /* parse all together */
    for( ptr = symbol_start, ch_ptr = compressed_token_start ; ptr <= symbol_end ; ptr++ ) {
        char buf[128];
        int len = 0;

        printf("%lx ", *ptr);
        for( i=1 ; i <= (int)(ch_ptr[0]) ; i++ ) {
            if ( ch_ptr[i] < token_count )
                len += snprintf(buf+len, 128, "%s", token[ch_ptr[i]]);
            else
                len += snprintf(buf+len, 128, "%c", ch_ptr[i]);
        }
        printf("%c %s\n", buf[0], &(buf[1]));
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
    uint64_t *mem, *ptr;
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

    if ( (mem = (uint64_t*)mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED ) {
        fprintf(stderr, "mmap failed\n");
        return -4;
    }

    for ( i=0 ; i < sizeof(patterns)/sizeof(patterns[0]) ; i++ ) {
        for( ptr = mem ; ((unsigned long)ptr-(unsigned long)mem) < (len-patterns[i].size) ; ptr++ ) {
            if ( memcmp(patterns[i].pattern, ptr, patterns[i].size) == 0 ) {
                // printf("we got pattern %d\n", i);
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
