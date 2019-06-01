#!/bin/bash
CUR_DIR="."
TMP_DIR="./tmp"

declare -A DIR_MAP=( ["./test"]=1 ["./zmq_ops"]=1)

create_tmp() {
    if [ -d "$TMP_DIR" ]; then
        rm -rf $TMP_DIR
    else
        mkdir ${TMP_DIR}
    fi
}

format_files() {
    # echo "format dir $1"
    array=()
    while IFS=  read -r -d $'\0'; do
        array+=("$REPLY")
    done < <(find $1 -type f \( -iname \*.cc -o -name \*.h \) -print0)
    reg="^(.+\/)*(.*?)$"
    for file in ${array[@]};
    do
        if [[ $file =~ $reg ]]; then
            formated_file="$TMP_DIR/${BASH_REMATCH[2]}"
            echo "$file"
            clang-format -style=file $file > $formated_file
            cp $formated_file $file
        fi
    done
}

search_dir() {
    for dir in "$1"/*;
    do 
    if [ -d "$dir" ]; then 
        if [[ ! -z "${DIR_MAP[$dir]+unset}" ]]; then
            format_files "$dir"
            search_dir "$dir"
        fi
    fi
    done
}

del_tmp() {
    if [ -d "$TMP_DIR" ]; then
        rm -rf $TMP_DIR
    fi
}


create_tmp;
{
    search_dir "$CUR_DIR";
}
del_tmp

