#include "UserScoreWriter.hh"

#include "G4MultiFunctionalDetector.hh"
#include "G4SDParticleFilter.hh"
#include "G4VPrimitiveScorer.hh"
#include "G4VScoringMesh.hh"
#include <fstream>
#include <map>

// Based on https://github.com/Geant4/geant4/tree/geant4-10.4-release/examples/extended/runAndEvent/RE03

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
UserScoreWriter::UserScoreWriter() : G4VScoreWriter() { ; }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
UserScoreWriter::~UserScoreWriter() { ; }

void UserScoreWriter::DumpQuantityToFile(const G4String &psName, const G4String &fileName, const G4String &option) {

//
if (verboseLevel > 0) {
  G4cout << "User-defined DumpQuantityToFile() method is invoked." << G4endl;
}

// change the option string into lowercase to the case-insensitive.
G4String opt = option;
std::transform(opt.begin(), opt.end(), opt.begin(), (int (*)(int))(tolower));

// confirm the option
if (opt.size() == 0) opt = "csv";

// two options are possible: csv (default one) and sparse
if (opt.find("csv") == std::string::npos && opt.find("sparse") == std::string::npos) {
  G4cerr << "ERROR : DumpToFile : Unknown option -> " << option << G4endl;
  return;
}

// retrieve the map
auto fSMap = fScoringMesh->GetScoreMap();

std::map<G4int, G4StatDouble*>* score;
try {
  score = fSMap.at(psName)->GetMap();
}
catch (const std::out_of_range& e) {
  G4cerr << "ERROR : DumpToFile : Unknown quantity, \"" << psName << "\"." << G4endl;
  return;
}

G4double unitValue = fScoringMesh->GetPSUnitValue(psName);
G4String unit = fScoringMesh->GetPSUnit(psName);
G4String divisionAxisNames[3];
fScoringMesh->GetDivisionAxisNames(divisionAxisNames);

int xi, yi, zi;
double x, y, z, x0, y0, z0, dx, dy, dz;
dx = (2.0 * fScoringMesh->GetSize()[0] / CLHEP::cm) / fNMeshSegments[0];
dy = (2.0 * fScoringMesh->GetSize()[1] / CLHEP::cm) / fNMeshSegments[1];
dz = (2.0 * fScoringMesh->GetSize()[2] / CLHEP::cm) / fNMeshSegments[2];
x0 = (fScoringMesh->GetTranslation()[0] - fScoringMesh->GetSize()[0]) / CLHEP::cm;
y0 = (fScoringMesh->GetTranslation()[1] - fScoringMesh->GetSize()[1]) / CLHEP::cm;
z0 = (fScoringMesh->GetTranslation()[2] - fScoringMesh->GetSize()[2]) / CLHEP::cm;

// open the file
std::ofstream ofile(fileName);
if (!ofile) {
  G4cerr << "ERROR : DumpToFile : File open error -> " << fileName << G4endl;
  return;
}

// print some metadata
ofile << "# mesh name: " << fScoringMesh->GetWorldName() << G4endl;
ofile << "# mesh half-size: " << fScoringMesh->GetSize() / CLHEP::cm << " [cm] " << G4endl;
ofile << "# mesh center: " << fScoringMesh->GetTranslation() / CLHEP::cm << " [cm] " << G4endl;
ofile << "# mesh segments: " << fNMeshSegments[0] << " " << fNMeshSegments[1] << " " << fNMeshSegments[2] << G4endl;
ofile << "# primitive scorer name: " << psName << std::endl;
ofile << "# mesh dx dy dz: " << dx << " " << dy << " " << dz << G4endl;

// index order
ofile << "# i" << divisionAxisNames[0] << ", i" << divisionAxisNames[1] << ", i" << divisionAxisNames[2];
ofile << ", " << divisionAxisNames[0] << " [cm]"
      << ", " << divisionAxisNames[1] << " [cm]"
      << ", " << divisionAxisNames[2] << " [cm]";  // unit of scored value
ofile << ", total(value) ";
if (unit.size() > 0) ofile << "[" << unit << "]";
ofile << ", total(val^2), entry" << G4endl;

// write quantity
ofile << std::setprecision(16);  // for double value with 8 bytes

// sparse option - we print only these items for which a non-zero entry exists
// this will result in much smaller file
if (opt.find("sparse") != std::string::npos) {
  for (auto item : *score) {
    // calculate and write x,y,z indices
    xi = item.first / (fNMeshSegments[2] * fNMeshSegments[1]);
    yi = (item.first - xi * fNMeshSegments[2] * fNMeshSegments[1]) / fNMeshSegments[2];
    zi = item.first - yi * fNMeshSegments[2] - xi * fNMeshSegments[2] * fNMeshSegments[1];
    ofile << xi << "," << yi << "," << zi << ",";

    // calculate and write x,y,z positions
    ofile << x0 + xi* dx << "," << y0 + yi* dy << "," << z0 + zi* dz << ",";

    // write scored value
    ofile << (item.second->sum_wx()) / unitValue << "," << (item.second->sum_wx2()) / unitValue / unitValue << ","
          << item.second->n() << G4endl;
  }

  // default option - all (including zero) data is written
} else {
  for (xi = 0, x = x0; xi < fNMeshSegments[0]; xi++, x += dx) {
    for (yi = 0, y = y0; yi < fNMeshSegments[1]; yi++, y += dy) {
      for (zi = 0, z = z0; zi < fNMeshSegments[2]; zi++, z += dz) {
        G4int idx = GetIndex(xi, yi, zi);

        ofile << xi << "," << yi << "," << zi << ",";
        ofile << x << "," << y << "," << z << ",";

        std::map<G4int, G4StatDouble*>::iterator value = score->find(idx);
        if (value == score->end()) {
          ofile << 0. << "," << 0. << "," << 0;
        } else {
          ofile << (value->second->sum_wx()) / unitValue << "," << (value->second->sum_wx2()) / unitValue / unitValue
                << "," << value->second->n();
        }
        ofile << G4endl;

      }  // z
    }    // y
  }      // x
  ofile << std::setprecision(6);
}

// close the file
ofile.close();

}
