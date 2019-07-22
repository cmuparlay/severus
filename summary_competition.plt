set term pdf
set boxwidth 0.5
set style fill solid 0.8
set yrange [0:*]
set output name."-numAttempt.pdf"
plot name.".dat" using 1:2 with boxes title "CAS attempts"
set output name."-numSuccess.pdf"
plot name.".dat" using 1:3 with boxes title "CAS successes"
set output name."-successRatio.pdf"
plot name.".dat" using 1:($3/$2) with boxes title "CAS success ratio"
