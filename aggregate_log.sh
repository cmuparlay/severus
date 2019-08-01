x=$1

ns=$(sed -nE "s/^.*\"worker\" -> ([0-9]+).*\"numAttempt\" -> ([0-9]+).*\"numSuccess\" -> ([0-9]+).*$/\1,\2,\3/p" <&0)
ns2=$(echo "$(for n in ${ns}; do
    IFS=,
    m=(${n})
    echo $(./numa_configure ${m[0]}) ${m[1]} ${m[2]}
done)" | sort -n | sed -E "s/ /,/g")
echo "$(for n in ${ns2}; do
    IFS=,
    m=(${n})
    data[$((2 * m[1]))]=$((data[2 * m[1]] + m[4]))
    data[$((2 * m[1] + 1))]=$((data[2 * m[1] + 1] + m[5]))
    numAttempt=$((numAttempt + m[4]))
    numSuccess=$((numSuccess + m[5]))
done
echo "${x} ${numAttempt} ${numSuccess} ${data[@]}")"
