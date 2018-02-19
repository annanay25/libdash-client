#include <iostream>
#include <cpr/cpr.h>
#include <unistd.h>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <fstream>
#include <libdash.h>

using namespace dash;
using namespace dash::network;
using namespace std;
using namespace dash::mpd;
using namespace dash::xml;
using namespace dash::metrics;

uint32_t getAverageBandwidth(vector<uint32_t> bandwidth){
  float sum = 0, count = 0;

  for(auto it = bandwidth.begin(); it != bandwidth.end(); it ++){
    sum += *it;
    count ++;
  }

  return (sum/count);
}

std::chrono::duration<double> getAverageDuration(vector< std::chrono::duration<double> > duration){
  std::chrono::duration<double> sum;
  int count = 0;

  sum = *(duration.begin());
  for(auto it = duration.begin()+1; it != duration.end(); it ++){
    sum += *it;
    count ++;
  }

  return (sum/count);
}

int main(int argc, char** argv) {

  srand (time(NULL));

//  auto r = cpr::Get(cpr::Url{"localhost:8000/BigBuckBunny_1s_simple_2014_05_09.mpd"});

  auto r_mpd = cpr::Get(
      cpr::Url{"localhost:8000/BigBuckBunny_1s_simple_2014_05_09.mpd"});

  std::ofstream outfile;
  outfile.open("/Users/Avini/Desktop/code/tin/libdash/client/cpr-example/test.mpd", std::ios::out | std::ios::trunc);
  outfile << r_mpd.text;
  outfile.close();
  
  cout << r_mpd.text.length() << endl;

  /* MPD parsing */
  IDASHManager *manager = CreateDashManager();
  IMPD *mpd = manager->Open("/Users/Avini/Desktop/code/tin/libdash/client/cpr-example/test.mpd");

  /* Trying something new. */
  //uint8_t *p_data = new uint8_t[32768];
  //manager->start();
  //ret = manager->read(p_data, 32768);

  /* End of trying something new. */

  if (mpd) {

    uint32_t bandwidth;
    vector<uint32_t> bandwidth_values_available;

    /*
     * Get segment template (media path)
     * */
    ISegmentTemplate* segTemplate;
    std::string segTemplateMediaRegex;
    if(mpd->GetPeriods().at(0)->GetAdaptationSets().at(0)->GetSegmentTemplate()){
      cout << "LILILILIL" << endl;
      segTemplate = mpd->GetPeriods().at(0)->GetAdaptationSets().at(0)->GetSegmentTemplate();
      segTemplateMediaRegex = segTemplate->Getmedia();
      cout << segTemplateMediaRegex << endl;
    }

    /*
     * Get representations (just use bandwidth for now).
     * */
    auto temp = mpd->GetPeriods().at(0)->GetAdaptationSets().at(0)->GetRepresentation();
    if(mpd->GetPeriods().at(0)->GetAdaptationSets().at(0)->GetRepresentation().at(0)){
      cout << "LELELEL " << temp[0]->GetBandwidth() << endl;
      if(mpd->GetPeriods().at(0)->GetAdaptationSets().at(0)->GetRepresentation().at(0)->GetBandwidth()){
        bandwidth = mpd->GetPeriods().at(0)->GetAdaptationSets().at(0)->GetRepresentation().at(0)->GetBandwidth();
        cout << "YESSSSSS " << bandwidth << endl;
      }
    }

    for(auto it = temp.begin() ; it != temp.end(); ++it){
      bandwidth_values_available.push_back((*it)->GetBandwidth());
    }
    /*
     * Convert Segment Template to bitstream switching segment.
     * 
     * We are going to use the following function to get a segment URI.
     * virtual ISegment* GetMediaSegmentFromNumber (const std::vector<IBaseUrl*>& baseurls, const std::string& representationID, uint32_t bandwidth, uint32_t number) const = 0;
     *
     * */


    std::vector<dash::mpd::IBaseUrl *> baseurls;
    //baseurls.push_back(NewIBaseUrl);
    baseurls.push_back(mpd->GetBaseUrls().at(0));

    cout << "WOHOOOOOO  " << mpd->GetBaseUrls().at(0)->GetUrl() << endl;
    /*
     * Decide bandwidth everytime.
     * */
    
    cout << "Bandwidths available: " << endl;
    for(int i=0; i<14; i++){
      cout << bandwidth_values_available[i];
      cout << endl;
    }

    int bandwidth_index = 0;
    vector< std::chrono::duration<double> > past_data_duration;
    vector< uint32_t > past_data_bandwidth;

    for(uint32_t i=1; i<597; i++){

      auto start = std::chrono::system_clock::now();

      ISegment *seg = segTemplate->GetMediaSegmentFromNumber(baseurls, "\"\"", bandwidth_values_available[bandwidth_index], i);
      if(seg){
        cout << "Downloading" << endl;
        seg->StartDownload();
      }

      auto end = std::chrono::system_clock::now();

      /* Sleep to simulate network behavior */
      unsigned int microseconds;
      microseconds = rand() % 10000;
      std::cout << "Sleeping for " << microseconds << " microseconds." << std::endl;
      usleep(microseconds);

      auto elapsed = end - start;

      /*
       * Make new bandwidth decision based on old bandwidth and time taken.
       * */

      past_data_duration.push_back(elapsed);
      past_data_bandwidth.push_back(bandwidth_values_available[bandwidth_index]);

      uint32_t avg_bandwidth = getAverageBandwidth(past_data_bandwidth);
      std::chrono::duration<double> avg_duration = getAverageDuration(past_data_duration);

      cout << "Current bandwidth: " << bandwidth_index << " Average bandwidth: " << avg_bandwidth << endl;
      if(bandwidth_values_available[bandwidth_index] <= avg_bandwidth){
        /* Increase bandwidth */
        if(bandwidth_index < 13)
          bandwidth_index ++;
      }
      if(elapsed > avg_duration){
        /* Decrease bandwidth */
        if(bandwidth_index > 0)
          bandwidth_index --;
      }

      if(elapsed < avg_duration){
        /* Increase bandwidth */
        if(bandwidth_index < 13)
          bandwidth_index ++;
      }

      cout << "=========================================" << endl;
    }
  }


  //auto r = cpr::Get(
  //    cpr::Url{"localhost:8000/bunny_537825bps/BigBuckBunny_1s363.m4s"});

}
