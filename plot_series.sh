name_out=$1
shift

if [ -z "$2" ]; then
    names_in="${name_out}"
    mode="single"
else
    names_in="$@"
    mode="multiple"
fi

fix_close_paren () {
    sed -E "s/, (.)$/\1/g"
}

make numa_configure | grep -v -E "Nothing to be done|is up to date" 1>&2

gnuplot -e "names_in=\"${names_in}\"; name_out=\"${name_out}\"" series_${mode}.plt
