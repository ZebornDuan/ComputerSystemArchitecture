/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2009 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */

extern "C" {
#include "xed-interface.h"
#include "xed-portability.h"
#include "xed-examples-util.h"
}
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <vector>
#include "xed-symbol-table.H"
using namespace std;


void make_symbol_vector(xed_symbol_table_t* symbol_table)
{
    map<xed_uint64_t,char*>::iterator i = symbol_table->global_sym_map.begin();
    for( ; i != symbol_table->global_sym_map.end() ;i ++) {
        symbol_table->global_sym_vec.push_back(i->first);
    }
    sort(symbol_table->global_sym_vec.begin(), symbol_table->global_sym_vec.end());
}



xed_uint64_t find_symbol_address(vector<xed_uint64_t>& sym_vec, xed_uint64_t tgt) 
{
    vector<xed_uint64_t>::iterator i = lower_bound(sym_vec.begin(), sym_vec.end(), tgt);
    if (i == sym_vec.end())
        return 0;
    if (*i > tgt) {
        // take previous value
        if (i != sym_vec.begin())
            return *(i-1);
    }
    if (*i == tgt) {
        return *i;
    }
    return 0;
    
}


xed_uint64_t find_symbol_address_global(xed_uint64_t tgt, xed_symbol_table_t* symbol_table) 
{
    return find_symbol_address(symbol_table->global_sym_vec, tgt);
}


char* get_symbol(xed_uint64_t a, void* caller_data) {
    xed_symbol_table_t* symbol_table = reinterpret_cast<xed_symbol_table_t*>(caller_data);
    map<xed_uint64_t,char*>::iterator i = symbol_table->global_sym_map.find(a);
    if (i != symbol_table->global_sym_map.end()) {
         return i->second;
    }
    return 0;
}


int xed_disassembly_callback_function(
    xed_uint64_t address,
    char* symbol_buffer,
    xed_uint32_t buffer_length,
    xed_uint64_t* offset,
    void* caller_data) 
{
    xed_symbol_table_t* symbol_table = reinterpret_cast<xed_symbol_table_t*>(caller_data);
    xed_uint64_t symbol_address = find_symbol_address_global(address, symbol_table);
    if (symbol_address) {
        char* symbol  = get_symbol(symbol_address, caller_data);
        if (symbol) {
            if (xed_strlen(symbol) < buffer_length)
                xed_strncpy(symbol_buffer, symbol, buffer_length);
            else {
                xed_strncpy(symbol_buffer, symbol, buffer_length-1);
                symbol_buffer[buffer_length-1]=0;
            }
            *offset = address - symbol_address;
            return 1;
        }
    }
    return 0;
}
