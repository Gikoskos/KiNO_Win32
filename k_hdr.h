/* k_hdr.h
 * Main header file for k_xmlbase.c and k_algorithm.c
 * 
 * Copyright (C) George Koskeridis 2016
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
  ****************/

#ifndef __K_HEADER__
#define __K_HEADER__

/* Headers */
#define _XOPEN_SOURCE 700 //for strerror()
#define _GNU_SOURCE     //for qsort_r()
#define _CRT_RAND_S     //for rand_s()

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>

#include <assert.h> //#define NDEBUG to disable

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <curl/curl.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/threads.h>
#include "external/sort_r/sort_r.h"


#include <direct.h>
#include <io.h>
#define k_access(x, y) _access(x, y)
#define k_unlink(x) _unlink(x)
#define k_rmdir(x) _rmdir(x)
#define k_mkdir(x) _mkdir(x)
#undef __CRT__NO_INLINE
#include <strsafe.h> //win32 native string handling

#include <process.h>

#define k_free(x) HeapFree(GetProcessHeap(), 0, x)
#define k_malloc(x) HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, x)
#define k_realloc(x, y) HeapReAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, x, y)
#define k_calloc(x, y) HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, x * y)

#define str(x) #x

#define concat(x, y) x ## y


/* XML URL and file macros */
#define WEB_SERVICE_KINO_LOTTERY_URL "http://applications.opap.gr/DrawsRestServices/kino/"
#define XML_FOLDER "xml"
#define XML_FNAME_PREFIX "kino_lottery"


/* Flags for download_KINO_lotteries() function */
#define SEQUENTIAL_DOWNLOADS         0x00      //0b00000000
#define PARALLEL_DOWNLOADS           0x01      //0b00000001
#define DELETE_FILES                 0x02      //0b00000010
#define GET_LAST_LOTTERY             0x04      //0b00000100

/* Flags for print_lottery_data() function */
#define P_UNPROCESSED                0x00
#define P_PROCESSED                  0x01


/* Debugging macros */
#ifdef __DBG
# define P_ERR_MSG(err_msg1, err_msg2) fprintf(stderr, "== Error at %s():%d ->" \
                                   " %s %s ==\n", __func__, __LINE__, err_msg1,\
                                   (err_msg2) ? err_msg2 : "");

# define P_ERR_INT(err_msg, err_int) fprintf(stderr, "== Error at %s():%d ->" \
                                   " %s %d ==\n", __func__, __LINE__, err_msg, err_int);

# define P_ERRNO() fprintf(stderr, "== Error number %d at %s():%d ->" \
                          " %s ==\n", errno, __func__, __LINE__, strerror(errno));
#else
# define P_ERR_MSG(x, y)
# define P_ERR_INT(x, y)
# define P_ERRNO()
#endif

#define BIT_IS_ENABLED(x, y)   ((x >> y) & 1)


/* Declarations */
typedef struct lottery_data {
    //lottery number
    uint32_t lottery_num;

    //winning numbers from 1-80
    uint8_t results[20];

    //magic number/KINO bonus
    uint8_t magic_number;

    //results[] split in 8 groups of 1-10, 11-20 ... 71-80
    uint8_t winning_tens[8][10];

    //the index of which group the magic number belongs to
    size_t magic_number_group_idx;

    //total numbers in each group (eg. winning_tens_total[0] is the total numbers in winning_tens[0])
    uint8_t winning_tens_total[8];

    //price of each group from winning_tens
    float winning_tens_payoff[8];

    //sum from the payoffs of all groups
    float total_payoff;

    //if true, then the data in this struct is valid, if false then
    //it's invalid and it shouldn't be used anywhere
    bool valid_data;

    //is only used if the lottery tickets are custom and not 1-10, 11-20 etc
    bool magic_number_group[8];
} lottery_data;


extern uint8_t lottery_tickets[8][10];

/* k_xmlbase.c */
bool download_xml(const char *url, const char *fname);
bool parse_lottery_xml(const char *fname_toparse, lottery_data *data_tosave);
bool download_KINO_lotteries(uint32_t min, uint32_t max, lottery_data **dat, HWND hProgressBar,
                             uint8_t max_threads, uint8_t flag);
uint32_t get_last_lottery_number(void);
bool download_single_KINO_lottery(uint32_t lottery_num, lottery_data *dat, bool del_file);
bool get_KINO_lotterynums_by_date(const char *date, uint32_t lottery_num[2]);

/* k_algorithm.c */
void print_lottery_data(lottery_data *to_print, size_t data_len, FILE *to_write, int flag);
bool process_lotteries(lottery_data *to_process, size_t data_len);
void reset_lottery_ticket_groups(void);
bool default_lottery_ticket_groups(void);
void copy_lottery_ticket_groups(uint8_t from[8][10], uint8_t to[8][10]);
bool randomize_lottery_ticket_groups(void);
void set_group_func(bool is_default);

#endif
