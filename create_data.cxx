
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

//c++ classes
#include <sstream>
#include <cstring>
#include <dirent.h>//class used to manipulate and traverse directories
#include <mutex>
//c++ classes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#pragma pack(1)
#include <stdint.h>
#include <unistd.h>
#include <ctime>

//ROOT classes
#include "TRandom.h"
#include "TRandom3.h"
#include "TClass.h"
#include "TMath.h"


//custom classes
#include "functions.h"

using namespace std;
//This macro mimic the hodoscope data acquisition system
//It produces random numbers and saves them to binary files according to the hodoscope data structure
//The data files here are saved in the folder ./data/ with the naming structure runN-data_n.dat with N number of run, n number of file, which increases during the acquisition
//The number of events stored in each file is fixed to 10000 for this simulation

int main(int argc,char ** argv){

  cout<<"CREATING DATA FILES ..."<<endl;
  char test_fileHead[10];//Buffer for file header
  char test_eventHead[11];//Buffer for evet header
  char test_hitHead[7];//Buffer for data main part
  char test_data[7];//Buffer for data
  char test_sample[2];//Buffer for charge
  char test_fileEnd[10];
  //FIX HERE SOME PARAMETERS
  uint32_t runN = 3022;
  uint16_t beg_file = 0xF0F0;
  uint16_t end_file = 0xF1F1;//FILE HEADER
  int N_samples = 1024;//Number of samples
  int file_counter = 0;
  int inv_modules = 0;//N of modules touched in the event
  int inv_PM = 0;//N of PM written in the event
  int block_N = 0;//N of blocks for the data
  int PM_N = 0;//N of PM for the data
  int temp_block_N[6];
  int block_time = 0;//Detected time for the block
  int N_files = 10;//Number of files to be created
  int ev_in_file = 1000;//Number of event in each file
  int oct_counter = 0;
  srand((int)time(0));
  int ll = 0;//loop variables
  int loo = 0;
  int PMnum = 4;//PM number
  int FE=0;
//  int tot_invoved_mod=0;
  gRandom = new TRandom3();
  TRandom *r4 = new TRandom3();
  double rand[10000];
  double rand2=0;
  int rand_int=0;
  int Ech_final[N_samples];

  TH1F* hist = new TH1F("hist", "hist", N_samples, 0, N_samples);
  //srand (time(NULL));

  //Packing file header parameters
  pack_header((unsigned char *)test_fileHead, beg_file, runN, ev_in_file);
  //Creating data file
  fstream output_file;
  char buffer[50]; // make sure it's big enough -> it's for the file name
  snprintf(buffer, sizeof(buffer), "../../data/simu_monitoring_data/%d-%d.dat", runN, file_counter);
  output_file.open(buffer, std::fstream::out | std::fstream::binary);
  if(!output_file){
    cout<<"Error opening file"<<endl;
  }
  else{
    //if the file is open, write the file header
    output_file.write((char *)test_fileHead, sizeof(test_fileHead));
    oct_counter+=sizeof(test_fileHead);
    output_file.flush();
  }
int pourcentage = 0;
int prctg = 0;
prctg=floor(ev_in_file/100);
  for(int files = 0; files<N_files; files++){
   pourcentage = 0;
   cout << "File " << files+1 << "/" << N_files << endl;
   for(ll = 0; ll<ev_in_file; ll++){
    if(ll%prctg==0){
     pourcentage++;
     ProgressBar(prctg, pourcentage);
    }

    //inv_modules = (rand() % 6)+1;//Extract a number of blocks involved in the event
    rand2 = r4->Rndm();
    for (int i = 0; i<6;i++){
     if (rand2 > (i/6.) && rand2 <= ((i+1)/6.)){
      inv_modules=i+1;
      break;
     }
    }

    for (int poi=0 ; poi<6 ; poi++){
     temp_block_N[poi]=10000;
    }

    for (int N_module=0; N_module<inv_modules; N_module++){

     do{
      rand2 = r4->Rndm();
      for (int hij = 0; hij<6;hij++){
       if (rand2 > (hij/6.) && rand2 <= ((hij+1)/6.)){
        block_N=hij;
        break;
       }
      }
     }
     while (temp_block_N[0]==block_N || temp_block_N[1]==block_N || temp_block_N[2]==block_N || temp_block_N[3]==block_N || temp_block_N[4]==block_N || temp_block_N[5]==block_N);
     temp_block_N[loo]=block_N;

     //Pack up event header and data main section
     inv_PM=4;
     for(loo = 0; loo<inv_PM; loo++){
     pack_eventHeader((unsigned char *)test_eventHead, (uint16_t)0xABCD, (uint32_t)ll, (uint32_t)ll, (uint16_t)1);
     output_file.write((char *) test_eventHead, sizeof(test_eventHead));
     oct_counter+=sizeof(test_eventHead);
     output_file.flush();


     pack_dataMain((unsigned char *)test_hitHead, (uint16_t)0x00EB, (uint8_t)FE, (uint32_t)ll,  (uint8_t)22, (uint8_t)1);
     //Write event on file

     output_file.write((char *) test_hitHead, sizeof(test_hitHead));
     oct_counter+=sizeof(test_hitHead);
     output_file.flush();
     //Creating the data for each involved blocks with random extraction (or fixing the parameters for test)

     PM_N=(block_N*4)+loo;
//cout << "PM_N : " << PM_N << endl;

      //block_time = rand() % 1028;
      rand2 = r4->Rndm();
      for (int i = 0; i<1028;i++){
       if (rand2>(i/1028.) && rand2 <= ((i+1)/1028.)){
        block_time=i;
        break;
       }
      }

      pack_data((unsigned char *) test_data, (uint8_t)PM_N, (uint32_t) block_time, (uint16_t) N_samples);
      output_file.write((char *) test_data, sizeof(test_data));
      oct_counter+=sizeof(test_data);
      output_file.flush();
      bzero(test_data, 7);

///////////////////////sample///////////////////////////
      rand2 = r4->Rndm();
      for (int i = 0; i<9000;i++){
       if (rand2>(i/9000.) && rand2 <= ((i+1)/9000.)){
        rand_int=i+1000;
        break;
       }
      }
      hist->Add(hist,-1);
      for (int gauspoints=0; gauspoints<rand_int; gauspoints++){
       rand[gauspoints] = gRandom->Gaus(N_samples/2., N_samples/10);
       hist->Fill(rand[gauspoints]);
      }
      for (int NumEch=0; NumEch<N_samples; NumEch++){
       Ech_final[NumEch]=hist->GetBinContent(NumEch);
       pack_sample((unsigned char *) test_sample, (uint16_t) Ech_final[NumEch]);
       output_file.write((char *) test_sample, sizeof(test_sample));
       oct_counter+=sizeof(test_sample);
       output_file.flush();
       bzero(test_sample, 2);
      }
      //Pack up the data structure for each involved blocks
     }
    }
    bzero(test_eventHead, 11);
    bzero(test_hitHead, 7);
   }
   pack_fileEnd((unsigned char *) test_fileEnd, (uint16_t) end_file, (uint16_t) ev_in_file, (uint32_t)oct_counter);
   output_file.write((char *) test_fileEnd, sizeof(test_fileEnd));
   oct_counter+=sizeof(test_fileEnd);
   output_file.close();
   if(files!=(N_files-1)){cout<<"File "<<file_counter+1<<" written -> Opening new file!"<<endl;}
   else{cout<<"File "<<file_counter+1<<" written -> Finish!"<<endl; break;}
   file_counter++;
   snprintf(buffer, sizeof(buffer), "../../data/simu_monitoring_data/%d-%d.dat", runN, file_counter);
   output_file.open(buffer, std::fstream::out | std::fstream::binary);
   if(!output_file){
    cout<<"Error opening file"<<endl;
   }
   else{
    output_file.write((char *)test_fileHead, sizeof(test_fileHead));
    oct_counter = sizeof(test_fileHead);
    output_file.flush();
   }
  }
  return 0;

}
