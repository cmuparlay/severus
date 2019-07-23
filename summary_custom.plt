set term pdf
set boxwidth 0.5
set style fill solid 0.8
set yrange [0:*]
set output name."-numAttempt.pdf"
plot name.".dat" using 1:5 with boxes title "Operation attempts"
set output name."-numSuccess.pdf"
plot name.".dat" using 1:6 with boxes title "Operation successes"
set output name."-successRatio.pdf"
plot name.".dat" using 1:($6/$5) with boxes title "Operation success ratio"
