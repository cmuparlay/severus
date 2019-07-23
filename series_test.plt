names_in="output/srs"
name_out="output/srs"
nodes="0 1"
set term pdf
set yrange [0:*]
set output name_out."-numAttempt.pdf"
plot for [node=0:1] names_in.".dat" using 1:(column(2 * int(node) + 4)) with lines lw 6 title "CAS attempts (node ".node.")"
set output name_out."-numSuccess.pdf"
plot for [node in nodes] names_in.".dat" using 1:(column(2 * int(node) + 5)) with lines lw 6 title "CAS attempts (node ".node.")"
set output name_out."-successRatio.pdf"
plot for [node in nodes] names_in.".dat" using 1:(column(2 * int(node) + 5) / column(2 * int(node) + 4)) with lines lw 6 title "CAS attempts (node ".node.")"
