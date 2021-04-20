/**
 * \file            microrl.h
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

#ifndef MICRORL_HDR_H
#define MICRORL_HDR_H

#include "microrl_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup        MICRORL Micro Read Line library
 * \brief           Micro Read Line library
 * \{
 */

#ifndef false
#define false           0
#endif /* false */
#ifndef true
#define true            (!false)
#endif /* true */

/**
 * \brief           MicroRL result enumeration
 */
typedef enum {
    microrlOK = 0x00,                           /*!< Everything OK */
    microrlERR,
    microrlERRPAR,                              /*!< Parameter error */
    microrlERRMEM,                              /*!< Memory error */
} microrlr_t;

/**
 * \brief           ESC seq internal codes
 */
typedef enum {
    MICRORL_ESC_BRACKET = 0x01,
    MICRORL_ESC_HOME,
    MICRORL_ESC_END
} microrl_esq_code_t;

/**
 * \brief           List of possible echo modes
 */
typedef enum {
    MICRORL_ECHO_ONCE,                          /*!< Echo is disabled until Enter is pressed */
    MICRORL_ECHO_ON,                            /*!< Echo is always enabled */
    MICRORL_ECHO_OFF                            /*!< Echo is always disabled */
} microrl_echo_t;

/* Forward declarations */
struct microrl;
#if MICRORL_CFG_USE_HISTORY
struct microrl_hist_rbuf;
#endif /* MICRORL_CFG_USE_HISTORY */
#if MICRORL_CFG_USE_QUOTING
struct microrl_quoted_tkn;
#endif /* MICRORL_CFG_USE_QUOTING */

#if MICRORL_CFG_USE_HISTORY || __DOXYGEN__
/**
 * \brief           History struct, contains internal variable
 *
 * History stores in static ring buffer for memory saving
 *
 */
typedef struct microrl_hist_rbuf {
    char ring_buf[MICRORL_CFG_RING_HISTORY_LEN];
    int begin;
    int end;
    int cur;
} microrl_hist_rbuf_t;
#endif /* MICRORL_CFG_USE_HISTORY || __DOXYGEN__ */


#if MICRORL_CFG_USE_QUOTING || __DOXYGEN__
/**
 * \brief           Quoted token struct, points to begin and end marks
 */
typedef struct microrl_quoted_tkn {
    char* begin;
    char* end;
} microrl_quoted_tkn_t;
#endif /* MICRORL_CFG_USE_QUOTING || __DOXYGEN__ */

/**
 * \brief           Command execute function prototype
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       argc: argument count
 * \param[in]       argv: pointer array to token string
 * \return          
 */
typedef int       (*microrl_exec_fn)(struct microrl* mrl, int argc, const char* const *argv);

/**
 * \brief           Auto-complete function prototype
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       argc: argument count
 * \param[in]       argv: pointer array to token string
 * \return          NULL-terminated string, contain complite variant split by 'Whitespace'
 *                  If complite token found, it's must contain only one token to be complitted
 *                  Empty string if complite not found, and multiple string if there are some token
 */
typedef char **   (*microrl_get_compl_fn)(struct microrl* mrl, int argc, const char* const *argv);

/**
 * \brief           Character output function prototype
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       ch: Character to print
 */
typedef void      (*microrl_print_fn)(struct microrl* mrl, const char* ch);

/**
 * \brief           Ctrl+C terminal signal function prototype
 * \param[in,out]   mrl: \ref microrl_t working instance
 */
typedef void      (*microrl_sigint_fn)(struct microrl* mrl);

/**
 * \brief           MicroRL struct, contains internal library data
 */
typedef struct microrl {
#if MICRORL_CFG_USE_ESC_SEQ
    microrl_esq_code_t escape_seq;              /*!< member of \ref microrl_esq_code_t */
    char escape;
#endif /* MICRORL_CFG_USE_ESC_SEQ */
    char last_endl;                             /*!< either 0 or the CR or LF that just triggered a newline */
#if MICRORL_CFG_USE_HISTORY
    microrl_hist_rbuf_t ring_hist;              /*!< history object */
#endif /* MICRORL_CFG_USE_HISTORY */
    const char* prompt_str;                     /*!< pointer to prompt string */
    char cmdline[MICRORL_CFG_CMDLINE_LEN];      /*!< cmdline buffer */
    int cmdlen;                                 /*!< last position in command line */
    int cursor;                                 /*!< input cursor */
#if MICRORL_CFG_USE_QUOTING
    microrl_quoted_tkn_t quotes[_QUOTED_TOKEN_NMB];   /*!< pointers to quoted tokens */
#endif /* MICRORL_CFG_USE_QUOTING */
    microrl_exec_fn execute;                    /*!< ptr to 'execute' callback */
#if MICRORL_CFG_USE_COMPLETE
    microrl_get_compl_fn get_completion;        /*!< ptr to 'completion' callback */
#endif /* MICRORL_CFG_USE_COMPLETE */
    microrl_print_fn print;                     /*!< ptr to 'print' callback */
#if MICRORL_CFG_USE_CTRL_C
    microrl_sigint_fn sigint;
#endif /* MICRORL_CFG_USE_CTRL_C */
    microrl_echo_t echo;
    int start_password;                         /*!< position when start printing '*' chars */
    void* userdata;                             /*!< generic user data storage */
} microrl_t;

microrlr_t  microrl_init(microrl_t* mrl, microrl_print_fn print);

#if MICRORL_CFG_USE_COMPLETE
void        microrl_set_complete_callback(microrl_t* mrl, microrl_get_compl_fn get_completion);
#endif /* MICRORL_CFG_USE_COMPLETE */
void        microrl_set_execute_callback(microrl_t* mrl, microrl_exec_fn execute);
#if MICRORL_CFG_USE_CTRL_C
void        microrl_set_sigint_callback(microrl_t* mrl, microrl_sigint_fn sigint);
#endif /* MICRORL_CFG_USE_CTRL_C */

void        microrl_set_echo(microrl_t* mrl, microrl_echo_t echo);

void        microrl_insert_char(microrl_t* mrl, int ch);
int         microrl_insert_text(microrl_t* mrl, char* text, int len);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* MICRORL_HDR_H */
