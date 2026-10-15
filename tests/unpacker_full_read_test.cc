#include "GetAsAd.hh"
#include <array>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace hddaq::unpacker;
class Decoder : public GetAsAd {
public:
  std::array<std::array<Data, k_n_data_type>, k_n_channel> values;
  Decoder() : GetAsAd("GetAsAd") {
    resize_fe_data(); m_back = &m_fe_data;
    m_null_ch.resize(k_n_channel);
    for(unsigned c=0;c<k_n_channel;++c) {
      m_null_ch[c].resize(k_n_data_type, false);
      for(unsigned d=0;d<k_n_data_type;++d) m_fe_data[c][d]=&values[c][d];
    }
  }
  bool run(std::vector<uint32_t>& payload, unsigned n, unsigned size=2, unsigned type=kFullRead) {
    for(auto& ch: values) for(auto& data: ch) data.clear();
    m_error_state.reset();
    m_module_data_first=payload.begin(); m_data_last=payload.end();
    m_n_data=n; m_item_size=size; m_frame_type=type;
    decode(); return !m_error_state.any();
  }
};
static unsigned adc(unsigned a,unsigned c,unsigned t) { return (a*701+c+t*3)&4095; }
int main(int argc,char** argv) {
  Decoder d;
  for(unsigned buckets: {170u,512u}) {
    std::vector<uint32_t> words(4*68*buckets/2);
    auto bytes=reinterpret_cast<unsigned char*>(words.data()); unsigned pos=0;
    for(unsigned t=0;t<buckets;++t) for(unsigned c=0;c<68;++c)
      for(unsigned a: {3u,1u,0u,2u}) {
        unsigned sample=(a<<14)|adc(a,c,t);
        bytes[pos++]=sample>>8;bytes[pos++]=sample&255;
      }
    assert(d.run(words,4*68*buckets));
    for(unsigned a=0;a<4;++a) for(unsigned c=0;c<68;++c) {
      const auto& v=d.values[a*68+c][0];assert(v.size()==buckets);
      for(unsigned t=0;t<buckets;++t) assert(v[t]==adc(a,c,t));
    }
  }
  std::vector<uint32_t> one(1,0);assert(!d.run(one,3));assert(!d.run(one,1,4));
  assert(d.run(one,1));assert(d.values[0][0].size()==1);
  std::vector<uint32_t> empty;assert(d.run(empty,0));
  // Partial-readout regression: AGET 2, channel 67, ADC 0xabc.
  one[0]=(2u<<6)|33u|(1u<<15)|(10u<<16)|(188u<<24);
  assert(d.run(one,1,4,GetAsAd::kPartialRead));assert(d.values[203][0][0]==0xabc);
  if(argc>1) {
    std::ifstream in(argv[1],std::ios::binary); unsigned char h[256];
    in.read(reinterpret_cast<char*>(h),256);assert(in);
    unsigned n=(unsigned(h[12])<<24)|(unsigned(h[13])<<16)|(unsigned(h[14])<<8)|h[15];
    unsigned hs=((unsigned(h[8])<<8)|h[9])*256;
    assert(h[6]==2 && h[10]==0 && h[11]==2);
    std::vector<uint32_t> w((n+1)/2); in.seekg(hs);
    in.read(reinterpret_cast<char*>(w.data()),n*2);assert(in);assert(d.run(w,n));
    size_t total=0;for(const auto& ch:d.values) total+=ch[0].size();assert(total==n);
    std::cout<<"Real full-readout frame: "<<n<<" samples, ch0="<<d.values[0][0].size()<<" buckets\n";
  }
  std::cout<<"PASS: 170/512 buckets, 4 interleaved AGETs, channel mapping, ADC, bounds, empty/odd payload, partial regression\n";
}
