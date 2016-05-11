
#
#
#			Delta                                                                   Init                                                                    Total
#			comp               messages         hellos         packets              comp             msgs              hellos          packets               comp              msgs            hellos              packets
# avg std      483.8  631.9      863.3   50.1     2119.6   78.4  1087247.9  907.9    64680.0    0.0     1806.2    5.9    16583.8  191.5  4066878.0 20722.1   286935.0 1446.1     7705.6   82.9    74696.7  452.6    16213.8 13895.7
# avg std     1056.5 1372.2      867.5   48.6     2207.9  161.9  2140244.0 16960.3   126865.2 1013.7     1802.8    7.0    24843.1  230.9  7921816.0 87401.1   563762.4 6153.7     7685.2   84.5   112153.1  838.6    37028.0 39935.3
# avg std     1435.9 1314.2      861.8   52.1     2226.9  170.0  3232865.2 21455.1   187695.2 1145.5     1805.0    5.3    33834.8  395.5 11770786.7 107967.8   835415.5 7927.5     7696.6   86.8   153103.8 1410.4    48235.0 43551.1
# avg std     1842.0 1866.7      859.7   63.7     2282.2  211.2  4348905.2 28284.6   246661.8 1506.5     1805.1    6.3    41966.9  236.7 15528534.6 117664.5  1096574.3 8124.4     7671.8  100.4   189409.8 1235.2    64329.3 63560.3
# avg std     2620.5 2813.8      858.9   55.7     2343.6  290.6  5450632.7 50025.6   302271.2 2524.8     1807.5    3.9    49735.2  471.6 19156315.4 243597.0  1344004.6 16795.5     7686.9   92.8   224237.5 2691.0    96798.4 97747.7


# Need to have GDFONTPATH set to "/usr/share/fonts/bitstream-vera"
# do not use truecolor option to png.  Causes bad graphs.
#set terminal png nocrop enhanced font "Vera,7"  size 500,750
#set terminal png nocrop enhanced font "Helvetica,20" size 1600,1200
#set terminal postscript enhanced color portrait 12
set terminal pdf enhanced color font "Helvetica,10"
set pointsize 1.5

set key outside top
set grid xtics ytics

set style line 1 lt 1 lw 2 lc 1 pt 1
set style line 2 lt 1 lw 2 lc 2 pt 2
set style line 3 lt 1 lw 2 lc 3 pt 12
set style line 4 lt 1 lw 2 lc 4 pt 12
set style line 5 lt 1 lw 2 lc 5 pt 4
set style line 6 lt 1 lw 2 lc 6 pt 4
set style line 7 lt 0 lw 2 lc 7 pt 4
set style line 8 lt 0 lw 2 lc 8 pt 1
set style line 9 lt 0 lw 2 lc 9 pt 2
set style line 10 lt 0 lw 2 lc 10
set style line 11 lt 0 lw 2 lc 11
set style line 12 lt 0 lw 2 lc 12
set style line 13 lt 1 lw 2 lc 0

set xrange[0:6]
set xtics ("1" 1, "2" 2, "3" 3, "4" 4, "5" 5)

set xlabel 'Number of Replicas'
set key on
set ytics autofreq

#set encoding utf8

###################################################################
set output 'output/cost-init.pdf'
set title "Computational Cost (Initialization)"
set ylabel 'Computational Cost'
set yrange[0:*]
set format y '%.0sx10^{%S}'

plot 'output/link_failure.dat' using 1:12 title 'NFP' with lp ls 1

###################################################################
set output 'output/cost-delta.pdf'
set title "Computational Cost (Link Failure)"
set ylabel 'Computational Cost'
set yrange[0:*]
set format y '%.0sx10^{%S}'

plot 'output/link_failure.dat' using 1:4 title 'NFP' with lp ls 1

###################################################################
set output 'output/messages-init.pdf'
set title "Messages (Initialization)"
set ylabel 'Messages'
set yrange[0:*]
set format y '%.0sx10^{%S}'

plot 'output/link_failure.dat' using 1:12 title 'NFP' with lp ls 1

###################################################################
set output 'output/messages-delta.pdf'
set title "Messages (Link Failure)"
set ylabel 'Messages'
set yrange[0:*]
set format y '%.0sx10^{%S}'
#set format y '%.0f'

plot 'output/link_failure.dat' using 1:6 title 'NFP' with lp ls 1

