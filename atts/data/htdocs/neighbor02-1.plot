set terminal png font "/usr/share/fonts/truetype/msttcorefonts/Arial.ttf" 10 size 1024,320
set ylabel "byte per second"
set xlabel "file size"
set output "data/htdocs/neighbor02-1.png"
plot 'data/htdocs/neighbor02-1.dat' title 'Throughput' with linespoints
