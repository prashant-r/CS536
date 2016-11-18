set terminal png transparent nocrop enhanced size 450,320 font "arial,8" 
set output 'client_plot.png'
set   autoscale                        # scale axes automatically
unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set xtic auto                          # set xtics automatically
set ytic auto                          # set ytics automatically
set title "Q(t) vs  Time"
set ylabel "Q(t) (B)"
set xlabel "Time (Seconds)"
plot "client.dat" w lp


set terminal png transparent nocrop enhanced size 450,320 font "arial,8" 
set output 'server_plot.png'
set   autoscale                        # scale axes automatically
unset log                              # remove any log-scaling
unset label                            # remove any previous labels
set xtic auto                          # set xtics automatically
set ytic auto                          # set ytics automatically
set title "Lambda vs  Time"
set ylabel "Lambda (ms)"
set xlabel "Time (Seconds)"
plot "server.dat" w lp
