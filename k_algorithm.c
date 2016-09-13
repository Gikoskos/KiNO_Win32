/* k_algorithm.c
 * Various algorithms for processing KINO lottery data
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

#include "k_hdr.h"


static void (*make_groups_func)(lottery_data*);

static int compare_int(const void *a, const void *b, void *arg);
static void make_groups_of_tens(lottery_data *dat);
static void make_custom_groups(lottery_data *dat);
static float find_payoff(uint8_t total_winning_numbers, bool magic_number);


int compare_int(const void *a, const void *b, void *arg)
{
    uint8_t *p = (uint8_t*)a, *q = (uint8_t*)b, *c = (uint8_t*)arg;

    if (*p > *q) *c+=1;

    return *p - *q;
}

void make_groups_of_tens(lottery_data *dat)
{
    uint8_t next_group = 10, group_idx = 0, group_num_idx = 0;
    bool mn_exist = false;

    //set uninitialized values to 0
    memset(dat->winning_tens_total, 0, sizeof(dat->winning_tens_total));
    memset(dat->winning_tens_payoff, 0, sizeof(dat->winning_tens_payoff));
    memset(dat->winning_tens, 0, sizeof(dat->winning_tens));
    
    dat->total_payoff = 0;

    for (uint8_t next_result = 0; next_result < 20; next_result++) {

        if (dat->results[next_result] > next_group) {
            dat->winning_tens_total[group_idx] = group_num_idx;
            dat->winning_tens_payoff[group_idx] = find_payoff(dat->winning_tens_total[group_idx], mn_exist);

            dat->total_payoff += dat->winning_tens_payoff[group_idx];

            next_group += 10;
            group_num_idx = 0;
            group_idx++;
            mn_exist = false;
        }

        if (dat->magic_number == dat->results[next_result]) {
            dat->magic_number_group_idx = group_idx;
            mn_exist = true;
        }

        assert(group_idx < 8);
        assert(group_num_idx < 10);

        dat->winning_tens[group_idx][group_num_idx++] = dat->results[next_result];

    }

    dat->winning_tens_total[group_idx] = group_num_idx;
    dat->winning_tens_payoff[group_idx] = find_payoff(dat->winning_tens_total[group_idx], mn_exist);
    dat->total_payoff += dat->winning_tens_payoff[group_idx];
}

void make_custom_groups(lottery_data *dat)
{
    uint8_t ticket_idx[8] = {};

    //set uninitialized values to 0
    memset(dat->winning_tens_total, 0, sizeof(dat->winning_tens_total));
    memset(dat->winning_tens_payoff, 0, sizeof(dat->winning_tens_payoff));
    memset(dat->winning_tens, 0, sizeof(dat->winning_tens));
    memset(dat->magic_number_group, false, sizeof(dat->magic_number_group));
    dat->total_payoff = 0;

    for (uint8_t curr_result = 0; curr_result < 20; curr_result++) {
        for (uint8_t curr_ticket = 0; curr_ticket < 8; curr_ticket++) {
            for (ticket_idx[curr_ticket] = 0;
                 (lottery_tickets[curr_ticket][ticket_idx[curr_ticket]]) &&
                 (ticket_idx[curr_ticket] < 10); ticket_idx[curr_ticket]++) {
                if (dat->results[curr_result] == lottery_tickets[curr_ticket][ticket_idx[curr_ticket]]) {
                    if (dat->magic_number == dat->results[curr_result]) {
                        dat->magic_number_group[curr_ticket] = true;
                    }
                    dat->winning_tens[curr_ticket][dat->winning_tens_total[curr_ticket]] = dat->results[curr_result];
                    dat->winning_tens_total[curr_ticket]++;
                }
            }
        }
    }

    for (uint8_t i = 0; i < 8; i++) {
        dat->winning_tens_payoff[i] = find_payoff(dat->winning_tens_total[i], dat->magic_number_group[i]);
        dat->total_payoff += dat->winning_tens_payoff[i];
    }
}

float find_payoff(uint8_t total_winning_numbers, bool magic_number)
{
    switch (total_winning_numbers) {
    case 0:
        return (magic_number == true) ? 0 : 2.0;
    case 1:
        return (magic_number == true) ? 2.0 : 0;
    case 2:
        return (magic_number == true) ? 2.5 : 0;
    case 3:
        return (magic_number == true) ? 3.0 : 0;
    case 4:
        return (magic_number == true) ? 4.0 : 0;
    case 5:
        return (magic_number == true) ? 9.0 : 2.0;
    case 6:
        return (magic_number == true) ? 80.0 : 20.0;
    case 7:
        return (magic_number == true) ? 260.0 : 80.0;
    case 8:
        return (magic_number == true) ? 2000.0 : 500.0;
    case 9:
        return (magic_number == true) ? 35000.0 : 10000.0;
    case 10:
        return (magic_number == true) ? 350000.0 : 100000.0;
    default:
        errno = EINVAL;
        P_ERRNO();
        break;
    }

    return 0;
}

void set_group_func(bool is_default)
{
    if (is_default) {
        make_groups_func = make_groups_of_tens;
    } else {
        make_groups_func = make_custom_groups;
    }
}

bool process_lotteries(lottery_data *to_process, size_t data_len)
{
    if (!to_process) {
        errno = EINVAL;
        P_ERRNO();
        return false;
    }

    int counter;

    for (size_t i = 0; i <= data_len; i++, to_process++) {
        if (!(*to_process).valid_data) continue;

        counter = 0;
        (*to_process).magic_number = (*to_process).results[19]; //last winning number is KINO bonus
        sort_r((*to_process).results,
               (sizeof((*to_process).results) / sizeof((*to_process).results[0])),
               sizeof((*to_process).results[0]), compare_int, &counter);

        make_groups_func(to_process);
    }

    return true;
}

void reset_lottery_ticket_groups(void)
{
    uint8_t z = 1;
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 10; j++)
            lottery_tickets[i][j] = z++;
}

bool default_lottery_ticket_groups(void)
{
    uint8_t z = 1;

    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 10; j++)
            if (lottery_tickets[i][j] != z++)
                return false;

    return true;
}

void copy_lottery_ticket_groups(uint8_t from[8][10], uint8_t to[8][10])
{
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 10; j++)
            to[i][j] = from[i][j];
}

bool randomize_lottery_ticket_groups(void)
{
    uint8_t num_arr[] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9,
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
        20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
        30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
        40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
        50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
        60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
        70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
        80
    };

    for (uint8_t i = 0; i < 80; i++) {
        UINT rand_idx;

        if (rand_s(&rand_idx)) {
            return false;
        }
        rand_idx = rand_idx%80;

        uint8_t tmp = num_arr[i];
        num_arr[i] = num_arr[rand_idx];
        num_arr[rand_idx] = tmp;
    }

    uint8_t curr_num = 0;
    for (uint8_t i = 0; i < 8; i++) {
        for (uint8_t j = 0; j < 10; j++) {
            lottery_tickets[i][j] = num_arr[curr_num];
            curr_num++;
        }
    }

    return true;
}

void print_lottery_data(lottery_data *to_print, size_t data_len, FILE *to_write, int flag)
{
#ifndef _WIN32
    char C_RED[] __attribute__((unused)) = "\x1B[31m";
    char C_GRN[] __attribute__((unused)) = "\x1B[32m";
    char C_YEL[] __attribute__((unused)) = "\x1B[33m";
    char C_BLU[] __attribute__((unused)) = "\x1B[34m";
    char C_MAG[] __attribute__((unused)) = "\x1B[35m";
    char C_CYN[] __attribute__((unused)) = "\x1B[36m";
    char C_WHT[] __attribute__((unused)) = "\x1B[37m";
    char C_RESET[] __attribute__((unused)) = "\x1B[0m";

    char *eurochar = "€"; //euro character doesn't work on CMD.exe unless you have specific fonts

    if (to_write != stdout)
        C_RED[0] = C_GRN[0] = C_YEL[0] = C_BLU[0] = C_MAG[0] = C_CYN[0] = C_WHT[0] =  C_RESET[0] = 0;
#else //letter coloring only supported on linux consoles for now
    char *C_RED = "";
    char *C_GRN = "";
    char *C_YEL = "";
    char *C_BLU = "";
    char *C_MAG = "";
    char *C_CYN = "";
    char *C_WHT = "";
    char *C_RESET = "";

    char *eurochar = "€";

    (VOID)C_RED;
    (VOID)C_GRN;
    (VOID)C_YEL;
    (VOID)C_BLU;
    (VOID)C_MAG;
    (VOID)C_CYN;
    (VOID)C_WHT;
    (VOID)C_RESET;
#endif

    for (size_t i = 0; i <= data_len; i++, to_print++) {
        if (!(*to_print).valid_data) continue;

        fprintf(to_write, "Lottery %u %s results: ", (unsigned int)(*to_print).lottery_num,
                (flag == P_PROCESSED) ? "processed" : "unprocessed");

        for (int i = 0; i < 20; i++) {
            if (flag == P_PROCESSED && (*to_print).results[i] == (*to_print).magic_number)
                fprintf(to_write, "%s(%s%02u%s)%s ", C_GRN, C_RED, (unsigned int)(*to_print).results[i], C_GRN, C_RESET);
            else
                fprintf(to_write, "%02u ", (unsigned int)(*to_print).results[i]);
        }
        fputc('\n', to_write);

        if (flag == P_PROCESSED) {
            fprintf(to_write, "\t--- Groups of tens ---\n");
            for (size_t j = 0; j < 8; j++) {
                fprintf(to_write, "Group %zu -> Total winning numbers: %s"  "%02u%s" 
                        " with Payoff: %s%01.1f %s%s%s\n",
                        j + 1, C_YEL, (unsigned int)(*to_print).winning_tens_total[j], C_RESET,
                        C_YEL, (*to_print).winning_tens_payoff[j], eurochar, C_RESET,
                        (j == (*to_print).magic_number_group_idx) ? " (KINO bonus is in this group)" : "");
            }
            fprintf(to_write, "Sum of payoffs: %s%02.1f %s%s\n", C_MAG, (*to_print).total_payoff, eurochar, C_RESET);
        }
        fputc('\n', to_write);
    }
}
