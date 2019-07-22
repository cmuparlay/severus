DIR=$1
# For now, let's just do one trial.
# TRIALS=$2
TIME_SERIES=$3
if [ -z "${DIR}" ]; then
    DIR="./output"
fi
if [ -z "${TRIALS}" ]; then
    TRIALS=1
fi
if [ -z "${TIME_SERIES}" ]; then
    TIME_SERIES=2
fi

# opts_series="--time 10 --wait 256"
opts_series="--time ${TIME_SERIES} --wait 256"

make numa_configure | grep -v -E "Nothing to be done|is up to date" 1>&2
num_nodes=$(./numa_configure | grep "#define NUM_NODES " | sed -E "s/.* ([0-9]+)$/\1/g")
mkdir -p "${DIR}"

#Throughput experiments, all nodes
for ((TRIAL=0;TRIAL<TRIALS;TRIAL++)); do
	for ((NODE=0;NODE<num_nodes;NODE++)); do
        name="${DIR}/Throughput${NODE}-${TRIAL}"
    	./go.sh sequence "${name}" --mapping ${NODE}
    done
done

#Throughput experiments, single nodes
for ((TRIAL=0;TRIAL<TRIALS;TRIAL++)); do
	for ((NODE=0;NODE<num_nodes;NODE++)); do
        name="${DIR}/ThroughputSingle${NODE}-${TRIAL}"
    	./go.sh sequence "${name}" --nodes ${NODE}
	done
done

#Fairness experiments, all nodes
for ((TRIAL=0;TRIAL<TRIALS;TRIAL++)); do
    name="${DIR}/Fairness-${TRIAL}"
    ./go.sh series "${name}" ${opts_series}
done


if [ $num_nodes -eq 8 ]; then
	#Focus-AMD experiments, single nodes. Should do this but with mapping to get interconnect traffic
	for ((TRIAL=0;TRIAL<TRIALS;TRIAL++)); do
        name0="${DIR}/Focus-AMD-d0-${TRIAL}"
        name1="${DIR}/Focus-AMD-d1-${TRIAL}"
        name2="${DIR}/Focus-AMD-d2-${TRIAL}"
		./go.sh --no-plot series "${name0}" ${opts_series} --mapping 0 1 2 3 4 5 6 7
		./go.sh --no-plot series "${name1}" ${opts_series} --mapping 1 0 3 2 5 4 7 6
		./go.sh --no-plot series "${name2}" ${opts_series} --mapping 3 2 1 0 7 6 5 4
        nameAll="${DIR}/Focus-AMD-${TRIAL}"
        ./plot_series.sh "${nameAll}" "${name0}" "${name1}" "${name2}"
	done
elif [ $num_nodes -eq 4 ]; then
	#Focus-Intel experiments, single nodes.
	for ((TRIAL=0;TRIAL<TRIALS;TRIAL++)); do
        name0="${DIR}/Focus-Intel-Split0-${TRIAL}"
        name1="${DIR}/Focus-Intel-Split1-${TRIAL}"
        name2="${DIR}/Focus-Intel-Grouped0-${TRIAL}"
        name3="${DIR}/Focus-Intel-Grouped1-${TRIAL}"
		./go.sh --no-plot series "${name0}" ${opts_series} --mapping 0 1 2 3 --mapping2 1 0 3 2
		./go.sh --no-plot series "${name1}" ${opts_series} --mapping 3 2 1 0 --mapping2 1 0 3 2
		./go.sh --no-plot series "${name2}" ${opts_series} --mapping 0 1 2 3
		./go.sh --no-plot series "${name3}" ${opts_series} --mapping 1 0 3 2
        nameAll="${DIR}/Focus-Intel-${TRIAL}"
        ./plot_series.sh "${nameAll}" "${name0}" "${name1}" "${name2}" "${name3}"
	done
fi
