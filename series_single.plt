set term pdf
set yrange [0:*]
set output name_out."-numAttempt.pdf"
plot names_in.".dat" using 1:2 with lines lw 6 title "CAS attempts (".names_in.")"
set output name_out."-numSuccess.pdf"
plot names_in.".dat" using 1:3 with lines lw 6 title "CAS successes (".names_in.")"
set output name_out."-successRatio.pdf"
plot names_in.".dat" using 1:($3/$2) with lines lw 6 title "CAS success ratio (".names_in.")"
