# For more accurate plots, increase these times (e.g. to 10).
# If you are getting segfaults, decrease them (e.g. to 0.5).
TIME_SEQUENCE=2
TIME_SERIES=2

# Increase TRIALS to run more trials.
# But the trials are not combined in the plots: each trial gets its own plot.
TRIALS=1

echoerr() { echo "$@" 1>&2; }
echo_help() {
    echoerr "Choose one of the following modes:"
    echoerr "  intel, amd, easy, mapping"
    echoerr "  (If in doubt, start with easy mode.)"
    echoerr
    echoerr "Run './paper.sh --output DIR MODE [ARGS]' to output to DIR."
    echoerr "  (--output DIR has to come before MODE.)"
    echoerr "  Default: output directory is './output'."
    echoerr "  Note: this script avoids rerunning experiments if it sees"
    echoerr "  relevant files in DIR, running only what is needed."
    echoerr "  To rerun everything, choose a nonexistent or empty directory."
    echoerr
    echoerr "Run './paper.sh intel' or './paper.sh amd' to replicate all the"
    echoerr "  experiments from the paper for that machine."
    echoerr "  Note: intel needs 4 NUMA nodes, amd needs 8 NUMA nodes."
    echoerr
    echoerr "Run './paper.sh easy' to replicate most of the experiments"
    echoerr "  from the paper, omitting the focus experiments."
    echoerr
    echoerr "Run './paper.sh mapping MAP' to replicate most of the experiments"
    echoerr "  from the paper, with focus experiments adapted to your machine."
    echoerr "  MAP should be a list mapping each node to some other node."
    echoerr "  It specifies where each node's cores will access during the"
    echoerr "  experiment."
    echoerr "  Example: './paper.sh mapping 1 2 3 0' has node 0 accessing data"
    echoerr "  on node 1, node 1 accessing node 2, etc."
    echoerr "  Consider permuting the nodes such that each accesses a node"
    echoerr "  that is one or two hops away."
    echoerr "  (Use 'numactl -H' to learn the node layout on your machine.)"
}

make_quiet() {
    make $1 | grep -v -E "Nothing to be done|is up to date" 1>&2
}

case $1 in
    --output | -output )
        shift
        DIR=$1
        shift
        ;;
    * )
        DIR="./output"
esac

mode=$1
shift
case ${mode} in
    intel | -intel | --intel )
        mode=intel
        ;;
    amd | -amd | --amd )
        mode=amd
        ;;
    easy | -easy | --easy )
        mode=easy
        ;;
    mapping | -mapping | --mapping )
        mode=mapping
        mapping=($@)
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
echoerr "Mode: ${mode} ${mapping[@]}"

make_quiet numa_configuration.hpp
num_nodes=$(sed -nE "s/^#define NUM_NODES ([0-9]+)$/\1/p" numa_configuration.hpp)

case ${mode} in
    intel )
        if [ ! ${num_nodes} -eq 4 ]; then
            echoerr "Error: intel mode requires exactly 4 NUMA nodes"
            exit 1
        fi
        ;;
    amd )
        if [ ! ${num_nodes} -eq 8 ]; then
            echoerr "Error: amd mode requires exactly 8 NUMA nodes"
            exit 1
        fi
        ;;
    mapping )
        if [ ! ${num_nodes} -eq ${#mapping[@]} ]; then
            echoerr "Error: the mapping should have length ${num_nodes}, one entry per NUMA node"
            exit 1
        fi
esac

mkdir -p "${DIR}"
mkdir -p "${DIR}/sequence"
mkdir -p "${DIR}/fairness"
mkdir -p "${DIR}/focus"

opts_sequence="--time ${TIME_SEQUENCE}"
opts_series="--time ${TIME_SERIES} --wait 256"

#Sequence experiments, all nodes
for ((TRIAL=0;TRIAL<TRIALS;TRIAL++)); do
	for ((NODE=0;NODE<num_nodes;NODE++)); do
        name="${DIR}/sequence/SequenceAll${NODE}-${TRIAL}"
    	./go.sh sequence "${name}" ${opts_sequence} --mapping ${NODE}
    done
done

#Sequence experiments, single nodes
for ((TRIAL=0;TRIAL<TRIALS;TRIAL++)); do
	for ((NODE=0;NODE<num_nodes;NODE++)); do
        name="${DIR}/sequence/SequenceSingle${NODE}-${TRIAL}"
    	./go.sh sequence "${name}" ${opts_sequence} --nodes ${NODE}
	done
done

all_nodes="$(seq 0 $((num_nodes - 1)))"

case ${mode} in
    intel )
        fairness_nodes="0 3"
        ;;
    amd )
        fairness_nodes="0 4 7"
        ;;
    * )
        fairness_nodes="${all_nodes}"
esac

#Fairness experiments, all nodes
for ((TRIAL=0;TRIAL<TRIALS;TRIAL++)); do
    name="${DIR}/fairness/Fairness-${TRIAL}"
    ./go.sh --no-plot series "${name}" ${opts_series}
    ./plot_series.sh "${name}" --nodes ${fairness_nodes}
done

case ${mode} in
    amd )
	    #Focus-AMD experiments.
	    for ((TRIAL=0;TRIAL<TRIALS;TRIAL++)); do
            name0="${DIR}/focus/Focus-AMD-d0-${TRIAL}"
            name1="${DIR}/focus/Focus-AMD-d1-${TRIAL}"
            name2="${DIR}/focus/Focus-AMD-d2-${TRIAL}"
		    ./go.sh --no-plot series "${name0}" ${opts_series} --mapping 0 1 2 3 4 5 6 7
		    ./go.sh --no-plot series "${name1}" ${opts_series} --mapping 1 0 3 2 5 4 7 6
		    ./go.sh --no-plot series "${name2}" ${opts_series} --mapping 3 2 1 0 7 6 5 4
            nameAll="${DIR}/focus/Focus-AMD-${TRIAL}"
            ./plot_series.sh "${nameAll}" "${name0}" "${name1}" "${name2}"
	    done
        ;;
    intel )
	    #Focus-Intel experiments.
	    for ((TRIAL=0;TRIAL<TRIALS;TRIAL++)); do
            name0="${DIR}/focus/Focus-Intel-Split0-${TRIAL}"
            name1="${DIR}/focus/Focus-Intel-Split1-${TRIAL}"
            name2="${DIR}/focus/Focus-Intel-Grouped0-${TRIAL}"
            name3="${DIR}/focus/Focus-Intel-Grouped1-${TRIAL}"
		    ./go.sh --no-plot series "${name0}" ${opts_series} --mapping 0 1 2 3 --mapping2 1 0 3 2
		    ./go.sh --no-plot series "${name1}" ${opts_series} --mapping 3 2 1 0 --mapping2 1 0 3 2
		    ./go.sh --no-plot series "${name2}" ${opts_series} --mapping 0 1 2 3
		    ./go.sh --no-plot series "${name3}" ${opts_series} --mapping 1 0 3 2
            nameAll="${DIR}/focus/Focus-Intel-${TRIAL}"
            ./plot_series.sh "${nameAll}" "${name0}" "${name1}" "${name2}" "${name3}"
	    done
        ;;
    mapping )
	    for ((TRIAL=0;TRIAL<TRIALS;TRIAL++)); do
            name0="${DIR}/focus/Focus-Local-${TRIAL}"
            name1="${DIR}/focus/Focus-Remote-${TRIAL}"
            name2="${DIR}/focus/Focus-Split-${TRIAL}"
		    ./go.sh --no-plot series "${name0}" ${opts_series} --mapping ${all_nodes}
		    ./go.sh --no-plot series "${name1}" ${opts_series} --mapping ${mapping[@]}
		    ./go.sh --no-plot series "${name2}" ${opts_series} --mapping ${all_nodes} --mapping2 ${mapping[@]}
            nameAll="${DIR}/focus/Focus-${TRIAL}"
            ./plot_series.sh "${nameAll}" "${name0}" "${name1}" "${name2}"
	    done
        ;;
esac
