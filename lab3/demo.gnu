set terminal png transparent nocrop enhanced size 450,320 font "arial,8" 
set output 'throughput-VS-BlockSize.png'
set   autoscale                        # scale axes automatically
unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set xtic auto                          # set xtics automatically
set ytic auto                          # set ytics automatically
set title "Throughput vs Blocksize graph for a file server-client pair"
set ylabel "Throughput (MBps)"
set xlabel "Block Size (Bytes)"
plot "throughput.dat" w lp