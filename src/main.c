#include "scraw.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void print_usage(char const *const arg0)
{
    printf("Usage: %s <file_out> <pcsc_reader_name> <byte_num>\n", arg0);
}

int32_t main(int32_t const argc, char *const argv[argc])
{
    if (argc != 4)
    {
        printf("Wrong number of arguments received=%u expected=%u.\n", argc,
               4U);
        print_usage(argv[0U]);
        return EXIT_FAILURE;
    }

    char const *const arg_file_out = argv[1U];
    char const *const arg_pcsc_reader_name = argv[2U];
    uint64_t const arg_byte_num = strtoull(argv[3U], NULL, 10U);
    printf("Reading %llu bytes from the card.\n", arg_byte_num);

    int32_t main_ret = EXIT_FAILURE;
    int32_t ret;
    scraw_st ctx;
    char *reader_name_selected = NULL;
    int const err_init = scraw_init(&ctx);
    if (err_init == 0)
    {
        int const err_reader_search_begin = scraw_reader_search_begin(&ctx);
        if (err_reader_search_begin == 0)
        {
            bool reader_found = false;
            char const *reader_name_tmp = NULL;
            for (uint32_t reader_idx = 0U;; ++reader_idx)
            {
                ret = scraw_reader_search_next(&ctx, &reader_name_tmp);
                if (ret == 0)
                {
                    printf("Found reader %u: \"%s\".\n", reader_idx,
                           reader_name_tmp);
                    if (reader_found == false)
                    {
                        if (memcmp(arg_pcsc_reader_name, reader_name_tmp,
                                   min(strlen(arg_pcsc_reader_name),
                                       strlen(reader_name_tmp))) != 0)
                        {
                            printf("Skipping reader since \"%s\"!=\"%s\".\n",
                                   arg_pcsc_reader_name, reader_name_tmp);
                            continue;
                        }

                        printf("Selecting reader %u.\n", reader_idx);
                        uint64_t const reader_name_len =
                            strlen(reader_name_tmp) + 1 /* null-terminator */;
                        reader_name_selected = malloc(reader_name_len);
                        if (reader_name_selected == NULL)
                        {
                            printf(
                                "Failed to allocate memory for reader name.\n");
                        }
                        else
                        {
                            memcpy(reader_name_selected, reader_name_tmp,
                                   reader_name_len);
                            if (scraw_reader_select(&ctx,
                                                    reader_name_selected) == 0)
                            {
                                printf("Selected reader %u \"%s\".\n",
                                       reader_idx, ctx.reader_selected.name);
                                reader_found = true;
                                break; /* Selected a reader so can carry on. */
                            }
                        }
                    }
                }
                else if (ret == 1)
                {
                    printf("Reached end of reader list "
                           "scraw_reader_search_next()=%i reason=0x%llx.\n",
                           ret, ctx.err_reason);
                    break;
                }
                else
                {
                    printf("Error while searching reader list "
                           "scraw_reader_search_next()=%i reason=0x%llx.\n",
                           ret, ctx.err_reason);
                    break;
                }
            }
            int const err_reader_search_end = scraw_reader_search_end(&ctx);
            if (err_reader_search_end != 0)
            {
                printf("Failed to end search scraw_reader_search_end()=%i  "
                       "reason=0x%llx.\n",
                       err_reader_search_end, ctx.err_reason);
                /**
                 * This failure leads to a memory leak but is not critical so we
                 * carry on.
                 */
            }
            if (reader_found)
            {
                int const err_connect =
                    scraw_card_connect(&ctx, SCRAW_PROTO_T0);
                if (err_connect == 0)
                {
                    printf("Connected to card scraw_card_connect()=%i.\n",
                           err_connect);

                    uint8_t raw_data[] = {0x00, 0x84, 0x00, 0x00, 0xFF};
                    uint8_t res_data[1024U] = {0};
                    scraw_raw_st raw = {
                        .buf = raw_data,
                        .len = sizeof(raw_data) / sizeof(raw_data[0]),
                    };
                    scraw_res_st res = {
                        .buf = res_data,
                        .buf_len = sizeof(res_data) / sizeof(res_data[0]),
                        .res_len = 0,
                    };

                    FILE *file_rand = fopen(arg_file_out, "a+b");
                    if (file_rand != NULL)
                    {
                        int64_t const time_start = time(NULL);

                        printf("Started timer at time=%lli.\n", time_start);
                        uint64_t res_data_rem = arg_byte_num;
                        for (; res_data_rem > 0;)
                        {
                            uint8_t const res_data_len_exp =
                                res_data_rem > 0xFF ? 0xFF
                                                    : (uint8_t)res_data_rem;
                            raw_data[4] = res_data_len_exp;
                            uint16_t const res_raw_len_exp =
                                res_data_len_exp + 2U;

                            int const err_send = scraw_send(&ctx, &raw, &res);
                            if (err_send == 0)
                            {
                                /* Expected len + SW1 + SW2 (status bytes). */
                                if (res.res_len != res_raw_len_exp)
                                {
                                    printf("Response has invalid length "
                                           "res_len=%u expected=%u.\n",
                                           res.res_len, res_raw_len_exp);
                                    break;
                                }
                                res_data_rem -= res_data_len_exp;

                                int64_t const time_intermediate = time(NULL);
                                printf("Sent data to card scraw_send()=%i "
                                       "apdu=%02X%02X%02X%02X%02X.\n",
                                       err_send, raw_data[0U], raw_data[1U],
                                       raw_data[2U], raw_data[3U],
                                       raw_data[4U]);
                                uint64_t const err_fwrite =
                                    fwrite(res.buf, sizeof(res.buf[0]),
                                           res_data_len_exp, file_rand);
                                if (err_fwrite == res_data_len_exp)
                                {
                                    printf("Response time_elapsed=%lli "
                                           "data_start=%llu data_end=%llu "
                                           "rand=%02X...%02X.\n",
                                           time_intermediate - time_start,
                                           arg_byte_num - (res_data_rem +
                                                           res_data_len_exp),
                                           (arg_byte_num - res_data_rem) - 1,
                                           res.buf[0U],
                                           res.buf[res_data_len_exp - 1U]);

                                    if (res_data_rem == 0)
                                    {
                                        main_ret = EXIT_SUCCESS;
                                    }
                                }
                                else
                                {
                                    printf("Failed to write all bytes to file "
                                           "fwrite()=%llu res_len=%u.\n",
                                           err_fwrite, res.res_len);
                                    break;
                                }
                            }
                            else
                            {
                                printf("Failed to send data to card "
                                       "scraw_send()=%i "
                                       "reason=0x%llx.\n",
                                       err_send, ctx.err_reason);
                                break;
                            }
                        }

                        int64_t const time_end = time(NULL);
                        int64_t const time_tot = time_end - time_start;
                        double const datarate =
                            res_data_rem == arg_byte_num
                                ? 0.0
                                : (double)arg_byte_num /
                                      (time_tot == 0 ? 1.0 : (double)time_tot);
                        if (main_ret == EXIT_SUCCESS)
                        {
                            printf("Received all %llu bytes in %lli sec "
                                   "(rate=%lf B/s).\n",
                                   arg_byte_num, time_tot, datarate);
                        }
                        else
                        {
                            printf("Failed to receive all %llu bytes "
                                   "(transferred %llu bytes) in %lli sec "
                                   "(rate=%lf B/s).\n",
                                   arg_byte_num,
                                   arg_byte_num - (arg_byte_num - res_data_rem),
                                   time_tot, datarate);
                        }

                        int const err_fclose = fclose(file_rand);
                        if (err_fclose != 0)
                        {
                            printf(
                                "Failed to close file fclose()=%i errno=%i.\n",
                                err_fclose, errno);
                            /**
                             * The random bytes might have been received in
                             * full, but this still causes the program itself to
                             * fail.
                             */
                            main_ret = EXIT_FAILURE;
                        }
                    }
                    else
                    {
                        printf("Failed to open file fopen()=%p errno=%i.\n",
                               file_rand, errno);
                    }

                    int const err_disconnect = scraw_card_disconnect(&ctx);
                    if (err_disconnect == 0)
                    {
                        printf("Disconnected from card "
                               "scraw_card_disconnect()=%i.\n",
                               err_disconnect);
                    }
                    else
                    {
                        printf("Failed to disconnect from card "
                               "scraw_card_disconnect()=%i reason=0x%llx.\n",
                               err_disconnect, ctx.err_reason);
                        /**
                         * The random bytes might have been received in full,
                         * but this still causes the program itself to fail.
                         */
                        main_ret = EXIT_FAILURE;
                    }
                }
                else
                {
                    printf("Failed to connect to card scraw_card_connect()=%i "
                           "reason=0x%llx.\n",
                           err_connect, ctx.err_reason);
                }
            }
        }
        else
        {
            printf("Failed to search for readers "
                   "scraw_reader_search_begin()=%i reason=0x%llx.\n",
                   err_reader_search_begin, ctx.err_reason);
        }

        int const err_fini = scraw_fini(&ctx);
        if (err_fini != 0)
        {
            printf("Failed to deinitialize the library scraw_fini()=%i "
                   "reason=0x%llx.\n",
                   err_fini, ctx.err_reason);
            /**
             * The random bytes might have been received in full, but this still
             * causes the program itself to fail.
             */
            main_ret = EXIT_FAILURE;
        }
    }
    else
    {
        printf(
            "Failed to initialize the library scraw_init()=%i reason=0x%llx.\n",
            err_init, ctx.err_reason);
    }

    if (reader_name_selected != NULL)
    {
        free(reader_name_selected);
    }
    return main_ret;
}
