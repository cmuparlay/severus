set term pdf
set boxwidth 0.5
set style fill solid 0.8
set yrange [0:*]
set output name."-numOperation.pdf"
plot name.".dat" using 1:5 with boxes title "Fetch-and-Add operations"
