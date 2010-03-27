set grid; 
set title "Source Code Statistics"
set xdata time
set timefmt "%Y-%m-%d"
set xrange ["2005-04-16":"2005-05-15"]
set format x "%Y-%m-%d"
set ylabel "SLOC[-]"
plot "src_stats.dat" using 1:2 title 'Actual framework' w lines lw 2, \
    "test_stats.dat" using 1:2 title 'Test cases' w lines lw 2

