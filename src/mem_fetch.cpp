////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
// Tayler Hetherington
// 2015
// mem_fetch.cpp
// Memory request object
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

#include "mem_fetch.h"

memory_fetch::memory_fetch(mem_addr addr, unsigned size, mem_access_type type, sram_type s_type) :
m_addr(addr), m_size(size), m_type(type), m_access_complete_cycle(0), m_sram_type(s_type), m_is_complete(false){
    
}

memory_fetch::~memory_fetch(){
    
}