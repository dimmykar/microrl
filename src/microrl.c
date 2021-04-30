/**
 * \file            microrl.c
 * \brief           Micro Read Line library
 */

/*
 * Copyright (c) 2011 Eugene SAMOYLOV
 * Copyright (c) 2021 Dmitry KARASEV
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file is part of MicroRL - Micro Read Line library for small and embedded devices.
 *
 * Authors:         Eugene SAMOYLOV aka Helius <ghelius@gmail.com>,
 *                  Dmitry KARASEV <karasevsdmitry@yandex.ru>
 * Version:         1.7.0-dev
 */

/*
 * BUGS and TODO:
 * -- rewrite history for use more than 256 byte buffer
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#if MICRORL_CFG_USE_LIBC_STDIO
#include <stdio.h>
#endif /* MICRORL_CFG_USE_LIBC_STDIO */
#include "microrl.h"

/**
 * \brief           List of ASCII key codes
 */
typedef enum {
    MICRORL_KEY_NUL = 0,                            /*!< ^@ Null character */
    MICRORL_KEY_SOH = 1,                            /*!< ^A Start of heading, = console interrupt */
    MICRORL_KEY_STX = 2,                            /*!< ^B Start of text, maintenance mode on HP console */
    MICRORL_KEY_ETX = 3,                            /*!< ^C End of text */
    MICRORL_KEY_EOT = 4,                            /*!< ^D End of transmission, not the same as ETB */
    MICRORL_KEY_ENQ = 5,                            /*!< ^E Enquiry, goes with ACK; old HP flow control */
    MICRORL_KEY_ACK = 6,                            /*!< ^F Acknowledge, clears ENQ logon hand */
    MICRORL_KEY_BEL = 7,                            /*!< ^G Bell, rings the bell... */
    MICRORL_KEY_BS = 8,                             /*!< ^H Backspace, works on HP terminals/computers */
    MICRORL_KEY_HT = 9,                             /*!< ^I Horizontal tab, move to next tab stop */
    MICRORL_KEY_LF = 10,                            /*!< ^J Line Feed */
    MICRORL_KEY_VT = 11,                            /*!< ^K Vertical tab */
    MICRORL_KEY_FF = 12,                            /*!< ^L Form Feed, page eject */
    MICRORL_KEY_CR = 13,                            /*!< ^M Carriage Return*/
    MICRORL_KEY_SO = 14,                            /*!< ^N Shift Out, alternate character set */
    MICRORL_KEY_SI = 15,                            /*!< ^O Shift In, resume defaultn character set */
    MICRORL_KEY_DLE = 16,                           /*!< ^P Data link escape */
    MICRORL_KEY_DC1 = 17,                           /*!< ^Q XON, with XOFF to pause listings; "okay to send". */
    MICRORL_KEY_DC2 = 18,                           /*!< ^R Device control 2, block-mode flow control */
    MICRORL_KEY_DC3 = 19,                           /*!< ^S XOFF, with XON is TERM=18 flow control */
    MICRORL_KEY_DC4 = 20,                           /*!< ^T Device control 4 */
    MICRORL_KEY_NAK = 21,                           /*!< ^U Negative acknowledge */
    MICRORL_KEY_SYN = 22,                           /*!< ^V Synchronous idle */
    MICRORL_KEY_ETB = 23,                           /*!< ^W End transmission block, not the same as EOT */
    MICRORL_KEY_CAN = 24,                           /*!< ^X Cancel line, MPE echoes !!! */
    MICRORL_KEY_EM = 25,                            /*!< ^Y End of medium, Control-Y interrupt */
    MICRORL_KEY_SUB = 26,                           /*!< ^Z Substitute */
    MICRORL_KEY_ESC = 27,                           /*!< ^[ Escape, next character is not echoed */
    MICRORL_KEY_FS = 28,                            /*!< ^\ File separator */
    MICRORL_KEY_GS = 29,                            /*!< ^] Group separator */
    MICRORL_KEY_RS = 30,                            /*!< ^^ Record separator, block-mode terminator */
    MICRORL_KEY_US = 31,                            /*!< ^_ Unit separator */

    MICRORL_KEY_DEL = 127                           /*!< Delete (not a real control character...) */
} microrl_key_ascii_t;

#define IS_CONTROL_CHAR(x)                  ((x) <= 31)

/**
 * \brief           History ring buffer memory status
 */
typedef enum
{
    MICRORL_HIST_FULL = 0,                          /*!< History ring buffer is full */
    MICRORL_HIST_NOT_FULL                           /*!< History ring buffer is not full or empty */
} microrl_hist_status_t;

/**
 * \brief           Direction of history navigation
 */
typedef enum {
    MICRORL_HIST_DIR_UP = 0,                        /*!< Previous record in history ring buffer */
    MICRORL_HIST_DIR_DOWN                           /*!< Next record in history ring buffer */
} microrl_hist_dir_t;

static char* prompt_default = MICRORL_CFG_PROMPT_STRING;

#if MICRORL_CFG_USE_HISTORY || __DOXYGEN__

#if _HISTORY_DEBUG || __DOXYGEN__
/**
 * \brief           Print history buffer content on screen
 * \param[in]       prbuf: Pointer to \ref microrl_hist_rbuf_t structure
 */
static void print_hist(microrl_hist_rbuf_t* prbuf) {
    printf("\n");
    for (size_t i = 0; i < MICRORL_CFG_RING_HISTORY_LEN; i++) {
        if (i == prbuf->begin) {
            printf("b");
        } else {
            printf(" ");
        }
    }
    printf("\n");
    for (size_t i = 0; i < MICRORL_CFG_RING_HISTORY_LEN; i++) {
        if (isalpha(prbuf->ring_buf[i])) {
            printf("%c", prbuf->ring_buf[i]);
        } else {
            printf("%d", prbuf->ring_buf[i]);
    }
    printf("\n");
    for (size_t i = 0; i < MICRORL_CFG_RING_HISTORY_LEN; i++) {
        if (i == prbuf->end) {
            printf("e");
        } else {
            printf(" ");
        }
    }
    printf("\n");
}
#endif /* _HISTORY_DEBUG || __DOXYGEN__ */

/**
 * \brief           Remove older record from ring buffer
 * \param[in,out]   prbuf: Pointer to \ref microrl_hist_rbuf_t structure
 */
static void hist_erase_older(microrl_hist_rbuf_t* prbuf) {
    int new_pos = prbuf->begin + prbuf->ring_buf[prbuf->begin] + 1;
    if (new_pos >= MICRORL_CFG_RING_HISTORY_LEN) {
        new_pos = new_pos - MICRORL_CFG_RING_HISTORY_LEN;
    }

    prbuf->begin = new_pos;
}

/**
 * \brief           Check space for new line, remove older while not space
 * \param[in]       prbuf: Pointer to \ref microrl_hist_rbuf_t structure
 * \param[in]       len: Length of new line to save in history
 * \return          Member of \ref microrl_hist_status_t enumeration
 */
static microrl_hist_status_t hist_is_space_for_new(microrl_hist_rbuf_t* prbuf, int len) {
    if (prbuf->ring_buf[prbuf->begin] == 0) {
        return MICRORL_HIST_NOT_FULL;
    }
    if (prbuf->end >= prbuf->begin) {
        if ((MICRORL_CFG_RING_HISTORY_LEN - prbuf->end + prbuf->begin - 1) > len) {
            return MICRORL_HIST_NOT_FULL;
        }
    } else {
        if ((prbuf->begin - prbuf->end - 1) > len) {
            return MICRORL_HIST_NOT_FULL;
        }
    }
    return MICRORL_HIST_FULL;
}

/**
 * \brief           Put line to ring buffer
 * \param[in,out]   prbuf: Pointer to \ref microrl_hist_rbuf_t structure
 * \param[in]       line: Record to save in history
 * \param[in]       len: Record length
 */
static void hist_save_line(microrl_hist_rbuf_t* prbuf, char* line, int len) {
    if (len > (MICRORL_CFG_RING_HISTORY_LEN - 2)) {
        return;
    }

    while (hist_is_space_for_new(prbuf, len) == MICRORL_HIST_FULL) {
        hist_erase_older(prbuf);
    }

    // if it's first line
    if (prbuf->ring_buf[prbuf->begin] == 0) {
        prbuf->ring_buf[prbuf->begin] = len;
    }

    // store line
    if (len < (MICRORL_CFG_RING_HISTORY_LEN - prbuf->end - 1)) {
        memcpy(prbuf->ring_buf + prbuf->end + 1, line, len);
    } else {
        int part_len = MICRORL_CFG_RING_HISTORY_LEN - prbuf->end - 1;
        memcpy(prbuf->ring_buf + prbuf->end + 1, line, part_len);
        memcpy(prbuf->ring_buf, line + part_len, len - part_len);
    }

    prbuf->ring_buf[prbuf->end] = len;
    prbuf->end = prbuf->end + len + 1;
    if (prbuf->end >= MICRORL_CFG_RING_HISTORY_LEN) {
        prbuf->end -= MICRORL_CFG_RING_HISTORY_LEN;
    }
    prbuf->ring_buf[prbuf->end] = 0;
    prbuf->cur = 0;
#if _HISTORY_DEBUG
    print_hist(prbuf);
#endif /* _HISTORY_DEBUG */
}

/**
 * \brief           Copy saved line to 'line' and return size of line
 * \param[in]       prbuf: Pointer to \ref microrl_hist_rbuf_t structure
 * \param[out]      line: Line to restore from history
 * \param[in]       dir: Record search direction, member of \ref microrl_hist_dir_t
 * \return          Size of restored line. 0 is returned, if history is empty
 */
static int hist_restore_line(microrl_hist_rbuf_t* prbuf, char* line, microrl_hist_dir_t dir) {
    int cnt = 0;
    // count history record
    int header = prbuf->begin;
    while (prbuf->ring_buf[header] != 0) {
        header += prbuf->ring_buf[header] + 1;
        if (header >= MICRORL_CFG_RING_HISTORY_LEN) {
            header -= MICRORL_CFG_RING_HISTORY_LEN;
        }
        cnt++;
    }

    if (dir == MICRORL_HIST_DIR_UP) {
        if (cnt >= prbuf->cur) {
            int header = prbuf->begin;
            int j = 0;
            // found record for 'prbuf->cur' index
            while ((prbuf->ring_buf[header] != 0) && ((cnt - j - 1) != prbuf->cur)) {
                header += prbuf->ring_buf[header] + 1;
                if (header >= MICRORL_CFG_RING_HISTORY_LEN) {
                    header -= MICRORL_CFG_RING_HISTORY_LEN;
                }
                j++;
            }
            if (prbuf->ring_buf[header]) {
                prbuf->cur++;
                // obtain saved line
                if ((prbuf->ring_buf[header] + header) < MICRORL_CFG_RING_HISTORY_LEN) {
                    memset(line, 0, MICRORL_CFG_CMDLINE_LEN);
                    memcpy(line, prbuf->ring_buf + header + 1, prbuf->ring_buf[header]);
                } else {
                    int part0 = MICRORL_CFG_RING_HISTORY_LEN - header - 1;
                    memset(line, 0, MICRORL_CFG_CMDLINE_LEN);
                    memcpy(line, prbuf->ring_buf + header + 1, part0);
                    memcpy(line + part0, prbuf->ring_buf, prbuf->ring_buf[header] - part0);
                }
                return prbuf->ring_buf[header];
            }
        }
    } else {
        if (prbuf->cur > 0) {
            prbuf->cur--;
            int header = prbuf->begin;
            int j = 0;

            while ((prbuf->ring_buf[header] != 0) && ((cnt - j) != prbuf->cur)) {
                header += prbuf->ring_buf[header] + 1;
                if (header >= MICRORL_CFG_RING_HISTORY_LEN) {
                    header -= MICRORL_CFG_RING_HISTORY_LEN;
                }
                j++;
            }
            if ((prbuf->ring_buf[header] + header) < MICRORL_CFG_RING_HISTORY_LEN) {
                memcpy(line, prbuf->ring_buf + header + 1, prbuf->ring_buf[header]);
            } else {
                int part0 = MICRORL_CFG_RING_HISTORY_LEN - header - 1;
                memcpy(line, prbuf->ring_buf + header + 1, part0);
                memcpy(line + part0, prbuf->ring_buf, prbuf->ring_buf[header] - part0);
            }
            return prbuf->ring_buf[header];
        } else {
            /* empty line */
            return 0;
        }
    }
    return -1;
}
#endif /* MICRORL_CFG_USE_HISTORY || __DOXYGEN__ */








#if MICRORL_CFG_USE_QUOTING || __DOXYGEN__
/**
 * \brief           Restore end quote marks in command line
 * \param[in,out]   mrl: \ref microrl_t working instance
 */
static void restore(microrl_t* mrl) {
    size_t iq;
    for (iq = 0; iq < MICRORL_CFG_QUOTED_TOKEN_NMB; ++iq) {
        if (mrl->quotes[iq].end == 0) {
            break;
        }

        *mrl->quotes[iq].end = *mrl->quotes[iq].begin;
        mrl->quotes[iq].begin = 0;
        mrl->quotes[iq].end = 0;
    }
}
#endif /* MICRORL_CFG_USE_QUOTING || __DOXYGEN__ */

/**
 * \brief           Split command line to tokens array
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       limit: Command line symbols limit
 * \param[in]       tkn_arr: Tokens buffer stored split words
 * \return          Number of split tokens
 */
static int split(microrl_t* mrl, int limit, const char** tkn_arr) {
    int i = 0;
    int ind = 0;
#if MICRORL_CFG_USE_QUOTING
    int iq = 0;
    char quote = 0;
    for (iq = 0; iq < MICRORL_CFG_QUOTED_TOKEN_NMB; ++iq) {
        mrl->quotes[iq].begin = 0;
        mrl->quotes[iq].end = 0;
    }
    iq = 0;
#endif /* MICRORL_CFG_USE_QUOTING */
    while (1) {
        // go to the first whitespace (zero for us)
        while ((mrl->cmdline[ind] == '\0') && (ind < limit)) {
            ind++;
        }

        if (!(ind < limit)) {
            return i;
        }

#if MICRORL_CFG_USE_QUOTING
        if ((mrl->cmdline[ind] == '\'') || (mrl->cmdline[ind] == '"')) {
            if (iq >= MICRORL_CFG_QUOTED_TOKEN_NMB) {
                restore (mrl);
                return -1;
            }
            quote = mrl->cmdline[ind];
            mrl->quotes[iq].begin = mrl->cmdline + ind;
            ind++;
        }
#endif /* MICRORL_CFG_USE_QUOTING */
        tkn_arr[i++] = mrl->cmdline + ind;
        if (i >= MICRORL_CFG_CMD_TOKEN_NMB) {
#if MICRORL_CFG_USE_QUOTING
            restore(mrl);
#endif /* MICRORL_CFG_USE_QUOTING */
            return -1;
        }
        // go to the first NOT whitespace (not zero for us)
        while (ind < limit) {
            if (mrl->cmdline[ind] == '\0') {
#if MICRORL_CFG_USE_QUOTING
                if (quote == '\0') {
#endif /* MICRORL_CFG_USE_QUOTING */
                    break;
#if MICRORL_CFG_USE_QUOTING
                }
                mrl->cmdline[ind] = ' ';
            } else if (mrl->cmdline[ind] == quote) {
                if (mrl->cmdline[ind + 1] != '\0') {
                    restore(mrl);
                    return -1;
                }
                quote = 0;
                mrl->quotes[iq++].end = mrl->cmdline + ind;
                mrl->cmdline[ind++] = '\0';
                break;
#endif /* MICRORL_CFG_USE_QUOTING */
            }
            ind++;
        }
        if (!(ind < limit)) {
#if MICRORL_CFG_USE_QUOTING
            if (quote != '\0') {
                restore(mrl);
                return -1;
            }
#endif /* MICRORL_CFG_USE_QUOTING */
            return i;
        }
    }
    return i;
}

/**
 * \brief           Print default prompt defined in \ref MICRORL_CFG_PROMPT_STRING config
 * \param[in,out]   mrl: \ref microrl_t working instance
 */
inline static void print_prompt(microrl_t* mrl) {
    mrl->print(mrl, mrl->prompt_str);
}

/**
 * \brief           Clear last symbol in command line and move cursor
 *                  to its position
 * \param[in,out]   mrl: \ref microrl_t working instance
 */
inline static void terminal_backspace(microrl_t* mrl) {
    mrl->print(mrl, "\033[D \033[D");
}

/**
 * \brief           Print end line symbol defined in \ref MICRORL_CFG_END_LINE config
 * \param[in,out]   mrl: \ref microrl_t working instance
 */
inline static void terminal_newline(microrl_t* mrl) {
    mrl->print(mrl, MICRORL_CFG_END_LINE);
}

/**
 * \brief           Set cursor at current position + offset (positive or negative)
 *                  in string. The provided string must be at least 7 bytes long
 * \param[in]       str: The original string before moving the cursor
 * \param[in]       offset: Positive or negative interval to move cursor
 * \return          The original string after moving the cursor
 */
static char * generate_move_cursor(char* str, int offset) {
    char c = 'C';
    if (offset > 999) {
        offset = 999;
    }
    if (offset < -999) {
        offset = -999;
    }
    if (offset < 0) {
        offset = -offset;
        c = 'D';
    } else if (offset == 0) {
        *str = '\0';
        return str;
    }

#if MICRORL_CFG_USE_LIBC_STDIO
    str += sprintf(str, "\033[%d%c", offset, c);
#else
    *str++ = '\033';
    *str++ = '[';
    char tmp_str[4] = {0,};
    size_t i = 0;
    size_t j;
    while (offset > 0) {
        tmp_str[i++] = (offset % 10) + '0';
        offset /= 10;
    }
    for (j = 0; j < i; ++j) {
        *str++ = tmp_str[i - j - 1];
    }
    *str++ = c;
    *str = '\0';
#endif /* MICRORL_CFG_USE_LIBC_STDIO */
    return str;
}

/**
 * \brief           Set cursor at current position + offset (positive or negative)
 *                  in terminal's command line
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       offset: Positive or negative interval to move cursor
 */
static void terminal_move_cursor(microrl_t* mrl, int offset) {
    char str[16] = {0,};
    if (offset != 0) {
        generate_move_cursor(str, offset);
        mrl->print(mrl, str);
    }
}

/**
 * \brief           Print command line to screen, replace '\0' to wihitespace
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       pos: Start position from which the line will be printed
 * \param[in]       reset: Reset the cursor position
 */
static void terminal_print_line(microrl_t* mrl, int pos, int reset) {
    if (mrl->echo != MICRORL_ECHO_OFF) {
        char str[MICRORL_CFG_PRINT_BUFFER_LEN];
        char* j = str;

        if (reset) {
#if MICRORL_CFG_USE_CARRIAGE_RETURN
            *j++ = '\r';
            j = generate_move_cursor(j, MICRORL_CFG_PROMPT_LEN + pos);
#else
            j = generate_move_cursor(j, -(MICRORL_CFG_CMDLINE_LEN + MICRORL_CFG_PROMPT_LEN + 2));
            j = generate_move_cursor(j, MICRORL_CFG_PROMPT_LEN + pos);
#endif /* MICRORL_CFG_USE_CARRIAGE_RETURN */
        }

        for (size_t i = pos; i < mrl->cmdlen; i++) {
            *j++ = (mrl->cmdline[i] == '\0') ? ' ' : mrl->cmdline[i];
            if ((j - str) == strlen(str)) {
                *j = '\0';
                mrl->print(mrl, str);
                j = str;
            }
        }

        if ((j - str + 3 + 6 + 1) > MICRORL_CFG_PRINT_BUFFER_LEN) {
            *j = '\0';
            mrl->print(mrl, str);
            j = str;
        }

        *j++ = '\033';   // delete all past end of text
        *j++ = '[';
        *j++ = 'K';
        generate_move_cursor(j, mrl->cursor - mrl->cmdlen);
        mrl->print(mrl, str);
    }
}

/**
 * \brief           Initialize MicroRL library data
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       print: Callback function for character output
 * \return          \ref microrlOK on success, member of \ref microrlr_t otherwise
 */
microrlr_t microrl_init(microrl_t* mrl, microrl_print_fn print) {
    memset(mrl, 0, sizeof(microrl_t));

    mrl->prompt_str = prompt_default;
    mrl->print = print;
#if MICRORL_CFG_ENABLE_INIT_PROMPT
    print_prompt(mrl);
#endif /* MICRORL_CFG_ENABLE_INIT_PROMPT */
    mrl->echo = MICRORL_ECHO_ON;
    mrl->start_password = -1;

    return microrlOK;
}

#if MICRORL_CFG_USE_COMPLETE || __DOXYGEN__
/**
 * \brief           Set pointer to callback complition func, that called when user press 'Tab'
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       get_completion: Auto-complete string callback
 */
void microrl_set_complete_callback(microrl_t* mrl, microrl_get_compl_fn get_completion) {
    mrl->get_completion = get_completion;
}
#endif /* MICRORL_CFG_USE_COMPLETE || __DOXYGEN__ */

/**
 * \brief           Pointer to callback func, that called when user press 'Enter'
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       execute: Command execute callback
 */
void microrl_set_execute_callback(microrl_t* mrl, microrl_exec_fn execute) {
    mrl->execute = execute;
}

#if MICRORL_CFG_USE_CTRL_C || __DOXYGEN__
/**
 * \brief           Set callback for Ctrl+C terminal signal
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       sigint: Ctrl+C terminal signal callback
 */
void microrl_set_sigint_callback(microrl_t* mrl, microrl_sigint_fn sigint) {
    mrl->sigint = sigint;
}
#endif /* MICRORL_CFG_USE_CTRL_C || __DOXYGEN__ */

/**
 * \brief           Set echo mode used to mask user input
 *
 * Use \ref MICRORL_ECHO_ONCE to disable echo for password input, echo mode will enabled after user press Enter.
 * Use \ref MICRORL_ECHO_ON or \ref MICRORL_ECHO_OFF to turn on or off the echo manualy.
 *
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       echo: Member of \ref microrl_echo_t enumeration
 */
void microrl_set_echo(microrl_t* mrl, microrl_echo_t echo) {
    mrl->echo = echo;
}

#if MICRORL_CFG_USE_HISTORY || __DOXYGEN__
/**
 * \brief           Restore record to command line from history buffer
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       dir: Member of \ref microrl_hist_dir_t enumeration
 */
static void hist_search(microrl_t* mrl, microrl_hist_dir_t dir) {
    int len = hist_restore_line(&mrl->ring_hist, mrl->cmdline, dir);
    if (len >= 0) {
        mrl->cmdline[len] = '\0';
        mrl->cursor = mrl->cmdlen = len;
        terminal_print_line(mrl, 0, 1);
    }
}
#endif /* MICRORL_CFG_USE_HISTORY || __DOXYGEN__ */

#if MICRORL_CFG_USE_ESC_SEQ || __DOXYGEN__
/**
 * \brief           Handle escape sequences
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       ch: Input character
 * \return          '1' if escape sequence processed, '0' otherwise
 */
static int escape_process(microrl_t* mrl, char ch) {
    if (ch == '[') {
        mrl->escape_seq = MICRORL_ESC_BRACKET;
        return 0;
    } else if (mrl->escape_seq == MICRORL_ESC_BRACKET) {
        if (ch == 'A') {
#if MICRORL_CFG_USE_HISTORY
            if (mrl->echo == MICRORL_ECHO_ON) {
                hist_search(mrl, MICRORL_HIST_DIR_UP);
            }
#endif /* MICRORL_CFG_USE_HISTORY */
            return 1;
        } else if (ch == 'B') {
#if MICRORL_CFG_USE_HISTORY
            if (mrl->echo == MICRORL_ECHO_ON) {
                hist_search(mrl, MICRORL_HIST_DIR_DOWN);
            }
#endif /* MICRORL_CFG_USE_HISTORY */
            return 1;
        } else if (ch == 'C') {
            if (mrl->cursor < mrl->cmdlen) {
                terminal_move_cursor(mrl, 1);
                mrl->cursor++;
            }
            return 1;
        } else if (ch == 'D') {
            if (mrl->cursor > 0) {
                terminal_move_cursor(mrl, -1);
                mrl->cursor--;
            }
            return 1;
        } else if (ch == '7') {
            mrl->escape_seq = MICRORL_ESC_HOME;
            return 0;
        } else if (ch == '8') {
            mrl->escape_seq = MICRORL_ESC_END;
            return 0;
        }
    } else if (ch == '~') {
        if (mrl->escape_seq == MICRORL_ESC_HOME) {
            terminal_move_cursor(mrl, -mrl->cursor);
            mrl->cursor = 0;
            return 1;
        } else if (mrl->escape_seq == MICRORL_ESC_END) {
            terminal_move_cursor(mrl, mrl->cmdlen - mrl->cursor);
            mrl->cursor = mrl->cmdlen;
            return 1;
        }
    }

    /* unknown escape sequence, stop */
    return 1;
}
#endif /* MICRORL_CFG_USE_ESC_SEQ || __DOXYGEN__ */

/**
 * \brief           Insert len char of text at cursor position
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       text: Record to store in cmdline of \ref microrl_t
 * \param[in]       len: Length of text to store
 * \return          \ref microrlOK on success, \ref microrlERR otherwise
 */
microrlr_t microrl_insert_text(microrl_t* mrl, char* text, int len) {
    if ((mrl->cmdlen + len) < MICRORL_CFG_CMDLINE_LEN) {
        if ((mrl->echo == MICRORL_ECHO_ONCE) & (mrl->start_password == -1)) {
            mrl->start_password = mrl->cmdlen;
        }
        memmove(mrl->cmdline + mrl->cursor + len,
                mrl->cmdline + mrl->cursor,
                mrl->cmdlen - mrl->cursor);
        for (size_t i = 0; i < len; i++) {
            mrl->cmdline[mrl->cursor + i] = text[i];
            if (mrl->cmdline[mrl->cursor + i] == ' ') {
                mrl->cmdline[mrl->cursor + i] = 0;
            }
        }
        mrl->cursor += len;
        mrl->cmdlen += len;
        mrl->cmdline[mrl->cmdlen] = '\0';
        return microrlOK;
    }
    return microrlERR;
}

/**
 * \brief           Remove len chars backwards at cursor
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       len: Number of chars to remove
 */
static void microrl_backspace(microrl_t* mrl, int len) {
    if (mrl->cursor >= len) {
        memmove(mrl->cmdline + mrl->cursor - len,
                mrl->cmdline + mrl->cursor,
                mrl->cmdlen - mrl->cursor + len);
        mrl->cursor -= len;
        mrl->cmdline[mrl->cmdlen] = '\0';
        mrl->cmdlen -= len;
    }
}

/**
 * \brief           Remove one char forward at cursor
 * \param[in,out]   mrl: \ref microrl_t working instance
 */
static void microrl_delete(microrl_t* mrl) {
    if (mrl->cmdlen > 0) {
      memmove(mrl->cmdline + mrl->cursor,
              mrl->cmdline + mrl->cursor + 1,
              mrl->cmdlen - mrl->cursor + 1);
      mrl->cmdline[mrl->cmdlen] = '\0';
      mrl->cmdlen--;
    }
}

#if MICRORL_CFG_USE_COMPLETE || __DOXYGEN__

/**
 * \brief           
 * \param[in]       arr: 
 * \return          
 */
static int common_len(char** arr) {
    size_t i;
    char* shortest = arr[0];
    size_t shortlen = strlen(shortest);

    for (i = 0; arr[i] != NULL; ++i) {
        if (strlen(arr[i]) < shortlen) {
            shortest = arr[i];
            shortlen = strlen(shortest);
        }
    }

    for (i = 0; i < shortlen; ++i) {
        for (size_t j = 0; arr[j] != 0; ++j) {
            if (shortest[i] != arr[j][i]) {
                return i;
            }
        }
    }

    return i;
}

/**
 * \brief           Auto-complete activities to complete input in
 *                  command line
 * \param[in,out]   mrl: \ref microrl_t working instance
 */
static void microrl_get_complite(microrl_t* mrl) {
    const char* tkn_arr[MICRORL_CFG_CMD_TOKEN_NMB];
    char** compl_token;

    if (mrl->get_completion == NULL) { // callback was not set
        return;
    }

    int status = split(mrl, mrl->cursor, tkn_arr);
    if (status < 0) {
        return;
    }

    if (mrl->cmdline[mrl->cursor - 1] == '\0') {
        tkn_arr[status++] = "";
    }
    compl_token = mrl->get_completion(mrl, status, tkn_arr);
#if MICRORL_CFG_USE_QUOTING
    restore(mrl);
#endif /* MICRORL_CFG_USE_QUOTING */
    if (compl_token[0] != NULL) {
        size_t i = 0;
        size_t len;
        int pos = mrl->cursor;

        if (compl_token[1] == NULL) {
            len = strlen(compl_token[0]);
        } else {
            len = common_len(compl_token);
            terminal_newline(mrl);
            while (compl_token[i] != NULL) {
                mrl->print(mrl, compl_token[i]);
                mrl->print(mrl, " ");
                i++;
            }
            terminal_newline(mrl);
            print_prompt(mrl);
            pos = 0;
        }

        if (len != 0) {
            microrl_insert_text(mrl, compl_token[0] + strlen(tkn_arr[status - 1]),
                                len - strlen(tkn_arr[status - 1]));
            if (compl_token[1] == NULL) {
                microrl_insert_text(mrl, " ", 1);
            }
        }
        terminal_print_line(mrl, pos, 0);
    }
}

#endif /* MICRORL_CFG_USE_COMPLETE || __DOXYGEN__ */

/**
 * \brief           Processing input string and calling execute() callback
 * \param[in,out]   mrl: \ref microrl_t working instance
 */
static void new_line_handler(microrl_t* mrl) {
    const char* tkn_arr[MICRORL_CFG_CMD_TOKEN_NMB];
    int status;

    terminal_newline(mrl);
#if MICRORL_CFG_USE_HISTORY
    if ((mrl->cmdlen > 0) && (mrl->echo == MICRORL_ECHO_ON)) {
        hist_save_line(&mrl->ring_hist, mrl->cmdline, mrl->cmdlen);
    }
#endif /* MICRORL_CFG_USE_HISTORY */
    if (mrl->echo == MICRORL_ECHO_ONCE) {
        microrl_set_echo(mrl, MICRORL_ECHO_ON);
        mrl->start_password = -1;
    }
    status = split(mrl, mrl->cmdlen, tkn_arr);
    if (status == -1) {
//        mrl->print(mrl, "ERROR: Max token amount exseed\n");
#if MICRORL_CFG_USE_QUOTING
        mrl->print(mrl, "ERROR:too many tokens or invalid quoting");
#else
        mrl->print(mrl, "ERROR:too many tokens");
#endif /* MICRORL_CFG_USE_QUOTING */
        mrl->print(mrl, MICRORL_CFG_END_LINE);
    }
    if ((status > 0) && (mrl->execute != NULL)) {
        mrl->execute(mrl, status, tkn_arr);
    }
    print_prompt(mrl);
    mrl->cmdlen = 0;
    mrl->cursor = 0;
    memset(mrl->cmdline, 0, MICRORL_CFG_CMDLINE_LEN);
#if MICRORL_CFG_USE_HISTORY
    mrl->ring_hist.cur = 0;
#endif /* MICRORL_CFG_USE_HISTORY */
}

/**
 * \brief           Insert char to command line
 *
 * For example calls in usart RX interrupt
 *
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       ch: Printing to terminal character 
 */
void microrl_insert_char(microrl_t* mrl, int ch) {
#if MICRORL_CFG_USE_ESC_SEQ
    if (mrl->escape != 0) {
        if (escape_process(mrl, ch)) {
            mrl->escape = 0;
        }
    } else {
#endif /* MICRORL_CFG_USE_ESC_SEQ */
        if ((ch == MICRORL_KEY_CR) || (ch == MICRORL_KEY_LF)) {
            // Only trigger a newline if ch doen't follow its companion's
            // triggering a newline.
            if (mrl->last_endl == (ch == MICRORL_KEY_CR ? MICRORL_KEY_LF : MICRORL_KEY_CR)) {
                mrl->last_endl = 0;      // ignore char, but clear newline state
            } else {
                mrl->last_endl = ch;
                new_line_handler(mrl);
            }
            return;
        }
        mrl->last_endl = 0;
        switch (ch) {
            //-----------------------------------------------------
#if MICRORL_CFG_USE_COMPLETE
            case MICRORL_KEY_HT: {
                microrl_get_complite(mrl);
                break;
            }
#endif /* MICRORL_CFG_USE_COMPLETE */
            //-----------------------------------------------------
            case MICRORL_KEY_ESC: {
#if MICRORL_CFG_USE_ESC_SEQ
                mrl->escape = 1;
#endif /* MICRORL_CFG_USE_ESC_SEQ */
                break;
            }
            //-----------------------------------------------------
            case MICRORL_KEY_NAK: { // ^U
                if (mrl->cursor > 0) {
                    microrl_backspace(mrl, mrl->cursor);
                }
                terminal_print_line(mrl, 0, 1);
                break;
            }
            //-----------------------------------------------------
            case MICRORL_KEY_VT: { // ^K
                mrl->print(mrl, "\033[K");
                mrl->cmdlen = mrl->cursor;
                break;
            }
            //-----------------------------------------------------
            case MICRORL_KEY_ENQ: { // ^E
                terminal_move_cursor(mrl, mrl->cmdlen - mrl->cursor);
                mrl->cursor = mrl->cmdlen;
                break;
            }
            //-----------------------------------------------------
            case MICRORL_KEY_SOH: { // ^A
                terminal_move_cursor(mrl, -mrl->cursor);
                mrl->cursor = 0;
                break;
            }
            //-----------------------------------------------------
            case MICRORL_KEY_ACK: { // ^F
                if (mrl->cursor < mrl->cmdlen) {
                    terminal_move_cursor(mrl, 1);
                    mrl->cursor++;
                }
                break;
            }
            //-----------------------------------------------------
            case MICRORL_KEY_STX: { // ^B
                if (mrl->cursor != 0) {
                    terminal_move_cursor(mrl, -1);
                    mrl->cursor--;
                }
                break;
            }
            //-----------------------------------------------------
            case MICRORL_KEY_DLE: { //^P
#if MICRORL_CFG_USE_HISTORY
                hist_search(mrl, MICRORL_HIST_DIR_UP);
#endif /* MICRORL_CFG_USE_HISTORY */
                break;
            }
            //-----------------------------------------------------
            case MICRORL_KEY_SO: { //^N
#if MICRORL_CFG_USE_HISTORY
                hist_search(mrl, MICRORL_HIST_DIR_DOWN);
#endif /* MICRORL_CFG_USE_HISTORY */
                break;
            }
            //-----------------------------------------------------
            case MICRORL_KEY_DEL: // Backspace
            case MICRORL_KEY_BS: { // ^H
                if (mrl->cursor > 0) {
                    microrl_backspace(mrl, 1);
                    if (mrl->cursor == mrl->cmdlen) {
                        terminal_backspace(mrl);
                    } else {
                        terminal_print_line(mrl, mrl->cursor, 1);
                    }
                }
                break;
            }
            //-----------------------------------------------------
            case MICRORL_KEY_EOT: { // ^D
                microrl_delete(mrl);
                terminal_print_line(mrl, mrl->cursor, 0);
                break;
            }
            //-----------------------------------------------------
            case MICRORL_KEY_DC2: { // ^R
                terminal_newline(mrl);
                print_prompt(mrl);
                terminal_print_line(mrl, 0, 0);
                break;
            }
            //-----------------------------------------------------
#if MICRORL_CFG_USE_CTRL_C
            case MICRORL_KEY_ETX: {
                if (mrl->sigint != NULL) {
                    mrl->sigint(mrl);
                }
                break;
            }
#endif /* MICRORL_CFG_USE_CTRL_C */
            //-----------------------------------------------------
            default: {
                if (((ch == ' ') && (mrl->cmdlen == 0)) || IS_CONTROL_CHAR(ch)) {
                    break;
                }
                if (microrl_insert_text(mrl, (char*)&ch, 1) == microrlOK) {
                    if (mrl->cursor == mrl->cmdlen) {
                        char nch [] = {0, 0};
                        if ((mrl->cursor >= mrl->start_password) & (mrl->echo == MICRORL_ECHO_ONCE)) {
                            nch[0] = '*';
                        } else {
                            nch[0] = ch;
                        }
                        mrl->print(mrl, nch);
                    } else {
                        terminal_print_line(mrl, mrl->cursor - 1, 0);
                    }
                }
            }
        }
#if MICRORL_CFG_USE_ESC_SEQ
    }
#endif /* MICRORL_CFG_USE_ESC_SEQ */
}
