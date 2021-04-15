/**
 * \file            example.c
 * \brief           Platform independent interface for implementing some
 *                  specific function for AVR, linux PC or ARM
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

#include <stdio.h>
#include <string.h>
#include "microrl.h"
#include "example_misc.h"

// create microrl object and pointer to it
microrl_t rl;
microrl_t* prl = &rl;

/**
 * \brief           Program entry point
 */
int main (void/*int argc, char** argv*/) {
    init();
	
    // call init with ptr to microrl instance and print callback
    microrl_init(prl, print);
	
    // set callback for execute
    microrl_set_execute_callback(prl, execute);

#if MICRORL_CFG_USE_COMPLETE
    // set callback for completion
    microrl_set_complete_callback(prl, complet);
#endif /* MICRORL_CFG_USE_COMPLETE */

#if MICRORL_CFG_USE_CTRL_C
    // set callback for Ctrl+C
    microrl_set_sigint_callback(prl, sigint);
#endif /* MICRORL_CFG_USE_CTRL_C */

    while (1) {
        // put received char from stdin to microrl lib
        microrl_insert_char(prl, get_char());
    }
    return 0;
}
