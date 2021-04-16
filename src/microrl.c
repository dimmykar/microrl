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
 * \brief           List of key codes
 */
#define KEY_NUL         0      /*!< ^@ Null character */
#define KEY_SOH         1      /*!< ^A Start of heading, = console interrupt */
#define KEY_STX         2      /*!< ^B Start of text, maintenance mode on HP console */
#define KEY_ETX         3      /*!< ^C End of text */
#define KEY_EOT         4      /*!< ^D End of transmission, not the same as ETB */
#define KEY_ENQ         5      /*!< ^E Enquiry, goes with ACK; old HP flow control */
#define KEY_ACK         6      /*!< ^F Acknowledge, clears ENQ logon hand */
#define KEY_BEL         7      /*!< ^G Bell, rings the bell... */
#define KEY_BS          8      /*!< ^H Backspace, works on HP terminals/computers */
#define KEY_HT          9      /*!< ^I Horizontal tab, move to next tab stop */
#define KEY_LF          10     /*!< ^J Line Feed */
#define KEY_VT          11     /*!< ^K Vertical tab */
#define KEY_FF          12     /*!< ^L Form Feed, page eject */
#define KEY_CR          13     /*!< ^M Carriage Return*/
#define KEY_SO          14     /*!< ^N Shift Out, alternate character set */
#define KEY_SI          15     /*!< ^O Shift In, resume defaultn character set */
#define KEY_DLE         16     /*!< ^P Data link escape */
#define KEY_DC1         17     /*!< ^Q XON, with XOFF to pause listings; "okay to send". */
#define KEY_DC2         18     /*!< ^R Device control 2, block-mode flow control */
#define KEY_DC3         19     /*!< ^S XOFF, with XON is TERM=18 flow control */
#define KEY_DC4         20     /*!< ^T Device control 4 */
#define KEY_NAK         21     /*!< ^U Negative acknowledge */
#define KEY_SYN         22     /*!< ^V Synchronous idle */
#define KEY_ETB         23     /*!< ^W End transmission block, not the same as EOT */
#define KEY_CAN         24     /*!< ^X Cancel line, MPE echoes !!! */
#define KEY_EM          25     /*!< ^Y End of medium, Control-Y interrupt */
#define KEY_SUB         26     /*!< ^Z Substitute */
#define KEY_ESC         27     /*!< ^[ Escape, next character is not echoed */
#define KEY_FS          28     /*!< ^\ File separator */
#define KEY_GS          29     /*!< ^] Group separator */
#define KEY_RS          30     /*!< ^^ Record separator, block-mode terminator */
#define KEY_US          31     /*!< ^_ Unit separator */

#define KEY_DEL         127    /*!< Delete (not a real control character...) */

#define IS_CONTROL_CHAR(x)    ((x) <= 31)


/**
 * \brief           Direction of history navigation
 */
#define _HIST_UP        0
#define _HIST_DOWN      1


/**
 * \brief           ESC seq internal codes
 */
#define _ESC_BRACKET    1
#define _ESC_HOME       2
#define _ESC_END        3

static char* prompt_default = MICRORL_CFG_PROMPT_STRING;

#if MICRORL_CFG_USE_HISTORY || __DOXYGEN__

#if _HISTORY_DEBUG || __DOXYGEN__
/**
 * \brief           Print history buffer content on screen
 * \param[in]       pThis: Pointer to \ref microrl_hist_rbuf_t structure
 */
static void print_hist(microrl_hist_rbuf_t* pThis) {
    printf("\n");
    for (int i = 0; i < MICRORL_CFG_RING_HISTORY_LEN; i++) {
        if (i == pThis->begin) {
            printf("b");
        } else {
            printf(" ");
        }
    }
    printf("\n");
    for (int i = 0; i < MICRORL_CFG_RING_HISTORY_LEN; i++) {
        if (isalpha(pThis->ring_buf[i])) {
            printf("%c", pThis->ring_buf[i]);
        } else {
            printf("%d", pThis->ring_buf[i]);
    }
    printf("\n");
    for (int i = 0; i < MICRORL_CFG_RING_HISTORY_LEN; i++) {
        if (i == pThis->end) {
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
 * \param[in,out]   pThis: Pointer to \ref microrl_hist_rbuf_t structure
 */
static void hist_erase_older(microrl_hist_rbuf_t* pThis) {
    int new_pos = pThis->begin + pThis->ring_buf[pThis->begin] + 1;
    if (new_pos >= MICRORL_CFG_RING_HISTORY_LEN) {
        new_pos = new_pos - MICRORL_CFG_RING_HISTORY_LEN;
    }

    pThis->begin = new_pos;
}

/**
 * \brief           Check space for new line, remove older while not space
 * \param[in]       pThis: Pointer to \ref microrl_hist_rbuf_t structure
 * \param[in]       len: 
 * \return          
 */
static int hist_is_space_for_new(microrl_hist_rbuf_t* pThis, int len) {
    if (pThis->ring_buf[pThis->begin] == 0) {
        return true;
    }
    if (pThis->end >= pThis->begin) {
        if ((MICRORL_CFG_RING_HISTORY_LEN - pThis->end + pThis->begin - 1) > len) {
            return true;
        }
    } else {
        if ((pThis->begin - pThis->end - 1) > len) {
            return true;
        }
    }
    return false;
}

/**
 * \brief           Put line to ring buffer
 * \param[in,out]   pThis: Pointer to \ref microrl_hist_rbuf_t structure
 * \param[in]       line: Record to save in history
 * \param[in]       len: Record length
 */
static void hist_save_line(microrl_hist_rbuf_t* pThis, char* line, int len) {
    if (len > (MICRORL_CFG_RING_HISTORY_LEN - 2)) {
        return;
    }

    while (!hist_is_space_for_new(pThis, len)) {
        hist_erase_older(pThis);
    }

    // if it's first line
    if (pThis->ring_buf[pThis->begin] == 0) {
        pThis->ring_buf[pThis->begin] = len;
    }

    // store line
    if (len < (MICRORL_CFG_RING_HISTORY_LEN - pThis->end - 1)) {
        memcpy(pThis->ring_buf + pThis->end + 1, line, len);
    } else {
        int part_len = MICRORL_CFG_RING_HISTORY_LEN - pThis->end - 1;
        memcpy(pThis->ring_buf + pThis->end + 1, line, part_len);
        memcpy(pThis->ring_buf, line + part_len, len - part_len);
    }

    pThis->ring_buf[pThis->end] = len;
    pThis->end = pThis->end + len + 1;
    if (pThis->end >= MICRORL_CFG_RING_HISTORY_LEN) {
        pThis->end -= MICRORL_CFG_RING_HISTORY_LEN;
    }
    pThis->ring_buf[pThis->end] = 0;
    pThis->cur = 0;
#if _HISTORY_DEBUG
    print_hist(pThis);
#endif /* _HISTORY_DEBUG */
}

/**
 * \brief           Copy saved line to 'line' and return size of line
 * \param[in]       pThis: Pointer to \ref microrl_hist_rbuf_t structure
 * \param[out]      line: Restored line from history
 * \param[in]       dir: Record search direction
 * \return          Size of restored line. 0 is returned, if history is empty
 */
static int hist_restore_line(microrl_hist_rbuf_t* pThis, char* line, int dir) {
    int cnt = 0;
    // count history record
    int header = pThis->begin;
    while (pThis->ring_buf[header] != 0) {
        header += pThis->ring_buf[header] + 1;
        if (header >= MICRORL_CFG_RING_HISTORY_LEN) {
            header -= MICRORL_CFG_RING_HISTORY_LEN;
        }
        cnt++;
    }

    if (dir == _HIST_UP) {
        if (cnt >= pThis->cur) {
            int header = pThis->begin;
            int j = 0;
            // found record for 'pThis->cur' index
            while ((pThis->ring_buf[header] != 0) && ((cnt - j - 1) != pThis->cur)) {
                header += pThis->ring_buf[header] + 1;
                if (header >= MICRORL_CFG_RING_HISTORY_LEN) {
                    header -= MICRORL_CFG_RING_HISTORY_LEN;
                }
                j++;
            }
            if (pThis->ring_buf[header]) {
                pThis->cur++;
                // obtain saved line
                if ((pThis->ring_buf[header] + header) < MICRORL_CFG_RING_HISTORY_LEN) {
                    memset(line, 0, MICRORL_CFG_CMDLINE_LEN);
                    memcpy(line, pThis->ring_buf + header + 1, pThis->ring_buf[header]);
                } else {
                    int part0 = MICRORL_CFG_RING_HISTORY_LEN - header - 1;
                    memset(line, 0, MICRORL_CFG_CMDLINE_LEN);
                    memcpy(line, pThis->ring_buf + header + 1, part0);
                    memcpy(line + part0, pThis->ring_buf, pThis->ring_buf[header] - part0);
                }
                return pThis->ring_buf[header];
            }
        }
    } else {
        if (pThis->cur > 0) {
            pThis->cur--;
            int header = pThis->begin;
            int j = 0;

            while ((pThis->ring_buf[header] != 0) && ((cnt - j) != pThis->cur)) {
                header += pThis->ring_buf[header] + 1;
                if (header >= MICRORL_CFG_RING_HISTORY_LEN) {
                    header -= MICRORL_CFG_RING_HISTORY_LEN;
                }
                j++;
            }
            if ((pThis->ring_buf[header] + header) < MICRORL_CFG_RING_HISTORY_LEN) {
                memcpy(line, pThis->ring_buf + header + 1, pThis->ring_buf[header]);
            } else {
                int part0 = MICRORL_CFG_RING_HISTORY_LEN - header - 1;
                memcpy(line, pThis->ring_buf + header + 1, part0);
                memcpy(line + part0, pThis->ring_buf, pThis->ring_buf[header] - part0);
            }
            return pThis->ring_buf[header];
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
 * \param[in,out]   pThis: \ref microrl_t working instance
 */
static void restore(microrl_t* pThis) {
    int iq;
    for (iq = 0; iq < MICRORL_CFG_QUOTED_TOKEN_NMB; ++iq) {
        if (pThis->quotes[iq].end == 0) {
            break;
        }

        *pThis->quotes[iq].end = *pThis->quotes[iq].begin;
        pThis->quotes[iq].begin = 0;
        pThis->quotes[iq].end = 0;
    }
}
#endif /* MICRORL_CFG_USE_QUOTING || __DOXYGEN__ */

/**
 * \brief           Split command line to tokens array
 * \param[in,out]   pThis: \ref microrl_t working instance
 * \param[in]       limit: 
 * \param[in]       tkn_arr: 
 * \return          Number of split tokens
 */
static int split(microrl_t* pThis, int limit, const char** tkn_arr) {
    int i = 0;
    int ind = 0;
#if MICRORL_CFG_USE_QUOTING
    int iq = 0;
    char quote = 0;
    for (iq = 0; iq < MICRORL_CFG_QUOTED_TOKEN_NMB; ++iq) {
        pThis->quotes[iq].begin = 0;
        pThis->quotes[iq].end = 0;
    }
    iq = 0;
#endif /* MICRORL_CFG_USE_QUOTING */
    while (1) {
        // go to the first NOT whitespace (not zero for us)
        while ((pThis->cmdline[ind] == '\0') && (ind < limit)) {
            ind++;
        }

        if (!(ind < limit)) {
            return i;
        }

#if MICRORL_CFG_USE_QUOTING
        if (pThis->cmdline[ind] == '\'' || pThis->cmdline[ind] == '"') {
            if (iq >= MICRORL_CFG_QUOTED_TOKEN_NMB) {
                restore (pThis);
                return -1;
            }
            quote = pThis->cmdline[ind];
            pThis->quotes[iq].begin = pThis->cmdline + ind;
            ind++;
        }
#endif /* MICRORL_CFG_USE_QUOTING */
        tkn_arr[i++] = pThis->cmdline + ind;
        if (i >= MICRORL_CFG_CMD_TOKEN_NMB) {
#if MICRORL_CFG_USE_QUOTING
            restore(pThis);
#endif /* MICRORL_CFG_USE_QUOTING */
            return -1;
        }
        // go to the first whitespace (zero for us)
        while (ind < limit) {
            if (pThis->cmdline[ind] == '\0') {
#if MICRORL_CFG_USE_QUOTING
                if (!quote) {
#endif /* MICRORL_CFG_USE_QUOTING */
                    break;
#if MICRORL_CFG_USE_QUOTING
                }
                pThis->cmdline[ind] = ' ';
            } else if (pThis->cmdline[ind] == quote) {
                if (pThis->cmdline[ind + 1] != '\0') {
                    restore(pThis);
                    return -1;
                }
                quote = 0;
                pThis->quotes[iq++].end = pThis->cmdline + ind;
                pThis->cmdline[ind++] = '\0';
                break;
#endif /* MICRORL_CFG_USE_QUOTING */
            }
            ind++;
        }
        if (!(ind < limit)) {
#if MICRORL_CFG_USE_QUOTING
            if (quote) {
                restore(pThis);
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
 * \param[in,out]   pThis: \ref microrl_t working instance
 */
inline static void print_prompt(microrl_t* pThis) {
    pThis->print(pThis, pThis->prompt_str);
}

/**
 * \brief           Clear last symbol in command line and move cursor
 *                  to its position
 * \param[in,out]   pThis: \ref microrl_t working instance
 */
inline static void terminal_backspace(microrl_t* pThis) {
    pThis->print(pThis, "\033[D \033[D");
}

/**
 * \brief           Print end line symbol defined in \ref MICRORL_CFG_END_LINE config
 * \param[in,out]   pThis: \ref microrl_t working instance
 */
inline static void terminal_newline(microrl_t* pThis) {
    pThis->print(pThis, MICRORL_CFG_END_LINE);
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
    int i = 0, j;
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
 * \param[in,out]   pThis: \ref microrl_t working instance
 * \param[in]       offset: Positive or negative interval to move cursor
 */
static void terminal_move_cursor(microrl_t* pThis, int offset) {
    char str[16] = {0,};
    if (offset != 0) {
        generate_move_cursor(str, offset);
        pThis->print(pThis, str);
    }
}

/**
 * \brief           Print command line to screen, replace '\0' to wihitespace
 * \param[in,out]   pThis: \ref microrl_t working instance
 * \param[in]       pos: 
 * \param[in]       reset: 
 */
static void terminal_print_line(microrl_t* pThis, int pos, int reset) {
    if (pThis->echo != MICRORL_ECHO_OFF) {
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

        for (int i = pos; i < pThis->cmdlen; i++) {
            *j++ = (pThis->cmdline[i] == '\0') ? ' ' : pThis->cmdline[i];
            if ((j - str) == (sizeof(str) - 1)) {
                *j = '\0';
                pThis->print(pThis, str);
                j = str;
            }
        }

        if ((j - str + 3 + 6 + 1) > MICRORL_CFG_PRINT_BUFFER_LEN) {
            *j = '\0';
            pThis->print(pThis, str);
            j = str;
        }

        *j++ = '\033';   // delete all past end of text
        *j++ = '[';
        *j++ = 'K';
        generate_move_cursor(j, pThis->cursor - pThis->cmdlen);
        pThis->print(pThis, str);
    }
}

/**
 * \brief           Initialize MicroRL lib data
 * \param[in,out]   pThis: \ref microrl_t working instance
 * \param[in]       print: Callback function for character output
 * \return          \ref microrlOK on success, member of \ref microrlr_t otherwise
 */
microrlr_t microrl_init(microrl_t* pThis, microrl_print_fn print) {
    memset(pThis, 0, sizeof(microrl_t));
	
    pThis->prompt_str = prompt_default;
    pThis->print = print;
#if MICRORL_CFG_ENABLE_INIT_PROMPT
    print_prompt(pThis);
#endif /* MICRORL_CFG_ENABLE_INIT_PROMPT */
    pThis->echo = MICRORL_ECHO_ON;
    pThis->start_password = -1;
	
	return microrlOK;
}

#if MICRORL_CFG_USE_COMPLETE || __DOXYGEN__
/**
 * \brief           Set pointer to callback complition func, that called when user press 'Tab'
 * \param[in,out]   pThis: \ref microrl_t working instance
 * \param[in]       get_completion: Auto-complete string callback
 */
void microrl_set_complete_callback(microrl_t* pThis, microrl_get_compl_fn get_completion) {
    pThis->get_completion = get_completion;
}
#endif /* MICRORL_CFG_USE_COMPLETE || __DOXYGEN__ */

/**
 * \brief           Pointer to callback func, that called when user press 'Enter'
 * \param[in,out]   pThis: \ref microrl_t working instance
 * \param[in]       execute: Command execute callback
 */
void microrl_set_execute_callback(microrl_t* pThis, microrl_exec_fn execute) {
    pThis->execute = execute;
}

#if MICRORL_CFG_USE_CTRL_C || __DOXYGEN__
/**
 * \brief           Set callback for Ctrl+C terminal signal
 * \param[in,out]   pThis: \ref microrl_t working instance
 * \param[in]       sigint: Ctrl+C terminal signal callback
 */
void microrl_set_sigint_callback(microrl_t* pThis, microrl_sigint_fn sigint) {
    pThis->sigint = sigint;
}
#endif /* MICRORL_CFG_USE_CTRL_C || __DOXYGEN__ */

/**
 * \brief           Set echo mode used to mask user input
 *
 * Use \ref MICRORL_ECHO_ONCE to disable echo for password input, echo mode will enabled after user press Enter.
 * Use \ref MICRORL_ECHO_ON or \ref MICRORL_ECHO_OFF to turn on or off the echo manualy.
 *
 * \param[in,out]   pThis: \ref microrl_t working instance
 * \param[in]       echo: Member of \ref microrl_echo_t enumeration
 */
void microrl_set_echo(microrl_t* pThis, microrl_echo_t echo) {
    pThis->echo = echo;
}

#if MICRORL_CFG_USE_HISTORY || __DOXYGEN__
/**
 * \brief           Restore record to command line from history buffer
 * \param[in,out]   pThis: \ref microrl_t working instance
 * \param[in]       dir: Search direction in history ring buffer
 */
static void hist_search(microrl_t* pThis, int dir) {
    int len = hist_restore_line(&pThis->ring_hist, pThis->cmdline, dir);
    if (len >= 0) {
        pThis->cmdline[len] = '\0';
        pThis->cursor = pThis->cmdlen = len;
        terminal_print_line(pThis, 0, 1);
    }
}
#endif /* MICRORL_CFG_USE_HISTORY || __DOXYGEN__ */

#if MICRORL_CFG_USE_ESC_SEQ || __DOXYGEN__
/**
 * \brief           Handle escape sequences
 * \param[in,out]   pThis: \ref microrl_t working instance
 * \param[in]       ch: Input character
 * \return          '1' if escape sequence processed, '0' otherwise
 */
static int escape_process(microrl_t* pThis, char ch) {
    if (ch == '[') {
        pThis->escape_seq = _ESC_BRACKET;
        return 0;
    } else if (pThis->escape_seq == _ESC_BRACKET) {
        if (ch == 'A') {
#if MICRORL_CFG_USE_HISTORY
            hist_search(pThis, _HIST_UP);
#endif /* MICRORL_CFG_USE_HISTORY */
            return 1;
        } else if (ch == 'B') {
#if MICRORL_CFG_USE_HISTORY
            hist_search(pThis, _HIST_DOWN);
#endif /* MICRORL_CFG_USE_HISTORY */
            return 1;
        } else if (ch == 'C') {
            if (pThis->cursor < pThis->cmdlen) {
                terminal_move_cursor(pThis, 1);
                pThis->cursor++;
            }
            return 1;
        } else if (ch == 'D') {
            if (pThis->cursor > 0) {
                terminal_move_cursor(pThis, -1);
                pThis->cursor--;
            }
            return 1;
        } else if (ch == '7') {
            pThis->escape_seq = _ESC_HOME;
            return 0;
        } else if (ch == '8') {
            pThis->escape_seq = _ESC_END;
            return 0;
        }
    } else if (ch == '~') {
        if (pThis->escape_seq == _ESC_HOME) {
            terminal_move_cursor(pThis, -pThis->cursor);
            pThis->cursor = 0;
            return 1;
        } else if (pThis->escape_seq == _ESC_END) {
            terminal_move_cursor(pThis, pThis->cmdlen - pThis->cursor);
            pThis->cursor = pThis->cmdlen;
            return 1;
        }
    }

    /* unknown escape sequence, stop */
    return 1;
}
#endif /* MICRORL_CFG_USE_ESC_SEQ || __DOXYGEN__ */

/**
 * \brief           Insert len char of text at cursor position
 * \param[in,out]   pThis: \ref microrl_t working instance
 * \param[in]       text: 
 * \param[in]       len: 
 * \return          
 */
int microrl_insert_text(microrl_t* pThis, char* text, int len) {
    int i;
    if (pThis->cmdlen + len < MICRORL_CFG_CMDLINE_LEN) {
        if ((pThis->echo == MICRORL_ECHO_ONCE) & (pThis->start_password == -1)) {
            pThis->start_password = pThis->cmdlen;
        }
        memmove(pThis->cmdline + pThis->cursor + len,
                pThis->cmdline + pThis->cursor,
                pThis->cmdlen - pThis->cursor);
        for (i = 0; i < len; i++) {
            pThis->cmdline[pThis->cursor + i] = text[i];
            if (pThis->cmdline[pThis->cursor + i] == ' ') {
                pThis->cmdline[pThis->cursor + i] = 0;
            }
        }
        pThis->cursor += len;
        pThis->cmdlen += len;
        pThis->cmdline[pThis->cmdlen] = '\0';
        return true;
    }
    return false;
}

/**
 * \brief           Remove len chars backwards at cursor
 * \param[in,out]   pThis: \ref microrl_t working instance
 * \param[in]       len: Number of chars to remove
 */
static void microrl_backspace(microrl_t* pThis, int len) {
    if (pThis->cursor >= len) {
        memmove(pThis->cmdline + pThis->cursor - len,
                pThis->cmdline + pThis->cursor,
                pThis->cmdlen - pThis->cursor + len);
        pThis->cursor -= len;
        pThis->cmdline[pThis->cmdlen] = '\0';
        pThis->cmdlen -= len;
    }
}

/**
 * \brief           Remove one char forward at cursor
 * \param[in,out]   pThis: \ref microrl_t working instance
 */
static void microrl_delete(microrl_t* pThis) {
    if (pThis->cmdlen > 0) {
      memmove(pThis->cmdline + pThis->cursor,
              pThis->cmdline + pThis->cursor + 1,
              pThis->cmdlen - pThis->cursor + 1);
      pThis->cmdline[pThis->cmdlen] = '\0';
      pThis->cmdlen--;
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
    size_t j;
    char* shortest = arr[0];
    size_t shortlen = strlen(shortest);

    for (i = 0; arr[i] != NULL; ++i) {
        if (strlen(arr[i]) < shortlen) {
            shortest = arr[i];
            shortlen = strlen(shortest);
        }
    }

    for (i = 0; i < shortlen; ++i) {
        for (j = 0; arr[j] != 0; ++j) {
            if (shortest[i] != arr[j][i]) {
                return i;
            }
        }
    }

    return i;
}

/**
 * \brief           
 * \param[in,out]   pThis: \ref microrl_t working instance
 */
static void microrl_get_complite(microrl_t* pThis) {
    const char* tkn_arr[MICRORL_CFG_CMD_TOKEN_NMB];
    char** compl_token;

    if (pThis->get_completion == NULL) { // callback was not set
        return;
    }

    int status = split(pThis, pThis->cursor, tkn_arr);
    if (status < 0) {
        return;
    }

    if (pThis->cmdline[pThis->cursor - 1] == '\0') {
        tkn_arr[status++] = "";
    }
    compl_token = pThis->get_completion(pThis, status, tkn_arr);
#if MICRORL_CFG_USE_QUOTING
    restore(pThis);
#endif /* MICRORL_CFG_USE_QUOTING */
    if (compl_token[0] != NULL) {
        int i = 0;
        int len;
        int pos = pThis->cursor;

        if (compl_token[1] == NULL) {
            len = strlen(compl_token[0]);
        } else {
            len = common_len(compl_token);
            terminal_newline(pThis);
            while (compl_token[i] != NULL) {
                pThis->print(pThis, compl_token[i]);
                pThis->print(pThis, " ");
                i++;
            }
            terminal_newline(pThis);
            print_prompt(pThis);
            pos = 0;
        }

        if (len) {
            microrl_insert_text(pThis, compl_token[0] + strlen(tkn_arr[status - 1]),
                                len - strlen(tkn_arr[status - 1]));
            if (compl_token[1] == NULL) {
                microrl_insert_text (pThis, " ", 1);
            }
        }
        terminal_print_line (pThis, pos, 0);
    }
}

#endif /* MICRORL_CFG_USE_COMPLETE || __DOXYGEN__ */

/**
 * \brief           Processing input string and calling execute() callback
 * \param[in,out]   pThis: \ref microrl_t working instance
 */
static void new_line_handler(microrl_t* pThis) {
    const char* tkn_arr[MICRORL_CFG_CMD_TOKEN_NMB];
    int status;

    terminal_newline(pThis);
#if MICRORL_CFG_USE_HISTORY
    if ((pThis->cmdlen > 0) && (pThis->echo == MICRORL_ECHO_ON)) {
        hist_save_line(&pThis->ring_hist, pThis->cmdline, pThis->cmdlen);
    }
#endif /* MICRORL_CFG_USE_HISTORY */
    if (pThis->echo == MICRORL_ECHO_ONCE) {
        microrl_set_echo(pThis, MICRORL_ECHO_ON);
        pThis->start_password = -1;
    }
    status = split(pThis, pThis->cmdlen, tkn_arr);
    if (status == -1) {
//        pThis->print(pThis, "ERROR: Max token amount exseed\n");
#if MICRORL_CFG_USE_QUOTING
        pThis->print(pThis, "ERROR:too many tokens or invalid quoting");
#else
        pThis->print(pThis, "ERROR:too many tokens");
#endif /* MICRORL_CFG_USE_QUOTING */
        pThis->print(pThis, MICRORL_CFG_END_LINE);
    }
    if ((status > 0) && (pThis->execute != NULL)) {
        pThis->execute(pThis, status, tkn_arr);
    }
    print_prompt(pThis);
    pThis->cmdlen = 0;
    pThis->cursor = 0;
    memset(pThis->cmdline, 0, MICRORL_CFG_CMDLINE_LEN);
#if MICRORL_CFG_USE_HISTORY
    pThis->ring_hist.cur = 0;
#endif /* MICRORL_CFG_USE_HISTORY */
}

/**
 * \brief           Insert char to command line
 *
 * For example calls in usart RX interrupt
 *
 * \param[in,out]   pThis: \ref microrl_t working instance
 * \param[in]       ch: Printing to terminal character 
 */
void microrl_insert_char(microrl_t* pThis, int ch) {
#if MICRORL_CFG_USE_ESC_SEQ
    if (pThis->escape) {
        if (escape_process(pThis, ch)) {
            pThis->escape = 0;
        }
    } else {
#endif /* MICRORL_CFG_USE_ESC_SEQ */
        if (ch == KEY_CR || ch == KEY_LF) {
            // Only trigger a newline if ch doen't follow its companion's
            // triggering a newline.
            if (pThis->last_endl == (ch == KEY_CR ? KEY_LF : KEY_CR)) {
                pThis->last_endl = 0;      // ignore char, but clear newline state
            } else {
                pThis->last_endl = ch;
                new_line_handler(pThis);
            }
            return;
        }
        pThis->last_endl = 0;
        switch (ch) {
            //-----------------------------------------------------
#if MICRORL_CFG_USE_COMPLETE
            case KEY_HT:
                microrl_get_complite(pThis);
                break;
#endif /* MICRORL_CFG_USE_COMPLETE */
            //-----------------------------------------------------
            case KEY_ESC:
#if MICRORL_CFG_USE_ESC_SEQ
                pThis->escape = 1;
#endif /* MICRORL_CFG_USE_ESC_SEQ */
                break;
            //-----------------------------------------------------
            case KEY_NAK: // ^U
                if (pThis->cursor > 0) {
                    microrl_backspace(pThis, pThis->cursor);
                }
                terminal_print_line(pThis, 0, 1);
                break;
            //-----------------------------------------------------
            case KEY_VT:  // ^K
                pThis->print(pThis, "\033[K");
                pThis->cmdlen = pThis->cursor;
                break;
            //-----------------------------------------------------
            case KEY_ENQ: // ^E
                terminal_move_cursor(pThis, pThis->cmdlen - pThis->cursor);
                pThis->cursor = pThis->cmdlen;
                break;
            //-----------------------------------------------------
            case KEY_SOH: // ^A
                terminal_move_cursor(pThis, -pThis->cursor);
                pThis->cursor = 0;
                break;
            //-----------------------------------------------------
            case KEY_ACK: // ^F
                if (pThis->cursor < pThis->cmdlen) {
                    terminal_move_cursor(pThis, 1);
                    pThis->cursor++;
                }
                break;
            //-----------------------------------------------------
            case KEY_STX: // ^B
                if (pThis->cursor) {
                    terminal_move_cursor(pThis, -1);
                    pThis->cursor--;
                }
                break;
            //-----------------------------------------------------
            case KEY_DLE: //^P
#if MICRORL_CFG_USE_HISTORY
                hist_search(pThis, _HIST_UP);
#endif /* MICRORL_CFG_USE_HISTORY */
                break;
            //-----------------------------------------------------
            case KEY_SO: //^N
#if MICRORL_CFG_USE_HISTORY
                hist_search(pThis, _HIST_DOWN);
#endif /* MICRORL_CFG_USE_HISTORY */
                break;
            //-----------------------------------------------------
            case KEY_DEL: // Backspace
            case KEY_BS: // ^H
                if (pThis->cursor > 0) {
                    microrl_backspace(pThis, 1);
                    if (pThis->cursor == pThis->cmdlen) {
                        terminal_backspace(pThis);
                    } else {
                        terminal_print_line(pThis, pThis->cursor, 1);
                    }
                }
                break;
            //-----------------------------------------------------
            case KEY_EOT: // ^D
                microrl_delete(pThis);
                terminal_print_line(pThis, pThis->cursor, 0);
                break;
            //-----------------------------------------------------
            case KEY_DC2: // ^R
                terminal_newline(pThis);
                print_prompt(pThis);
                terminal_print_line(pThis, 0, 0);
                break;
            //-----------------------------------------------------
#if MICRORL_CFG_USE_CTRL_C
            case KEY_ETX:
                if (pThis->sigint != NULL) {
                    pThis->sigint(pThis);
                }
                break;
#endif /* MICRORL_CFG_USE_CTRL_C */
            //-----------------------------------------------------
            default:
                if (((ch == ' ') && (pThis->cmdlen == 0)) || IS_CONTROL_CHAR(ch)) {
                    break;
                }
                if (microrl_insert_text(pThis, (char*)&ch, 1)) {
                    if (pThis->cursor == pThis->cmdlen) {
                        char nch [] = {0, 0};
                        if ((pThis->cursor >= pThis->start_password) & (pThis->echo == MICRORL_ECHO_ONCE)) {
                            nch[0] = '*';
                        } else {
                            nch[0] = ch;
                        }
                        pThis->print(pThis, nch);
                    } else {
                        terminal_print_line(pThis, pThis->cursor - 1, 0);
                    }
                }
                break;
        }
#if MICRORL_CFG_USE_ESC_SEQ
    }
#endif /* MICRORL_CFG_USE_ESC_SEQ */
}
