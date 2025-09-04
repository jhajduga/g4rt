#include "SavePhSpSD.hh"
#include "G4SystemOfUnits.hh"
#include "G4AutoLock.hh"

namespace { G4Mutex sdMutex = G4MUTEX_INITIALIZER; }

/**
 * @brief Create a sensitive detector that records phase-space crossings and enforces a killer plane.
 *
 * Constructs a SavePhSpSD instance by merging header and user phase-space position lists, initializing a small PDG-to-code mapping for supported particle types (gamma, electron, positron, neutron, proton), and obtaining the G4 analysis manager used for ntuple recording. The maximum value among the supplied phase-space positions is stored as killerPosition; any track that crosses from below to at-or-above this z position during a step will be stopped and killed. The constructor acquires the file-local mutex (sdMutex) to serialize initialization across threads.
 *
 * @param phspUsr User-provided phase-space positions (lengths/coordinates used to form the detector's phase-space list).
 * @param phspHead Header-provided phase-space positions (merged with phspUsr).
 * @param id_ Integer identifier for this sensitive detector instance.
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
 * @brief Handle a step: record phase-space data at geometry boundaries and kill tracks crossing the killer plane.
 *
 * When a track's pre-step point is at a geometry boundary and the track's PDG encoding is present in the detector's
 * particle mapping, this records a single ntuple row with the particle's position, momentum direction, kinetic energy,
 * mapped particle code, and detector id. Position values are written in centimeters and kinetic energy in keV.
 *
 * Ntuple column mapping (columns 0–8):
 *  - 0–2: pre-step position x, y, z (cm)
 *  - 3–5: momentum unit vector x, y, z
 *  - 6:   kinetic energy (keV)
 *  - 7:   mapped particle code (int)
 *  - 8:   detector id (int)
 *
 * Independently of recording, if the step crosses the configured killer plane in z (pre-step z < killerPosition ≤ post-step z),
 * the track is stopped and killed.
 *
 * @param step The Geant4 step being processed (pre/post step points and track accessed as needed).
 * @return G4bool Always returns true to indicate processing completed.
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