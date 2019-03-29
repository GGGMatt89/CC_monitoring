//**********************************************//
//												//
//					BGO							//
//												//
//**********************************************//
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
 int update = 1;//to be inserted by the user
 std::cout<<"Type the run number : (return to confirm)"<<std::endl;
 std::cin>>runN;
 //std::cout<<"Choose the frequency for updating plots : (number of events to analyse before updating)"<<std::endl;
 //std::cin>>update;

  char path[100] = "/media/daq/porsche2/data_gamhadron/";//"/home/daq/gamhadron/daqGh/data/";//folder where the data are saved
  char filemain[100];//main part of the file name - only numbering stuff after that
  int xx = sprintf(filemain, "%d-", runN);
  char format[30] =".dat";//data format -> the acquisition software creates .dat files
  //-----------------------------------------------------------------------------------------------------------------------------//
  //---------------------------------------------DON'T MODIFY SINCE NOW ON (if not needed :-) )----------------------------------//
  //-----------------------------------------------------------------------------------------------------------------------------//

  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();
  double elapsed_seconds=0;

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
  //Creating the desired plots
  TPaveText *pt;
  int N_samples=1023;////Number of samples
  TH1F** Hist_Channel = new TH1F*[24];
  for (int init=0; init<24; init++){
   Hist_Channel[init]=new TH1F(Form("Channel_%d",init), Form("Channel_%d",init), N_samples, 0, N_samples);
   if (init<4)Hist_Channel[init]->SetLineColor(kRed);
   if (init>3 && init<8)Hist_Channel[init]->SetLineColor(kGreen);
   if (init>7 && init<12)Hist_Channel[init]->SetLineColor(kBlue);
   if (init>11 && init<16)Hist_Channel[init]->SetLineColor(kOrange);
   if (init>15 && init<20)Hist_Channel[init]->SetLineColor(kBlack);
   if (init>19)Hist_Channel[init]->SetLineColor(kCyan);
   Hist_Channel[init]->GetXaxis()->SetTitle(" ");
   Hist_Channel[init]->GetYaxis()->SetTitle(" ");
   Hist_Channel[init]->SetMinimum(0);
   Hist_Channel[init]->GetYaxis()->SetRangeUser(0,4000);
  }
/////////////////////////////////////////////////////////////////////////////////////////////////
  TCanvas *complex_canv = new TCanvas("all data", "all data", 1200, 1000);
  complex_canv->SetFillColor(0); //
  complex_canv->SetBorderMode(0);	//
  complex_canv->SetLeftMargin(0.1409396); //
  complex_canv->SetRightMargin(0.14865772); //
  gStyle->SetOptStat(000); //

  TPad *RUN_NUM = new TPad("RUN_NUM", "RUN_NUM", 0.03, 0.85, 0.98, 0.99, 0);
  TPad *block_1 = new TPad("block_1", "block_1", 0.03, 0.71, 0.98, 0.85, 0);
  TPad *block_2 = new TPad("block_2", "block_2", 0.03, 0.57, 0.98, 0.71, 0);
  TPad *block_3 = new TPad("block_3", "block_3", 0.03, 0.43, 0.98, 0.57, 0);
  TPad *block_4 = new TPad("block_4", "block_4", 0.03, 0.29, 0.98, 0.43, 0);
  TPad *block_5 = new TPad("block_5", "block_5", 0.03, 0.15, 0.98, 0.29, 0);
  TPad *block_6 = new TPad("block_6", "block_6", 0.03, 0.01, 0.98, 0.15, 0);
  RUN_NUM->Draw();
  block_1->Draw();
  block_2->Draw();
  block_3->Draw();
  block_4->Draw();
  block_5->Draw();
  block_6->Draw();

  RUN_NUM->cd();
  pt = new TPaveText(.05,.1,.95,.9);
  pt->AddText(Form("#bf{RUN %d}", runN));
  pt->Draw();

  block_1->Divide(4);
  block_1->cd(1);
  Hist_Channel[0]->Draw("hist");
  block_1->cd(2);
  Hist_Channel[1]->Draw("hist");
  block_1->cd(3);
  Hist_Channel[2]->Draw("hist");
  block_1->cd(4);
  Hist_Channel[3]->Draw("hist");

  block_2->Divide(4);
  block_2->cd(1);
  Hist_Channel[4]->Draw("hist");
  block_2->cd(2);
  Hist_Channel[5]->Draw("hist");
  block_2->cd(3);
  Hist_Channel[6]->Draw("hist");
  block_2->cd(4);
  Hist_Channel[7]->Draw("hist");

  block_3->Divide(4);
  block_3->cd(1);
  Hist_Channel[8]->Draw("hist");
  block_3->cd(2);
  Hist_Channel[9]->Draw("hist");
  block_3->cd(3);
  Hist_Channel[10]->Draw("hist");
  block_3->cd(4);
  Hist_Channel[11]->Draw("hist");

  block_4->Divide(4);
  block_4->cd(1);
  Hist_Channel[12]->Draw("hist");
  block_4->cd(2);
  Hist_Channel[13]->Draw("hist");
  block_4->cd(3);
  Hist_Channel[14]->Draw("hist");
  block_4->cd(4);
  Hist_Channel[15]->Draw("hist");

  block_5->Divide(4);
  block_5->cd(1);
  Hist_Channel[16]->Draw("hist");
  block_5->cd(2);
  Hist_Channel[17]->Draw("hist");
  block_5->cd(3);
  Hist_Channel[18]->Draw("hist");
  block_5->cd(4);
  Hist_Channel[19]->Draw("hist");

  block_6->Divide(4);
  block_6->cd(1);
  Hist_Channel[20]->Draw("hist");
  block_6->cd(2);
  Hist_Channel[21]->Draw("hist");
  block_6->cd(3);
  Hist_Channel[22]->Draw("hist");
  block_6->cd(4);
  Hist_Channel[23]->Draw("hist");


  //----------------------------------------------------END creation of desired plots----------------------------------------------------//

  //Variables for preliminary analysis
int num_PM;
  int blocknum=0;
  int temp_count_event = -1;
  int temp_num_int = 0;
  int integral[4];
  int bloc_hit_per_event=0;
  double mean_block_hit_per_event=0;
  double temp_mean_block=0;
  for (int init=0;init<4;init++){
   integral[init]=0;
  }
  int SUMINT=0;
  double PosX=0;
  double PosY=0;
  double temp_mean_X=0;
  double temp_mean_Y=0;
  int count_posXY=0;

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
//       N_ev_toRead = (int)file_beg.Ntot_events/5;
             N_ev_toRead = 5;

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
//        N_ev_toRead = (int)file_beg.Ntot_events/5;
       N_ev_toRead = 5;
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
//        N_ev_toRead = (int)file_beg.Ntot_events/5;
              N_ev_toRead = 5;

      }
     }
     }else if(wait_loop > time_to_wait){std::cout<<"The acquisition probably stopped! EXIT! "<<std::endl; complex_canv->SaveAs(Form("monitoring_run%d.png", runN)); return 0;
     }else{std::cout<<"Waiting for data files to analyse ..."<<std::endl; usleep(1000000); wait_loop++; continue;}
    }
    if(fp){
     //std::cout << "N_event : " << N_event << std::endl;
    for (int reset=0; reset<24; reset++){
//      Hist_Channel[reset]->Add(Hist_Channel[reset],-1);
      Hist_Channel[reset]->Reset("ICESM");
    }
//---------------------------------------reading the event header----------------------------------------//  
     fp.read((char *)event_header, sizeof(event_header));
     unpack_eventHead((unsigned char *)event_header, event_id);
     if(event_id.event_beg_id != beginning_event){
      std::cout<<"PROBLEM IN READING DATA -> CHECK THE DATA STRUCTURE! "/*<<std::hex<< "event_id.event_beg_id : " <<event_id.event_beg_id*/<<std::endl;
      std::cout<<"Decimal : "<<std::endl;
      std::cout<<"sizeof(event_header)\t: "<<sizeof(event_header)<<std::endl;
      std::cout<<"event_id.event_beg_id\t: "<<event_id.event_beg_id<<std::endl;
      std::cout<<"event_id.event_number\t: "<<event_id.event_number<<std::endl;
      std::cout<<"event_id.trigger_number\t: "<<event_id.trigger_number<<std::endl;
      std::cout<<"event_id.hit_in_trig\t: "<<event_id.hit_in_trig<<std::endl;
      std::cout<<std::endl;  
      std::cout<<"Hex : "<<std::endl;
      std::cout<<std::hex<<"sizeof(event_header)\t: "<<sizeof(event_header)<<std::endl;
      std::cout<<"event_id.event_beg_id\t: "<<event_id.event_beg_id<<std::endl;
      std::cout<<"event_id.event_number\t: "<<event_id.event_number<<std::endl;
      std::cout<<"event_id.trigger_number\t: "<<event_id.trigger_number<<std::endl;
      std::cout<<"event_id.hit_in_trig\t: "<<event_id.hit_in_trig<<std::endl;
	  return 0;
     }
     //std::cout << "event_id.hit_in_trig : " << event_id.hit_in_trig << std::endl;
     for (int Hit_In_Trigger = 0 ; Hit_In_Trigger<event_id.hit_in_trig ; Hit_In_Trigger++){
      fp.read((char *)data_main_structure, sizeof(data_main_structure));//reading the data main part
      unpack_dataMain((unsigned char *)data_main_structure, data_struct);
      if(data_struct.modules_num>0){
      if (data_struct.mode_num != 7){
       for (int SampleNum=0; SampleNum<data_struct.modules_num;SampleNum++){
        fp.read((char *)hit_structure_mode0, sizeof(hit_structure_mode0));
        unpack_data((unsigned char *)hit_structure_mode0, data_block);
        bzero(hit_structure_mode0, 7);
        events_read++;
	if (events_read%4==0)N_event++;
        //Hist_Channel[num_PM]->Add(Hist_Channel[num_PM],-1);
        for (int NUMSAMPLE=0; NUMSAMPLE<data_block.Nb_samples_rec ; NUMSAMPLE++){
	     fp.read((char *)sample_structure_mode, sizeof(sample_structure_mode));
	     unpack_sample((unsigned char *)sample_structure_mode, Echt);
         bzero(sample_structure_mode, 2);
         num_PM=data_block.N_PM_rec;
         Hist_Channel[num_PM]->Fill(NUMSAMPLE,Echt.sample);
        }
       }
      }
      else if(data_struct.mode_num==7){
	   for(int w = 0; w<data_struct.modules_num; w++){//reading the data from each involved fiber
	    fp.read((char *)hit_structure_mode1, sizeof(hit_structure_mode1));
	   }
      }
      else{std::cout<<"Unknown data format - EXIT"<<std::endl; return 0;}

// for (int setyaxismax=0; setyaxismax<24; setyaxismax++){
 Hist_Channel[num_PM]->GetYaxis()->SetRangeUser(0,4000);
 if (runN==128 || runN==93) Hist_Channel[num_PM]->GetYaxis()->SetRangeUser(0,500);

// }
        //gSystem->ProcessEvents();
	//gSystem->Sleep(5);
//      if(events_read%update == 0){
      complex_canv->ClearPadSave();
      //complex_canv->Reset("ICESM");
      complex_canv->cd();
	  RUN_NUM->cd();
	  pt->Clear();
	  //pt->AddText(Form("#bf{RUN %d - monitoring stats}", runN)); ((TText*)pt->GetListOfLines()->Last())->SetTextColor(kBlue); ((TText*)pt->GetListOfLines()->Last())->SetTextSize(0.18);
	  //pt->AddText(Form("#bf{-> Analysed events : %d }", events_read));
	  pt->Draw();
	  RUN_NUM->Update();
      for(int pd2 = 1; pd2 < 5; pd2++){
	   block_1->cd(pd2);
	   gPad->Modified();
	   gPad->Update();
// 	   gPad->Clear();
      }
      for(int pd2 = 1; pd2 < 5; pd2++){
	   block_2->cd(pd2);
	   gPad->Modified();
 	   gPad->Update();
	  }
      for(int pd2 = 1; pd2 < 5; pd2++){
	   block_3->cd(pd2);
	   gPad->Modified();
	   gPad->Update();
// 	   gPad->Clear();
	  }
      for(int pd2 = 1; pd2 < 5; pd2++){
	   block_4->cd(pd2);
	   gPad->Modified();
	   gPad->Update();
// 	   gPad->Clear();
	  }
      for(int pd2 = 1; pd2 < 5; pd2++){
	   block_5->cd(pd2);
	   gPad->Modified();
	   gPad->Update();
// 	   gPad->Clear();
	  }
      for(int pd2 = 1; pd2 < 5; pd2++){
	   block_6->cd(pd2);
	   gPad->Modified();
	   gPad->Update();
// 	   gPad->Clear();
	  }
	  std::cout<<"Event analyzed "<<N_event<<std::endl;
//      }else{continue;}
 

    }else{continue;}
   }
   } 
  }//end of while loop for reading events and files
  
 complex_canv->Connect("Closed()", "TApplication", gApplication, "Terminate()");
 theApp.Run();


 return 0;
}
