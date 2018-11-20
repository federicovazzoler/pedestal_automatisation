#!/bin/sh

set -x

root -l -b -q $scriptsPATH/utility/create_nTuple_from_data_NoCrystalTree.C+\(\"$treePATH\",\"$outFileNAME_today\",\"$runSummary\",\"$fillSummary\",\"$todayFileList\",$numRunAnalyze,$filesAlreadyProcessed,$files_to_process\)

exit
