#ifdef ROOT_FOUND
 
#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Logger.hh"
#include "eudaq/FileSerializer.hh"


#include "include/SimpleStandardEvent.hh"
#include <stdlib.h>

//# include<inttypes.h>
# include "TFile.h"
# include "TTree.h"
# include "TRandom.h"
# include "TString.h"
# include "TH1F.h"
#include "TSystem.h"
#include "TInterpreter.h"


using namespace std;

template <typename T>
inline T unpack_fh (vector <unsigned char >::iterator &src, T& data){ //unpack from host-byte-order
    data=0;
    for(unsigned int i=0;i<sizeof(T);i++){
        data+=((uint64_t)*src<<(8*i));
        src++;
    }
    return data;
}

template <typename T>
inline T unpack_fn(vector<unsigned char>::iterator &src, T& data){            //unpack from network-byte-order
    data=0;
    for(unsigned int i=0;i<sizeof(T);i++){
        data+=((uint64_t)*src<<(8*(sizeof(T)-1-i)));
        src++;
    }
    return data;
}

template <typename T>
inline T unpack_b(vector<unsigned char>::iterator &src, T& data, unsigned int nb){            //unpack number of bytes n-b-o only
    data=0;
    for(unsigned int i=0;i<nb;i++){
        data+=(uint64_t(*src)<<(8*(nb-1-i)));
        src++;
    }
    return data;
}

typedef pair<vector<unsigned char>::iterator,unsigned int> datablock_t;


namespace eudaq {

    class FileWriterTreeDRS4 : public FileWriter {
    public:
        FileWriterTreeDRS4(const std::string &);
        virtual void StartRun(unsigned);
        virtual void WriteEvent(const DetectorEvent &);
        virtual uint64_t FileBytes() const;
        float Calculate(std::vector<float> * data, int min, int max, bool _abs=false);
        float CalculatePeak(std::vector<float> * data, int min, int max);
        std::pair<int, float> FindMaxAndValue(std::vector<float> * data, int min, int max);
        float avgWF(float, float, int);
        virtual ~FileWriterTreeDRS4();
    private:
        TFile * m_tfile; // book the pointer to a file (to store the otuput)
        TTree * m_ttree; // book the tree (to store the needed event info)
        // Book variables for the Event_to_TTree conversion
        unsigned m_noe;
        short chan;
        int n_pixels;
        
        // Scalar Branches     
        int   f_nwfs;
        int   f_event_number;
        Double_t f_time;

        int   f_pulser;
        float f_pulser_int;
        int   f_trig_time;
        
        // Vector Branches     
        
        // DUT
        std::vector< std::string >  * v_sensor_name;
        std::vector< std::string >  * v_type_name;
        std::vector<float>  * v_sig;
        std::vector<float>  * v_sig_time;
        std::vector<float>  * v_ped;
        
        // std::vector<float> * f_wf0;
        // std::vector<float> * f_wf1;
        // std::vector<float> * f_wf2;
        // std::vector<float> * f_wf3;
        
        // TELESCOPE
        std::vector<int> * f_plane;
        std::vector<int> * f_col;
        std::vector<int> * f_row;
        std::vector<int> * f_adc;
        std::vector<int> * f_charge;  

        // average waveforms of channels
        TH1F * avgWF_0;
        TH1F * avgWF_1;
        TH1F * avgWF_2;
        TH1F * avgWF_3;
        
    };

    namespace {
        static RegisterFileWriter<FileWriterTreeDRS4> reg("drs4tree");
    }
    
    FileWriterTreeDRS4::FileWriterTreeDRS4(const std::string & /*param*/)
      : m_tfile(0), m_ttree(0),m_noe(0),chan(4),n_pixels(90*90+60*60)
    {
    
        f_nwfs          =  0;	  
        f_event_number  = -1;
        f_time          = -1.;
        f_pulser        = -1;
        f_pulser_int    =  0.;
        f_trig_time     =  0.;
        
        // dut
        v_sensor_name = new std::vector< std::string >;
        v_type_name   = new std::vector< std::string >;
        v_ped       = new std::vector<float>;
        v_sig       = new std::vector<float>;
        v_sig_time  = new std::vector<float>;
        
        // f_wf0 = new std::vector<float>;
        // f_wf1 = new std::vector<float>;
        // f_wf2 = new std::vector<float>;
        // f_wf3 = new std::vector<float>;
        
        // telescope
        f_plane  = new std::vector<int>;
        f_col    = new std::vector<int>;	  
        f_row    = new std::vector<int>;	  
        f_adc    = new std::vector<int>;	  
        f_charge = new std::vector<int>;  

        // average waveforms of channels
        avgWF_0 = new TH1F("avgWF_0","avgWF_0", 1024, 0, 1024);
        avgWF_1 = new TH1F("avgWF_1","avgWF_1", 1024, 0, 1024);
        avgWF_2 = new TH1F("avgWF_2","avgWF_2", 1024, 0, 1024);
        avgWF_3 = new TH1F("avgWF_3","avgWF_3", 1024, 0, 1024);
    }
    
    void FileWriterTreeDRS4::StartRun(unsigned runnumber) {
        EUDAQ_INFO("Converting the inputfile into a DRS4 TTree " );
        std::string foutput(FileNamer(m_filepattern).Set('X', ".root").Set('R', runnumber));
        EUDAQ_INFO("Preparing the outputfile: " + foutput);

        // ---------------------------------------------------------------------
        // the following line is needed to have std::vector<float> in the tree
        // ---------------------------------------------------------------------
        gROOT->ProcessLine("#include <vector>");

        m_tfile = new TFile(foutput.c_str(), "RECREATE");
        m_ttree = new TTree("tree", "a simple Tree with simple variables");
        
        // Set Branch Addresses
        m_ttree->Branch("event_number"  ,&f_event_number , "event_number/I");
        m_ttree->Branch("time"          ,&f_time         , "time/D");
        m_ttree->Branch("pulser"        ,&f_pulser       , "pulser/I");
        m_ttree->Branch("pulser_int"    ,&f_pulser_int   , "pulser_int/F");
        m_ttree->Branch("trig_time"     ,&f_trig_time    , "trig_time/I");
        m_ttree->Branch("nwfs"          , &f_nwfs        , "n_waveforms/I");
        
        // m_ttree->Branch("wf0" , &f_wf0);
        // m_ttree->Branch("wf1" , &f_wf1);
        // m_ttree->Branch("wf2" , &f_wf2);
        // m_ttree->Branch("wf3" , &f_wf3);
        
        // DUT
        m_ttree->Branch("sensor_name" , &v_sensor_name);
        m_ttree->Branch("type_name"   , &v_type_name);
        m_ttree->Branch("ped"         , &v_ped);
        m_ttree->Branch("sig"         , &v_sig);
        m_ttree->Branch("sig_time"    , &v_sig_time);
        
        
        // telescope
        m_ttree->Branch("plane", &f_plane);
        m_ttree->Branch("col", &f_col);
        m_ttree->Branch("row", &f_row);
        m_ttree->Branch("adc", &f_adc);
        m_ttree->Branch("charge", &f_charge);

    
    }
    
    void FileWriterTreeDRS4::WriteEvent(const DetectorEvent & ev) {
    
        if (ev.IsBORE()) {
            eudaq::PluginManager::Initialize(ev);
            //firstEvent =true;
            cout << "loading the first event...." << endl;
            return;
        } else if (ev.IsEORE()) {
            cout << "loading the last event...." << endl;
            return;
        }
        
        StandardEvent sev = eudaq::PluginManager::ConvertToStandard(ev);
        
        f_event_number = sev.GetEventNumber();
        f_time = sev.GetTimestamp()/384066.;

        
        // --------------------------------------------------------------------
        // ---------- get the number of waveforms -----------------------------
        // --------------------------------------------------------------------
        unsigned int nwfs = (unsigned int) sev.NumWaveforms();
        f_nwfs = nwfs;
        
        if(f_event_number <= 10){
            cout << "event number " << f_event_number << endl;
            cout << "number of waveforms " << nwfs << endl;
            if(nwfs ==0){
                cout << "----------------------------------------" << endl;
                cout << "WARNING!!! NO WAVEFORMS IN THIS EVENT!!!" << endl;
                cout << "----------------------------------------" << endl;
            }
        }
        
        // --------------------------------------------------------------------
        // ---------- reset/clear all the vectors -----------------------------
        // --------------------------------------------------------------------
        v_sensor_name->clear();
        v_type_name->clear();
        v_ped->clear();
        v_sig->clear();
        v_sig_time->clear();
        
        // f_wf0->clear();
        // f_wf1->clear();
        // f_wf2->clear();
        // f_wf3->clear();
        
        f_plane->clear();
        f_col->clear();
        f_row->clear();
        f_adc->clear();
        f_charge->clear();
        
        
        // --------------------------------------------------------------------
        // ---------- verbosity level and some printouts ----------------------
        // --------------------------------------------------------------------
        int verbose = 0;
        
        if(verbose > 3) cout << "event number " << f_event_number << endl;
        if(verbose > 3) cout << "number of waveforms " << nwfs << endl;
        
        
        // --------------------------------------------------------------------
        // ---------- get and save all info for all waveforms -----------------
        // --------------------------------------------------------------------

        std::vector<float> * data;
        for (unsigned int iwf = 0; iwf < nwfs;iwf++){
            const eudaq::StandardWaveform & waveform = sev.GetWaveform(iwf);
                // get the sensor name. see eventually what this actually does!

                std::string type_name;
                type_name = waveform.GetType();

                std::string sensor_name;
                sensor_name = waveform.GetSensor();

                v_type_name->push_back(type_name);
                v_sensor_name->push_back(sensor_name);

                // SimpleStandardWaveform simpWaveform(sensorname,waveform.ID());
                // simpWaveform.setNSamples(waveform.GetNSamples());

                int n_samples = waveform.GetNSamples();
                if (verbose > 3) std::cout << "number of samples in my wf " << n_samples << std::endl;

                // load the wafeforms into the vector
                data = waveform.GetData();
                
                // calculate the signal and the baseline. this is very hardcoded!!!
                // float sig = CalculatePeak(data, 1075, 1150);
                std::pair<int, float> sig = FindMaxAndValue(data,   10,  300);
                float ped = Calculate    (data,  300,  700);

                // save the values in the event
                v_sig     ->push_back(sig.second);
                v_sig_time->push_back(sig.first);

                v_ped->push_back(ped);
    
                if(iwf == 1){ // trigger WF
                    for (int j=0; j<data->size(); j++){
                        if( abs(data->at(j)) > 20. ) {f_trig_time = j; break;}
                    }
                }
                if(iwf == 2){ // pulser WF
                    f_pulser_int = Calculate(data, 0, n_samples, true);
                    f_pulser     = (f_pulser_int > 20.);
                }
        
        
                for (int j=0; j<data->size(); j++){
                    if     (iwf == 0) {avgWF_0->SetBinContent(j+1, avgWF(avgWF_0->GetBinContent(j+1),data->at(j),f_event_number+1)); }
                    else if(iwf == 1) {avgWF_1->SetBinContent(j+1, avgWF(avgWF_1->GetBinContent(j+1),data->at(j),f_event_number+1)); }
                    else if(iwf == 2) {avgWF_2->SetBinContent(j+1, avgWF(avgWF_2->GetBinContent(j+1),data->at(j),f_event_number+1)); }
                    else if(iwf == 3) {avgWF_3->SetBinContent(j+1, avgWF(avgWF_3->GetBinContent(j+1),data->at(j),f_event_number+1)); }
                }

                data->clear();
        
        }
        
        // --------------------------------------------------------------------
        // ---------- save all infor for the telescope ------------------------
        // --------------------------------------------------------------------
        
        for (size_t iplane = 0; iplane < sev.NumPlanes(); ++iplane) {
            
            const eudaq::StandardPlane & plane = sev.GetPlane(iplane);
            std::vector<double> cds = plane.GetPixels<double>();
            
            for (size_t ipix = 0; ipix < cds.size(); ++ipix) {
                f_plane->push_back(iplane);
                f_col->push_back(plane.GetX(ipix));
                f_row->push_back(plane.GetY(ipix));
                f_adc->push_back((int)plane.GetPixel(ipix));
                f_charge->push_back(42);	
            }
        }
        
        m_ttree->Fill();
        
    }
    
    
    FileWriterTreeDRS4::~FileWriterTreeDRS4() {
        std::cout<<"Tree has " << m_ttree->GetEntries() << " entries" << std::endl;
        m_ttree->Write();
        avgWF_0->Write();
        avgWF_1->Write();
        avgWF_2->Write();
        avgWF_3->Write();
        
        if(m_tfile->IsOpen()) m_tfile->Close();
    }

    float FileWriterTreeDRS4::Calculate(std::vector<float> * data, int min, int max, bool _abs) {
        float integral = 0;
        int i;
        for (i = min; i <= int(max+1) && i < data->size() ;i++){
            if(!_abs) integral += data->at(i);
            else integral += abs(data->at(i));
        }
        return integral/(float)(i-(int)min);
    }
    float FileWriterTreeDRS4::CalculatePeak(std::vector<float> * data, int min, int max) {
        int mid = (int)( (max + min)/2 );
        int n = 1;
        while( (data->at(mid-1)/data->at(mid) -1)*(data->at(mid+1)/data->at(mid) -1) < 0 ){
            // jump up and down from the center position to find the max or min
            mid += pow(-1, n)*n;
            n+=1;
        }

        // extremal value is now at mid
        
        float integral = Calculate(data, mid-3, mid+6);
        return integral;
        
    }
    std::pair<int, float> FileWriterTreeDRS4::FindMaxAndValue(std::vector<float> * data, int min, int max) {
        float maxVal = -999;
        int imax = min;
        for (int i = min; i <= int(max+1) && i < data->size() ;i++){
            if (abs(data->at(i)) > maxVal){ maxVal = abs(data->at(i)); imax = i; }
        }
        maxVal = data->at(imax);
        std::pair<int, float> res = make_pair(imax, maxVal);
        return res;
    }
    float FileWriterTreeDRS4::avgWF(float old_avg, float new_value, int n) {
        float avg = old_avg;
        avg -= old_avg/n;
        avg += new_value/n;
        return avg;
    }

    uint64_t FileWriterTreeDRS4::FileBytes() const { return 0; }

}
#endif // ROOT_FOUND
