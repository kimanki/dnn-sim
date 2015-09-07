#/**************************************************/
#/* Compile Script for Synopsys                    */
#/*                                                */
#/* dc_shell-t -f compile_dc.tcl                   */
#/*                                                */
#/* OSU FreePDK 45nm                               */
#/**************************************************/

#/* All verilog files, separated by spaces         */
set my_verilog_files [list ../common.v ../integer_ops/int_add.v ../fixed_point_ops/qtwosComp.v ../fixed_point_ops/qmult.v ../mult_piped.v ../nfu-1-pipe.v ../nfu-2-pipe.v ../nfu-3.v shifter.v rounder.v packer.v unpacker.v sb_unpacker.v nbin_unpacker.v nbout_packer.v ./proteus_top.v ]

#/* Top-level Module                               */
#set my_toplevel proteus_top_pipeline
set my_toplevel base_top_pipeline

#/* The name of the clock pin. If no clock-pin     */
#/* exists, pick anything                          */
set my_clock_pin clk

#/* Target frequency in MHz for optimization       */
set my_clk_freq_MHz 1000.0

#/* Delay of input signals (Clock-to-Q, Package etc.)  */
#set my_input_delay_ns 0.1

#/* Reserved time for output signals (Holdtime etc.)   */
#set my_output_delay_ns 0.1

set_host_options -max_cores 2 

#/**************************************************/
#/* No modifications needed below                  */
#/**************************************************/
set PDK_DIR /ubc/ece/home/ta/grads/taylerh/FreePDK45/FreePDK45/
set OSU_FREEPDK [format "%s%s"  $PDK_DIR "/osu_soc/lib/files"]
set search_path [concat  $search_path $OSU_FREEPDK]
set alib_library_analysis_path $OSU_FREEPDK

set link_library [set target_library [concat  [list gscl45nm.db] [list dw_foundation.sldb]]]
set target_library "gscl45nm.db"

define_design_lib WORK -path ./WORK

#set verilogout_show_unconnected_pins "true"

#analyze -format sverilog $my_verilog_files
analyze -format verilog $my_verilog_files

elaborate $my_toplevel

current_design $my_toplevel

link
uniquify


set my_period [expr 1000.0 / $my_clk_freq_MHz]

set find_clock [ find port [list $my_clock_pin] ]
if {  $find_clock != [list] } {
   set clk_name $my_clock_pin
   create_clock -period $my_period $clk_name
} else {
   set clk_name vclk
   create_clock -period $my_period -name $clk_name
}


puts -nonewline "Clk freq: "
puts -nonewline $my_clk_freq_MHz
puts " MHz"

puts -nonewline "Clk period: "
puts -nonewline $my_period
puts " ns"


set_switching_activity -static_probability 0.5 -toggle_rate 0.5 -base_clock $my_clock_pin i_inputs
set_switching_activity -static_probability 0.5 -toggle_rate 0.5 -base_clock $my_clock_pin i_synapses
set_switching_activity -static_probability 0.5 -toggle_rate 0.0078125 -base_clock $my_clock_pin i_nbout_to_nfu2
set_switching_activity -static_probability 0.015625 -toggle_rate 0.015625 -base_clock $my_clock_pin i_load_nbout
set_switching_activity -static_probability 0.015625 -toggle_rate 0.015625 -base_clock $my_clock_pin i_nbout_nfu2_nfu3

quit

compile_ultra -no_seq_output_inversion

check_design
report_constraint -all_violators

set filename [format "%s%s"  $my_toplevel ".vh"]
write -f verilog -output $filename

set filename [format "%s%s"  $my_toplevel ".sdc"]
write_sdc $filename

set filename [format "%s%s"  $my_toplevel ".db"]
write -f db -hier -output $filename -xg_force_db

redirect timing.rep { report_timing }
redirect cell.rep { report_cell }
redirect power.rep { report_power }

report_units

quit
