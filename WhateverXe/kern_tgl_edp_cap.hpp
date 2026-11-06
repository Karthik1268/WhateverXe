//
//  kern_tgl_edp_cap.hpp
//  WhateverXe
//
//  Opt-in Tiger Lake internal-panel link-rate cap (-wex-tgl-edp-cap,
//  -wex-tgl-edp-notable). Some eDP 1.4 panels report MAX_LINK_RATE as 0 or a
//  non-standard code and list their real rates only in SUPPORTED_LINK_RATES
//  (DPCD 0x010..0x01F, 200 kHz units), sometimes with non-standard entries
//  such as 10800/21600/24300. The TGL donor then derives "Invalid bit rate
//  0x0" and never trains the panel. The receiver caps are rewritten in the
//  donor's read buffer only, after a successful native read on the internal
//  combo port, so the donor sees a standard rate it understands (RBR or HBR).
//  Port of the Display13EDPKeep::capLinkCaps idea (WhateverXe issue #43).
//

#ifndef kern_tgl_edp_cap_hpp
#define kern_tgl_edp_cap_hpp

#include <stdint.h>

namespace TGLEDPCap {

// DPCD rate codes and their SUPPORTED_LINK_RATES equivalents (200 kHz units).
static constexpr uint8_t RBR=0x06,HBR=0x0A,HBR2=0x14,HBR3=0x1E;
static constexpr uint16_t tableRBR=8100,tableHBR=13500,tableHBR2=27000,tableHBR3=40500;

inline bool standardCode(uint8_t code) {return code==RBR || code==HBR || code==HBR2 || code==HBR3;}

inline uint8_t codeForTable(uint32_t rate) {
	switch(rate) {
		case tableRBR: return RBR;
		case tableHBR: return HBR;
		case tableHBR2: return HBR2;
		case tableHBR3: return HBR3;
		default: return 0;
	}
}

// Highest standard rate the table lists, at or below cap; 0 if none.
inline uint8_t bestTableCode(const uint8_t *table,uint32_t length,uint8_t cap) {
	uint8_t best=0;
	if(!table)return 0;
	for(uint32_t i=0;i+1<length && i<16;i+=2) {
		const auto code=codeForTable(uint32_t(table[i])|(uint32_t(table[i+1])<<8));
		if(code && code<=cap && code>best)best=code;
	}
	return best;
}

// Chooses the rate code reported for MAX_LINK_RATE / extended MAX_LINK_RATE.
// A standard code is capped; a 0 or unknown code is replaced by the highest
// standard table rate at or below the cap, else HBR (the eDP default rate).
inline uint8_t chooseCode(uint8_t reported,const uint8_t *table,uint32_t tableLength,bool tableValid,uint8_t cap) {
	if(standardCode(reported))return reported>cap ? cap : reported;
	if(tableValid) {
		const auto best=bestTableCode(table,tableLength,cap);
		if(best)return best;
	}
	return cap<HBR ? cap : HBR;
}

// Rewrites the receiver caps inside one completed DPCD read [address, address+length).
// table/tableValid: the panel's SUPPORTED_LINK_RATES (16 bytes), if known.
// dropTable: zero the whole table so the donor falls back to MAX_LINK_RATE
// and LINK_BW_SET; otherwise keep only standard entries at or below cap.
// Returns the number of bytes changed.
inline uint32_t capLinkCaps(uint32_t address,uint8_t *data,uint32_t length,
	const uint8_t *table,bool tableValid,bool dropTable,uint8_t cap=HBR) {
	if(!data || !length)return 0;
	uint32_t changed=0;
	const auto at=[&](uint32_t reg)->uint8_t * {return reg>=address && reg-address<length ? data+(reg-address) : nullptr;};
	const auto set=[&](uint8_t *b,uint8_t v) {if(b && *b!=v) {*b=v;++changed;}};
	const uint32_t rates[]={0x001U,0x2201U};
	for(const auto reg:rates)
		if(auto b=at(reg))set(b,chooseCode(*b,table,16,tableValid,cap));
	if(auto b=at(0x100U); b && *b>cap)set(b,cap);
	for(uint32_t i=0;i<8;++i) {
		auto lo=at(0x10+2*i),hi=at(0x11+2*i);
		if(!lo || !hi)continue;
		const auto code=codeForTable(uint32_t(*lo)|(uint32_t(*hi)<<8));
		if(dropTable || !code || code>cap) {set(lo,0);set(hi,0);}
	}
	return changed;
}

// True when a read covers any register capLinkCaps may rewrite.
inline bool coversLinkCaps(uint32_t address,uint32_t length) {
	const auto covers=[&](uint32_t lo,uint32_t hi) {return length && address<=hi && address+length>lo;};
	return covers(0x001,0x001) || covers(0x010,0x01F) || covers(0x100,0x100) || covers(0x2201,0x2201);
}

}

#endif /* kern_tgl_edp_cap_hpp */
