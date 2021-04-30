/**
 * \file            unix_misc.c
 * \brief           Linux PC platform specific implementation routines
 */

/*
 * Copyright (c) 2011 Eugene SAMOYLOV
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
 * Author:          Eugene SAMOYLOV aka Helius <ghelius@gmail.com>
 * Version:         1.7.0-dev
 */

#include <termios.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h> 
#include <string.h>
#include "microrl.h"

// definition commands word
#define _CMD_HELP           "help"
#define _CMD_CLEAR          "clear"
#define _CMD_LIST           "list"
#define _CMD_LISP           "lisp"    // for demonstration completion on 'l + <TAB>'
#define _CMD_NAME           "name"
#define _CMD_VER            "version"
// sub commands for version command
#define _SCMD_MRL           "microrl"
#define _SCMD_DEMO          "demo"

#define _NUM_OF_CMD         6
#define _NUM_OF_VER_SCMD    2

#define _NAME_LEN           8

//available  commands
char* keyword[] = {_CMD_HELP, _CMD_CLEAR, _CMD_LIST, _CMD_NAME, _CMD_VER, _CMD_LISP};
// version subcommands
char* ver_keyword[] = {_SCMD_MRL, _SCMD_DEMO};

// array for comletion
char* compl_word[_NUM_OF_CMD + 1];

// 'name' var for store some string
char name[_NAME_LEN];
int val;

/**
 * \brief           Init Linux PC platform
 * \note            Dummy function, no need on Linux-PC
 */
void init(void) {
};

/**
 * \brief           Print to IO stream callback for MicroRL library
 * \param[in]       mrl: \ref microrl_t working instance
 * \param[in]       str: Output string
 */
void print(microrl_t* mrl, const char* str) {
    fprintf(stdout, "%s", str);
}

/**
 * \brief           Get char user pressed, no waiting Enter input
 * \return          Input character
 */
char get_char(void) {
    struct termios oldt, newt;
    int ch;
    tcgetattr( STDIN_FILENO, &oldt );
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
    return ch;
}

/**
 * \brief           HELP command callback
 * \param[in]       mrl: \ref microrl_t working instance
 */
void print_help(microrl_t* mrl) {
    print(mrl, "Use TAB key for completion\n\rCommand:\n\r");
    print(mrl, "\tversion {microrl | demo} - print version of microrl lib or version of this demo src\n\r");
    print(mrl, "\thelp  - this message\n\r");
    print(mrl, "\tclear - clear screen\n\r");
    print(mrl, "\tlist  - list all commands in tree\n\r");
    print(mrl, "\tname[string] - print 'name' value if no 'string', set name value to 'string' if 'string' present\n\r");
    print(mrl, "\tlisp - dummy command for demonstation auto-completion, while inputed 'l+<TAB>'\n\r");
}

/**
 * \brief           Execute callback for MicroRL library
 *
 * Do what you want here, but don't write to argv!!! read only!!
 *
 * \param[in]       mrl: \ref microrl_t working instance
 * \param[in]       argc: argument count
 * \param[in]       argv: pointer array to token string
 * \return          '0' on success, '1' otherwise. Not used by library
 */
int execute(microrl_t* mrl, int argc, const char* const *argv) {
    int i = 0;
    // just iterate through argv word and compare it with your commands
    while (i < argc) {
        if (strcmp (argv[i], _CMD_HELP) == 0) {
            print(mrl, "microrl library based shell v 1.0\n\r");
            print_help();        // print help
        } else if (strcmp (argv[i], _CMD_NAME) == 0) {
            if ((++i) < argc) { // if value preset
                if (strlen(argv[i]) < _NAME_LEN) {
                    strcpy(name, argv[i]);
                } else {
                    print(mrl, "name value too long!\n\r");
                }
            } else {
                print(mrl, name);
                print(mrl, "\n\r");
            }
        } else if (strcmp(argv[i], _CMD_VER) == 0) {
            if (++i < argc) {
                if (strcmp(argv[i], _SCMD_DEMO) == 0) {
                    print(mrl, "demo v 1.0\n\r");
                } else if (strcmp(argv[i], _SCMD_MRL) == 0) {
                    print(mrl, "microrl v 1.2\n\r");
                } else {
                    print(mrl, (char*)argv[i]);
                    print(mrl, " wrong argument, see help\n\r");
                }
            } else {
                print(mrl, "version needs 1 parametr, see help\n\r");
            }
        } else if (strcmp(argv[i], _CMD_CLEAR) == 0) {
            print(mrl, "\033[2J");    // ESC seq for clear entire screen
            print(mrl, "\033[H");     // ESC seq for move cursor at left-top corner
        } else if (strcmp(argv[i], _CMD_LIST) == 0) {
            print(mrl, "available command:\n");// print all command per line
            for (int i = 0; i < _NUM_OF_CMD; i++) {
                print(mrl, "\t");
                print(mrl, keyword[i]);
                print(mrl, "\n\r");
            }
        } else {
            print(mrl, "command: '");
            print(mrl, (char*)argv[i]);
            print(mrl, "' Not found.\n\r");
        }
        i++;
    }
    return 0;
}

#if MICRORL_CFG_USE_COMPLETE || __DOXYGEN__
/**
 * \brief           Completion callback for MicroRL library
 * \param[in,out]   mrl: \ref microrl_t working instance
 * \param[in]       argc: argument count
 * \param[in]       argv: pointer array to token string
 * \return          NULL-terminated string, contain complite variant split by 'Whitespace'
 */
char ** complet(microrl_t* mrl, int argc, const char* const *argv) {
    (void)mrl;
    int j = 0;

    compl_word[0] = NULL;

    // if there is token in cmdline
    if (argc == 1) {
        // get last entered token
        char* bit = (char*)argv[argc - 1];
        // iterate through our available token and match it
        for (int i = 0; i < _NUM_OF_CMD; i++) {
            // if token is matched (text is part of our token starting from 0 char)
            if (strstr(keyword[i], bit) == keyword[i]) {
                // add it to completion set
                compl_word[j++] = keyword[i];
            }
        }
    }  else if ((argc > 1) && (strcmp (argv[0], _CMD_VER)==0)) { // if command needs subcommands
        // iterate through subcommand for command _CMD_VER array
        for (int i = 0; i < _NUM_OF_VER_SCMD; i++) {
            if (strstr(ver_keyword[i], argv[argc - 1]) == ver_keyword[i]) {
                compl_word[j++] = ver_keyword[i];
            }
        }
    } else { // if there is no token in cmdline, just print all available token
        for (; j < _NUM_OF_CMD; j++) {
            compl_word[j] = keyword[j];
        }
    }

    // note! last ptr in array always must be NULL!!!
    compl_word[j] = NULL;
    // return set of variants
    return compl_word;
}
#endif /* MICRORL_CFG_USE_COMPLETE || __DOXYGEN__ */

#if MICRORL_CFG_USE_CTRL_C || __DOXYGEN__
/**
 * \brief           Ctrl+C terminal signal function
 * \param[in]       mrl: \ref microrl_t working instance
 */
void sigint(microrl_t* mrl) {
    print(mrl, "^C catched!\n\r");
}
#endif /* MICRORL_CFG_USE_CTRL_C || __DOXYGEN__ */
