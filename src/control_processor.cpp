////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
// Tayler Hetherington
// 2015
// control_processor.cpp
// Control Processor 
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

#include "control_processor.h"

// Testing
static bool is_test_complete = false;


control_processor::control_processor(dnn_config const * const cfg){
    
    m_dnn_config = cfg;
    
}

control_processor::~control_processor(){
    
}

void control_processor::cycle(){

}

// FIXME: Note - DRAM memory_fetch currently pulls in the complete data in one request to fill the entire SRAMs.
//        This needs to be separated out into multiple accesses and multiple writes to the SRAMs.

void control_processor::do_cp_inst(cp_inst *inst){
    // FSM for each instruction
    memory_fetch *mf = NULL;
    bool pending_req = false;
    
    switch(inst->m_state){
        // Always start with LOAD_NBIN if both LOAD_NBIN and LOAD_SB are set
      case cp_inst::LOAD_NBIN: // Load from DRAM into the NBin SRAM
            
            mf = new memory_fetch(inst->nbin_address, inst->nbin_size, READ, NBin);
            m_dram_interface->do_access(mf);
            
            if(inst->sb_read_op == cp_inst::LOAD){
                inst->m_state = cp_inst::LOAD_SB;
            }else{
                inst->m_state = cp_inst::DO_OP;
            }
            
            m_mem_requests.push(mf); // Add memory fetch to pending queue
            
            break;
            
        case cp_inst::LOAD_SB: // Load from DRAM into the SB SRAM
            
            mf = new memory_fetch(inst->sb_address, inst->sb_size, READ, SB);
            m_dram_interface->do_access(mf);
            
            inst->m_state = cp_inst::DO_OP;
            
            m_mem_requests.push(mf); // Add memory fetch to pending queue
            
            
            break;
            
        case cp_inst::DO_OP: // All data is loaded into the SRAMs, push pipe_ops into the main dnn_sim pipeline
            
            // First wait for all loads to complete, write data to SRAMs
            if(m_mem_requests.size() > 0){
                memory_fetch *mf = m_mem_requests.front();
                
                if(mf->m_is_complete){
                    // Write the data to the SRAM
                    if(m_datapath->write_sram(mf->m_addr, mf->m_size, mf->m_sram_type)){
                        // Write went through, pop the request from the mem_req queue
                        m_mem_requests.pop();
                    }else {
                        // Otherwise, all SRAM ports were busy, try again next cycle
                        return;
                    }
                    
                }else{
                    pending_req = true;
                }
            }
            
            // Then start doing the main operation if no pending DRAM READS
            if(!pending_req){
                // This is where I would start creating "pipe_ops" to perform the convolution, cycling through the different filters loaded into SB
                
            }
            
            break;
            
        case cp_inst::STORE_NBOUT:
            // Write out NBout to DRAM
            std::cout << "STORE_NBOUT not implemented" << std::endl;
            break;
            
        default:
            std::cout << "Error: Undefined instruction state. Aborting" << std::endl;
            abort();
    }
    
}

void control_processor::test(){
    
    // Test full load into SB and NBin
    cp_inst *m_inst = new cp_inst();
    
    
    // Set test data
    m_inst->sb_read_op = cp_inst::LOAD;
    m_inst->sb_reuse = 0;
    m_inst->sb_address = 0;
    m_inst->sb_size = 32768;
    

    m_inst->nbin_read_op = cp_inst::LOAD;
    m_inst->nbin_reuse = 0;
    m_inst->nbin_stride = 0;
    m_inst->nbin_stride_begin = 0;
    m_inst->nbin_stride_end = 0;
    m_inst->nbin_address = 4194304;
    m_inst->nbin_size = 2048;
    
    // TODO: Add main NFU stages and NBout config
    
    m_inst->m_state = cp_inst::LOAD_NBIN;
    
    // Cycle through the state machine for this test instruction
    while(!is_test_complete){
        do_cp_inst(m_inst);
        cycle();
    }
    
    
  
    
    delete m_inst;
}




