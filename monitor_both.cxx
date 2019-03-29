/************************************************/
/*												*/
/*					BGO+Hodo 					*/
/*												*/
/************************************************/
//c++ classes
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <cstring>
#include <dirent.h>//class used to manipulate and traverse directories
#include <stdio.h>
#include <mutex>
//ROOT classes
#include "TLatex.h"
#include "TROOT.h"
#include "TObject.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TF1.h"
#include "TFile.h"
#include "TApplication.h"
#include "TRint.h"
#include "TAxis.h"
#include "TTimer.h"
#include "TStopwatch.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TGWindow.h"
#include "TGClient.h"
#include "TPaveText.h"
//custom classes
#include "functions.h"

extern int alphasort();

int file_select(const struct dirent *entry)//selection of files to be included in the return of the folder scan
{//ignore all files with the following features
  if((strcmp(entry->d_name, ".directory") == 0) ||(strcmp(entry->d_name, ".DS_Store") == 0) || (strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0) || strstr(entry->d_name,"tmp")!=NULL||strstr(entry->d_name,"pulse")!=NULL||strstr(entry->d_name,"T")!=NULL){
    return 0;
  }  else {
    return 1;
  }
}

int main (int argc,char ** argv)
{
  //Variables to be modified run by run and for changing file name structure
  //HERE YOU CAN SET THE VARIABLES TO ADAPT THIS APPLICATION TO YOUR SYSTEM
  //YOU JUST NEED TO FIX THE PATH TO THE FOLDER WHERE THE DATA WILL BE SAVED (variable path), AND THE FILENAME WHO COMES BEFORE THE FILE NUMBER (variable filemain)
  //I SUPPOSED A FILENAME LIKE file_name_N.dat WITH INCREASING N STARTING FROM 1, but this can be easily changed
  int runN = 0;//to be inserted by the user
  int update = 0;//to be inserted by the user
  std::cout<<"Type the run number : (return to confirm)"<<std::endl;
  std::cin>>runN;
  std::cout<<"Choose the frequency for updating plots : (number of events to analyse before updating)"<<std::endl;
  std::cin>>update;

  char path[100] = "/media/daq/porsche2/data_gamhadron/";//"/home/daq/gamhadron/daqGh/data/";//folder where the data are saved
  char filemain[100];//main part of the file name - only numbering stuff after that
  int xx = sprintf(filemain, "%d-", runN);
  char format[30] =".dat";//data format -> the acquisition software creates .dat files

  //ROOT STYLING
  gROOT->SetStyle("Plain");
  gROOT->ForceStyle();
  gStyle -> SetStatW(0.28);
  gStyle -> SetStatH(0.13);
  gStyle -> SetStatColor(0);
  gStyle -> SetStatX(0.87);
  gStyle -> SetStatY(0.85);
  gStyle -> SetStatFont(0);
  gStyle -> SetOptStat(111);
  gStyle->SetPalette(1);
  //-------------------------------//
  std::cout<<"STARTING monitor for run "<<runN<<" ... "<<std::endl;
  //Creating data structure to read the data from file
  FILEHEADER file_beg;//file header - it contains 0xF0F0 (16 bit), the run number (32 bit), the total number of events recorder in the file (32 bit)
  FILEEND file_end;//file footer - it contains 0xF1F1 (16 bit), the total number of events recorder in the file (32 bit), the total number of octets recorded in the file (32 bit)
  EVENTHEADER event_id;//event header (one per event) - it contains 0xABCD (16 bit), the event number (32 bit), the trigger number (24 bit), the number of hits recorded in this trigger (16 bit)
  DATAMAIN data_struct;//main structure of data (one per event) - it contains 0x00EB (16 bit), the front end number (8 bit), the trigger number (24 bit), the mode number (8 bit), the number of involved detector modules (8 bit - example number of touched fibers for the hodoscope)
  hit_fiber_receive data_fiber;//data for each hit mode 0 - optimal (one per involved detector module) - it contains the number of the hit module (8 bit) and the recorded time for the interaction (32 bit)
  hit_block_receive data_block;
  sample_receive Echt;

  //Create ID to check what I read
  uint16_t beginning_file_id = 0xF0F0;
  uint16_t end_file_id = 0xF1F1;
  uint16_t beginning_event = 0xABCD;
  int N_ev_toRead = 0;//Set the number of events you want to read from each file for monitoring purpose - depending on the speed and accuracy you need
  int length;
  int N_event=0;
  int count = 0;
  int count1 = 0;
  int files_read = 0;
  int events_read=0;
  int wait_loop = 0;//counter for waiting loops
  int time_to_wait = 50;//how many waiting loops you accept before stopping the program

  //Initializing char buffers for reading from file
  char init_file[10];//file header
  char end_file[8];//file end - for debugging
  uint8_t event_header[11];//event header
  uint8_t data_main_structure[7];//data main structure
  uint8_t hit_structure_mode1[5];//just time data - optimal mode
  uint8_t hit_structure_mode0[7];//BGO mini ech mode
  uint8_t sample_structure_mode[2];//BGO mini ech mode
  //Starting ROOT in the c++ program
  TApplication theApp("App", &argc, argv);
  if (gROOT->IsBatch()) {
    fprintf(stderr, "%s: cannot run in batch mode\n", argv[0]);
    return 1;
  }
/************************************************/
/*												*/
/*					BGO plots 					*/
/*												*/
/************************************************/
  TH2F *BGO_pos_2D = new TH2F("2d pos", "2D pos", 160, 0, 7, 240, 0, 10.5);//2D map of coincidences
  BGO_pos_2D->GetXaxis()->SetTitle("Position X");
  BGO_pos_2D->GetYaxis()->SetTitle("Position Y");
  BGO_pos_2D->SetMinimum(0);
  TH1F *BGO_block_hit = new TH1F("Number of blocks hit", "Number of blocks hit", 6, 1, 7);
  BGO_block_hit->GetXaxis()->SetTitle("Number of blocks hit");
  BGO_block_hit->GetYaxis()->SetTitle("Entries");
  BGO_block_hit->SetMinimum(0);


  TH1F *BGO_position_X = new TH1F("position X", "position X", 640, 0, 7);
  TH1F *BGO_position_Y = new TH1F("position Y", "position Y", 960, 0, 10.5);
  BGO_position_X->SetLineColor(kRed);
  BGO_position_X->GetXaxis()->SetTitle("position X");
  BGO_position_X->GetYaxis()->SetTitle("Entries");
  BGO_position_X->SetMinimum(0);
  BGO_position_Y->SetLineColor(kGreen);
  BGO_position_Y->GetXaxis()->SetTitle("position Y");
  BGO_position_Y->GetYaxis()->SetTitle("Entries");
  BGO_position_Y->SetMinimum(0);

  //TPaveText *BGO_pt;
  TCanvas *BGO_complex_canv = new TCanvas("all BGO data", "all BGO data", 1200, 1000);
  BGO_complex_canv->SetFillColor(0); //
  BGO_complex_canv->SetBorderMode(0);	//
  BGO_complex_canv->SetLeftMargin(0.1409396); //
  BGO_complex_canv->SetRightMargin(0.14865772); //
  gStyle->SetOptStat(000); //
  
  
  
  TPad *BGO_raw_posx_pad = new TPad("BGO_raw_posx_pad", "BGO_raw_posx_pad", 0.03, 0.33, 0.48, 0.66, 0);
  TPad *BGO_raw_posy_pad = new TPad("BGO_raw_posy_pad", "BGO_raw_posy_pad", 0.03, 0.03, 0.48, 0.33, 0);
  TPad *BGO_block_hit_pad = new TPad("BGO_block_hit_pad", "BGO_block_hit_pad", 0.03, 0.67, 0.48, 0.98, 0);
  TPad *BGO_map_pad = new TPad("BGO_map_pad", "BGO_map_pad", 0.5, 0.03, 0.98, 0.98, 0);
  /*TPad *BGO_raw_posx_pad = new TPad("BGO_raw_posx_pad", "BGO_raw_posx_pad", 0.03, 0.33, 0.48, 0.66, 0);
  TPad *BGO_raw_posy_pad = new TPad("BGO_raw_posy_pad", "BGO_raw_posy_pad", 0.03, 0.03, 0.48, 0.33, 0);
  TPad *BGO_map_pad = new TPad("BGO_map_pad", "BGO_map_pad", 0.03, 0.67, 0.98, 0.98, 0);
  TPad *BGO_block_hit_pad = new TPad("BGO_block_hit_pad", "BGO_block_hit_pad", 0.5, 0.36, 0.98, 0.67, 0);*/
  //TPad *BGO_stats_pad = new TPad("BGO_stats_pad", "BGO_stats_pad", 0.5, 0.03, 0.98, 0.34, 0);
  BGO_raw_posx_pad->Draw();
  BGO_raw_posy_pad->Draw();
  BGO_map_pad->Draw();
  BGO_block_hit_pad->Draw();
  //BGO_stats_pad->Draw();

  BGO_raw_posx_pad->cd();
  BGO_position_X->Draw();
  BGO_raw_posy_pad->cd();
  BGO_position_Y->Draw();
  BGO_map_pad->cd();
  BGO_pos_2D->Draw("colz");
  BGO_block_hit_pad->cd();
  BGO_block_hit->Draw();
  /*BGO_stats_pad->cd();
  BGO_pt = new TPaveText(.05,.1,.95,.9);
  BGO_pt->AddText(Form("#bf{RUN %d - monitoring stats}", runN));
  BGO_pt->Draw();*/
/************************************************/
/*												*/
/*					Hodo plots 					*/
/*												*/
/************************************************/
  TH1F *Hodo_flight_temp_spectra = new TH1F("flight_temp_spectra", "flight_temp_spectra", 20, 0.5, 20.5);//flight temp spectra
  Hodo_flight_temp_spectra->SetLineColor(kOrange);
  Hodo_flight_temp_spectra->GetXaxis()->SetTitle("flight temp");
  Hodo_flight_temp_spectra->GetYaxis()->SetTitle("Entries");
  Hodo_flight_temp_spectra->SetMinimum(0);
  TH1F *Hodo_cluster = new TH1F("Hodo_cluster size", "Hodo_cluster size", 20, 0.5, 20.5);//raw Hodo_cluster size - total number of involved fibers per event
  TH1F *Hodo_position_X = new TH1F("fiber X", "fiber X", 32, 0.5, 32.5);//number of fibers touched in the X plane
  TH1F *Hodo_position_Y = new TH1F("fiber Y", "fiber Y", 32, 0.5, 32.5);//number of fibers touched in the Y plane
  Hodo_cluster->SetLineColor(kBlue);
  Hodo_cluster->GetXaxis()->SetTitle("N involved fibers");
  Hodo_cluster->GetYaxis()->SetTitle("Entries");
  Hodo_cluster->SetMinimum(0);
  Hodo_position_X->SetLineColor(kRed);
  Hodo_position_X->GetXaxis()->SetTitle("Fiber N X plane");
  Hodo_position_X->GetYaxis()->SetTitle("Entries");
  Hodo_position_X->SetMinimum(0);
  Hodo_position_Y->SetLineColor(kGreen);
  Hodo_position_Y->GetXaxis()->SetTitle("Fiber N Y plane");
  Hodo_position_Y->GetYaxis()->SetTitle("Entries");
  Hodo_position_Y->SetMinimum(0);

  TH1F *Hodo_cluster_X = new TH1F("Hodo_cluster size X", "Hodo_cluster size X", 21, -0.5, 20.5);//Hodo_cluster size for the X plane
  TH1F *Hodo_cluster_Y = new TH1F("Hodo_cluster size Y", "Hodo_cluster size Y", 21, -0.5, 20.5);//Hodo_cluster sieze for the Y plane
  TH1F *Hodo_cog_X = new TH1F("position X", "position X", 96, 0.5, 32.5);//center of gravity of interaction in the X plane
  TH1F *Hodo_cog_Y = new TH1F("position Y", "position Y", 96, 0.5, 32.5);//center of gravity of interaction in the Y plane
  Hodo_cluster_X->SetLineColor(kRed);
  Hodo_cluster_X->GetXaxis()->SetTitle("N involved fibers X");
  Hodo_cluster_X->GetYaxis()->SetTitle("Entries");
  Hodo_cluster_X->SetMinimum(0);
  Hodo_cluster_Y->SetLineColor(kGreen);
  Hodo_cluster_Y->GetXaxis()->SetTitle("N involved fibers Y");
  Hodo_cluster_Y->GetYaxis()->SetTitle("Entries");
  Hodo_cluster_Y->SetMinimum(0);
  Hodo_cog_X->SetLineColor(kRed);
  Hodo_cog_X->GetXaxis()->SetTitle("Position X plane");
  Hodo_cog_X->GetYaxis()->SetTitle("Entries");
  Hodo_cog_X->SetMinimum(0);
  Hodo_cog_Y->SetLineColor(kGreen);
  Hodo_cog_Y->GetXaxis()->SetTitle("Position Y plane");
  Hodo_cog_Y->GetYaxis()->SetTitle("Entries");
  Hodo_cog_Y->SetMinimum(0);

  TH2F *Hodo_pos_2D = new TH2F("2d pos", "2D pos", 96, 0.5, 32.5, 96, 0.5, 32.5);//2D map of coincidences
  Hodo_pos_2D->GetXaxis()->SetTitle("Position X");
  Hodo_pos_2D->GetYaxis()->SetTitle("Position Y");
  Hodo_pos_2D->SetMinimum(0);

  //Preparing stats to be shown
  TPaveText *Hodo_pt;
  //Preparing canvas
  TCanvas *Hodo_complex_canv = new TCanvas("all hodo data", "all hodo data", 1200, 1000);
  Hodo_complex_canv->SetFillColor(0); //
  Hodo_complex_canv->SetBorderMode(0);	//
  Hodo_complex_canv->SetLeftMargin(0.1409396); //
  Hodo_complex_canv->SetRightMargin(0.14865772); //
  gStyle->SetOptStat(000); //
  //Divide canvas to fill with all the desired plots
  TPad *Hodo_raw_dt_pad = new TPad("Hodo_raw_dt_pad", "Hodo_raw_dt_pad", 0.03, 0.69, 0.48, 0.98, 0);
  TPad *Hodo_treat_cluster_pad = new TPad("Hodo_treat_cluster_pad", "Hodo_treat_cluster_pad", 0.03, 0.36, 0.48, 0.67, 0);
  TPad *Hodo_treat_positions_pad = new TPad("Hodo_treat_positions_pad", "Hodo_treat_positions_pad", 0.03, 0.03, 0.48, 0.34, 0);
  TPad *Hodo_map_pad = new TPad("Hodo_map_pad", "Hodo_map_pad", 0.5, 0.36, 0.98, 0.98, 0);
  TPad *Hodo_stats_pad = new TPad("Hodo_stats_pad", "Hodo_stats_pad", 0.5, 0.03, 0.98, 0.31, 0);
  Hodo_raw_dt_pad->Draw();
  Hodo_treat_cluster_pad->Draw();
  Hodo_treat_positions_pad->Draw();
  Hodo_map_pad->Draw();
  Hodo_stats_pad->Draw();

  Hodo_raw_dt_pad->Divide(3, 1);
  Hodo_raw_dt_pad->cd(1);
  Hodo_cluster->Draw();
  Hodo_raw_dt_pad->cd(2);
  Hodo_position_X->Draw();
  Hodo_raw_dt_pad->cd(3);
  Hodo_position_Y->Draw();
  Hodo_treat_cluster_pad->Divide(3, 1);
  Hodo_treat_cluster_pad->cd(1);
  Hodo_cluster_X->Draw();
  Hodo_treat_cluster_pad->cd(2);
  Hodo_cluster_Y->Draw();
  Hodo_treat_cluster_pad->cd(3);
  Hodo_flight_temp_spectra->Draw();
  Hodo_treat_positions_pad->Divide(2,1);
  Hodo_treat_positions_pad->cd(1);
  Hodo_cog_X->Draw();
  Hodo_treat_positions_pad->cd(2);
  Hodo_cog_Y->Draw();
  Hodo_map_pad->cd();
  Hodo_pos_2D->Draw("colz");
  Hodo_stats_pad->cd();
  Hodo_pt = new TPaveText(.05,.1,.95,.9);
  Hodo_pt->AddText(Form("#bf{RUN %d - monitoring stats}", runN));
  Hodo_pt->Draw();
/************************************************/
/*												*/
/*				BGO Variables					*/
/*												*/
/************************************************/
  int blocknum=0;
  int temp_count_event = -1;
  int temp_num_int = 0;
  int integral[4];
  int bloc_hit_per_event=0;
  double mean_BGO_block_hit_per_event=0;
  double temp_mean_block=0;
  int N_samples=1024;////Number of samples
  for (int init=0;init<4;init++){
   integral[init]=0;
  }
  int SUMINT=0;
  double BGO_PosX=0;
  double BGO_PosY=0;
  double BGO_temp_mean_X=0;
  double BGO_temp_mean_Y=0;
  int BGO_count_PosXY=0;
  double samplepm[1023];
  double baseline=0;
  
  //PM equilibration variables
  double ref_en=1250;
  double max_PM0[6];
  double max_PM1[6];
  double max_PM2[6];
  double max_PM3[6];
  double corrPM[6][4];

  //block 7624
  corrPM[0][0]=1048650;
  corrPM[0][1]=1346240;
  corrPM[0][2]=1048650;
  corrPM[0][3]=1095890;
  max_PM0[0]=89000;
  max_PM1[0]=91000;
  max_PM2[0]=87000;
  max_PM3[0]=87000;
  //block 7651
  corrPM[1][0]=836760;
  corrPM[1][1]=1288690;
  corrPM[1][2]=977525;
  corrPM[1][3]=1107180;
  max_PM0[1]=103000;
  max_PM1[1]=105000;
  max_PM2[1]=103000;
  max_PM3[1]=105000;
  //block 7369
  corrPM[2][0]=1535190;
  corrPM[2][1]=1535190;
  corrPM[2][2]=940009;
  corrPM[2][3]=1360420;
  max_PM0[2]=85000;
  max_PM1[2]=87000;
  max_PM2[2]=85000;
  max_PM3[2]=83000;
  //block 7586
  corrPM[3][0]=1388760;
  corrPM[3][1]=1029760;
  corrPM[3][2]=1289560;
  corrPM[3][3]=916391;
  max_PM0[3]=85000;
  max_PM1[3]=83000;
  max_PM2[3]=85000;
  max_PM3[3]=81000;
  //block 31210
  corrPM[4][0]=1049470;
  corrPM[4][1]=1049470;
  corrPM[4][2]=1080350;
  corrPM[4][3]=1116370;
  max_PM0[4]=99000;
  max_PM1[4]=93000;
  max_PM2[4]=95000;
  max_PM3[4]=99000;
  //block 3171
  corrPM[5][0]=689189;
  corrPM[5][1]=802419;
  corrPM[5][2]=982557;
  corrPM[5][3]=758671;
  max_PM0[5]=97000;
  max_PM1[5]=99000;
  max_PM2[5]=101000;
  max_PM3[5]=101000;

    //block 7624
  //block 7651
  //block 7369
  //block 7586
//   corrPM[3][0]=1077000;
//   corrPM[3][1]=783023;
//   corrPM[3][2]=987246;
//   corrPM[3][3]=650760;

  //block 3171


/************************************************/
/*												*/
/*				Hodo Variables					*/
/*												*/
/************************************************/
  int coinc_counter = 0;
  int molt_X = 0, molt_Y = 0;
  int conv_X=0., conv_Y=0.;
  bool flag_X = false, flag_Y = false;
  double temp_pos_X = 0., temp_pos_Y = 0.;
  std::vector <int> vec_x, vec_y;
  double fin_pos_x, fin_pos_y;
  int fin_molt_X = 0, fin_molt_Y = 0;


  int cnt = 0;
  int cnt1 = 0;
  //Creating string to check that the selected files refer to the present run - all other data files are ignored
  std::stringstream  s;
  s<<runN;
  s<<"-";
  //Creating structures and variables to search in the data folder for files
  struct dirent **files;
  std::stringstream  ss;
  struct dirent **files2;
  std::stringstream  sss;
  std::fstream fp;
  bool cond = true;
  bool flag = 0;

  while(cond){//while to wait the presence of at least one complete file to start the monitoring
   std::cout<<"Waiting for data files to begin analysis ..."<<std::endl;
   count = scandir(path, &files, file_select, alphasort);//how many files in the folder?
   if(count>0 && flag == 0){
    std::cout<<"Number of data file found "<<count<<std::endl;
    std::cout<<"Recovered file list"<<std::endl;
    for(int x = 0; x<count; x++){
     if(strstr(files[x]->d_name, s.str().c_str())){
	  printf("-> %s", files[x]->d_name);
	  std::cout<<std::endl;
	  cnt++;
	 }else{continue;}
    }
    if(cnt>0)flag = 1;
   }
   if(cnt>1){
    sss.str("");
    sss<<path;
    sss<<filemain;
    sss<<0;
    sss<<format;
    fp.open(sss.str().c_str(), std::fstream::in | std::fstream::binary);
    if(fp){
     std::cout<<"FILE open"<<std::endl;
     std::cout<<"Analysing file "<<sss.str().c_str()<<" ..... "<<std::endl;
     fp.seekg(0, std::ios::end);
     length = fp.tellg();
     std::cout<<"The file size is "<<length<<std::endl;
     fp.seekg(0, std::ios::beg);//put the read pointer at the beginning of the file
//---------------------------------------Reading the file header----------------------------------------//  
     fp.read((char *)init_file, sizeof(init_file));//reading the file header
     unpack_fileHead((unsigned char *)init_file, file_beg);
     if(file_beg.file_beg_id==beginning_file_id){
      std::cout<<"Beginning of new file read well "<<std::endl;
      std::cout<<"This is run "<<file_beg.run_number<<" and this file contains "<<file_beg.Ntot_events<<std::endl;
	  std::cout<<sizeof(init_file)<<std::endl;
//       N_ev_toRead = (int)file_beg.Ntot_events;
       N_ev_toRead=50;
     }
    }else{std::cout<<"Problem opening the data file "<<std::endl;}
    cond = !cond;
    std::cout<<"First loop over"<<std::endl;
    break;//go out from this first while loop and start loopin on events
   }
   else{
    std::cout<<"Still waiting "<<std::endl;
    flag = 0;
    usleep(1000000);
   }
  }

  while(true){//Main while on events and files after first while used for inizialization
   if(N_event>=N_ev_toRead || fp.eof()){//if we are reading more events than requested or we are at the end of the file
    sss.str("");
    sss << path;
    count1 = scandir(path, &files, file_select, alphasort);//how many files in the folder?
    cnt1=0;
    for(int w = 0; w<count1; w++){
     if(strstr(files[w]->d_name, s.str().c_str())){
	  cnt1++;
	 }else{continue;}
    }
    if(cnt1>=cnt && cnt>(files_read+1)){//we found new files that have been closed for sure
     std::cout<<"There are "<< cnt1 <<" files, and still "<<cnt1-files_read<<" files to be analyzed! GO! "<<std::endl;
     N_event=0;
     fp.close();//closing the old file
     files_read++;//increment the number of files already analysed
     std::cout<<"End of analysis for file "<<files_read<<std::endl;
     sss<<filemain;
     sss<<files_read;
     sss<<format;
     fp.open(sss.str().c_str(), std::fstream::in | std::fstream::binary);//opening the new file
     if(fp){
      std::cout<<"FILE open"<<std::endl;
      std::cout<<"Analysing file "<<sss.str().c_str()<<" ..... "<<std::endl;
      fp.seekg(0, std::ios::end);
      length = fp.tellg();
      std::cout<<"The file size is "<<length<<std::endl;
      fp.seekg(0, std::ios::beg);//put the read pointer at the beginning of the file
//---------------------------------------Reading the file header----------------------------------------//  
      fp.read((char *)init_file, sizeof(init_file));//reading the file header
      unpack_fileHead((unsigned char *)init_file, file_beg);
      if(file_beg.file_beg_id==beginning_file_id){
       std::cout<<"Beginning of new file read well "<<std::endl;
       std::cout<<"This is run "<<file_beg.run_number<<" and this file contains "<<file_beg.Ntot_events<<std::endl;
//        N_ev_toRead = (int)file_beg.Ntot_events;
        N_ev_toRead=50;

      }
     }
    }
    else if(cnt1>(files_read+2) && cnt<=(files_read+1)){
     std::cout<<"There are "<< cnt1<<" files "<<std::endl;
     N_event=0;
     fp.close();//closing the old file
     files_read++;//increment the number of files already analysed
     std::cout<<"End of analysis for file "<<files_read<<std::endl;
     cnt = cnt1-1;
     sss<<filemain;
     sss<<files_read;
     sss<<format;
     fp.open(sss.str().c_str(), std::fstream::in | std::fstream::binary);//opening the new file
     if(fp){
      std::cout<<"FILE open"<<std::endl;
      std::cout<<"Analysing file "<<sss.str().c_str()<<" ..... "<<std::endl;
      fp.seekg(0, std::ios::end);
      length = fp.tellg();
      std::cout<<"The file size is "<<length<<std::endl;
      fp.seekg(0, std::ios::beg);//put the read pointer at the beginning of the file
//--------------------------------------Reading the file header----------------------------------------//  
      fp.read((char *)init_file, sizeof(init_file));//reading the file header
      unpack_fileHead((unsigned char *)init_file, file_beg);
      if(file_beg.file_beg_id==beginning_file_id){
       std::cout<<"Beginning of new file read well "<<std::endl;
       std::cout<<"This is run "<<file_beg.run_number<<" and this file contains "<<file_beg.Ntot_events<<std::endl;
//        N_ev_toRead = (int)file_beg.Ntot_events;
        N_ev_toRead=50;
      }
     }
     }else if(wait_loop > time_to_wait){std::cout<<"The acquisition probably stopped! EXIT! "<<std::endl; BGO_complex_canv->SaveAs(Form("monitoring_run%d.png", runN)); return 0;
     }else{std::cout<<"Waiting for data files to analyse ..."<<std::endl; usleep(1000000); wait_loop++; continue;}
    }
    if(fp){
     //std::cout << "N_event : " << N_event << std::endl;
//---------------------------------------reading the event header----------------------------------------//  
     fp.read((char *)event_header, sizeof(event_header));
     unpack_eventHead((unsigned char *)event_header, event_id);
     if(event_id.event_beg_id != beginning_event){
      std::cout<<"PROBLEM IN READING DATA -> CHECK THE DATA STRUCTURE! "<<std::hex<< event_id.event_beg_id<<std::endl;
      std::cout<<sizeof(event_header)<<std::endl;
      std::cout<<event_id.event_beg_id<<std::endl;
      std::cout<<event_id.event_number<<std::endl;
	  std::cout<<event_id.trigger_number<<std::endl;
	  std::cout<<event_id.hit_in_trig<<std::endl;
	  return 0;
     }
     //std::cout << "event_id.hit_in_trig : " << event_id.hit_in_trig << std::endl;
     for (int Hit_In_Trigger = 0 ; Hit_In_Trigger<event_id.hit_in_trig ; Hit_In_Trigger++){
      fp.read((char *)data_main_structure, sizeof(data_main_structure));//reading the data main part
      unpack_dataMain((unsigned char *)data_main_structure, data_struct);
      if(data_struct.modules_num>0){
/************************************************/
/*												*/
/*				BGO Mode 22						*/
/*												*/
/************************************************/
       if (data_struct.mode_num == 22){
        if (temp_num_int==4){
         for (int init=0;init<4;init++){
          integral[init]=0;
         }
         temp_num_int=0;
        }
        //std::cout << "temp_count_event : " << temp_count_event << std::endl;
        //std::cout << "temp_num_int : " << temp_num_int << std::endl;
        for (int SampleNum=0; SampleNum<data_struct.modules_num;SampleNum++){
         fp.read((char *)hit_structure_mode0, sizeof(hit_structure_mode0));
         unpack_data((unsigned char *)hit_structure_mode0, data_block);
         bzero(hit_structure_mode0, 7);
         for (int NUMSAMPLE=0; NUMSAMPLE<data_block.Nb_samples_rec ; NUMSAMPLE++){
	      fp.read((char *)sample_structure_mode, sizeof(sample_structure_mode));
	      unpack_sample((unsigned char *)sample_structure_mode, Echt);
          bzero(sample_structure_mode, 2);
          samplepm[NUMSAMPLE]=Echt.sample;
         }
         baseline=0;
         for (int calcBL=0; calcBL<150; calcBL++){
	   baseline=baseline+samplepm[calcBL];
	 }
	 baseline=baseline/150;
	 for (int calcINT=150; calcINT<data_block.Nb_samples_rec; calcINT++){
	   integral[temp_num_int]=integral[temp_num_int]+samplepm[calcINT]-baseline;
	 }
        }
        temp_num_int++;
        if (temp_num_int==4){
         blocknum=floor(data_block.N_PM_rec/4);
	 integral[0] = (integral[0]-max_PM0[blocknum])*(ref_en/corrPM[blocknum][0]);
	 integral[1] = (integral[1]-max_PM1[blocknum])*(ref_en/corrPM[blocknum][1]);
	 integral[2] = (integral[2]-max_PM2[blocknum])*(ref_en/corrPM[blocknum][2]);
         integral[3] = (integral[3]-max_PM3[blocknum])*(ref_en/corrPM[blocknum][3]);
         bloc_hit_per_event++;
         BGO_count_PosXY++;
	 if(integral[0]>=0 && integral[1]>=0 && integral[2]>=0 && integral[3]>=0){
          SUMINT=integral[0]+integral[1]+integral[2]+integral[3];
          BGO_PosX=1.*((integral[0]+integral[1])-(integral[2]+integral[3]))/SUMINT;
          BGO_PosY=1.*((integral[2]+integral[0])-(integral[3]+integral[1]))/SUMINT;
	  if (blocknum<2){
	    BGO_pos_2D->Fill(((BGO_PosX+1)*1.75)+(blocknum*3.5),((BGO_PosY+1)*1.75));
            BGO_position_X->Fill(((BGO_PosX+1)*1.75)+(blocknum*3.5));
            BGO_position_Y->Fill(((BGO_PosY+1)*1.75));
	  }
	  if (blocknum>1 && blocknum<4){
	    BGO_pos_2D->Fill(((BGO_PosX+1)*1.75)+(floor(blocknum/3)*3.5),((BGO_PosY+1)*1.75)+(floor(blocknum/2)*3.5));
            BGO_position_X->Fill(((BGO_PosX+1)*1.75)+(floor(blocknum/2)*3.5));
            BGO_position_Y->Fill(((BGO_PosY+1)*1.75)+(floor(blocknum/2)*3.5));
	  }
	  if (blocknum>3 && blocknum<6){
	    BGO_pos_2D->Fill(((BGO_PosX+1)*1.75)+(floor(blocknum/5)*3.5),((BGO_PosY+1)*1.75)+(floor(blocknum/2)*3.5));
            BGO_position_X->Fill(((BGO_PosX+1)*1.75)+(floor(blocknum/2)*3.5));
            BGO_position_Y->Fill(((BGO_PosY+1)*1.75)+(floor(blocknum/2)*3.5));
	  }
          BGO_temp_mean_X=BGO_temp_mean_X+(((BGO_PosX+1)*1.75)+(blocknum*3.5));
          BGO_temp_mean_Y=BGO_temp_mean_Y+((BGO_PosY+1)*1.75);
         }
	}
       }
/************************************************/
/*												*/
/*				Hodo Mode 40						*/
/*												*/
/************************************************/
       else if(data_struct.mode_num==40){
        for(int w = 0; w<data_struct.modules_num; w++){//reading the data from each involved fiber
	     fp.read((char *)hit_structure_mode1, sizeof(hit_structure_mode1));
	     unpack_data1((unsigned char *)hit_structure_mode1, data_fiber);
	     if(data_fiber.N_fiber_rec>=32){
	 	  conv_X = convert_Xchannel_withCable(data_fiber.N_fiber_rec-32);
 		  if(conv_X > 0){
		   Hodo_position_X->Fill(conv_X);
		   molt_X++;
		   temp_pos_X += (conv_X);
           vec_x.push_back(conv_X);
		  }
		  else{std::cout<<"Problem 1"<<std::endl; return 0;}
	     }
	     else if(data_fiber.N_fiber_rec<32){
	      conv_Y = convert_Ychannel_withCable(data_fiber.N_fiber_rec);
		  if(conv_Y>0){
		   Hodo_position_Y->Fill(conv_Y);
		   molt_Y++;
		   temp_pos_Y += (conv_Y);
           vec_y.push_back(conv_Y);
		  }
		  else{std::cout<<"Problem 2"<<std::endl; return 0;}
	     }
	     bzero(hit_structure_mode1, 5);
        }
        if(molt_X>0){
         fin_molt_X = check_mult(vec_x, 3, fin_pos_x);
        }else{fin_molt_X = molt_X;}
        if(molt_Y>0){
         fin_molt_Y = check_mult(vec_y, 3, fin_pos_y);
        }else{fin_molt_Y = molt_Y;}
        temp_pos_X = fin_pos_x;
        temp_pos_Y = fin_pos_y;
        if(fin_molt_X>0){
         Hodo_cluster_X->Fill(fin_molt_X);
         Hodo_cog_X->Fill(temp_pos_X);
         flag_X = true;
        }
        if(fin_molt_Y>0){
         Hodo_cluster_Y->Fill(fin_molt_Y);
         Hodo_cog_Y->Fill(temp_pos_Y);
         flag_Y = true;
        }
        if(flag_X && flag_Y){
         Hodo_pos_2D->Fill(temp_pos_X, temp_pos_Y);
	 Hodo_flight_temp_spectra->Fill(data_fiber.hit_time_rec);
         coinc_counter++;
        }
        temp_pos_X = 0.; temp_pos_Y = 0.; molt_X = 0; molt_Y = 0; flag_X = false; flag_Y = false; conv_X = 0; conv_Y = 0;
        fin_molt_Y = 0; fin_pos_x = 0.; fin_pos_y = 0.; fin_molt_X = 0; vec_x.clear(); vec_y.clear();
       } 

       else{std::cout<<"Unknown data format - EXIT"<<std::endl; return 0;}
/************************************************/
/*												*/
/*					update						*/
/*												*/
/************************************************/
	   gSystem->ProcessEvents();
	   gSystem->Sleep(5);
       if (event_id.trigger_number != temp_count_event){
        temp_count_event=event_id.trigger_number;
        N_event++;
        events_read++;
        temp_mean_block=temp_mean_block+bloc_hit_per_event;
        if (bloc_hit_per_event != 0) BGO_block_hit->Fill(bloc_hit_per_event);
        bloc_hit_per_event=0;
        if(events_read%update == 0){
	     std::cout << "Event analyzed " << events_read << std::endl;
        }
        if(events_read%update == 0){
         //BGO update
	     BGO_complex_canv->cd();
	     BGO_raw_posx_pad->cd();
	     gPad->Modified();
	     gPad->Update();
	     BGO_raw_posy_pad->cd();
	     gPad->Modified();
	     gPad->Update();
	     BGO_complex_canv->cd();
	     BGO_map_pad->Modified();
	     BGO_map_pad->Update();
         BGO_block_hit_pad->Modified(); 
	     BGO_block_hit_pad->Update();
	     /*BGO_stats_pad->cd();
	     BGO_pt->Clear();
	     BGO_pt->AddText(Form("#bf{RUN %d - monitoring stats}", runN)); ((TText*)BGO_pt->GetListOfLines()->Last())->SetTextColor(kBlue); ((TText*)BGO_pt->GetListOfLines()->Last())->SetTextSize(0.18);
	     BGO_pt->AddLine(.0,.8,1.,.8);
	     BGO_pt->AddText(Form("#bf{-> Analysed events : %d }", events_read));
	     BGO_pt->AddText(Form("#bf{-> Number of blocks hit mean : %.2f }", temp_mean_block/events_read));
	     BGO_pt->AddText(Form("#bf{-> Average absobed gamma position : %.2f mm - %.2f mm }", BGO_temp_mean_X/BGO_count_PosXY, BGO_temp_mean_Y/BGO_count_PosXY));
	     BGO_pt->Draw();
	     BGO_stats_pad->Update();*/
         //Hodo update
	     for(int pd2 = 1; pd2 < 4; pd2++){
	      Hodo_complex_canv->cd();
	      Hodo_raw_dt_pad->cd(pd2);
	      gPad->Modified();
	      gPad->Update();
	       Hodo_treat_cluster_pad->cd(pd2);
	       gPad->Modified();
	       gPad->Update();
	      if(pd2<3){
	       Hodo_treat_positions_pad->cd(pd2);
	       gPad->Modified();
	       gPad->Update();
	      }
	     }
	     Hodo_complex_canv->cd();
	     Hodo_map_pad->Modified();
	     Hodo_map_pad->Update();
	     Hodo_stats_pad->cd();
	     Hodo_pt->Clear();
	     Hodo_pt->AddText(Form("#bf{RUN %d - monitoring stats}", runN)); ((TText*)Hodo_pt->GetListOfLines()->Last())->SetTextColor(kBlue); ((TText*)Hodo_pt->GetListOfLines()->Last())->SetTextSize(0.18);
	     Hodo_pt->AddLine(.0,.8,1.,.8);
	     Hodo_pt->AddText(Form("#bf{-> Analysed events : %d }", events_read));
	     Hodo_pt->AddText(Form("#bf{-> Detected coincidences : %d -> %.2f per cent of events }", coinc_counter, ((float)(coinc_counter)*100)/(float)(events_read)));
	     Hodo_pt->AddText(Form("#bf{->  Average beam position : %.2f mm - %.2f mm }", Hodo_cog_X->GetMean(), Hodo_cog_Y->GetMean()));
	     Hodo_pt->Draw();
	     Hodo_stats_pad->Update();
        }else{continue;}
       }
      }else{continue;}
     }
    } 
   }//end of while loop for reading events and files

  BGO_complex_canv->Connect("Closed()", "TApplication", gApplication, "Terminate()");
  Hodo_complex_canv->Connect("Closed()", "TApplication", gApplication, "Terminate()");
  theApp.Run();


 return 0;
}
