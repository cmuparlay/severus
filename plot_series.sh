name_out=$1
shift

if [ -z "$1" ]; then
    names_in="${name_out}"
    mode="single"
elif [ "$1" == "--nodes" ]; then
    names_in="${name_out}"
    shift
    nodes="$@"
    mode="nodes"
else
    names_in="$@"
    mode="multiple"
fi

gnuplot -e "names_in=\"${names_in}\"; name_out=\"${name_out}\"; nodes=\"${nodes}\"" series_${mode}.plt
