/* k_xmlbase.c
 * Functions for downloading and parsing KINO API .xml files
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

#include "k_win32gui.h"

/* Macro error messages for get_url_thrd_func() function */
#define THREAD_LOTTERY_DAT_NULL  0x0002
#define PROGRAM_LOTTERY_DAT_NULL 0x0003
#define DOWNLOAD_XML_FAILED      0x0004
#define PARSE_LOTTERY_XML_FAILED 0x0005
#define UNLINK_FAILED            0x0006
#define GET_URL_THREAD_SUCCESS   0x0000

extern HANDLE ProgressBarEv; //from k_win32gui_main.c


typedef struct thread_data {
    HWND prog_bar; //the window handle of the progress bar
    char url_buf[255];
    char fname_buf[255];
    lottery_data **lottery_dat;
    bool delete_file;
} thread_data;


/* Local functions */
static UINT CALLBACK get_xml_thrd_func(LPVOID void_dat);
static VOID wait_until_threads_finish(uint32_t up_to, HANDLE *thrd_arr, thread_data *thrd_dat);
static bool download_KINO_lotteriesxml_by_date(const char *drawDate);


bool download_xml(const char *url_tofetch, const char *fname_tosave)
{
    if (!url_tofetch || !fname_tosave) {
        _set_errno(EINVAL);
        P_ERRNO();
        return false;
    }

    if (k_access(fname_tosave, F_OK) != -1) { //return if file already exists
        //_set_errno(EEXIST);
        //P_ERR_MSG("file already exists", fname_tosave);
        return true;
    }

    bool retvalue = false;


    _set_errno(0);
    FILE *file = fopen(fname_tosave, "w");

    if (file) {
        CURL *curl = curl_easy_init();

        if (curl) {
            char errbuf[CURL_ERROR_SIZE];
            CURLcode res;

            curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
            errbuf[0] = '\0';

            curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
            curl_easy_setopt(curl, CURLOPT_URL, url_tofetch);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

            if ((res = curl_easy_perform(curl)) != CURLE_OK) {
                size_t len = strlen(errbuf);

                if (len) {
                    if (errbuf[len - 1] == '\n') errbuf[len - 1] = '\0';
                    P_ERR_MSG(errbuf, NULL);
                } else {
                    P_ERR_MSG(curl_easy_strerror(res), url_tofetch);
                }

                k_unlink(fname_tosave);
            } else {
                retvalue = true;
            }

            curl_easy_cleanup(curl);
        } else {
            P_ERR_MSG("curl_easy_init() failed!", NULL);
        }

        (void)fclose(file);

    } else {
        P_ERRNO();
    }

    return retvalue;
}

bool parse_lottery_xml(const char *fname_toparse, lottery_data *data_tosave)
{
    bool retvalue = false;
    xmlDocPtr doc = NULL;

    if (!fname_toparse || !data_tosave) {
        _set_errno(EINVAL);
        P_ERRNO();
        goto RET;
    }

    if (!(doc = xmlReadFile(fname_toparse, NULL, XML_PARSE_NOWARNING | XML_PARSE_NOERROR))) {
        P_ERR_MSG("xmlReadFile() failed to parse file", fname_toparse);
        goto RET;
    }

    xmlNodePtr curr;

    if (!(curr = xmlDocGetRootElement(doc))) {
        P_ERR_MSG("xmlDocGetRootElement() empty document", fname_toparse);
        goto RET;
    }

    if (xmlStrcmp(curr->name, (const xmlChar*)"draw")) {
        P_ERR_MSG("Wrong type document", fname_toparse);
        goto RET;
    }

    xmlChar *key = NULL;
    int j = 0;
    for (curr = curr->xmlChildrenNode; curr; curr = curr->next) {
        if (!xmlStrcmp(curr->name, (const xmlChar*)"drawNo")) {
            key = xmlNodeListGetString(doc, curr->xmlChildrenNode, 1);
            data_tosave->lottery_num = (uint32_t)atoi((const char*)key);
            xmlFree(key);
            continue;
        }

        if (!xmlStrcmp(curr->name, (const xmlChar*)"result")) {
            key = xmlNodeListGetString(doc, curr->xmlChildrenNode, 1);
            data_tosave->results[j++] = (uint32_t)atoi((const char*)key);
            xmlFree(key);
        }
    }
    retvalue = true;

RET:
    if (doc) xmlFreeDoc(doc);

    return retvalue;
}

UINT CALLBACK get_xml_thrd_func(LPVOID void_dat)
{
    thread_data *dat = (thread_data*)void_dat;
    uint8_t max_download_retries = 5, curr_try = 1;
    //boolean flag, indicating success in downloading the file and parsing it
    bool download_success = false;

    if (!dat || !dat->lottery_dat)
        _endthreadex(THREAD_LOTTERY_DAT_NULL);

    if (!(*dat->lottery_dat))
        _endthreadex(PROGRAM_LOTTERY_DAT_NULL);

    //if the download failed for some reason, we retry until we get it right
    //for a maximum of max_download_retries times
    while ((download_success != true) && (curr_try <= max_download_retries)) {

        //first we try to download the file from the dat->url_buf URL
        if (download_xml(dat->url_buf, dat->fname_buf) == false) {
            (*(*dat->lottery_dat)).valid_data = download_success;
            _endthreadex(DOWNLOAD_XML_FAILED);
        }

        //now we try to parse it. If parsing was successful, we set the success flag to be true
        if (parse_lottery_xml(dat->fname_buf, *dat->lottery_dat) == true)
            download_success = true;

        //If parsing failed we delete the file in order to retry again. We also delete the
        //file if parsing was successful and the DELETE_FILES flag was set on the download_KINO_lotteries() call
        if (dat->delete_file || (download_success != true)) {
            if (k_unlink(dat->fname_buf) == -1) {
                (*(*dat->lottery_dat)).valid_data = download_success;
                _endthreadex(UNLINK_FAILED);
            }
        }
        curr_try++;
    }
    (*(*dat->lottery_dat)).valid_data = download_success;

    process_lotteries(*dat->lottery_dat, 0);

    SendNotifyMessage(dat->prog_bar, PBM_STEPIT, 0, 0);

    if (download_success != true)
        _endthreadex(PARSE_LOTTERY_XML_FAILED);
    else
        _endthreadex(GET_URL_THREAD_SUCCESS);
}

VOID wait_until_threads_finish(uint32_t up_to, HANDLE *thrd_arr, thread_data *thrd_dat)
{
    //to get the return value of each thread from GetExitCodeThread()
    DWORD thrd_retvalue;

    for (uint32_t thread_to_wait = 0; thread_to_wait < up_to; thread_to_wait++) {
        if (WaitForSingleObject(thrd_arr[thread_to_wait], INFINITE) == WAIT_FAILED) {
            MSGBOX_ERR(NULL, L"WaitForSingleObject");
            exit(EXIT_FAILURE);
        }

        if (!GetExitCodeThread(thrd_arr[thread_to_wait], &thrd_retvalue)) {
            MSGBOX_ERR(NULL, L"GetExitCodeThread");
            exit(EXIT_FAILURE);
        }

        CloseHandle(thrd_arr[thread_to_wait]);

        switch (thrd_retvalue) {
        case THREAD_LOTTERY_DAT_NULL:
            P_ERR_INT("(!dat || !dat->lottery_dat) assertion failed in thread", (int)thread_to_wait);
            break;
        case PROGRAM_LOTTERY_DAT_NULL:
            P_ERR_INT("(!(*dat->lottery_dat)) assertion failed in thread", (int)thread_to_wait);
            break;
        case DOWNLOAD_XML_FAILED:
            P_ERR_INT("download_xml() failed in thread", (int)thread_to_wait);
            break;
        case PARSE_LOTTERY_XML_FAILED:
            P_ERR_INT("parse_lottery_xml() failed in thread", (int)thread_to_wait);
            break;
        case UNLINK_FAILED:
            P_ERR_INT("k_unlink() failed in thread", (int)thread_to_wait);
            break;
        default:
            break;
        }

        if (thrd_dat[thread_to_wait].lottery_dat) k_free(thrd_dat[thread_to_wait].lottery_dat);
    }
}

bool download_KINO_lotteries(uint32_t min, uint32_t max, lottery_data **dat, HWND hProgressBar,
                             uint8_t max_threads, uint8_t flag)
{
    bool retvalue = false;

    set_group_func(default_lottery_ticket_groups());

    if (*dat) {
        _set_errno(EINVAL);
        MSGBOX_ERRNO(NULL, L"download_KINO_lotteries");
        goto RET;
    }

    if (min <= max) {
        // calculate total number of lottery results to be downloaded
        size_t total_lotteries = max - min + 1;

        bool del_file = (BIT_IS_ENABLED(flag, 1)) ? true : false;
        uint8_t download_type = BIT_IS_ENABLED(flag, 0);

        //if the total lottery results to be downloaded are less than 2
        //we don't need to have the overhead of concurrency, so we default to sequential
        if (total_lotteries <= 2) download_type = SEQUENTIAL_DOWNLOADS;

        *dat = k_malloc(sizeof(lottery_data) * (total_lotteries));
        if (!dat) {
            MSGBOX_ERR(NULL, L"k_malloc");
            goto RET;
        }

        switch (download_type) {

        /* The parallel download system works like this: There's a maximum allowable number
         * of threads (max_threads) that can be run at once. If that number is reached
         * then we wait until all the threads, that are currently running, are finished doing their jobs
         * before we can run more threads. At any point in time, there can't be more than
         * max_threads threads running. */
        case PARALLEL_DOWNLOADS:
            {
                //arguments to pass to each thread
                thread_data *thrd_dat = k_malloc(sizeof(thread_data) * max_threads);
                if (!thrd_dat) {
                    MSGBOX_ERR(NULL, L"k_malloc");
                    goto RET;
                }

                //HANDLE array to store the identifiers of max_threads threads
                HANDLE *hThrdArr = k_malloc(sizeof(HANDLE) * max_threads);
                if (!hThrdArr) {
                    MSGBOX_ERR(NULL, L"k_malloc");
                    goto RET;
                }

                //0-based index for hThrdArr array to count how many threads are currently running and
                //which thread we're at in the array
                uint32_t current_thread_idx = 0;

                for (uint32_t curr_lottery = min; 
                     (curr_lottery <= max) && (WaitForSingleObject(ProgressBarEv, 0) == WAIT_TIMEOUT);
                     curr_lottery++) {

                    /* Ιf we reached the maximum running thread limit we wait until all threads
                     * are finished, before we start creating any more */
                    if (current_thread_idx >= max_threads) {
                        wait_until_threads_finish(current_thread_idx, hThrdArr, thrd_dat);
                        current_thread_idx = 0;

                        k_free(thrd_dat);

                        thrd_dat = k_malloc(sizeof(thread_data) * max_threads);
                        if (!thrd_dat) {
                            MSGBOX_ERR(NULL, L"k_malloc");
                            break;
                        }
                    }

                    thrd_dat[current_thread_idx].delete_file = del_file;
                    thrd_dat[current_thread_idx].prog_bar = hProgressBar;
                    thrd_dat[current_thread_idx].lottery_dat = k_malloc(sizeof(lottery_data*));
                    assert(thrd_dat[current_thread_idx].lottery_dat);

                    *thrd_dat[current_thread_idx].lottery_dat = (*dat + (curr_lottery - min));

                    snprintf(thrd_dat[current_thread_idx].url_buf, 255, WEB_SERVICE_KINO_LOTTERY_URL"%u.xml", (unsigned int)curr_lottery);
                    snprintf(thrd_dat[current_thread_idx].fname_buf, 255, XML_FOLDER"/"XML_FNAME_PREFIX"%u.xml", (unsigned int)curr_lottery);


                    _set_errno(0);
                    hThrdArr[current_thread_idx] = (HANDLE)_beginthreadex(NULL, 0, get_xml_thrd_func, (LPVOID)&thrd_dat[current_thread_idx], 0, NULL);
                    if (!hThrdArr[current_thread_idx]) {
                        MSGBOX_ERRNO(NULL, L"_beginthreadex");
                        break;
                    }

                    current_thread_idx++;
                }

                /* cleanup the rest of remaining threads if there are any, to fit the structured concurrency model */
                wait_until_threads_finish(current_thread_idx, hThrdArr, thrd_dat);
                k_free(hThrdArr);
                k_free(thrd_dat);
            }
            retvalue = true;
            break;
        case SEQUENTIAL_DOWNLOADS:
            for (uint32_t i = min; i <= max; i++) {
                if (!download_single_KINO_lottery(i, (*dat + (i - min)), del_file)) {
                    P_ERR_INT("download_single_KINO_lottery() failed!", i);
                    goto RET;
                }
                process_lotteries(*dat + (i - min), 0);

                SendNotifyMessage(hProgressBar, PBM_STEPIT, 0, 0);
            }
            retvalue = true;
            break;
        default:
            P_ERR_MSG("wrong flag entered!", NULL);
            break;
        }
    } else {
        P_ERR_MSG("min is bigger than max!", NULL);
    }

RET:
    return retvalue;
}

uint32_t get_last_lottery_number(void)
{
    uint32_t retvalue = 0;
    lottery_data *tmp = k_malloc(sizeof(lottery_data));

    if (!tmp) {
        P_ERR_MSG("k_malloc() failed!", NULL);
        goto RET;
    }

    char url_buf[255], fname_buf[255];

    snprintf(url_buf, 255, WEB_SERVICE_KINO_LOTTERY_URL"last.xml");
    snprintf(fname_buf, 255, XML_FOLDER"/"XML_FNAME_PREFIX"last.xml");

    _set_errno(0);
    if (k_unlink(fname_buf) == -1) {
        P_ERRNO();
    }

    if (!download_xml(url_buf, fname_buf)) {
        P_ERR_MSG("download_xml() failed!", NULL);
        goto RET;
    }

    if (!parse_lottery_xml(fname_buf, tmp)) {
        P_ERR_MSG("parse_lottery_xml() failed!", NULL);
        goto RET;
    }

    _set_errno(0);
    if (k_unlink(fname_buf) == -1) {
        P_ERRNO();
    }

    retvalue = tmp->lottery_num;
RET:
    if (tmp) k_free(tmp);
    return retvalue;
}


bool download_single_KINO_lottery(uint32_t lottery_num, lottery_data *dat, bool del_file)
{
    int max_download_retries = 5;
    bool download_success = false;
    char url_buf[255], fname_buf[255];

    snprintf(url_buf, 255, WEB_SERVICE_KINO_LOTTERY_URL"%u.xml", (unsigned int)lottery_num);
    snprintf(fname_buf, 255, XML_FOLDER"/"XML_FNAME_PREFIX"%u.xml", (unsigned int)lottery_num);

    for (int i = 0; (i < max_download_retries) && !download_success; i++) {
        if (!download_xml(url_buf, fname_buf)) {
            _set_errno(0);
            if (k_unlink(fname_buf) == -1) {
                MSGBOX_ERRNO(NULL, L"k_unlink");
                dat->valid_data = false;
                return false;
            }
            continue;
        }


        if (!parse_lottery_xml(fname_buf, dat)) {
            _set_errno(0);
            if (k_unlink(fname_buf) == -1) {
                MSGBOX_ERRNO(NULL, L"k_unlink");
                return false;
            }
            continue;
        } else
            download_success = true;
    }

    _set_errno(0);
    if (del_file != false) {
        if (k_unlink(fname_buf) == -1) {
            MSGBOX_ERRNO(NULL, L"k_unlink");
        }
    }

    dat->valid_data = download_success;
    return download_success;
}

bool download_KINO_lotteriesxml_by_date(const char *drawDate)
{
    int max_download_retries = 5;
    bool download_success = false;
    char url_buf[255], fname_buf[255];

    snprintf(url_buf, 255, WEB_SERVICE_KINO_LOTTERY_URL"drawDate/%s.xml", drawDate);
    snprintf(fname_buf, 255, XML_FOLDER"/"XML_FNAME_PREFIX"%s.xml", drawDate);

    _set_errno(0);
    if ((k_unlink(fname_buf) == -1) && (errno != ENOENT)) {
        P_ERRNO();
        return download_success;
    }

    for (int i = 0; (i < max_download_retries) && !download_success; i++) {
        if (!download_xml(url_buf, fname_buf)) {
            _set_errno(0);
            if (k_unlink(fname_buf) == -1) {
                P_ERRNO();
                return false;
            }
            continue;
        } else
            download_success = true;
    }

    return download_success;
}

bool get_KINO_lotterynums_by_date(const char *date, uint32_t lottery_num[2])
{
    if (!date) {
        errno = EINVAL;
        P_ERRNO();
        return false;
    }

    if (!download_KINO_lotteriesxml_by_date(date)) {
        P_ERR_MSG("download_KINO_lotteriesxml_by_date() failed to download lotteries for date", date);
        return false;
    }

    //parse the downloaded xml
    bool retvalue = false;
    xmlDocPtr doc = NULL;
    char fname_buf[255];

    snprintf(fname_buf, 255, XML_FOLDER"/"XML_FNAME_PREFIX"%s.xml", date);

    if (!(doc = xmlParseFile(fname_buf))) {
        P_ERR_MSG("xmlParseFile() failed to parse file", fname_buf);
        goto RET;
    }

    xmlNodePtr curr;

    if (!(curr = xmlDocGetRootElement(doc))) {
        P_ERR_MSG("xmlDocGetRootElement() empty document", fname_buf);
        goto RET;
    }

    if (xmlStrcmp(curr->name, (const xmlChar*)"draws")) {
        P_ERR_MSG("Wrong type document", fname_buf);
        goto RET;
    }

    curr = curr->xmlChildrenNode;

    xmlChar *key = xmlNodeListGetString(doc, curr->xmlChildrenNode->xmlChildrenNode, 1);
    if (!key) {
        P_ERR_MSG("Something happened while parsing", fname_buf);
        goto RET;
    }

    lottery_num[1] = lottery_num[0] = (uint32_t)atoi((const char*)key);

    xmlFree(key);

    
    for (curr = curr->next; curr; curr = curr->next) lottery_num[1]++;

    retvalue = true;

    _set_errno(0);
    if ((k_unlink(fname_buf) == -1) && (errno != ENOENT)) {
        P_ERRNO();
    }

RET:
    if (doc) xmlFreeDoc(doc);

    return retvalue;
}
