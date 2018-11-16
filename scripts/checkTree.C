void checkTree(TString fileToOpen_PATH)
{
    TFile *myFile = TFile::Open(fileToOpen_PATH); 
    TTree *myTree = (TTree*)myFile->Get("T");
    
    // Branches in myTree
    // FIXME names can be the same?
    Int_t run;
    Int_t run_type;
    Int_t seq_id_fed;
    Int_t las_id;
    Int_t fill_num;
    Int_t run_num_infill;
    Int_t run_time;
    Int_t run_time_stablebeam;
    Float_t lumi;
    Float_t bfield;
    Int_t nxt;
    Int_t time[54];
    Int_t fed[75848];
    Int_t chan[75848];
    Float_t ped[75848];
    Float_t pedrms[75848];
    Int_t id;
    //FIXME

    myTree->SetBranchAddress("run",&run);
    myTree->SetBranchAddress("run_type",&run_type);    
    myTree->SetBranchAddress("seq_id",&seq_id);    
    myTree->SetBranchAddress("las_id",&las_id);    
    myTree->SetBranchAddress("fill_num",&fill_num);    
    myTree->SetBranchAddress("run_num_infill",&run_num_infill);    
    myTree->SetBranchAddress("run_time",&run_time);    
    myTree->SetBranchAddress("run_time_stablebeam",&run_time_stablebeam);    
    myTree->SetBranchAddress("lumi",&lumi);    
    myTree->SetBranchAddress("bfield",&bfield);    
    myTree->SetBranchAddress("nxt",&nxt);    
    myTree->SetBranchAddress("time",&time);    
    myTree->SetBranchAddress("fed",&fed);    
    myTree->SetBranchAddress("chan",&chan);    
    myTree->SetBranchAddress("ped",&ped);    
    myTree->SetBranchAddress("pedrms",&pedrms);    
    myTree->SetBranchAddress("id",&id);    

    // loop over all entries
    for (Int_t i = 0; i < myTree->GetEntries(); i++) 
    {
        myTree->GetEntry(i);     
        // FIXME Needed?
        cout << "Checking entry = " << i << " | RUN = " << run << endl;
        // FIXME
    }
}
