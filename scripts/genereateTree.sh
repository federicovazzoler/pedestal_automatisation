#!/bin/sh

set -x

if [ "$1" ]; then
	root -l -b -q $scriptsPATH/utility/create_nTuple_from_data_NoCrystalTree.C+\(\"$treePATH\",\"$outFileNAME_today\",\"$runSummary\",\"$fillSummary\",\"$todayFileList\",$numRunAnalyze,$filesAlreadyProcessed,$files_to_process\)
else
	root -l -b -q $scriptsPATH/utility/create_nTuple_from_data.C+\(\"$treePATH\",\"$outFileNAME_today\",\"$runSummary\",\"$fillSummary\",\"$todayFileList\",$numRunAnalyze,$filesAlreadyProcessed,$files_to_process\)
fi

exit
