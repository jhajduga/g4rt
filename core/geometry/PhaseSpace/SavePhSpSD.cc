#include "SavePhSpSD.hh"
#include "G4SystemOfUnits.hh"
#include "G4AutoLock.hh"

namespace { G4Mutex sdMutex = G4MUTEX_INITIALIZER; }

/**
 * @brief Constructs a sensitive detector for recording phase space data and managing particle termination.
 *
 * Merges user-defined and header phase space positions, determines the maximum position as the "killer" plane for particle termination, and initializes mappings for supported particle types. Prepares the detector for recording particle data and killing tracks that cross the killer plane.
 *
 * @param phspUsr Vector of user-defined phase space positions.
 * @param phspHead Vector of header-defined phase space positions.
 * @param id_ Identifier for the sensitive detector instance.
 */
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
/**
 * @brief Processes a particle hit, records phase space data, and kills tracks crossing the killer plane.
 *
 * Records particle position, momentum direction, kinetic energy, particle type, and detector ID to the analysis ntuple when a recognized particle crosses a geometry boundary. If the particle crosses the defined killer plane in the z-direction during the step, its track is stopped and killed.
 *
 * @param step The current Geant4 step to process.
 * @return G4bool Always returns true to indicate successful processing.
 */
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