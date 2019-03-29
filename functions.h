#ifndef FUNCTIONS_
#define FUNCTIONS_

#include <stdint.h>
#include <vector>
#include <algorithm>

//VARIABLES
#define buffer_size 1472
#define BUFLEN 2048


//STRUCTURES
typedef struct hit_block_receive hit_block_rec;
struct hit_block_receive{
  uint32_t N_PM_rec;
  uint32_t hit_time_rec;
  uint16_t Nb_samples_rec; 
};

/*typedef struct sample_receive sample_rec;
struct sample_receive{
  uint16_t sample[1024]; 
};*/

typedef struct sample_receive sample_rec;
struct sample_receive{
  uint16_t sample; 
};

typedef struct _fileHeader {
  uint16_t file_beg_id;
  uint32_t run_number;
  uint32_t Ntot_events;
}FILEHEADER;

typedef struct _fileEnd {
  uint16_t file_end_id;
  uint32_t evnt_number;
  uint32_t Ntot_oct;
}FILEEND;

typedef struct _eventHeader {
  uint16_t event_beg_id;
  uint32_t event_number;
  uint32_t trigger_number;
  uint16_t hit_in_trig;
}EVENTHEADER;

typedef struct _dataMain {
  uint8_t data_beg_id;
  uint32_t fe_number;
  uint32_t trigger_number;
  uint32_t mode_num;
  uint32_t modules_num;
}DATAMAIN;

typedef struct hit_fiber_receive hit_fiber_rec;
struct hit_fiber_receive{
  uint32_t N_fiber_rec;
  uint32_t hit_time_rec;
};

//FUNCTIONS

inline void packi8(unsigned char *buf, uint8_t i)
{
   *buf++ = i&0x0FF;
}

inline uint32_t unpacki8(unsigned char *buf)
{
  return buf[0];
}

inline void packi16(unsigned char *buf, uint16_t i)
{
    *buf++ = i>>8; *buf++ = i;
}

inline uint16_t unpacki16(unsigned char *buf)
{
    return (buf[0]<<8) | buf[1];
}

inline void packi32(unsigned char *buf, uint32_t i)
{
    *buf++ = i>>24; *buf++ = i>>16;
    *buf++ = i>>8;  *buf++ = i;
}

inline uint32_t unpacki32(unsigned char *buf)
{
    return (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];
}

inline void convert32to24(unsigned char *buf, uint32_t i)
{
    //*buf++ = i>>24;
    *buf++ = i>>16;
    *buf++ = i>>8;
    *buf++ = i;
}

inline uint32_t unpacki24(unsigned char *buf)
{
    return  (00000000 | (buf[0]<<16) | (buf[1]<<8) | buf[2]);
}

inline void pack_header(unsigned char* buf, uint16_t id, uint32_t runOct_n, uint32_t evnt_n){

  packi16((unsigned char*)(buf), id);
  packi32((unsigned char*)(buf+2), runOct_n);
  packi32((unsigned char*)(buf+6), evnt_n);

}

inline void unpack_fileHead(unsigned char* buf, FILEHEADER &file_head){
  file_head.file_beg_id = unpacki16((unsigned char *)(buf));
  file_head.run_number = unpacki32((unsigned char *)(buf+2));
  file_head.Ntot_events = unpacki32((unsigned char *)(buf+6));
}

inline void pack_fileEnd(unsigned char* buf, uint16_t id, uint32_t evnt_n, uint32_t oct_n){

  packi16((unsigned char*)(buf), id);
  packi32((unsigned char*)(buf+2), evnt_n);
  packi32((unsigned char*)(buf+6), oct_n);

}

inline void unpack_fileend(unsigned char* buf, FILEEND &file_end){
  file_end.file_end_id = unpacki16((unsigned char *)(buf));
  file_end.evnt_number = unpacki32((unsigned char *)(buf+2));
  file_end.Ntot_oct = unpacki32((unsigned char *)(buf+6));
}

inline void pack_eventHeader(unsigned char* buf, uint16_t id, uint32_t runOct_n, uint32_t trig,  uint16_t evnt_n){

  packi16((unsigned char*)(buf), id);
  packi32((unsigned char*)(buf+2), runOct_n);
  convert32to24((unsigned char*)(buf+6), trig);
  packi16((unsigned char*)(buf+9), evnt_n);

}

inline void unpack_eventHead(unsigned char* buf, EVENTHEADER &event_head){
  event_head.event_beg_id = unpacki16((unsigned char *)(buf));
  event_head.event_number = unpacki32((unsigned char *)(buf+2));
  event_head.trigger_number = unpacki24((unsigned char *)(buf+6));
  event_head.hit_in_trig = unpacki16((unsigned char *)(buf+9));
}

inline void pack_dataMain(unsigned char* buf, uint16_t id, uint8_t fe, uint32_t trig,  uint8_t mode, uint8_t N_modules){

  packi8((unsigned char*)(buf), id);
  packi8((unsigned char*)(buf+1), fe);
  convert32to24((unsigned char*)(buf+2), trig);
  packi8((unsigned char*)(buf+5), mode);
  packi8((unsigned char*)(buf+6), N_modules);

}

inline void unpack_dataMain(unsigned char* buf, DATAMAIN &data){
  data.data_beg_id = unpacki8((unsigned char *)(buf));
  data.fe_number = unpacki8((unsigned char *)(buf+1));
  data.trigger_number = unpacki24((unsigned char *)(buf+2));
  data.mode_num = unpacki8((unsigned char *)(buf+5));
  data.modules_num = unpacki8((unsigned char *)(buf+6));

}

inline void pack_data(unsigned char* buf, uint8_t fib, uint32_t tim, uint16_t nbsample){

  packi8((unsigned char*)(buf), fib);
  packi32((unsigned char*)(buf+1), tim);
  packi16((unsigned char*)(buf+5), nbsample);

}

inline void unpack_data(unsigned char* buf, hit_block_receive &data){
  data.N_PM_rec = unpacki8((unsigned char *)(buf));
  data.hit_time_rec = unpacki32((unsigned char *)(buf+1));
  data.Nb_samples_rec = unpacki16((unsigned char *)(buf+5));
}

inline void pack_data1(unsigned char* buf, uint8_t fib, uint32_t tim){

  packi8((unsigned char*)(buf), fib);
  packi32((unsigned char*)(buf+1), tim);

}

inline void unpack_data1(unsigned char* buf, hit_fiber_receive &data1){
  data1.N_fiber_rec = unpacki8((unsigned char *)(buf));
  data1.hit_time_rec = unpacki32((unsigned char *)(buf+1));
}

inline void pack_sample(unsigned char* buf, uint16_t echanti){
   packi16((unsigned char*)(buf), echanti);
}

inline void unpack_sample(unsigned char* buf, sample_receive &ech){
   ech.sample = unpacki16((unsigned char *)(buf));
}

void ProgressBar(Int_t prctg, Int_t pourcentage){//make a ProgressBar
 int barWidth = 20;
 std::cout << " \033[31m[\033[0m";
 int pos = barWidth * pourcentage/100;
 for (int wxc = 0; wxc < barWidth; ++wxc) {
  if (wxc < pos) std::cout << "\033[31m=\033[0m";
  else if (wxc == pos) std::cout << "\033[31m>\033[0m";
  else std::cout << " ";
 }
 std::cout << "\033[31m]\033[0m " << int(pourcentage) << " %\r";
 std::cout.flush();
 if(pourcentage==100){
  std::cout << " \033[32m[\033[0m";
  for (int wxc = 0; wxc < barWidth; ++wxc) {
   if (wxc < pos) std::cout << "\033[32m=\033[0m";
   else if (wxc == pos) std::cout << "\033[32m>\033[0m";
   else std::cout << " ";
  }
  std::cout << "\033[32m]\033[0m " << int(pourcentage) << std::endl;
  std::cout.flush();
 }
}

inline int check_mult(std::vector <int> hits, int distance, double &final_pos){

  std::sort (hits.begin(), hits.end());
  int temp_pos = hits[0];
  final_pos = hits[0];
  int mult = 1;
  for(int i = 1; i<hits.size(); i++){
    if((hits[i]-temp_pos) <= distance){
      mult++;
      temp_pos = hits[i];
      final_pos+=hits[i];
    }
    else{
      if(mult==1){
        temp_pos = hits[i];
        final_pos = hits[i];
      }
      else{
        if(i == (hits.size()-1) && mult>1){
          break;
        }
        else{
          mult = 0;
        }
      }
    }
  }

  if(mult>1){
    final_pos /= mult;
  }

return mult;

}

inline int convert_Xchannel_withCable(int fe_ch){
  //COnfiguration December 2017 NICE test CAL
  // if(fe_ch>=0 &&fe_ch < 8){
  //  return 32 - (2*fe_ch);
  // }
  // else if(fe_ch>=8 && fe_ch < 16){
  //   return 2*fe_ch - 15;
  // }
  // else if(fe_ch>=16 && fe_ch < 24){
  //   return 64 - (2*fe_ch) - 1;
  // }
  // else if(fe_ch>=24 && fe_ch<32){
  //   return 64 - (2*fe_ch);
  // }
  // else{/*std::cout<<"**ERROR IN FIBRE NUMBER DECODING -> EXIT! **"std::endl;*/
  //   return 0;
  // }
  if(fe_ch>=0 &&fe_ch < 8){
   return 32 - (2*fe_ch);
  }
  else if(fe_ch>=8 && fe_ch < 16){
    return 2*fe_ch - 15;
  }
  else if(fe_ch>=16 && fe_ch < 24){
    return 64 - (2*fe_ch) - 1;
  }
  else if(fe_ch>=24 && fe_ch<32){
    return 64 - (2*fe_ch);
  }
  else{/*std::cout<<"**ERROR IN FIBRE NUMBER DECODING -> EXIT! **"std::endl;*/
    return 0;
  }
}

inline int convert_Ychannel_withCable(int fe_ch){
  //Configuration December 2017 NICE test CAL
  // if(fe_ch>=0 &&fe_ch < 8){
  //   return 32 - (2*fe_ch);
  // }
  // else if(fe_ch>=8 && fe_ch < 16){
  //   return (2*fe_ch) + 1;
  // }
  // else if(fe_ch>=16 && fe_ch < 24){
  //   return  (2*fe_ch) - 31;
  // }
  // else if(fe_ch>=24 && fe_ch<32){
  //   return 2*fe_ch - 46;
  // }
  // else{/*std::cout<<"**ERROR IN FIBRE NUMBER DECODING -> EXIT! **"std::endl;*/
  //   return 0;
  // }
  if(fe_ch>=0 &&fe_ch < 8){
    return 32 - (2*fe_ch);
  }
  else if(fe_ch>=8 && fe_ch < 16){
    return (2*fe_ch) + 1;
  }
  else if(fe_ch>=16 && fe_ch < 24){
    return  (2*fe_ch) - 31;
  }
  else if(fe_ch>=24 && fe_ch<32){
    return 2*fe_ch - 46;
  }
  else{/*std::cout<<"**ERROR IN FIBRE NUMBER DECODING -> EXIT! **"std::endl;*/
    return 0;
  }
}
#endif
