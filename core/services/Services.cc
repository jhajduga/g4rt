//
// Created by brachwal on 2018-09-23.
//
#include "Services.hh"
#include <algorithm>
#include "colors.hh"
#include "LogSvc.hh"
#include "G4Box.hh"
#include "IPhysicalVolume.hh"

#include <regex>

namespace fs = std::filesystem;


////////////////////////////////////////////////////////////////////////////////////////
// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
std::string svc::currentDateTime() {
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
	// for more information about date/time format
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
	return buf;
}

////////////////////////////////////////////////////////////////////////////////////////
// Get current date format as YYYY-MM-DD
std::string svc::currentDate() {
    auto dateTime = currentDateTime();
	return dateTime.substr(0,dateTime.find_first_of('.'));
}



////////////////////////////////////////////////////////////////////////////////////
///
std::string svc::createCurrentDateDirIfNotExits(const std::string& path){
  std::string dateDirPath = path;
	auto date = svc::currentDate();
	dateDirPath+=path.at(path.length()-1)=='/' ? date : "/"+date;
	fs::path dp (dateDirPath);
	if(!fs::exists (dp)){
		std::cout << "[INFO]:: Created directory: "<< dateDirPath <<std::endl;
		fs::create_directories(dp);
	}
	return dateDirPath;

}

////////////////////////////////////////////////////////////////////////////////////
///
std::string svc::getOutputDir(){ // const std::string& path
  auto configSvc = Service<ConfigSvc>();
  auto output_dir = configSvc->GetValue<std::string>("RunSvc", "OutputDir");
    if(output_dir.empty()){
      LOGSVC_CRITICAL("OutputDir must be not empty!");
      std::exit(EXIT_FAILURE);
      
      // configSvc->SetValue("RunSvc", "OutputDir", output_dir);
      //   output_dir = svc::createOutputDir();
    }
    else {
      if(!svc::checkIfFileExist(output_dir)){
        output_dir = svc::createOutputDir(output_dir);
      }
    }
  return output_dir;
}

////////////////////////////////////////////////////////////////////////////////////
///
std::string svc::createDirIfNotExits(const std::string& path) {
  fs::path dp (path);
  if(!fs::exists (dp)){
		std::cout << "[INFO]:: Created directory: "<< path <<std::endl;
		fs::create_directories(dp);
	}
	return path;
}

////////////////////////////////////////////////////////////////////////////////////
///
std::string svc::createOutputDir(const std::string& userArgPath) {
    // define the output dir path
    std::string output_dir_path;
    std::string project_path = PROJECT_LOCATION_PATH;
    project_path+="/output";
    if(!userArgPath.empty())
        output_dir_path=userArgPath;
    else
        output_dir_path = project_path;

    // create current day-date dir if not exists
    auto date_output_dir = svc::createCurrentDateDirIfNotExits(output_dir_path);
    // std::cout << "[INFO]:: Output location is defined as:\n"
    //                 "[INFO]:: "<< date_output_dir << std::endl;
    return date_output_dir;
}

////////////////////////////////////////////////////////////////////////////////////
///
bool svc::checkIfFileExist(const std::string& file_full_or_relative_path){
  if(file_full_or_relative_path.empty()) 
    return false;
  // try to get file from full path
	if(fs::exists(fs::path(file_full_or_relative_path)))
    return true;
  // try to get file from relative path
  std::string data_path = PROJECT_DATA_PATH;
  auto file = data_path+"/"+file_full_or_relative_path;
  if(fs::exists(fs::path(file)))
    return true;
  return false;
}

////////////////////////////////////////////////////////////////////////////////////
///
void svc::deleteFileIfExists(const std::string& file_full_path){
	fs::path fp = file_full_path;
	if(fs::exists(fp)){
		std::cout << "[INFO]:: Remove file:\n"
			   		 "[INFO]:: " << file_full_path <<std::endl;
		fs::remove(fp);
	}
}

////////////////////////////////////////////////////////////////////////////////////
///
size_t svc::countDirsInLocation(const std::string& path,const std::string& substr){
  auto dirIter = std::filesystem::directory_iterator(path);
  std::string patternNum = substr + std::string("_([0-9]+)");
  std::string pattern0 = substr + std::string("$");
  LOGSVC_TRACE("pattern: {}", patternNum);
  std::regex regexPatternNum(patternNum);
  std::regex regexPattern0(pattern0);
  int maxNumber = -1; // Initialize with a minimum value
  for (auto& entry : dirIter){
      if (entry.is_directory()){
        std::smatch match;
        std::string path = entry.path().string(); //nedded for regexp search
        LOGSVC_TRACE("Path: {}", path);
        int number = -1;
        if (std::regex_search(path, match, regexPattern0)) {
          LOGSVC_TRACE("Match first.");
          number = 0;
        }
        else if (std::regex_search(path, match, regexPatternNum)) {
            std::string numberStr = match[1].str();
            LOGSVC_TRACE("Matchstr: {}", numberStr);
            try {
                number = std::stoi(numberStr);
            } catch (const std::exception& e) {
                LOGSVC_TRACE("Conversion to int error. Skiping.")
            }
        }
        // Update the maximum number if necessary
        if (number > maxNumber) {
            maxNumber = number;
        }
        LOGSVC_TRACE("Maxnum: {}", maxNumber);
      }
  }
  return maxNumber + 1;
}

////////////////////////////////////////////////////////////////////////////////////
///
std::vector<std::string> svc::getFilesInDir(const std::string& path, const std::string& extension){
  std::vector<std::string> files;
  if(path.empty())
    return files;
  for (const auto & file: std::filesystem::directory_iterator(path)) {
    auto file_path = static_cast<std::string>(file.path());
    if(extension.empty()){
      files.push_back(file_path);
    }
    else {
      if(file_path.find(extension) != std::string::npos)
        files.push_back(file_path);
    }
  }
  return files;
}

////////////////////////////////////////////////////////////////////////////////
///
std::string svc::getFileExtenstion(const std::string& filePath){
  // Find the last dot position
  std::size_t dotPos = filePath.rfind('.');
  // If there's no dot, or it's the first character (hidden files in Unix), return an empty string
  if (dotPos == std::string::npos || dotPos == 0)
      return "";
  // Extract and return the substring after the last dot
  return filePath.substr(dotPos + 1);
}

////////////////////////////////////////////////////////////////////////////////
///
std::string svc::getFileName(const std::string& filePath){
  // Find last occurrence of '/' or '\\' to handle different path separators
  size_t lastSlash = filePath.find_last_of("/\\");
  std::string fileName = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;

  // Find last occurrence of '.' to handle file extension
  size_t lastDot = fileName.find_last_of('.');
  return (lastDot != std::string::npos) ? fileName.substr(0, lastDot) : fileName;
}


////////////////////////////////////////////////////////////////////////////////
/// Parsing a String Containing a Number in Scientific Notation
double svc::scistr_to_double(const std::string &m_str) {

  std::stringstream ss(m_str);
  double converted = 0;
  ss >> converted;
  if (ss.fail()) {
    std::string str = "Unable to format ";
    str += m_str;
    str += " as a number!";
    throw (str);
  }
  return converted;
}

////////////////////////////////////////////////////////////////////////////////
/// Function template specialization for G4String type
template<>
std::string svc::to_string<G4String>(const G4String &v) {
  return std::string(v.data());
}

////////////////////////////////////////////////////////////////////////////////
/// Function template specialization for G4bool type
template<>
std::string svc::to_string<G4bool>(const G4bool &v) {
  return v ? "TRUE" : "FALSE";
}

////////////////////////////////////////////////////////////////////////////////
/// TODO: Note about RDF::MakeCsvDataFrame
bool svc::readCsv(const std::string &fname, std::map<std::string, std::vector<double> *> &data) {

  G4cout << "[INFO]:: Reading the csv file :: " << fname << G4endl;

  if (data.size() > 0) {
    G4cout << "[WARNING]:: Svc :: readCsv :: clearing the data container " << G4endl;
    for (auto &i: data) delete i.second;
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
        line = line.substr(0, end - 1);

        if (!isInitialized) {
          size_t n = std::count(line.begin(), line.end(), ',');
          for (size_t i = 0; i < n + 1; ++i) {
            data["Col" + std::to_string(i)] = new std::vector<double>{};
            isInitialized = true;
          }
        }

        std::istringstream ssLine(line);
        std::string val_str;
        int n(0);
        while (std::getline(ssLine, val_str, ',')) {
          std::size_t vSci = val_str.find('e'); // Number in Scientific Notation
          if (vSci != std::string::npos)
            data.at("Col" + std::to_string(n++))->push_back(svc::scistr_to_double(val_str));
          else
            data.at("Col" + std::to_string(n++))->push_back(std::stod(val_str));
        }
      }
    }
    return true;
  } else {
    return false;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void svc::invalidArgumentError(const std::string& caller, const std::string& message){
    std::cout << FRED("[ERROR]")<<"::"<< caller << ":: " << message << std::endl;
    throw std::invalid_argument("invalid argument");
}

////////////////////////////////////////////////////////////////////////////////
///
double svc::round_with_prec(double val, int prec){
  return  std::round(val * std::pow(10,prec)) / std::pow(10,prec);
}

////////////////////////////////////////////////////////////////////////////////
///
G4ThreeVector svc::round_with_prec(const G4ThreeVector& val, int prec){
  return  G4ThreeVector(
    svc::round_with_prec(val.getX(),prec),
    svc::round_with_prec(val.getY(),prec),
    svc::round_with_prec(val.getZ(),prec)
  );
}

////////////////////////////////////////////////////////////////////////////////
///
std::string svc::tolower(const std::string& source){
  auto data = source;
  std::transform(data.begin(), data.end(), data.begin(),
    [](unsigned char c){ return std::tolower(c); });
  return data;
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<double> svc::linearizeG4ThreeVector(const G4ThreeVector& vector){
  return std::vector<double>{vector.getX(),vector.getY(),vector.getZ()};
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<double> svc::linearizeG4ThreeVector(const std::vector<G4ThreeVector>& vector){
  std::vector<double> linearized_vector;
  for(const auto& iv : vector){ 
    for(const auto& i :  svc::linearizeG4ThreeVector(iv))
      linearized_vector.emplace_back(i);
  }
  return std::move(linearized_vector);
}

////////////////////////////////////////////////////////////////////////////////
///
G4ThreeVector svc::getHalfSize(G4VPhysicalVolume* volume){
  auto solid = dynamic_cast<G4Box*>(volume->GetLogicalVolume()->GetSolid());
  return G4ThreeVector(solid->GetXHalfLength(),solid->GetYHalfLength(),solid->GetZHalfLength());
}

////////////////////////////////////////////////////////////////////////////////
///
G4ThreeVector svc::transformPosition(const G4ThreeVector& localPosition, IPhysicalVolume* volumeOfLocalFrame, Transform direction){
  G4cout << "transformPosition: " << localPosition << "..." << G4endl;
  G4ThreeVector globalPosition = localPosition; // Start with local position
  // Traverse up the hierarchy
  auto currentVolume = volumeOfLocalFrame->GetParentPtr();
  while (currentVolume) {
    // G4cout << " current volume of local frame: " << currentVolume->GetName() << G4endl;
    auto pv = currentVolume->GetPhysicalVolume();
    if (pv){
      // G4cout << " got physical volume: " << pv->GetName() << G4endl;

      auto is_rotated = false;
      auto is_translated = false;
      // Get the frame rotation and translation of the current volume
      auto frameRotation = pv->GetFrameRotation();
      auto frameTranslation = pv->GetFrameTranslation();

      if (frameRotation)
        is_rotated = frameRotation->norm2() > 1e-10 ? true : false;

      is_translated = frameTranslation.mag2() > 1e-10 ? true : false;

      if(is_rotated){
        G4cout << " got frame rotation: " << *frameRotation << G4endl;
        G4cout << " performing inverse rotation... " << G4endl;
        if (direction==Transform::LocalToGlobal)
          globalPosition = frameRotation->inverse() * globalPosition;
        else if (direction==Transform::GlobalToLocal)
         globalPosition = *frameRotation * globalPosition;
      } else {
        G4cout << " no rotation. " << G4endl;
      }
      // if(is_translated){
      //   G4cout << " got frame translation: " << frameTranslation << G4endl;
      //   G4cout << " performing inverse translation... " << G4endl;
      //   globalPosition -= frameTranslation;
      // } else {
      //   G4cout << " no translation. " << G4endl;
      // }
    } else {
        G4cout << " no physical volume found." << G4endl;
    }
    // Move to the parent volume
    currentVolume = currentVolume->GetParentPtr();
    // G4LogicalVolume* motherLogical = currentVolume->GetMotherLogical();
    // if (motherLogical) {
    //     // G4cout << " end of geoemtry tree... " << G4endl;
    //     break; // This is the world volume, stop traversal
    // }
    // // Find the physical volume corresponding to the mother
    // currentVolume = motherLogical->GetDaughter(0); // Traverse upwards
  }
  return globalPosition;
}


////////////////////////////////////////////////////////////////////////////////
///
std::size_t svc::getHashedStrFromIndexes(const std::vector<int>& indexes){
  std::string hash_str;
  for(const auto& idx : indexes)
    hash_str+=std::to_string(idx)+"-";
  if (!hash_str.empty())
    hash_str.pop_back(); // Removes the last character
  else
    G4cout << "[WARNING]::Svc::getHashedStrFromIndexes:: Returning hashed key for empty string! " << G4endl;
  return std::hash<std::string>{}(hash_str);
}


