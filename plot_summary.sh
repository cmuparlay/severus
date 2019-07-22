mode=$1
name=$2

fix_close_paren () {
    sed -E "s/, (.)$/\1/g"
}

make numa_configure | grep -v -E "Nothing to be done|is up to date" 1>&2

ns=$(cat "${name}.txt" | grep numAttempt | sed -E "s/^.*\"worker\" -> ([0-9]+).*\"numAttempt\" -> ([0-9]+).*\"numSuccess\" -> ([0-9]+).*$/\1,\2,\3/g")
echo "$(for n in ${ns}; do
    IFS=,
    m=(${n})
    echo $(./numa_configure ${m[0]}) ${m[1]} ${m[2]}
done)" | sort -n > "${name}.dat"
gnuplot -e "$(./numa_configure -1 | fix_close_paren); name=\"${name}\"" summary_${mode}.plt
