/**
 * \file            microrl_config.h
 * \brief           MicroRL library default configurations
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

#ifndef MICRORL_HDR_DEFAULT_CONFIG_H
#define MICRORL_HDR_DEFAULT_CONFIG_H

/* Uncomment to ignore user configs (or set macro in compiler flags) */
/* #define MICRORL_IGNORE_USER_CONFIGS */

/* Include application options */
#ifndef MICRORL_IGNORE_USER_CONFIGS
#include "microrl_user_config.h"
#endif /* MICRORL_IGNORE_USER_CONFIGS */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup        MICRORL_CFG Configuration
 * \brief           MicroRL configs
 * \{
 */

/**
 * \brief           Command line length, define cmdline buffer size. Set max number of chars + 1,
 *                  because last byte of buffer need to contain '\0' - NULL terminator, and 
 *                  not use for storing inputed char.
 *                  If user input chars more then it parametrs - 1, chars not added to command line.
 */
#ifndef _COMMAND_LINE_LEN
#define _COMMAND_LINE_LEN      (1 + 60)
#endif

/**
 * \brief           Command token number, define max token it command line, if number of token 
 *                  typed in command line exceed this value, then prints message about it and
 *                  command line not to be parced and 'execute' callback will not calls.
 *                  Token is word separate by white space, for example 3 token line:
 *                  "IRin> set mode test"
 */
#ifndef _COMMAND_TOKEN_NMB
#define _COMMAND_TOKEN_NMB     8
#endif

/**
 * \brief           Define you prompt string here. You can use colors escape code, for highlight you prompt,
 *                  for example this prompt will green color (if you terminal supports color)
 */
#ifndef _PROMPT_DEFAULT
#define _PROMPT_DEFAULT        "\033[32mIRin >\033[0m "  // green color
//#define _PROMPT_DEFAULT        "IRin > "
#endif

/**
 * \brief           Define prompt text (without ESC sequence, only text) prompt length, it needs because if you use
 *                  ESC sequence, it's not possible detect only text length
 */
#ifndef _PROMPT_LEN
#define _PROMPT_LEN            7
#endif

/**
 * \brief           Define it, if you wanna use completion functional, also set completion callback in you code,
 *                  now if user press TAB calls 'copmlitetion' callback. If you no need it, you can just set 
 *                  NULL to callback ptr and do not use it, but for memory saving tune, 
 *                  if you are not going to use it - disable this define.
 */
#ifndef _USE_COMPLETE
#define _USE_COMPLETE          1
#endif

/**
 * \brief           Define it, if you want to allow quoting command arguments to include spaces.
 *                  Depends upon _QUOTED_TOKEN_NMB parameter
 */
#ifndef _USE_QUOTING
#define _USE_QUOTING           1
#endif

/**
 * \brief           Quoted token number, define max number of tokens allowed to be quoted.  If the
 *                  number of quoted tokens typed in the command line exceeds this value, then
 *                  prints message about it and the command line is not parsed and 'execute'
 *                  callback is not called.
 *                  Quoting protects whitespace, for example 2 quoted tokens:
 *                  "IRin> set wifi 'Home Net' 'this is a secret'"
 */
#ifndef _QUOTED_TOKEN_NMB
#define _QUOTED_TOKEN_NMB      2
#endif

/**
 * \brief           Define it, if you wanna use history. It s work's like bash history, and
 *                  set stored value to cmdline, if UP and DOWN key pressed. Using history add
 *                  memory consuming, depends from _RING_HISTORY_LEN parametr
 */
#ifndef _USE_HISTORY
#define _USE_HISTORY           1
#endif

/**
 * \brief           History ring buffer length, define static buffer size.
 *                  For saving memory, each entered cmdline store to history in ring buffer,
 *                  so we can not say, how many line we can store, it depends from cmdline len,
 *                  but memory using more effective. We not prefer dinamic memory allocation for
 *                  small and embedded devices. Overhead is 2 char on each saved line
 */
#ifndef _RING_HISTORY_LEN
#define _RING_HISTORY_LEN      64
#endif

/**
 * \brief           Size of the buffer used for piecemeal printing of part or all of the command
 *                  line.  Allocated on the stack.  Must be at least 16.                 
 */
#ifndef _PRINT_BUFFER_LEN
#define _PRINT_BUFFER_LEN      40
#endif

/**
 * \brief           Enable Handling terminal ESC sequence. If disabling, then cursor arrow, HOME, END will not work,
 *                  use Ctrl+A(B,F,P,N,A,E,H,K,U,C) see README, but decrease code memory.
 */
#ifndef _USE_ESC_SEQ
#define _USE_ESC_SEQ           1
#endif

/**
 * \brief           Use sprintf from you standard complier library, but it gives some overhead.
 *                  If not defined, use my own number conversion code, it's save about 800 byte of
 *                  code size on AVR (avr-gcc build).
 *                  Try to build with and without, and compare total code size for tune library.
 */
#ifndef _USE_LIBC_STDIO
#define _USE_LIBC_STDIO        1
#endif

/**
 * \brief           Use a single carriage return character to move the cursor to the left margin
 *                  rather than moving left by a large number.  This reduces the number of
 *                  characters sent to the terminal, but should be left undefined if the terminal
 *                  will also simulate a linefeed when it receives the carriage return.
 */
#ifndef _USE_CARRIAGE_RETURN
#define _USE_CARRIAGE_RETURN   1
#endif

/**
 * \brief           Enable 'interrupt signal' callback, if user press Ctrl+C
 */
#ifndef _USE_CTRL_C
#define _USE_CTRL_C            1
#endif

/**
 * \brief           Print prompt at 'microrl_init', if enable, prompt will print at startup, 
 *                  otherwise first prompt will print after first press Enter in terminal
 * \note            Enable it, if you call 'microrl_init' after your communication subsystem 
 *                  already initialize and ready to print message
 */
#ifndef _ENABLE_INIT_PROMPT
#define _ENABLE_INIT_PROMPT    1
#endif

/**
 * \brief           New line symbol
 *
 * The symbol must be "\r", "\n", "\r\n" or "\n\r"
 */
#ifndef ENDL
#define ENDL                   "\n"
#endif

/**
 * \}
 */

#if _RING_HISTORY_LEN > 256
#error "This history implementation (ring buffer with 1 byte iterator) allow 256 byte buffer size maximum"
#endif /* _RING_HISTORY_LEN > 256 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* MICRORL_HDR_DEFAULT_CONFIG_H */
