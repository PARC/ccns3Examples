#!/usr/bin/awk -f
#
# Copyright (c) 2014-2016, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Patent rights are not granted under this agreement. Patent rights are
#       available under FRAND terms.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL XEROX or PARC BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# @author Marc Mosko, Palo Alto Research Center (PARC)
# @copyright 2014-2015, Xerox Corporation (Xerox)and Palo Alto Research Center (PARC).  All rights reserved.
#

# Compute the mean and stddev of each column
# Delta comp 22488 msgs 818 hellos 752 pkts 2125 Init comp 1086284 msgs 64680 hellos 1808 pkts 16374 Total comp 4052127 msgs 286332 hellos 7498 pkts 74146
#
# Columns: 3, 5, 7, 9, 12, 14, 16, 18, 21, 23, 25, 27

BEGIN {
	column_array[0] = 3;
	column_array[1] = 5;
	column_array[2] = 7;
	column_array[3] = 9;

	column_array[4] = 12;
	column_array[5] = 14;
	column_array[6] = 16;
	column_array[7] = 18;

	column_array[8] = 21;
	column_array[9] = 23;
	column_array[10]= 25;
	column_array[11]= 27;
	column_count = 12

	row_count = 0
	for (i in column_array) {
		column_sum[i] = 0.0;
		column_sum2[i] = 0.0;
	}
}

{
	row_count++;
	for (i in column_array) {
		value = $column_array[i]
		#printf("i = %d, value = %d\n", column_array[i], value);
		column_sum[i] += value;
		column_sum2[i] += value * value;
	}
}

END {
	
	for (i in column_array) {
		column_avg[i] = 0.0;
		column_std[i] = 0.0;
		if (row_count > 0) {	
			column_avg[i] = column_sum[i] / row_count;
	
			if (row_count > 1) {
				x = column_sum2[i] / row_count - column_avg[i] * column_avg[i];
				column_std[i] = sqrt( row_count / (row_count - 1)) * sqrt( x );
			}
		}
	}


	printf("avg std");
	for (i=0; i < column_count; ++i) {
		printf(" %10.1f", column_avg[i]);
		printf(" %6.1f", column_std[i]);
	}
	printf("\n");

}

