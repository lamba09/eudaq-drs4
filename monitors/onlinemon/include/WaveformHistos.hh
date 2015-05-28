/*
 * WaveformHistos.hh
 *
 *  Created on: Jun 16, 2011
 *      Author: stanitz
 */

#ifndef WaveformHISTOS_HH_
#define WaveformHISTOS_HH_
//ROOT
//#include <TGraph.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TF1.h>
#include <TFile.h>
#include <TString.h>
#include <THStack.h>
#include <TGraph.h>
//std
#include <map>
#include <algorithm>    // std::min_element, std::max_element
#include <cstdlib>
//eudaq
#include "SimpleStandardEvent.hh"
#include "TGraphSet.hh"
#include "WaveformOptions.hh"


class RootMonitor;

class WaveformHistos {
  protected:
    std::string _sensor;
    int _id;
    bool _wait;
    std::vector<TH1F*> _Waveforms;
    THStack* h_wf_stack;
    int _n_wfs;
    unsigned int _n_samples;
    std::map<std::string, TH1*> profiles;
    std::map<std::string, TH1*> histos;
    std::map<std::string, std::pair<float,float> > rangesX;
    std::map<std::string, std::pair<float,float> > rangesY;
  public:
    WaveformHistos(SimpleStandardWaveform p, RootMonitor * mon);
    std::string getName() const {return (std::string)TString::Format("%s_%d",_sensor.c_str(),_id);};
    virtual ~WaveformHistos(){}
    void Fill(const SimpleStandardWaveform & wf);
    void FillPulserEvent(const SimpleStandardWaveform & wf);
    void FillSignalEvent(const SimpleStandardWaveform & wf);
    void FillEvent(const SimpleStandardWaveform & wf, bool isPulserEvent);
    unsigned int getNSamples() const {return _n_samples;}
    void Reset();
    void SetOptions(WaveformOptions* options){std::cout<<"Setting Options for "<<getName()<<std::endl;};

    void Calculate(const int currentEventNum);
    void Write();
    unsigned GetNWaveforms() const {return _n_wfs;};
    TH1F * getWaveformGraph(int i) { return _Waveforms[i%_n_wfs]; }
    THStack* getWaveformStack(){return h_wf_stack;}
    void setRootMonitor(RootMonitor *mon)  {_mon = mon; };
    // signal histos   
    TH1F* getDeltaVoltageHisto() const { return (TH1F*)histos.at("DeltaVoltage");};
    TH1F* getMinVoltageHisto() const { return (TH1F*)histos.at("MinVoltage");};
    TH1F* getMaxVoltageHisto() const { return (TH1F*)histos.at("MaxVoltage");};
    TH1F* getFullIntegralVoltageHisto() const { return (TH1F*)histos.at("FullIntegral");};
    TH1F* getSignalIntegralVoltageHisto() const{ return (TH1F*)histos.at("SignalIntegral");};
    TH1F* getSignalMinusPedestalHisto() const{ return (TH1F*)histos.at("SignalMinusPedestal");};
    TH1F* getPedestalIntegralVoltageHisto() const { return (TH1F*)histos.at("PedestalIntegral");};
    TH1F* getDeltaIntegralVoltageHisto() const { return (TH1F*)histos.at("DeltaIntegral");};
    TProfile* getProfileDeltaVoltage() const { return (TProfile*)profiles.at("DeltaVoltage");};
    TProfile* getProfileDeltaIntegral() const { return (TProfile*)profiles.at("DeltaIntegral");};
    TProfile* getProfileSignalIntegral() const { return (TProfile*)profiles.at("SignalIntegral");};
    TProfile* getProfilePedestalIntegral() const { return (TProfile*)profiles.at("PedestalIntegral");};
    TProfile* getProfile(std::string key) const;
    // pulser histos   
    TH1F* getPulserDeltaVoltageHisto() const { return (TH1F*)histos.at("Pulser_DeltaVoltage");};
    TH1F* getPulserMinVoltageHisto() const { return (TH1F*)histos.at("Pulser_MinVoltage");};
    TH1F* getPulserMaxVoltageHisto() const { return (TH1F*)histos.at("Pulser_MaxVoltage");};
    TH1F* getPulserFullIntegralVoltageHisto() const { return (TH1F*)histos.at("Pulser_FullIntegral");};
    TH1F* getPulserSignalIntegralVoltageHisto() const{ return (TH1F*)histos.at("Pulser_SignalIntegral");};
    TH1F* getPulserSignalMinusPedestalHisto() const{ return (TH1F*)histos.at("Pulser_SignalMinusPedestal");};
    TH1F* getPulserPedestalIntegralVoltageHisto() const { return (TH1F*)histos.at("Pulser_PedestalIntegral");};
    TH1F* getPulserDeltaIntegralVoltageHisto() const { return (TH1F*)histos.at("Pulser_DeltaIntegral");};
    TProfile* getPulserProfileDeltaVoltage() const { return (TProfile*)profiles.at("Pulser_DeltaVoltage");};
    TProfile* getPulserProfileDeltaIntegral() const { return (TProfile*)profiles.at("Pulser_DeltaIntegral");};
    TProfile* getPulserProfileSignalIntegral() const { return (TProfile*)profiles.at("Pulser_SignalIntegral");};
    TProfile* getPulserProfilePedestalIntegral() const { return (TProfile*)profiles.at("Pulser_PedestalIntegral");};
    TProfile* getPulserProfile(std::string key) const;

    TH1F* getHisto(std::string key) const;
    void SetMaxRangeX(std::string,float minx, float maxx);
    void SetMaxRangeY(std::string,float min, float max);
    void SetPedestalIntegralRange(float min, float max);
    void SetSignalIntegralRange(float min, float max);

  private:
    std::pair<float,float> pedestal_integral_range;
    std::pair<float,float> signal_integral_range;
    std::pair<float,float> pulser_integral_range;
    unsigned int n_fills;
    void InitHistos();
    void InitIntegralHistos();
    void InitProfiles();
    void Reinitialize_Waveforms();
    void UpdateRanges();
    void UpdateRange(TH1* histo);
    int SetHistoAxisLabelx(TH1* histo,std::string xlabel);
    int SetHistoAxisLabely(TH1* histo,std::string ylabel);
    int SetHistoAxisLabels(TH1* histo,std::string xlabel, std::string ylabel);
    RootMonitor * _mon;
    float min_wf;
    float max_wf;
};

#ifdef __CINT__
#pragma link C++ class WaveformHistos-;
#endif


#endif /* WaveformHISTOS_HH_ */

