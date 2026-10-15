#include <cassert>
#include <iostream>
#include <TCanvas.h>
#include <TH2Poly.h>
#include <TH1D.h>
#include <TROOT.h>
#include "UnpackerManager.hh"
#include "user_analyzer.hh"
int main(int argc, char** argv){
  assert(argc==2);
  assert(analyzer::process_begin({"corea","param/corea/analyzer.conf",argv[1],"19091"})==0);
  auto& u=hddaq::unpacker::GUnpacker::get_instance();u.initialize();assert(!u.eof());
  assert(analyzer::process_event()==0);
  assert(gROOT->GetListOfCanvases()->GetSize()==3);
  auto c=static_cast<TCanvas*>(gROOT->GetListOfCanvases()->FindObject("TPC_ADC2D"));assert(c);
  auto adc=dynamic_cast<TH2Poly*>(c->GetPrimitive("TPC_ADC2D"));assert(adc && adc->GetNumberOfBins()==1024);
  c=static_cast<TCanvas*>(gROOT->GetListOfCanvases()->FindObject("TPC_TDC2D"));assert(c);
  auto tdc=dynamic_cast<TH2Poly*>(c->GetPrimitive("TPC_TDC2D"));assert(tdc);
  for(unsigned asad=0;asad<4;++asad)for(unsigned a=0;a<4;++a)for(unsigned ch=0;ch<64;++ch){
    unsigned pad=asad*256+a*64+ch+1;
    assert(adc->GetBinContent(pad)==700+asad*100+a*10+ch);
    assert(tdc->GetBinContent(pad)==300+ch%8);
  }
  c=static_cast<TCanvas*>(gROOT->GetListOfCanvases()->FindObject("TDC_FADC"));assert(c);
  auto pulse=dynamic_cast<TH1D*>(c->GetPrimitive("TDC_FADC"));assert(pulse && pulse->GetNbinsX()==512);
  assert(pulse->GetBinContent(308)==1193);assert(pulse->GetBinContent(512)==100);
  std::cout<<"PASS: 3 canvases, all 1024 channel mappings, 512-bucket waveform, peak time and baseline\n";
}
