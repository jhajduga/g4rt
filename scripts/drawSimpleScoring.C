#include "stdio.h"
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <string.h>

std::string m_tgname;
std::string m_tgtitle;
std::pair<double, double> m_tgrange;
std::map<std::string, TGraph*> m_tgraphs;
std::vector<TCanvas*> m_canvas;

void ReadDoseFromCsvFile(const std::string& fname);
bool readCsv(const std::string& fname, std::map<std::string,std::vector<double>*>& data);
void DrawDose1Dim(const char* axis);
double scistr_to_double(const std::string& str);

//////////////////////////////////////////////////////////////////////
///
void drawSimpleScoring(){
  std::cout << "[INFO]:: Drawing simple scoring outcome..." << std::endl;
//  DrawDose1Dim("X");
//  DrawDose1Dim("Y");
  DrawDose1Dim("Z");
}

//////////////////////////////////////////////////////////////////////
///
void ReadDoseFromCsvFile(const std::string& fname){
  std::cout << "[INFO]:: ReadDoseFromCsvFile: " << fname << std::endl;

  std::map<std::string,std::vector<double>*> csvData;

  if(readCsv(fname, csvData)) {

    auto xData = csvData.at("Col5"); // Z [cm]
    auto yData = csvData.at("Col6"); // total(value)[Gy]
    double yMax(0.);
    for (int k = 0; k < yData->size(); ++k)
      yMax = yMax > yData->at(k) ? yMax : yData->at(k);

    for (int k = 0; k < yData->size(); ++k)
      yData->at(k) = (yData->at(k)/yMax ) *100.;


    m_tgraphs[m_tgname] = new TGraph(xData->size(), &(*xData)[0], &(*yData)[0]);
    m_tgraphs.at(m_tgname)->SetTitle(TString(m_tgtitle));

    m_tgraphs.at(m_tgname)->SetLineColor(kRed);
    m_tgraphs.at(m_tgname)->SetLineWidth(2);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////
///
bool readCsv(const std::string& fname, std::map<std::string,std::vector<double>*>& data) {

  std::cout << "[INFO]:: Reading the csv file :: " << fname <<std::endl;

  if(data.size()>0){
    std::cout << "[WARNING]:: Svc :: readCsv :: clearing the data container " <<std::endl;
    for(auto& i: data ) delete i.second;
    data.clear();
  }

  bool isInitialized(false);

  std::string line;
  std::ifstream file(fname.c_str());
  if (file.is_open()) {  // check if file exists
    while (getline(file, line)) {
      // get rid of commented out or empty lines:
      if (line.length() > 0 && line.at(0) != '#') {
        std::size_t lcomm = line.find('#');
        // comments at the end of the line (?)
        unsigned end = lcomm != std::string::npos ? lcomm - 1 : line.length();
        line = line.substr(0, end-1);

        if(!isInitialized){
          size_t n = std::count(line.begin(), line.end(), ',');
          for (int i = 0; i < n + 1; ++i) {
            data["Col"+std::to_string(i)] = new std::vector<double>{};
            isInitialized = true;
          }
        }

        std::istringstream ssLine(line);
        std::string val_str;
        int n(0);
        while(std::getline(ssLine, val_str, ',')) {
          std::size_t vSci = val_str.find('e'); // Number in Scientific Notation
          if(vSci!=std::string::npos)
            data.at("Col"+std::to_string(n++))->push_back(scistr_to_double(val_str));
          else
            data.at("Col"+std::to_string(n++))->push_back(std::stod(val_str));
        }
      }
    }
    return true;
  }
  else {
    return false;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////
///
void DrawDose1Dim(const char* axis){
  std::string path = "/home/brachwal/Workspace/Projects/TN-Dose3D/dose3d-geant4-linac/cmake-build-debug/";
  if(strcmp(axis,"X")==0) {
    m_tgrange.first = -25.;
    m_tgrange.second = 25.;
    m_tgname = "tgPDDx";
    m_tgtitle = "Percentage Depth Dose; X [cm]; Dose [%]";
    ReadDoseFromCsvFile(path+"dose1dx.csv");
  }
  if(strcmp(axis,"Y")==0) {
    m_tgrange.first = -25.;
    m_tgrange.second = 25.;
    m_tgname = "tgPDDy";
    m_tgtitle = "Percentage Depth Dose; Y [cm]; Dose [%]";
    ReadDoseFromCsvFile(path+"dose1dy.csv");
  }

  if(strcmp(axis,"Z")==0) {
    m_tgrange.first = 0.;
    m_tgrange.second = 50.;
    m_tgname = "tgPDDz";
    m_tgtitle = "Percentage Depth Dose; Z [cm]; Dose [%]";
    ReadDoseFromCsvFile(path+"dose1dz.csv");
  }

  m_canvas.push_back(new TCanvas(TString(m_tgname),"Patient Analysis :: Dose",600,400));
  m_tgraphs.at(m_tgname)->Draw("AL");

}

//////////////////////////////////////////////////////////////////////////////////////////
///
double scistr_to_double(const std::string& str) {

  std::stringstream ss(str);
  double d = 0;
  ss >> d;

  if (ss.fail()) {
    std::string s = "Unable to format ";
    s += str;
    s += " as a number!";
    throw (s);
  }

  return (d);
}