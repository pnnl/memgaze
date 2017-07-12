#!/usr/bin/env ruby
class Matmult_seqbaij_1_0_0
	def evalModel(funcs, loop1_its: 80000.0, loop1_1_its: 6.86, loop1_1_seg_0_unroll: 1)
		base_entry_0_0 = (0)/1.0 #cycles due to memop => 0, blks: 0->
		base_entry_0_1 = (0)/1.0 #cycles due to memop => 0, blks: 1->4->
		base_entry_0_2 = (0)/1.0 #cycles due to memop => 0, blks: 5->8->12->13->14->
		base_exit_0_0 = (0)/1.0 #cycles due to memop => 0, blks: 28->
		base_exit_0_1 = (0)/1.0 #cycles due to memop => 0, blks: 29->32->
		base_exit_0_2 = (0)/1.0 #cycles due to memop => 0, blks: 33->36->37->
		loop1_1_seg_0_vals = { 1 => 0, 2 => 0, 3 => 0, 4 => 0, 5 => 0, 6 => 0, 7 => 0, 8 => 0, 9 => 0, 10 => 0} #21->
		loop1_1_seg_0 = (loop1_1_seg_0_vals[loop1_1_seg_0_unroll]/loop1_1_seg_0_unroll) #cycles due to memop => 0, blks: 21->
		loop1_entry_0 = (0)*1.0/1.0 #cycles due to memop => 0, blks: 15->16->17->18->19->20->
		loop1_exit_0 = (0)*1.0/1.0 #cycles due to memop => 0, blks: 22->24->26->27->
		func_call_2=funcs["vecrestorearrayread"]
		func_call_3=funcs["vecrestorearray"]
		base_exit_0=(base_exit_0_0 + func_call_2 + base_exit_0_1 + func_call_3 + base_exit_0_2)*1.0
		func_call_0=funcs["vecgetarrayread"]
		func_call_1=funcs["vecgetarray"]
		base_entry_0=(base_entry_0_0 + func_call_0 + base_entry_0_1 + func_call_1 + base_entry_0_2)*1.0
		loop1_1_out_0=(loop1_1_seg_0)*1.0
		loop1_1=loop1_1_its*loop1_1_out_0
		loop1_path_0=(loop1_entry_0 + loop1_1 + loop1_exit_0)*1.0
		loop1_out_0=(loop1_entry_0 + loop1_1 + loop1_exit_0)*1.0
		loop1=loop1_its*(loop1_path_0) + loop1_out_0
		base_out_0=(base_entry_0 + loop1 + base_exit_0)*1.0
		return base_out_0
	end
