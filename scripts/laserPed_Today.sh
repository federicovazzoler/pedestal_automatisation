#!/bin/bash

####### Input data PATH #######
dstYear="2018"
inDataPATH="/eos/cms/store/group/dpg_ecal/alca_ecalcalib/laser/dst.merged/$dstYear"
###############################

####### DO NOT MODIFY BELOW THIS #######

# Some initializations #

# Output file name 
outFileNAME="laserPed$dstYear"

# Scripts PATH
scriptsPATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# The date of today
todayDate="$(date +%d).$(date +%m).$(date +%Y)"

# Output folder PATH
outFolderPATH=$scriptsPATH"/"$outFileNAME
mkdir -p $outFolderPATH
    ## Output metadata folder PATH
    outMetadataPATH="$outFolderPATH/metadata"
    mkdir -p $outMetadataPATH

    ## Output file list folder PATH
    outFilelistPATH="$outFolderPATH/filelist"
    mkdir -p $outFilelistPATH
    
    ## Output log folder PATH
    outLogfilePATH="$outFolderPATH/log"
    mkdir -p $outLogfilePATH
        
    ## Output tmp folder PATH
    tmpPATH="$outFolderPATH/tmp"
    mkdir -p $tmpPATH
    
    ## Tree folder PATH
    treePATH="$outFolderPATH/tree"
    mkdir -p $treePATH

# Calling the macros to fetch the metadata needed: #
# - new files to be processed                      #
# - runs and fills metadata                        #

# create the log file
outLogFile=$outLogfilePATH"/laserPed_"$todayDate".log"
touch $outLogFile
echo "Log file of $todayDate" >> $outLogFile
echo "" >> $outLogFile
echo "** BASH session:" >> $outLogFile
echo "" >> $outLogFile
echo "[INFO]: The script is running on $(hostname)" >> $outLogFile

# files already processed
filesAlreadyProcessed=$(find $outMetadataPATH -type f | grep "newFileList" | awk '{print "wc "$1}' | sh | awk '{ sum += $1 } END { print sum }')
if [ -z "$filesAlreadyProcessed" ]; then
    filesAlreadyProcessed=0;
fi
echo "[INFO]: Files already processed $filesAlreadyProcessed" >> $outLogFile

# file list of new files to be processed
newFile_List=$outMetadat\aPATH"/laserPed_"$todayDate"_newFileList.txt"
todayFile_List=$outFilelistPATH"/laserPed_"$todayDate"_FileList.txt"
#krenew -i -t $scriptsPATH/utility/generateFileList.sh $outFilelistPATH $inDataPATH $outLogFile $scriptsPATH $todayFile_List $newFile_List
$scriptsPATH/utility/generateFileList.sh $outFilelistPATH $inDataPATH $outLogFile $scriptsPATH $todayFile_List $newFile_List
if [ $? -ne 0 ]; then
    echo "[ERROR]: No input file list generated. Let us exit the main program." >> $outLogFile 
    exit 1
fi

# only because 1 file marc copied is corrupted
#sed -i '1d' $outMetadataPATH"/laserPed_"$todayDate"_newFileList.txt"                                                

# fetch the metadata
## Create a list of all new runs. Used to download the metadata for the runs and the fills
runList="$tmpPATH/runList.tmp"
cat $newFile_List | rev | sed -e 's;\.; ;g' | sed -e 's;\/; ;g' | awk '{print $6}' | sed -e 's;00tsd;;g' | rev | sort -u -n -k 1 > $runList
beginRun=$(awk 'NR==1{print}' $runList)
endRun=$(awk 'END{print}' $runList)
if [ $beginRun -eq 0 ] || [ $endRun -eq 0 ]; then
    echo "[ERROR]: Invalid run intervals. Impossible to fetch the metadata." >> $outLogFile
    exit 1
else
    echo "[INFO]: Fetching the metadata for runs in the interval: $beginRun - $endRun" >> $outLogFile
fi
numRunRunList=$(wc $runList | awk '{print $1}')

## RunSummary (metadata)
runSummary="$outMetadataPATH/runSummary_"$todayDate"_"$beginRun"_"$endRun".txt"
touch $runSummary
## FillSummary (metadata)
fillSummary="$outMetadataPATH/fillSummary_"$todayDate"_"$beginRun"_"$endRun".txt"
touch $fillSummary
## divide in bunches of approx 100 runs to avoid blanck donwloads
firstRun_wget=1
lastRun_wget=100
lastRunCycle_wget=$(( $numRunRunList - $numRunRunList % 100 ))
if [ "$(( $numRunRunList > 100 ))" ]; then
    for (( i = 1; i <= $lastRunCycle_wget; i+=100 )) 
    do 
        firstRun_wget=$(sed "${i}q;d" $runList)
        dummyForLastRun=$(( $i + 99 ))
        lastRun_wget=$(sed "${dummyForLastRun}q;d" $runList)
        echo "Fetching metadata for runs: $firstRun_wget - $lastRun_wget"
        $scriptsPATH/utility/run_char_wget.sh $firstRun_wget $lastRun_wget $runSummary $scriptsPATH
        $scriptsPATH/utility/fill_list_wget.sh $firstRun_wget $lastRun_wget $fillSummary $scriptsPATH
        wait
    done
    firstRun_wget=$(sed "${i}q;d" $runList)
    lastRun_wget=$(sed "${numRunRunList}q;d" $runList)
    echo "Fetching metadata for runs: $firstRun_wget - $lastRun_wget"
    $scriptsPATH/utility/run_char_wget.sh $firstRun_wget $lastRun_wget $runSummary $scriptsPATH
    $scriptsPATH/utility/fill_list_wget.sh $firstRun_wget $lastRun_wget $fillSummary $scriptsPATH
else
    echo "Fetching metadata for runs: $beginRun - $endRun"
    $scriptsPATH/utility/run_char_wget.sh $beginRun $endRun $runSummary $scriptsPATH
    $scriptsPATH/utility/fill_list_wget.sh $beginRun $endRun $fillSummary $scriptsPATH
fi
rm $runList
## avoid duplicates
sort -u $runSummary > $runSummary.tmp 
mv $runSummary.tmp $runSummary
sort -u $fillSummary > $fillSummary.tmp
mv $fillSummary.tmp $fillSummary
runsDownloaded=$(wc $runSummary | awk '{print $1}') 
fillsDownloaded=$(wc $fillSummary | awk '{print $1}')
#if [ $runsDownloaded -eq 0 ] || [ $fillsDownloaded -eq 0 ]; then
#    echo "[ERROR]: No runs or fills metadata present. Check the metadata." >> $outLogFile
#    exit 1
#else
#    echo "[INFO]: Runs downloaded: $runsDownloaded | Fills downloaded: $fillsDownloaded" >> $outLogFile
#    echo "" >> $outLogFile
#    echo "** ROOT session:" >> $outLogFile
#fi

if [ $runsDownloaded -eq 0 ]; then
    echo "[ERROR]: No runs metadata present. Check the metadata." >> $outLogFile
    exit 1
fi
if [ $fillsDownloaded -eq 0 ] && [ $runsDownloaded -ne 0 ]; then
    echo "[WARNING]: No stable fills downloaded in the run interval $beginRun - $endRun " >> $outLogFile
    echo "[INFO]: Runs downloaded: $runsDownloaded | Fills downloaded: $fillsDownloaded" >> $outLogFile
    echo "" >> $outLogFile
    echo "** ROOT session:" >> $outLogFile
else 
    echo "[INFO]: Runs downloaded: $runsDownloaded | Fills downloaded: $fillsDownloaded" >> $outLogFile
    echo "" >> $outLogFile
    echo "** ROOT session:" >> $outLogFile
fi

####
# eliminare da run list e fill list tutti i run che non ci sono nell'inputfile list
####

# Create the .root file for the day
numRunAnalyze=$(wc $runSummary | awk '{print $1}')
outFileNAME_today=$outFileNAME"_"$todayDate
todayFileList=$outMetadataPATH"/laserPed_"$todayDate"_newFileList.txt"
files_to_process=$(wc $todayFileList | awk '{print $1}')

if [ "$(ls $treePATH"/" | grep $outFileNAME".root")" ]; then
    printf "\n"
    export treePATH
    export outFileNAME_today
    export runSummary
    export fillSummary
    export todayFileList
    export numRunAnalyze
    export filesAlreadyProcessed
    export scriptsPATH
    export files_to_process
#    krenew -i -t $scriptsPATH/call_root_NoCrystalTree.sh >> $outLogFile
    $scriptsPATH/utility/call_root_NoCrystalTree.sh >> $outLogFile
    if [ $? -ne 0 ]; then
        echo "[ERROR]: Impossible to process the files with the ROOT macro. Let us exit the main program." >> $outLogFile 
        exit 1
    fi
    printf "Now the hadd with the previous file\n"
    # krenew here?
    hadd $tmpPATH"/"$outFileNAME"_tmp.root" $treePATH"/"$outFileNAME".root" $treePATH"/"$outFileNAME_today".root"
    if [ $? -ne 0 ]; then
        echo "[ERROR]: Impossible to hadd the nTuple for today. Let us exit the main program." >> $outLogFile 
        exit 1
    fi
    wait
    mv $tmpPATH"/"$outFileNAME"_tmp.root" $treePATH"/"$outFileNAME".root"
    if [ $? -ne 0 ]; then
        echo "[ERROR]: Impossible to process the files with the ROOT macro. Let us exit the main program." >> $outLogFile 
        exit 1
    else
        echo "[INFO]: Hadd completed." >> $outLogFile 
    fi
else
    # since is the first time the script is called ana_ped does not exist and the first tree should contain the crystal info
    export treePATH
    export outFileNAME_today
    export runSummary
    export fillSummary
    export todayFileList
    export numRunAnalyze
    export filesAlreadyProcessed
    export scriptsPATH
    export files_to_process
#    krenew -i -t $scriptsPATH/call_root.sh >> $outLogFile
    $scriptsPATH/utility/call_root.sh >> $outLogFile
    if [ $? -ne 0 ]; then
        echo "[ERROR]: Impossible to process the files with the ROOT macro. Let us exit the main program." >> $outLogFile 
        exit 1
    fi
    cp $treePATH"/"$outFileNAME_today".root" $treePATH"/"$outFileNAME".root"
    if [ $? -ne 0 ]; then
        echo "[ERROR]: Impossible to copy the nTuple(?). Let us exit the main program." >> $outLogFile 
        exit 1
    fi
fi

# Fix delle fed mancanti (opzionale)

echo "[INFO]: DONE for today. Unbelievable!" >> $outLogFile 

exit 0
