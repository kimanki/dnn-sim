////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
// Tayler Hetherington
// 2015
// datapath.cpp
// Main DNN simulator
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

#include "datapath.h"

// Main DNN_sim object. Creates all internal structures of the DianNao architecture
datapath::datapath(dnn_config const * const config) : m_config(config){

    // Currently hardcoded for 3 stages (NFU-1, NFU-2, NFU-3)
    m_n_stages = NUM_PIPE_STAGES;
    assert(m_n_stages == 3);

    m_max_buffer_size = m_config->max_buffer_size;
    
    m_pipe_stages = new pipe_stage *[m_n_stages];
    
    // Create pipeline stage registers ( (num_stages-1) + 2)
    m_pipe_regs = new pipe_reg[m_n_stages + 1];
    //request to the SRAMs from the pipeline
    m_pipe_requests = new pipe_reg[3]; //as many as SRAM types

    m_pipe_stages[NFU1] = new nfu_1(&m_pipe_regs[0], &m_pipe_regs[1], &m_pipe_requests[0], m_max_buffer_size, m_config->num_nfu1_pipeline_stages-1, m_config->num_nfu1_multipliers);
    m_pipe_stages[NFU2] = new nfu_2(&m_pipe_regs[1], &m_pipe_regs[2], m_max_buffer_size, m_config->num_nfu2_pipeline_stages-1, m_config->num_nfu2_adders, m_config->num_nfu2_shifters, m_config->num_nfu2_max);
    m_pipe_stages[NFU3] = new nfu_3(&m_pipe_regs[2], &m_pipe_regs[3], m_max_buffer_size, m_config->num_nfu3_pipeline_stages-1, m_config->num_nfu3_multipliers, m_config->num_nfu3_adders);
    
    // FIXME: Will need to fix this when not multiples of 8-bits
    // bytes per data element
    unsigned bytes = (m_config->bit_width / 8);
    
    
   //    sram_array(sram_type type, unsigned line_size, unsigned num_lines, unsigned bit_width,
   //                 unsigned num_read_write_ports, unsigned num_cycle_per_access, pipe_reg *p_reg);
    
    
    // Create SRAMs (SB, NBin, NBout)
    m_srams[NBin]   = new sram_array(NBin, m_config->nbin_line_length*bytes,
                                     m_config->nbin_num_lines, m_config->bit_width,
                                     m_config->nbin_num_ports, m_config->nbin_access_cycles,
                                     &m_pipe_requests[0], &m_pipe_regs[0]);
    
    m_srams[NBout]  = new sram_array(NBout, m_config->nbout_line_length*bytes,
                                     m_config->nbout_num_lines, m_config->bit_width,
                                     m_config->nbout_num_ports, m_config->nbout_access_cycles,
                                     &m_pipe_requests[2], &m_pipe_regs[3]);
    
    m_srams[SB]     = new sram_array(SB, m_config->sb_line_length*bytes,
                                     m_config->sb_num_lines, m_config->bit_width,
                                     m_config->sb_num_ports, m_config->sb_access_cycles,
                                     &m_pipe_requests[0], &m_pipe_regs[0]);
    
    // Stats
    m_tot_op_issue = 0;
    m_tot_op_complete = 0;
    
}


datapath::~datapath(){
    // Clean up pipeline stages
    for(unsigned i=0; i<m_n_stages; ++i){
        if(m_pipe_stages[i])
            delete m_pipe_stages[i];
    }
    delete[] m_pipe_stages;

    if(m_pipe_regs)
        delete[] m_pipe_regs;
    
    // Clean up SRAM arrays
    for(unsigned i=0; i<NUM_SRAM_TYPE; ++i){
        delete m_srams[i];
    }
}

// Cycle structures in reverse order
void datapath::cycle(){
    
    
    check_nb_out_complete();                    // Retire completed pipeline operation
    
    m_srams[NBout]->cycle();                    // Cycle NBout SRAM for write
    
    for(int i=(NUM_PIPE_STAGES-1); i>=0; --i){  // Cycle the NFU-# pipeline stages in reverse order
        m_pipe_stages[i]->cycle();
    }
    
    m_srams[NBin]->cycle();                     // Cycle NBin and SB SRAMs for read
    m_srams[SB]->cycle();
    
    //print_pipeline();
}

bool datapath::insert_op(pipe_op *op){
    
    if ( ! m_pipe_regs[0].empty() ) return false;
    std::cout << "Inserting Operation" << std::endl;
    
    m_tot_op_issue++;
    op->set_read();

    // we aren't using this right now
    m_pipe_requests[0].push(op);

    // pushing op directly to pipeline
    m_pipe_regs[0].push(op);

    // if pipe_reg[0] is empty then we should be able to read the buffers as well
    m_srams[NBin]->read(op);
    m_srams[SB]->read(op);

    
    return true;
}

// Basically the writeback stage. Check if the pipeline operation has written back to NBout successfully.
// If so, pop the operation from the pipeline. Otherwise, wait for completion.
bool datapath::check_nb_out_complete(){
    pipe_op *op;
    if(!m_pipe_regs[m_n_stages].empty()){
        op = m_pipe_regs[m_n_stages].front();
        if(!op->is_read() && op->is_write_complete()){
            m_pipe_regs[m_n_stages].pop();
            delete op;
            m_tot_op_complete++;
        }
    }
    return true;
}

bool datapath::read_sram(unsigned address, unsigned size, sram_type s_type){
    return m_srams[s_type]->read(address, size);
}
bool datapath::write_sram(unsigned address, unsigned size, sram_type s_type){
    return m_srams[s_type]->write(address, size);
}

void datapath::print_stats(){
    std::cout << std::endl << "=====================" << std::endl;
    std::cout << "Total operations issued: " << m_tot_op_issue << std::endl;
    std::cout << "Total operations completed: " << m_tot_op_complete << std::endl;
    std::cout << "=====================" << std::endl << std::endl;
}

void datapath::print_pipeline(){
    for(unsigned i=0; i<NUM_PIPE_STAGES; ++i)
        m_pipe_stages[i]->print_internal_pipeline();
}

///////////////////////////////////////////////
// DEBUG
///////////////////////////////////////////////
void datapath::insert_dummy_op(pipe_op *op){
    m_pipe_stages[NFU1]->push_op(op);
}


