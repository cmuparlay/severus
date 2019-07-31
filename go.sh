echoerr() { echo "$@" 1>&2; }
echo_help() {
    echoerr "Choose one of the following modes:"
    echoerr "  sequence, competition, custom, series."
    echoerr
    echoerr "Run './go.sh MODE --help' or './go.sh NAME MODE --help' to see the"
    echoerr "  possible options for MODE."
    echoerr "  Note: series mode does not support --atom."
    echoerr "  (It runs a series of experiments with various atomic delays.)"
    echoerr
    echoerr "Run './go.sh MODE NAME OPTS' to run an experiment and store the"
    echoerr "  results in NAME.txt, then generate plots NAME-*.pdf."
    echoerr "  (NAME has to be the first argument after MODE.)"
    echoerr
    echoerr "Run './go.sh --no-plot MODE NAME OPTS' to omit the plots."
    echoerr "  (--no-plot has to be before MODE.)"
    echoerr
    echoerr "Example: try './go.sh competition cmp --mapping 1 --wait 256 --atom 12'."
}

go_one() {
    mode=$1
    shift
    name=$1
    shift

    make ${mode} | grep -v -E "Nothing to be done|is up to date" 1>&2
    echoerr "${mode}: $@"
    if [ -s "${name}.txt" ]; then
        echoerr "Reusing ${name}.txt"
    else
       ./main_${mode} $@ > "${name}.txt" || exit 1
    fi

    if [ ${plot} -eq 1 ]; then
        if [ -x "$(command -v gnuplot)" ]; then
            ./plot_summary.sh ${mode} "${name}"
        else
            echoerr "failed to generate plot (gnuplot not found)"
        fi
    fi
}

go_series() {
    mode=$1
    shift
    name=$1
    shift

    make ${mode} | grep -v -E "Nothing to be done|is up to date" 1>&2
    echoerr "series: $@"

    if [ -s "${name}.dat" ]; then
        echoerr "Reusing ${name}.dat"
    else
        echoerr "running with atomic delays {$(echo {0..32..2} {36..64..4})}"
        for atom in {0..32..2} {36..64..4}; do
            echoerr "running atomic delay ${atom}"
            ./main_${mode} $@ --atom "${atom}" | ./aggregate_log.sh "${atom}"
        done > "${name}.dat"
    fi

    if [ ${plot} -eq 1 ]; then
        if [ -x "$(command -v gnuplot)" ]; then
            ./plot_series.sh "${name}"
        else
            echoerr "failed to generate plot (gnuplot not found)"
        fi
    fi
}

case $1 in
    --no-plot | --no-plots | --noplot | --noplots | -no-plot | -no-plots | -noplot | -noplots )
        plot=0
        shift
        ;;
    * )
        plot=1
        ;;
esac

script=go_one
mode=$1
shift
case ${mode} in
    sequence )
    ;;
    competition )
    ;;
    custom )
    ;;
    series )
        mode=competition
        script=go_series
        ;;
    help | -h | -help | --help )
        echo_help
        exit 1
        ;;
    * )
        echoerr "Invalid mode: ${mode}"
        echo_help
        exit 1
        ;;
esac
name=$1
shift
case ${name} in
    help | -h | -help | --help )
        make ${mode} | grep -v -E "Nothing to be done|is up to date" 1>&2
        ./main_${mode} --help
        exit 1
        ;;
esac

${script} "${mode}" "${name}" $@
