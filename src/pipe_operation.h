/*
 * pipe_operation.h
 *
 *  Created on: Jun 29, 2015
 *      Author: tayler
 */

#ifndef __PIPE_OPERATION_H__
#define __PIPE_OPERATION_H__

#include "common.h"

class pipe_op {

public:
    pipe_op(unsigned nb_in_addr, unsigned nb_in_size,
            unsigned sb_addr, unsigned sb_size,
            unsigned nb_out_addr, unsigned nb_out_size);

    ~pipe_op();

    void set_sram_op_pending(sram_type type);
    void set_sram_op_complete(sram_type type);

    unsigned get_sram_addr(sram_type type);
    unsigned get_sram_size(sram_type type);

private:

    sram_op m_sram_op[NUM_SRAM_TYPE];
    bool sram_op_complete[NUM_SRAM_TYPE];
};


#endif /* PIPE_OPERATION_H_ */
