#include "TFile.h"
#include "TTree.h"
#include "TBrowser.h"
#include "TRandom.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TLine.h"
#include "TH2.h"
#include "TF1.h"
#include "TH1.h"
#include "TTimeStamp.h"
#include "TArrayI.h"
#include "TArrayF.h"
#include "TSystem.h"

#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

// Read data (CERN staff) from an ascii file and create a root file with a Tree.
// see also a variant in staff.C
// Author: Rene Brun
void makeNTuple(string treePATH, string outFileNAME_today, string runSummary, string fillSummary, string todayFileList, Int_t numRunAnalyze, Int_t k, Int_t files_to_process){

  Int_t nrun=numRunAnalyze;

  // k == filesAlreadyProcessed  
	if (k != 0) k = k + 1;

  // Define a struct to store the channel (crystal) information (position and ID) 
  typedef struct {
    int iChannel;
    int ix;
    int iy;
    int iz;
  } Chan_t;

  Chan_t Chan;
  Chan.iChannel = -1;
  Chan.ix = -1;
  Chan.iy = -1;
  Chan.iz = -1;

  // Some variables
  Float_t bfield;
  Float_t lumi;
  Int_t   id=0; 
  Int_t   rr=0;
  Int_t   rrt=0;
  Int_t   seq_id=-1;
  Int_t   las_id=0;
  Int_t   fill_num=0;
  Int_t   run_num_infill=0;
  Int_t   run_time=0;
  Int_t   run_time_stablebeam=0;
  //Int_t   ttime;
  //Int_t   stime;
  Int_t   time[54]; // Why 54?
  Int_t   fed[75848]; // 75848 total number of crystals in ECAL
  Int_t   chan[75848];
  Float_t ped[75848];
  Float_t rms[75848];

  // Create the .root output file (where the data are stored)
  TFile *hfile = 0;
  hfile = TFile::Open((treePATH+"/"+outFileNAME_today+".root").c_str(),"RECREATE");
  
  // Output tree for channels. Chan_t struct is used
  TTree* tPedChan = new TTree("PedChan", "Channels"); 
  tPedChan->Branch("Channels", &Chan.iChannel, "iChannel/I");
  tPedChan->Branch("x", &Chan.ix, "ix/I"); 
  tPedChan->Branch("y", &Chan.iy, "iy/I");
  tPedChan->Branch("z", &Chan.iz, "iz/I");

  // ******************** CRYSTAL SCHEME ********************
  // Constuct the crystal scheme form barrel and endcaps
  //// BARREL (36 supermodules with 1700 crystals each. Total of 61200 crystals for the barrel)
  Int_t hashEB[36][1700]; // hashEB contains the iChannel (0:61200)
  ifstream fEB;
  fEB.open("/afs/cern.ch/user/f/fvazzole/work/ntuple_pedestal_automatization/final_script/utility/EB.txt"); // Contains informations about ID, FED, position ecc. of every crystal in the barrel
  string dummyLine;
  getline(fEB, dummyLine); // Skip first line (contains the titles of the columns)
  ////// Cycling over the crystals
  for (Int_t iChannel = 0; iChannel < 61200; iChannel++) { 
    Int_t cmsswId, dbID, hashedId, iphi, ieta, absieta, FED, SM, TT, iTT, strip, Xtal, phiSM, etaSM; 
    Int_t fed, id;
    string pos;
    fEB >> cmsswId >> dbID >> hashedId >> iphi >> ieta >> absieta >> pos >> FED >> SM >> TT >> iTT >> strip >> Xtal >> phiSM >> etaSM;
    // -- hashedID --
    if(hashedId != iChannel) {
      cout << "EB.txt strange hash = " << hashedId << " while iChannel = " << iChannel << endl;
      exit(-1);
    
    }
    // -- fed --
    fed = FED - 610; // In orderd to have fed running from 0 to 35
    if(fed < 0 || fed > 35) {
      cout << "EB.txt strange fed = " << fed << " while  FED = " << FED << " for iChannel = " << iChannel << endl;
      exit(-1);
    }
    // -- ID --
    id = dbID%10000 - 1; // In order to have id running from 0 to 1699 (dbID contains informations both on the supermodule and on the crystal. Extract only the numeric scheme of the crystal inside the supermodule so for every SM 1700 crystals)
    if(id < 0 || id > 1699) {
      cout << "EB.txt strange id = " << id << " while  dbID = " << dbID << " for iChannel = " << iChannel << endl;
      exit(-1);
    }
    // Fill the hashEB matrix
    hashEB[fed][id] = iChannel;
    // Fill the output tree with channels informations
    Chan.iChannel = iChannel;
    Chan.ix = ieta; // -85:-1 , 1:85
    Chan.iy = iphi; // 1:360
    Chan.iz = 0;
    tPedChan->Fill();
  }
  fEB.close();
  //// ENDCAPS
  Int_t hashEE[18][1025]; // 18 SM with 1025 crystals each
  ifstream fEE;
  fEE.open("/afs/cern.ch/user/f/fvazzole/work/ntuple_pedestal_automatization/final_script/utility/EE.txt");
  Int_t hashEE_temp[2][100][100]; // Since iz could be 0 or 1 depending on the endcap considered
  // Inizialization of hashEE_temp
  for(Int_t iz = 0; iz < 2; iz++){
    for(Int_t ix = 0; ix < 100; ix++){
      for(Int_t iy = 0; iy < 100; iy++){
        hashEE_temp[iz][ix][iy] = -1;
      }
    }
  } 
  getline(fEE, dummyLine); // skip first line
  for (Int_t iChannel = 0; iChannel < 14648; iChannel++) {
    Int_t cmsswId, dbID, hashedId, side, ix, iy, SC, iSC, FED, TT, strip, Xtal, quadrant;
    string pos;
    fEE >> cmsswId >> dbID >> hashedId >> side >> ix >> iy >> SC >> iSC >> FED >> pos >> TT >> strip >> Xtal >> quadrant;
    if(hashedId != iChannel) {
      cout << "EE.txt strange hash = " << hashedId << " while iChannel = " << iChannel << endl;
      exit(-1);
    }
    Int_t iz = 1;
    if(side == -1) iz = 0;
    hashEE_temp[iz][ix - 1][iy - 1] = iChannel;
  }
  fEE.close();
  fEE.open("/afs/cern.ch/user/f/fvazzole/work/ntuple_pedestal_automatization/final_script/utility/Numbering_EE_All_Data.txt");
  if(!fEE.is_open()) {
    cout << "EE file not opened. Let us exit " << endl;
    exit(-1);
  }
  getline(fEE, dummyLine); // skip first line
  for (Int_t fed = 0; fed < 18; fed++){ // initialization
    for (int id = 0; id < 1025; id++){
      hashEE[fed][id] = -1;
    }
  }     
  for (Int_t iChannel = 0; iChannel < 14648; iChannel++) {
    Int_t ixx, iyy, Dee, FED, sid, Ch, Harness, PNA, PNB;
    fEE >> ixx >> iyy >> Dee >> FED >> sid >> Ch >> Harness >> PNA >> PNB;
    Int_t izz = 0;
    //  -- fed --
    Int_t fed = FED - 601;    // 0...8  EE-
    if(FED > 645) {
      fed = FED - 637;   // 9...17  EE+
      izz = 1;
    }
    if(fed < 0 || fed > 17) {
      cout << "EE file strange fed = " << fed << " while  FED = " << FED << " for iChannel = " << iChannel << endl;
      exit(-1);
    }
    // -- Ch --
    if(Ch < 0 || Ch > 1024) {
      cout << "EE file strange Ch = " << Ch << " for iChannel = " << iChannel << endl;
      exit(-1);
    }
    // Controls
    if(hashEE_temp[izz][ixx - 1][iyy - 1] == -1) { // ixx, iyy goes from 1 to 100, indices from 0 to 99!
      cout << " Channel " << hashEE_temp[izz][ixx - 1][iyy - 1] << " iz " << izz << " ix " << ixx << " iy " << iyy << " not found :  fed " << fed << " lchan " << Ch << endl;
      exit(-1);
    }
    if(hashEE[fed][Ch] != -1) {
      cout << " Channel " << hashEE[fed][Ch] << " iz " << izz << " ix " << ixx << " iy " << iyy << " alreday found :  fed " << fed << " lchan " << Ch << endl;
      exit(-1);
    }
    // Fill the hashEE matrix
    hashEE[fed][Ch] = hashEE_temp[izz][ixx - 1][iyy - 1];
    // Fill the output tree with channels informations
    Chan.iChannel = hashEE[fed][Ch];
    Chan.ix = ixx;
    Chan.iy = iyy;
    Chan.iz = izz;
    // if(iChannel%1000 == 0) cout << " EE channel " << Chan.iChannel << " x " << Chan.ix << " y " << Chan.iy << " z " << Chan.iz << endl;
    tPedChan->Fill();
  } 
  
  tPedChan->Write();
  tPedChan->Print();

  // ******************** RUN INFORMATIONS ********************
  Int_t nxt=0; // Next line ?
  // Create output tree for pedestal data + run info
  TTree *tree = new TTree("T","Marc Ped data");
  tree->Branch("id",&id,"id/I");
  tree->Branch("run",&rr,"run/I");
  tree->Branch("run_type",&rrt,"run_type/I");
  tree->Branch("seq_id",&seq_id,"seq_id/I");
  tree->Branch("las_id",&las_id,"las_id/I");
  tree->Branch("fill_num",&fill_num,"fill_num/I");
  tree->Branch("run_num_infill",&run_num_infill,"run_num_infill/I");
  tree->Branch("run_time",&run_time,"run_time/I");
  tree->Branch("run_time_stablebeam",&run_time_stablebeam,"run_time_stablebeam/I");
  tree->Branch("lumi",&lumi,"lumi/F");
  tree->Branch("bfield",&bfield,"bfield/F");
  tree->Branch("nxt",&nxt,"nxt/I");
  tree->Branch("time",time,"time[54]/I");
  tree->Branch("fed",fed,"fed[75848]/I");
  tree->Branch("chan",chan,"chan[75848]/I"); // SM, chan number
  tree->Branch("ped",&ped,"ped[75848]/F");
  tree->Branch("pedrms",&rms,"pedrms[75848]/F");
  // tree->Branch("x",             x,             "x[75848]/I");
  // tree->Branch("y",             y,             "y[75848]/I");
  // tree->Branch("z",             z,             "z[75848]/I");
  
  // Some variables ???
  Int_t   nchan[54]={830,791,815,815,791,830,821,810,821,
  1700,1700,1700,1700,1700,1700,1700,1700,1700,1700,
  1700,1700,1700,1700,1700,1700,1700,1700,1700,1700,
  1700,1700,1700,1700,1700,1700,1700,1700,1700,1700,
  1700,1700,1700,1700,1700,1700,
  830,791,815,815,791,830,821,810,821};
  // Using TArray -> allows dynamic allocation. nrun is given to the macro from outside
  TArrayI *run_num = new TArrayI(nrun);
  TArrayI *runt = new TArrayI(nrun);
  TArrayI *mrun = new TArrayI(nrun);
  TArrayI *instlumi = new TArrayI(nrun);
  TArrayI *mfill = new TArrayI(nrun);
  TArrayI *time_sb = new TArrayI(nrun);
  TArrayI *time_fill = new TArrayI(nrun);
  TArrayF *mfield = new TArrayF(nrun);

  // ******** RUN CHAR ********
  //
  // Name and path of the run_char file. First the string then converted to char for fopen()
  //
  FILE *fps = fopen(runSummary.c_str(),"r");
  //// Some variables to read the file
  Char_t lines[400]; // Every line lenght is <= 400
  Int_t run_; 
  Int_t L = 0;
  Float_t aaa = 0;
  Bool_t isatest = false; 
  //// Read the run_char_all_sorted.txt file and store what we need
  while (fgets(lines,400,fps)) { // leggo una linea per volta da fps e la metto dentro lines
    // OCCHIO CHE QUESTO CICLO DEVE FERMARSI AL MAX DOPO 9636 ITERAZIONI (guarda definizione di runt ad esempio)!
    sscanf(&lines[0],"%d\t%f",&run_ ,&aaa); // legge le prime due colonne del file (in realta' la seconda colonna è del tipo 0.00000 quindi aaa = 0 mentre run_ = 289987 insomma il run)
    if((run_ > 200000 && run_ < 400000 && L < nrun) || (isatest)){
    // if(L<10) std::cout<<"reading characteristics of run="<<run_<<std::endl;
      //run_num[L]=run_; 
      run_num->AddAt(run_,L);
      char * pscoll;
      pscoll = strstr (lines,"collisions");
      char * pscosm;
      pscosm = strstr (lines,"cosmics");
      char * pscirc;
      pscirc = strstr (lines,"circulating");
      char * pstest;
      pstest = strstr (lines,"test");
      char * psvirgin;
      psvirgin = strstr (lines,"irgin");
      char * psecalpro;
      psecalpro = strstr (lines,"ecalpro");
      char * pssplash;
      pssplash = strstr (lines,"splash");
      char * psvdm;
      psvdm = strstr (lines,"vdm");
      char * pslowpu;
      pslowpu = strstr (lines,"lowpu");
      char * ps150;
      ps150 = strstr (lines,"150ind");
      char * pshibeta;
      pshibeta = strstr (lines,"hibeta");
      char * psromanpot;
      psromanpot = strstr (lines,"romanpot");

      // Associate at runt a number which reflects the char of the run
      runt->AddAt(0,L);
      if(pscoll!=NULL || psvdm!=NULL || pssplash!=NULL || pslowpu!=NULL || ps150!=NULL || pshibeta!=NULL 
    || psromanpot!=NULL) {
        runt->AddAt(1,L);
      } else if(pscosm!=NULL){
        runt->AddAt(2,L);
      } else if(pscirc!=NULL){
        runt->AddAt(3,L);
      } else if(pstest!=NULL || psvirgin!=NULL || psecalpro!=NULL){
        runt->AddAt(4,L);
      }
      if(runt->GetAt(L)==1 && aaa==0){
        runt->AddAt(4,L);
      }
      if(runt->GetAt(L)==0){
         std::cout<<"[WARNING]: Run type 0 in run = " << lines << std::endl;
      }
      L=L+1; 
    } 
  }
  fclose(fps);
  std::cout << "[INFO]: Finished processing run characteristics" << std::endl; 
  // Prima o poi runt sarà scritto sul tree in output!

  // ******** FILL SUMMARY ********
  FILE *fpf = fopen(fillSummary.c_str(),"r");
  Char_t linef[1024];

  // Some auxilliary variables
  //Int_t NF = 0;
  Int_t fill_;
  Float_t bfield_;
  Float_t peakl_;
  string timestart_string;
  string timesb_string;
  Int_t syear, smonth, sday, shour, smin, ssec, tyear, tmonth, tday, thour, tmin, tsec;
  Int_t energy_; //used in psc
  Int_t dummyRow1_,dummyRow2_,dummyRow3_,dummyRow4_,dummyRow5_,dummyRow6_; // da togliere direttamente da FillSumm??

  // read the input file
  while (fgets(linef,1024,fpf)) {
    sscanf(&linef[0],"%d\t%d.%d.%d %d:%d:%d\t%f\t%f\t%d.%d.%d %d:%d:%d\t%d.%d.%d %d:%d:%d\t%d", &fill_, &syear, &smonth, &sday, &shour, &smin, &ssec, &bfield_, &peakl_, &tyear, &tmonth, &tday, &thour,&tmin,&tsec,&dummyRow1_,&dummyRow2_,&dummyRow3_,&dummyRow4_,&dummyRow5_,&dummyRow6_,&energy_); // Spaces and .,: matters!
    //std::cout<<"this is fill "<<fill_<<std::endl;
    // Create the timestamp from the time data collected
    TTimeStamp *stime_= new TTimeStamp(syear,smonth,sday,shour,smin,ssec); // Start
    TTimeStamp *ttime_= new TTimeStamp(tyear,tmonth,tday,thour,tmin,tsec); // Stop
    // Number between end time and runs belonging to the fill
    // Mi serve per prendere tutti i run che appartengono a quel fill (guarda ultime colonne di FillSummary
    char * psc;
    psc = strstr (linef,"\t6500\t"); //<- hardcoded by francesca!! Ho preso direttamente la colonna energy_ dal file di testo 
    if(psc == NULL) {
      psc = strstr (linef,"\t4000\t");
    } 
    if(psc == NULL) {
      psc = strstr (linef,"\t6499\t");
    } 
    if(psc == NULL) {
      psc = strstr (linef,"\t2510\t");
    } 
    if(psc == NULL) {
      // convert int to char for strstr
      char energy_char[20] = {0};
      std::sprintf(energy_char, "\t%d\t", energy_);
      psc = strstr (linef,energy_char);
    }
    // std::cout<<"this is fill 2nd print "<<fill_<<std::endl;
    char * pscv;
    psc=psc+6;
    pscv = strstr (psc,"\0");
    int nr_in_fill=strlen(psc)/7;//(pscv-psc)/7 7 perche ogni run e' lungo 6 caratteri piu uno spazio;
    int runn=0;
    for(int nrf=0; nrf<nr_in_fill;nrf++){
      sscanf(psc,"%d ",&runn);
      //std::cout<<"this is run "<<runn<<std::endl;
      psc = psc+7;
      for(int jr=0; jr<nrun; jr++){
        if(runn==run_num->GetAt(jr)){
          mfill->AddAt(fill_,jr);
          mfield->AddAt(bfield_,jr);
          instlumi->AddAt(peakl_,jr);
          mrun->AddAt(nrf,jr);
          time_fill->AddAt(stime_->GetSec(),jr);
          time_sb->AddAt(ttime_->GetSec(),jr);
        }
      } 
    }
    // std::cout<<"this is fill 3rd print "<<fill_<<std::endl;
  }
  fclose(fpf);
  std::cout<<"[INFO]: Finished processing fill characteristics"<<std::endl; 
  std::cout<<"[INFO]: Going to check that we have all fills"<<std::endl;
  for(int j=0; j<L; j++){
    if(mfill->GetAt(j)==0 && runt->GetAt(j)==1 ){
      std::cout<<"[ERROR]: Missing fill information for run: "<<run_num->GetAt(j)<< std::endl;
    }
  }
  std::cout<<"[INFO]: Fills check finished"<<std::endl;

  // ******************** PEDESTALS FROM FILES ********************
  // 75848 cristalli in ECAL
  for(int jj=0;jj<75848;jj++){
    ped[jj]=0;
    fed[jj]=0;
    chan[jj]=0;
    rms[jj]=0;
  }
  int ntofill=0;
  // Open the txt with the files to be analyzed 
  FILE *fpl = fopen(todayFileList.c_str(),"r");
  char line2[1024];     

  //int run_id_new=-1;
  int run_id=-1;

/////////////////////////////////////////

    // dummy only for percentage
    Int_t processed_files=0;

////////////////////////////////////////

  while (fgets(line2,1024,fpl)) {
    //ADD BY ME get rid of new line char
    char *pos;
    if ((pos=strchr(line2, '\n')) != NULL) {
        *pos = '\0';
    }
    
    //
    //
    // FILE OPENING: handle with care
    FILE *fp; // pointer to the file to be opened
    //gSystem->Exec("krenew"); // update kerberos ticket
    for (Int_t try_open = 1;try_open < 7;try_open++){ // try to reopen the file several times if it is unavailable
        fp = fopen(line2,"r");
        if (fp==NULL && try_open == 6){
            cout << "[ERROR]: Failed to open: " << line2 << endl;
            break;
        }
        if (fp==NULL){
            cout << "[WARNING]: Try to reopen " << try_open << " times file: " << line2 << endl;
            usleep(3000000);
            delete fp;
            TString head_command = "head "+TString(line2)+" > /dev/null";
            gSystem->Exec(head_command.Data()); // try to head the file to dev/null (maybe it will open it?)
        }
        else if (fp!=NULL){
            //cout << "[INFO]: Opened after " << try_open << " times file: " << line2 << endl;
            break;
        }
    }
    if (fp==NULL) continue; // If the file cannot be opened continue with the next one
    // FILE OPENING: end
    //
    //

    char * pch;
    pch=strchr(line2,'d');
    if(pch!=NULL){
      //if(k%10000==0)  std::cout<<"processing file: "<<line2<<std::endl; 
      // Extract char from the name 
      
      //
      //CHECK IF THE PATH IS A PROBLEM!
      //
        //for (Int_t i = 0; i < 50; i++){
        //  cout << "line2[" << i << "] = " << line2[i] << endl;
        //}
      //
      //
      //
      char fed_string[4] = "000"; // Why [4] but only 3 char? perchè deve esserci il arattere di fine stringa \0
      char run_string[7] = "000000";
      char seq_string[5] = "0000";
      char las_string[4] = "000";
     
      // PATH ISSUE! 
      //int startFEDposition = 117;
      char *startFEDposition_CHAR = strrchr(line2, '/'); // pick the last occurence of "/" in string
      int startFEDposition = (int)(startFEDposition_CHAR-line2+1-4); // -4 since i pick the last "/" so to obtain the fistr FED char go back 4 char ("../FED/dst..\n")
      //fed_string[0]=line2[17];
      //fed_string[1]=line2[18];
      //fed_string[2]=line2[19];
      fed_string[0]=line2[startFEDposition];
      fed_string[1]=line2[startFEDposition + 1];
      fed_string[2]=line2[startFEDposition + 2];

      int fed_num = atoi( fed_string );

      run_string[0]=line2[startFEDposition + 9];
      run_string[1]=line2[startFEDposition + 10];
      run_string[2]=line2[startFEDposition + 11];
      run_string[3]=line2[startFEDposition + 12];
      run_string[4]=line2[startFEDposition + 13];
      run_string[5]=line2[startFEDposition + 14];
      int run_id_new = atoi( run_string );

      seq_string[0]=line2[startFEDposition + 16];
      seq_string[1]=line2[startFEDposition + 17];
      seq_string[2]=line2[startFEDposition + 18];
      seq_string[3]=line2[startFEDposition + 19];
      int seq_id_new = atoi( seq_string );

      las_string[0]=line2[startFEDposition + 21];
      las_string[1]=line2[startFEDposition + 22];
      las_string[2]=line2[startFEDposition + 23];
      int las_id_new = atoi( las_string );

      int n=0;
      for(int jj=0;jj<54;jj++){
        if(fed_num<601+jj && jj<9 && jj>44 ){
          n=n+nchan[jj];
        }
      }

//cout << fed_num  << " " << run_id_new << " " << seq_id_new << " " << las_id_new << endl;

//cout << run_string[0] << " " <<run_string[1] << " " << run_string[2] << " " << run_string[3] << " " << run_string[4] << " " << run_string[5] << endl;

//exit(-1);

      //if(k%1==0)  std::cout<<"run: "<<run_id_new<<" seq_string="<<seq_string<<" seq="<<seq_id_new << " las=" << las_id_new << " id=" << k << std::endl; 
    
    if(processed_files%1000==0) cout << "[INFO]: Processed " << processed_files << " files | " << double(processed_files)/double(files_to_process)*100 << "%" << endl;

      if( (seq_id_new!=seq_id || las_id_new!=las_id || run_id_new!=run_id ) && (run_id!=-1) ) {
        ntofill=ntofill+1;
        //if(k%1==0) std::cout<<"filling tree for run="<<run_id <<" and seq="<< seq_id << " and las=" << las_id << " id=" << k-1 << std::endl; 
	// mtto a -1 i valori non usati di time
	Int_t jjj;
        for(jjj=0;jjj<54;jjj++)
        {       
                //if(time[jjj]==0) time[jjj]=-1;
        }
	tree->Fill();
        // zero all vectors 
        for(int jj=0;jj<75848;jj++){
          ped[jj]=0;
          fed[jj]=0;
          chan[jj]=0;
        }
        for(jjj=0;jjj<54;jjj++)
        {       
                time[jjj]=0;
        }        
	nxt=0;
      } 

      char line[400];
      int i=0;
      int ch=0;
      //int chele=0;
      float p1, p2;//, p; 
     
      run_id=run_id_new;
      seq_id=seq_id_new;
      las_id=las_id_new;
      
      // READ THE SINGLE FILE
      // first line contains the time 
      char *dummyLinefgets;
      dummyLinefgets = fgets(line,400,fp);
      int idtime_;
      int idfed = fed_num - 601;
      float mf = 0;
      sscanf(&line[0],"%d %*d %*d %d %*d %*d %*d %*d %*d %*d %f ",&run_,&idtime_,&mf); //%* leggi ma ignora

      time[idfed] = idtime_;
      rr = run_;
      rrt = 0;
      fill_num = 0;
      bfield = 0;
      lumi = 0;
      run_num_infill = 0;
      run_time = 0;
      run_time_stablebeam = 0;
      for(int jr = 0; jr < nrun; jr++){
        if(run_ == run_num->GetAt(jr)){
          rrt = runt->GetAt(jr);
          fill_num = mfill->GetAt(jr);
          bfield = mfield->GetAt(jr);
          lumi=instlumi->GetAt(jr);
          run_num_infill=mrun->GetAt(jr);
          run_time=time_fill->GetAt(jr);
          run_time_stablebeam=time_sb->GetAt(jr);
        }
      }
      if(bfield == 0) bfield=mf; 
      if(rrt != 1 && fill_num > 0 ){
        rrt=1; 
        // std::cout<<"run recovered "<<run_<<std::endl;
      }
    //std::cout<< "Run="<<run_<<" seq_id="<<seq_id<<" fed="<<fed_num<<" las_id="<<las_id<<std::endl;
      if(rrt==1 && fill_num==0) std::cout<<"error run "<< run_ <<std::endl; 
      // second line does not matter 
      dummyLinefgets = fgets(line,400,fp);

      int elch=0; 
      while (fgets(line,400,fp)) {
        if(i<nchan[idfed]){
          sscanf(&line[0],"%d %d %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %*f %f %f ",&ch,&elch,&p1,&p2);
          // if(i<1) std::cout<< " siamo alla riga "<< i<< " ped="<<p1<<std::endl;
          id=k; 
          if(p1>-100.0) {
            int ixtal=ch; 
            int FED=fed_num; 
            int iChannel = -1;
            if(FED > 609 && FED < 646) { // EB
              int fed = FED - 610;    // 0...35
              iChannel = hashEB[fed][ixtal - 1];  // ixtal from 1 to 1700, id from 0 to 1699
              if(iChannel < 0 || iChannel > 61199) {
                cout << " EB strange channel " << iChannel << " for FED " << fed << " local ch " << elch << endl;
                exit(-1);
              } 
            } else { // EE
              int fed = FED - 601;    // 0...8
              if(FED > 645) fed = FED - 637;    // 9...17
              iChannel = hashEE[fed][elch];
              if(iChannel < 0 || iChannel > 14647) {
  		          cout << " EE strange channel " << iChannel << " for FED " << fed << " local ch " << elch << endl;
                exit(-1);
              }
              iChannel =iChannel+61200;
            }

            int chid=iChannel;
            if(chid>=0 && chid<75848){
              ped[chid]=p1;
              fed[chid]=fed_num;
              chan[chid]=ch;
              rms[chid]=p2;
            } else {
              std::cout<<"ERROR:chid="<<chid<<std::endl;
            }
            i=i+1; 
          } else {
            std::cout<<"why this line after "<<i<<" channels is crazy ? "<<line<<std::endl; 
          }
        }
      }
      nxt=nxt+i;
      k=k+1;
      processed_files=processed_files+1;
      fclose(fp);
    }
  }

	// Se non ho piu aperto files devo aggiungere l'ultimo bunches (che con l'algoritmo di francesca si perde
        if (!fgets(line2,1024,fpl))
        {
        	tree->Fill();
        } 

  // Print and write the tree
  tree->Print();
  tree->Write();
  fclose(fpl);
  delete hfile;
}
