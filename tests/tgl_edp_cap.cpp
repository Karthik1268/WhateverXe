#include "kern_tgl_edp_cap.hpp"
#include <assert.h>
#include <string.h>
using namespace TGLEDPCap;
static void put(uint8_t *t,int i,uint16_t v) {t[2*i]=uint8_t(v);t[2*i+1]=uint8_t(v>>8);}
int main() {
 // eDP 1.4 panel: MAX_LINK_RATE 0, non-standard table entries.
 uint8_t table[16] {};
 put(table,0,8100);put(table,1,10800);put(table,2,13500);put(table,3,21600);put(table,4,27000);
 uint8_t caps[16] {0x14,0x00,0x84};
 assert(coversLinkCaps(0,16) && coversLinkCaps(0x10,16) && coversLinkCaps(0x2200,16) && coversLinkCaps(0x100,1));
 assert(!coversLinkCaps(0x200,8) && !coversLinkCaps(0x002,8) && !coversLinkCaps(0,0));
 assert(capLinkCaps(0,caps,16,table,true,false)==1 && caps[1]==HBR && caps[0]==0x14 && caps[2]==0x84);
 // Table read: standard entries <= HBR kept, others removed.
 uint8_t t[16];memcpy(t,table,16);
 assert(capLinkCaps(0x10,t,16,nullptr,false,false)==6);
 assert(t[0]==uint8_t(8100) && t[1]==uint8_t(8100>>8) && !t[2] && !t[3] && t[4]==uint8_t(13500) && !t[6] && !t[7] && !t[8] && !t[9]);
 memcpy(t,table,16);capLinkCaps(0x10,t,16,nullptr,false,true);
 for(auto b:t)assert(!b);
 // Only RBR in the table, MAX 0: RBR. Nothing usable known: HBR default.
 uint8_t rbr[16] {};put(rbr,0,8100);put(rbr,1,10800);
 assert(chooseCode(0,rbr,16,true,HBR)==RBR);
 assert(chooseCode(0,nullptr,0,false,HBR)==HBR);
 assert(chooseCode(0x0C,table,16,true,HBR)==HBR);
 // Standard codes are capped, lower ones kept.
 assert(chooseCode(HBR3,nullptr,0,false,HBR)==HBR && chooseCode(RBR,nullptr,0,false,HBR)==RBR);
 assert(chooseCode(HBR2,nullptr,0,false,HBR2)==HBR2);
 // Extended caps and LINK_BW_SET.
 uint8_t ext[2] {0x14,0x14};
 assert(capLinkCaps(0x2200,ext,2,table,true,false)==1 && ext[1]==HBR && ext[0]==0x14);
 uint8_t bw=0x14;assert(capLinkCaps(0x100,&bw,1,nullptr,false,false)==1 && bw==HBR);
 bw=RBR;assert(capLinkCaps(0x100,&bw,1,nullptr,false,false)==0 && bw==RBR);
 // Partial and odd reads stay in bounds.
 uint8_t one=0x00;assert(capLinkCaps(0x001,&one,1,table,true,false)==1 && one==HBR);
 uint8_t half=0xFF;assert(capLinkCaps(0x10,&half,1,nullptr,false,true)==0 && half==0xFF);
 assert(capLinkCaps(0,nullptr,4,nullptr,false,false)==0);
 // A standard, already low panel is untouched.
 uint8_t good[16] {0x12,0x0A,0x84};uint8_t copy[16];memcpy(copy,good,16);
 assert(capLinkCaps(0,good,16,nullptr,false,false)==0 && !memcmp(copy,good,16));
 // Dell Latitude 7420 panel: MAX_LINK_RATE 0, table 8100/10800/12150/13500.
 uint8_t dell[16] {};put(dell,0,8100);put(dell,1,10800);put(dell,2,12150);put(dell,3,13500);
 assert(tableIndexForCode(dell,16,HBR)==3 && tableIndexForCode(dell,16,RBR)==0);
 assert(tableIndexForCode(dell,16,HBR2)==-1 && tableIndexForCode(dell,16,0x0C)==-1 && tableIndexForCode(nullptr,16,HBR)==-1);
 uint8_t wanted[2] {0x0A,0x82},out[2] {};
 assert(rateSelectWrite(0x100,wanted,2,dell,true,true,out,2)==3 && out[0]==0 && out[1]==0x82 && wanted[0]==0x0A);
 assert(rateSelectWrite(0x100,wanted,2,dell,true,false,out,2)==-1); // panel with a real MAX_LINK_RATE
 assert(rateSelectWrite(0x100,wanted,2,dell,false,true,out,2)==-1);
 assert(rateSelectWrite(0x101,wanted,1,dell,true,true,out,2)==-1);
 assert(rateSelectWrite(0x100,wanted,2,dell,true,true,out,1)==-1);
 uint8_t hbr2[2] {0x14,0x82};assert(rateSelectWrite(0x100,hbr2,2,dell,true,true,out,2)==-1);
 // The capped receiver caps of this panel: MAX_LINK_RATE 0 becomes HBR.
 uint8_t caps0[16] {0x14,0x00,0xC2,0x41};
 assert(capLinkCaps(0,caps0,16,dell,true,true)==1 && caps0[1]==HBR);
}
