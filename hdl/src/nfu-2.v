//----------------------------------------------//
//----------------------------------------------//
// NFU-2: Convolution addition trees
// Tayler Hetherington
// 2015
//----------------------------------------------//
//----------------------------------------------//

//---------------------------------------------//
// 16, 16-bit adder trees with 15 adders each. 
// Input is Tn x Tx 16-bit multiplication results.
// Each group of 16 needs to be added. Result
// is 16, 16-bit values to be fed into NFU-3.
//---------------------------------------------//

module Tn_adder_tree (
        i_nfu1,         // New multiplication data from NFU-1
        i_nbout,        // Partial SUM from NBOut
        i_nbout_en,     // Use i_nbout if i_nbout_en '1'
        o_results
    );


    parameter BIT_WIDTH = 16;
    parameter Tn = 16;
    parameter TnxTn = 256;


    //----------- Input Ports ---------------//
    input [((BIT_WIDTH*Tn) - 1):0] i_nfu1;   // Tn x Tn inputs
    input [(BIT_WIDTH - 1):0] i_nbout;     // Tn partial sums
    input i_nbout_en;
    
    //----------- Output Ports ---------------//
    output [(BIT_WIDTH - 1):0] o_results;
    
    //----------- Internal Signals ---------------//
    
    // Adder tree connection wires
    wire [ (BIT_WIDTH*(Tn/2)) - 1 : 0 ] level_1_out;
    wire [ (BIT_WIDTH*(Tn/4)) - 1 : 0 ] level_2_out;
    wire [ (BIT_WIDTH*(Tn/8)) - 1 : 0 ] level_3_out;
    wire [ (BIT_WIDTH*(Tn/16)) - 1 : 0 ] level_4_out;
    
    wire [ (BIT_WIDTH-1) : 0 ] partial_sum_out;
    
    //------------- Code Start -----------------//
    
    // Level 1 - 8 adders (Tn/2 = 16/2 = 8)
    genvar j;
    generate
        for(j=0; j<(Tn/2); j=j+1) begin : Level1_i
            qadd #(.Q(10), .N(16)) L1 (
                    i_nfu1[ (((2*j)+1)*BIT_WIDTH) - 1 : ((2*j)*BIT_WIDTH)   ],
                    i_nfu1[ (((2*j)+2)*BIT_WIDTH) - 1 :(((2*j)+1)*BIT_WIDTH)  ],
                    level_1_out[ ((j+1)*BIT_WIDTH) - 1: (j*BIT_WIDTH) ]
            );
        end
    endgenerate
    
    // Level 2 - 4 adders (Tn/4 = 16/4 = 4)
    generate
        for(j=0; j<(Tn/4); j=j+1) begin : Level2_i
            qadd #(.Q(10), .N(16)) L2 (
                    level_1_out[ (((2*j)+1)*BIT_WIDTH) - 1 : ((2*j)*BIT_WIDTH)   ],
                    level_1_out[ (((2*j)+2)*BIT_WIDTH) - 1 : (((2*j)+1)*BIT_WIDTH)  ],
                    level_2_out[ ((j+1)*BIT_WIDTH) - 1 : (j*BIT_WIDTH) ]
            );
        end
    endgenerate
    
    
    // Level 3 - 2 adders (Tn/8 = 16/8 = 2)
    generate
        for(j=0; j<(Tn/8); j=j+1) begin : Level3_i
            qadd #(.Q(10), .N(16)) L3 (
                    level_2_out[ (((2*j)+1)*BIT_WIDTH) - 1 : ((2*j)*BIT_WIDTH)   ],
                    level_2_out[ (((2*j)+2)*BIT_WIDTH) - 1 : (((2*j)+1)*BIT_WIDTH)  ],
                    level_3_out[ ((j+1)*BIT_WIDTH) - 1 : (j*BIT_WIDTH) ]
            );
        end
    endgenerate
    
    // Level 4 - 1 adders (Tn/16 = 16/16 = 1)
    qadd #(.Q(10), .N(16)) L4 (
        level_3_out[ BIT_WIDTH - 1 : 0 ],
        level_3_out[ (2*BIT_WIDTH) - 1 : BIT_WIDTH  ],
        level_4_out
    );
    
    // Level 5 - Not sure if this is correct, but there needs to be an adder for the partial sum.
    //           DianNao paper only mentions 16 x 15 adders in the tree. But since adders are only
    //           2 input, we have Tn (16) values per neuron to add (15 adders) plus the partial sum (16 adders)
    qadd #(.Q(10), .N(16)) L5 (
        level_4_out,
        i_nbout,
        partial_sum_out
    );

    assign o_results = partial_sum_out;
    
endmodule // End module Tn_adder_tree



//---------------------------------------------//
// Main NFU-2 module
//---------------------------------------------//
module nfu_2 (
        clk,
        i_nfu1_out,
        i_nbout,
        i_nbout_en,
        o_nfu2_out
    );

    parameter BIT_WIDTH = 16;
    parameter Tn = 16;
    parameter TnxTn = 256;

    //----------- Input Ports ---------------//
    input clk;
    input i_nbout_en;
    
    input [ ((BIT_WIDTH*TnxTn) - 1) : 0 ] i_nfu1_out;
    input [ ((BIT_WIDTH*Tn) - 1) : 0 ] i_nbout;
    
    //----------- Output Ports ---------------//
    output [ ((BIT_WIDTH*Tn) - 1) : 0 ] o_nfu2_out;
    
    //------------- Code Start -----------------//
    genvar i;
    generate
        for(i=0; i<Tn; i=i+1) begin : ADDER_TREES
            Tn_adder_tree T (
                i_nfu1_out[ ((i+1)*Tn*BIT_WIDTH) - 1  : (i*Tn*BIT_WIDTH)  ],
                i_nbout[ ((i+1)*BIT_WIDTH) - 1  : (i*BIT_WIDTH) ],
                i_nbout_en,
                o_nfu2_out [ ((i+1)*BIT_WIDTH) - 1  : (i*BIT_WIDTH) ]
            );
        end
    endgenerate
    
endmodule // End module nfu_2

