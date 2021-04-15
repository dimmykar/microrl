/**
 * \file            microrl.h
 * \brief           MicroRL library
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

#ifndef MICRORL_HDR_H
#define MICRORL_HDR_H

#include "microrl_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 * \brief           Boolean values that not implemented in C
 */
#ifndef false
#define false           0
#endif /* false */
#ifndef true
#define true            (!false)
#endif /* true */


/**
 * \brief           List of key codes
 */
#define KEY_NUL         0      /**< ^@ Null character */
#define KEY_SOH         1      /**< ^A Start of heading, = console interrupt */
#define KEY_STX         2      /**< ^B Start of text, maintenance mode on HP console */
#define KEY_ETX         3      /**< ^C End of text */
#define KEY_EOT         4      /**< ^D End of transmission, not the same as ETB */
#define KEY_ENQ         5      /**< ^E Enquiry, goes with ACK; old HP flow control */
#define KEY_ACK         6      /**< ^F Acknowledge, clears ENQ logon hand */
#define KEY_BEL         7      /**< ^G Bell, rings the bell... */
#define KEY_BS          8      /**< ^H Backspace, works on HP terminals/computers */
#define KEY_HT          9      /**< ^I Horizontal tab, move to next tab stop */
#define KEY_LF          10     /**< ^J Line Feed */
#define KEY_VT          11     /**< ^K Vertical tab */
#define KEY_FF          12     /**< ^L Form Feed, page eject */
#define KEY_CR          13     /**< ^M Carriage Return*/
#define KEY_SO          14     /**< ^N Shift Out, alternate character set */
#define KEY_SI          15     /**< ^O Shift In, resume defaultn character set */
#define KEY_DLE         16     /**< ^P Data link escape */
#define KEY_DC1         17     /**< ^Q XON, with XOFF to pause listings; "okay to send". */
#define KEY_DC2         18     /**< ^R Device control 2, block-mode flow control */
#define KEY_DC3         19     /**< ^S XOFF, with XON is TERM=18 flow control */
#define KEY_DC4         20     /**< ^T Device control 4 */
#define KEY_NAK         21     /**< ^U Negative acknowledge */
#define KEY_SYN         22     /**< ^V Synchronous idle */
#define KEY_ETB         23     /**< ^W End transmission block, not the same as EOT */
#define KEY_CAN         24     /**< ^X Cancel line, MPE echoes !!! */
#define KEY_EM          25     /**< ^Y End of medium, Control-Y interrupt */
#define KEY_SUB         26     /**< ^Z Substitute */
#define KEY_ESC         27     /**< ^[ Escape, next character is not echoed */
#define KEY_FS          28     /**< ^\ File separator */
#define KEY_GS          29     /**< ^] Group separator */
#define KEY_RS          30     /**< ^^ Record separator, block-mode terminator */
#define KEY_US          31     /**< ^_ Unit separator */

#define KEY_DEL         127    /**< Delete (not a real control character...) */


/**
 * \brief           brief
 */
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


/**
 * \brief           brief
 */
#define ECHO_IS_ON()         ((pThis->echo) == (ON))
#define ECHO_IS_OFF()        ((pThis->echo) == (OFF))
#define ECHO_IS_ONCE()       ((pThis->echo) == (ONCE))

/**
 * \brief           brief
 */
typedef enum echo_ {
    ONCE,
    ON,
    OFF
} echo_t;

/* Forward declarations */
struct microrl;
#if MICRORL_CFG_USE_HISTORY
struct ring_history;
#endif /* MICRORL_CFG_USE_HISTORY */
#if MICRORL_CFG_USE_QUOTING
struct quoted_token;
#endif /* MICRORL_CFG_USE_QUOTING */

#if MICRORL_CFG_USE_HISTORY
/**
 * \brief           History struct, contains internal variable
 *
 * History stores in static ring buffer for memory saving
 *
 */
typedef struct ring_history {
    char ring_buf [MICRORL_CFG_RING_HISTORY_LEN];
    int begin;
    int end;
    int cur;
} ring_history_t;
#endif /* MICRORL_CFG_USE_HISTORY */


#if MICRORL_CFG_USE_QUOTING
/**
 * \brief           Quoted token struct, points to begin and end marks
 */
typedef struct quoted_token {
    char* begin;
    char* end;
} quoted_token_t;
#endif /* MICRORL_CFG_USE_QUOTING */

/**
 * \brief           Command execute function prototype
 * \param[in,out]   pThis: \ref microrl_t working instance
 * \param[in]       argc: argument count
 * \param[in]       argv: pointer array to token string
 * \return          
 */
typedef int       (*exec_fn)(struct microrl* pThis, int argc, const char* const *argv);

/**
 * \brief           Auto-complete function prototype
 * \param[in,out]   pThis: \ref microrl_t working instance
 * \param[in]       argc: argument count
 * \param[in]       argv: pointer array to token string
 * \return          NULL-terminated string, contain complite variant split by 'Whitespace'
 *                  If complite token found, it's must contain only one token to be complitted
 *                  Empty string if complite not found, and multiple string if there are some token
 */
typedef char **   (*get_compl_fn)(struct microrl* pThis, int argc, const char* const *argv);

/**
 * \brief           Character output function prototype
 * \param[in,out]   pThis: \ref microrl_t working instance
 * \param[in]       ch: Character to print
 */
typedef void      (*print_fn)(struct microrl* pThis, const char* ch);

/**
 * \brief           Ctrl+C terminal signal function prototype
 * \param[in,out]   pThis: \ref microrl_t working instance
 */
typedef void      (*sigint_fn)(struct microrl* pThis);

/**
 * \brief           MicroRL struct, contains internal library data
 */
typedef struct microrl {
#if MICRORL_CFG_USE_ESC_SEQ
    char escape_seq;
    char escape;
#endif /* MICRORL_CFG_USE_ESC_SEQ */
    char last_endl;                    // either 0 or the CR or LF that just triggered a newline
#if MICRORL_CFG_USE_HISTORY
    ring_history_t ring_hist;          // history object
#endif /* MICRORL_CFG_USE_HISTORY */
    const char* prompt_str;            // pointer to prompt string
    char cmdline[MICRORL_CFG_CMDLINE_LEN];   // cmdline buffer
    int cmdlen;                        // last position in command line
    int cursor;                        // input cursor
#if MICRORL_CFG_USE_QUOTING
    struct quoted_token quotes[_QUOTED_TOKEN_NMB]; // pointers to quoted tokens
#endif /* MICRORL_CFG_USE_QUOTING */
    exec_fn execute;              // ptr to 'execute' callback
#if MICRORL_CFG_USE_COMPLETE
    get_compl_fn get_completion;  // ptr to 'completion' callback
#endif /* MICRORL_CFG_USE_COMPLETE */
    print_fn print;               // ptr to 'print' callback
#if MICRORL_CFG_USE_CTRL_C
    sigint_fn sigint;
#endif /* MICRORL_CFG_USE_CTRL_C */
    echo_t echo;
    int start_password;                // position when start printing '*' chars
    void* userdata;                    // generic user data storage
} microrl_t;

void microrl_init(microrl_t* pThis, print_fn print);
#if MICRORL_CFG_USE_COMPLETE
void microrl_set_complete_callback(microrl_t* pThis, get_compl_fn get_completion);
#endif /* MICRORL_CFG_USE_COMPLETE */
void microrl_set_execute_callback(microrl_t* pThis, exec_fn execute);
#if MICRORL_CFG_USE_CTRL_C
void microrl_set_sigint_callback(microrl_t* pThis, sigint_fn sigint);
#endif /* MICRORL_CFG_USE_CTRL_C */

void microrl_set_echo(microrl_t* pThis, echo_t echo);

void microrl_insert_char(microrl_t* pThis, int ch);
int microrl_insert_text(microrl_t* pThis, char* text, int len);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* MICRORL_HDR_H */
