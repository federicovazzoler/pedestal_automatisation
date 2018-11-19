#!/bin/bash

######################################################################
# Creates a files list of all the files in the DataPATH.             #
# Compares the new list with the old one generating a new files list #
# with the new files.                                                # 
######################################################################

outFilelistPATH=$1
inDataPATH=$2
outLogFile=$3
scriptsPATH=$4
todayFile_List=$5
newFile_List=$6

# files present in dataPATH yesterday
# check the last edited file
unset -v latest
for file in "$outFilelistPATH"/*; do
    [[ $file -nt $latest ]] && latest=$file
done
yesterdayFile_List="$latest"
# check if it is empty
if [ "$(wc $yesterdayFile_List | awk '{print $1}')" = 0 ]; then
    echo "[WARNING] file list of yesterday: $yesterdayFile_List was empty!" >> $outLogFile
    echo "" >> $yesterdayFile_List #NOT NEEDED?
fi

# all files in dataPATH now
find $inDataPATH/ -type f -mmin +59 | $scriptsPATH/utility/file_list_sort.sh | sed "s;^;$inDataPATH\/;" > $todayFile_List

# new files to be processed
diff $todayFile_List $yesterdayFile_List | grep \< | sed -e 's/< //' > $newFile_List
if [ "$(wc $newFile_List | awk '{print $1}')" = 0 ]; then
    echo "[ERROR] list of new files to be processed: $newFile_List is empty!" >> $outLogFile
    rm $newFile_List
    exit 1
fi
