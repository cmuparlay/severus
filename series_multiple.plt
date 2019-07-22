set term pdf
set yrange [0:*]
set output name_out."-numAttempt.pdf"
plot for [name in names_in] name.".dat" using 1:($3/$2) with lines lw 6 title "CAS attempts (".name.")"
set output name_out."-numSuccess.pdf"
plot for [name in names_in] name.".dat" using 1:($3/$2) with lines lw 6 title "CAS successes (".name.")"
set output name_out."-successRatio.pdf"
plot for [name in names_in] name.".dat" using 1:($3/$2) with lines lw 6 title "CAS success ratio (".name.")"
