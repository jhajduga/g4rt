#include "SavePhSpSD.hh"
#include "G4SystemOfUnits.hh"
#include "G4AutoLock.hh"

namespace { G4Mutex sdMutex = G4MUTEX_INITIALIZER; }

SavePhSpSD::SavePhSpSD(std::vector<G4double> phspUsr, std::vector<G4double> phspHead,
                                   G4int id_)
    : G4VSensitiveDetector(""), id(id_) {
  G4AutoLock lock(&sdMutex);
  phspPositions.insert(std::end(phspPositions), std::begin(phspHead), std::end(phspHead));
  phspPositions.insert(std::end(phspPositions), std::begin(phspUsr), std::end(phspUsr));
  killerPosition = *(std::max_element(std::begin(phspPositions), std::end(phspPositions)));

  particleCodesMapping[G4Gamma::Definition()->GetPDGEncoding()] = 1;
  particleCodesMapping[G4Electron::Definition()->GetPDGEncoding()] = 2;
  particleCodesMapping[G4Positron::Definition()->GetPDGEncoding()] = 3;
  particleCodesMapping[G4Neutron::Definition()->GetPDGEncoding()] = 4;
  particleCodesMapping[G4Proton::Definition()->GetPDGEncoding()] = 5;

  analysisManager = G4AnalysisManager::Instance();

  G4cout << "[DEBUG]:: SavePhSpSD at ";
  for (auto &ip : phspPositions) G4cout << G4BestUnit(ip, "Length") << ", ";
  G4cout << G4endl;
  G4cout << "[DEBUG]:: SavePhSpSD :: killerPosition:: " << G4BestUnit(killerPosition, "Length")
         << G4endl;
}

/// TODO: Handling in a proper way dumping data into given phsp directory within NTuple
/// it can be done with SavePhSpAnalysis::FillPhSp(G4Step *step)
G4bool SavePhSpSD::ProcessHits(G4Step *step, G4TouchableHistory *) {

  G4AutoLock lock(&sdMutex);

  auto preStepPoint = step->GetPreStepPoint();
  auto postStepPoint = step->GetPostStepPoint();
  G4int pdgcode = step->GetTrack()->GetDefinition()->GetPDGEncoding();

  G4double partMass = step->GetTrack()->GetDefinition()->GetPDGMass();
  G4double partMom = preStepPoint->GetMomentum().mag();
  G4double partEnergy = (std::sqrt(partMom * partMom + partMass * partMass) - partMass) / CLHEP::keV;

  if ((preStepPoint->GetStepStatus() == fGeomBoundary) &&
      (particleCodesMapping.count(pdgcode) == 1)) {

    analysisManager->FillNtupleDColumn(0, 0, preStepPoint->GetPosition().x() / CLHEP::cm);
    analysisManager->FillNtupleDColumn(0, 1, preStepPoint->GetPosition().y() / CLHEP::cm);
    analysisManager->FillNtupleDColumn(0, 2, preStepPoint->GetPosition().z() / CLHEP::cm);

    analysisManager->FillNtupleDColumn(0, 3, preStepPoint->GetMomentum().unit().x());
    analysisManager->FillNtupleDColumn(0, 4, preStepPoint->GetMomentum().unit().y());
    analysisManager->FillNtupleDColumn(0, 5, preStepPoint->GetMomentum().unit().z());

    analysisManager->FillNtupleDColumn(0, 6, partEnergy);

    analysisManager->FillNtupleIColumn(0, 7, particleCodesMapping[pdgcode]);
    analysisManager->FillNtupleIColumn(0, 8, id);

    analysisManager->AddNtupleRow(0);
  }

  // kill particles after last phase space plane - the killer plane
  if ((preStepPoint->GetPosition().z() < killerPosition) && (killerPosition <= postStepPoint->GetPosition().z())) {
    step->GetTrack()->SetTrackStatus(fStopAndKill);
  }

  return true;
}