// COREA display: mapping and orientation from online-corea/macro/online.C.
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <TCanvas.h>
#include <TH2D.h>
#include <TH2Poly.h>
#include <TList.h>
#include <TROOT.h>
#include <TSystem.h>
#include "ConfMan.hh"
#include "HttpServer.hh"
#include "UnpackerManager.hh"
#include "user_analyzer.hh"

namespace analyzer {
namespace {
TH2Poly* charge = nullptr;
TH2Poly* time_map = nullptr;
TH2D* pulse = nullptr;
constexpr unsigned n_asad = 4, n_aget = 4, n_ch = 64, n_bucket = 512;
constexpr double threshold = 50.;
unsigned long long events_processed = 0;
unsigned last_active_pads = 0;

void geometry(TH2Poly* hist)
{
  for(int section_z=0; section_z<4; ++section_z)
    for(int section_y=0; section_y<4; ++section_y)
      for(int col=7; col>=0; --col)
        for(int row=0; row<8; ++row){
          const double z=-48+24*section_z+3*col, y=48-24*section_y-3*row;
          double x[5]={z+.1,z+.1,z+2.9,z+2.9,z+.1};
          double yy[5]={y-2.9,y-.1,y-.1,y-2.9,y-2.9};
          hist->AddBin(5,x,yy);
        }
}
}

int process_begin(const std::vector<std::string>& argv)
{
  if(argv.size()<3 || argv.size()>4){
    std::cerr << "Usage: " << argv.front() << " config HDDAQ-input [port=9091]\n";
    return 1;
  }
  int port=9091;
  try {
    if(argv.size()==4){
      size_t used=0;
      port=std::stoi(argv[3],&used);
      if(used!=argv[3].size() || port<1 || port>65535)
        throw std::invalid_argument("port");
    }
  } catch(const std::exception&){
    std::cerr << "Invalid HTTP port\n"; return 1;
  }
  gROOT->SetBatch(kTRUE);
  ConfMan::getInstance().initialize(argv);
  charge=new TH2Poly("TPC_ADC2D","TPC_ADC2D;z [mm];y [mm]",-60,60,-60,60);
  time_map=new TH2Poly("TPC_TDC2D","TPC_TDC2D;z [mm];y [mm]",-60,60,-60,60);
  geometry(charge); geometry(time_map);
  pulse=new TH2D("TDC_FADC","TDC_FADC;Time bucket;ADC",
                 n_bucket,0,n_bucket,200,0,4096);
  for(auto hist: std::vector<TH1*>{charge,time_map,pulse}){
    hist->SetDirectory(nullptr); hist->SetStats(false);
  }
  time_map->SetMinimum(0); time_map->SetMaximum(n_bucket);
  auto& http=HttpServer::GetInstance();
  http.SetPort(port); http.Open();
  auto list=new TList; list->SetName("TPC");
  list->Add(charge); list->Add(time_map); list->Add(pulse);
  http.Register(list);
  for(auto hist: std::vector<TH1*>{charge,time_map,pulse}){
    auto canvas=new TCanvas(hist->GetName(),hist->GetName(),800,700);
    canvas->cd();
    if(hist==pulse) canvas->SetLogz();
    hist->Draw("colz");
    http.Register(canvas);
  }
  http.Begin();
  std::cout << "COREA: 1024 rectangular pads, AsAd 0..3, AGET 0..3, channels 0..63\n"
            << "Browser: http://localhost:" << port << "/\n";
  return 0;
}

int process_event()
{
  auto& unpacker=hddaq::unpacker::GUnpacker::get_instance();
  static const auto device=unpacker.get_device_id("TPC");
  static const auto adc_id=unpacker.get_data_id("TPC","adc");
  const auto run=unpacker.get_run_number(), event=unpacker.get_event_number();
  static int previous_run=-1;
  if(run!=previous_run){ pulse->Reset(); previous_run=run; }
  charge->Reset(""); time_map->Reset("");
  double best_peak=-1;
  std::vector<double> best_samples;
  unsigned best_pad=0;
  last_active_pads=0;
  for(unsigned asad=0; asad<n_asad; ++asad)
    for(unsigned aget=0; aget<n_aget; ++aget)
      for(unsigned ch=0; ch<n_ch; ++ch){
        const unsigned fe_ch=aget*68+ch;
        const unsigned count=unpacker.get_entries(device,asad,0,fe_ch,adc_id);
        if(!count) continue;
        ++last_active_pads;
        if(count>n_bucket){
          std::cerr << "COREA: waveform exceeds 512 buckets\n"; return 1;
        }
        std::vector<double> samples(count);
        double base=0, peak=-1;
        unsigned peak_time=0;
        for(unsigned t=0; t<count; ++t){
          samples[t]=unpacker.get(device,asad,0,fe_ch,adc_id,t);
          if(t<100) base+=samples[t];
          if(samples[t]>peak){ peak=samples[t]; peak_time=t; }
        }
        base/=std::min(count,100u);
        const double height=peak-base;
        const unsigned pad=asad*256+aget*64+ch+1;
        charge->SetBinContent(pad,height);
        if(height>threshold) time_map->SetBinContent(pad,peak_time);
        if(peak>best_peak){
          best_peak=peak; best_pad=pad; best_samples=std::move(samples);
        }
      }
  if(best_peak>0)
    for(size_t t=0; t<best_samples.size(); ++t) pulse->Fill(t,best_samples[t]);
  charge->SetTitle(Form("TPC_ADC2D run%d event%d (peak - baseline)",run,event));
  time_map->SetTitle(Form("TPC_TDC2D run%d event%d (threshold %.0f ADC)",run,event,threshold));
  pulse->SetTitle(Form("TDC_FADC run%d event%d pad%u;Time bucket;ADC",run,event,best_pad));
  ++events_processed;
  gSystem->ProcessEvents();
  return 0;
}

int process_end()
{
  std::cout << "COREA: processed " << events_processed
            << " events; last event has " << last_active_pads << " active pads.\n";
  if(!events_processed) return 0;
  std::cout << "Input finished. Keeping the last event available in the browser. "
            << "Press Ctrl+C to exit." << std::endl;
  for(;;){
    gSystem->ProcessEvents();
    gSystem->Sleep(50);
  }
}
}
