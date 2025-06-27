//
// Created by brachwal on 07.05.2020.
//

#include "VoxelHit.hh"

#include <G4Alpha.hh>
#include <G4Electron.hh>
#include <G4Gamma.hh>
#include <G4Neutron.hh>
#include <G4Positron.hh>
#include <G4Proton.hh>
#include <iomanip>
#include <numeric>

#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "Services.hh"

G4ThreadLocal G4Allocator<VoxelHit>* VoxelHitAllocator = nullptr;

////////////////////////////////////////////////////////////////////////////////
///
void VoxelHit::Fill(G4Step* aStep) {
  if (m_Voxel.m_primaryID < 0) {
    // Fill current position
    auto position = aStep->GetPreStepPoint()->GetPosition();  // in world volume frame
    SetGravCentre(position);

    // Fill mass of the voxel
    if (m_Voxel.m_Volume > 0.) {
      auto density = aStep->GetPreStepPoint()->GetMaterial()->GetDensity();
      m_Voxel.m_Mass = density * m_Voxel.m_Volume;
    }

    // Fill energy deposited along G4Step (i.e. ionization)
    auto edep = aStep->GetTotalEnergyDeposit();
    m_Voxel.m_Edep += edep;
    if (edep > 0) {  // don't increase the vector size with zeros
      m_Voxel.m_stepsEdep.emplace_back(edep);
      if (m_Voxel.m_Mass != 0.)
        m_Voxel.m_Dose = (m_Voxel.m_Edep / m_Voxel.m_Mass) / gray;
      else {
        G4cout << "[WARNING]::VoxelHit::GetDose() The voxel mass is not set!" << G4endl;
      }
    }

    /// Fill track info
    FillTrack(aStep);

    // Set voxel primary (incident) particle info
    auto trkType = GetTrackIdTypeMappingList(aStep);
    auto trkEnergy = aStep->GetPreStepPoint()->GetKineticEnergy();
    SetPrimary(trkType, trkEnergy);

  } else {  // this voxel hit has already been set, it should be updated..
    Update(aStep);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void VoxelHit::Update(G4Step* aStep) {
  // Fill current position
  auto position = aStep->GetPreStepPoint()->GetPosition();  // in world volume frame
  SetGravCentre(position);

  auto edep = aStep->GetTotalEnergyDeposit();
  m_Voxel.m_Edep += edep;
  if (edep > 0) {  // don't increase the vector size with zeros
    m_Voxel.m_stepsEdep.emplace_back(edep);
    if (m_Voxel.m_Mass != 0.)
      m_Voxel.m_Dose = (m_Voxel.m_Edep / m_Voxel.m_Mass) / gray;
    else {
      G4cout << "[WARNING]::VoxelHit::GetDose() The voxel mass is not set!" << G4endl;
    }
  }
  FillTrack(aStep);
}

////////////////////////////////////////////////////////////////////////////////
///
G4int VoxelHit::GetTrackIdTypeMappingList(G4Step* aStep) const {
  auto dynamic = aStep->GetTrack()->GetDynamicParticle();
  auto trkDef = dynamic->GetDefinition();
  G4int trkType = -1;
  if (trkDef == G4Gamma::Definition()) trkType = 1;
  if (trkDef == G4Electron::Definition()) trkType = 2;
  if (trkDef == G4Positron::Definition()) trkType = 3;
  if (trkDef == G4Neutron::Definition()) trkType = 4;
  if (trkDef == G4Proton::Definition()) trkType = 5;
  if (trkDef == G4Alpha::Definition()) trkType = 6;
  return trkType;
}

////////////////////////////////////////////////////////////////////////////////
///
/// Full identification of the physical process within the range of medical energies (0–10 MeV),
/// including the handling of secondary processes (e.g., electrons ejected via the photoelectric effect).
///
/// @param aStep – the current Geant4 simulation step.
/// @return int – ID representing the type of physical process that occurred at this step.

G4int VoxelHit::GetTrkIdProcessTypeMappingList(G4Step* aStep) const {
  auto proc = aStep->GetPostStepPoint()->GetProcessDefinedStep();
  if (!proc) return -1;  // Safety check in case no process is defined.

  auto procName = proc->GetProcessName();

  // Special user-defined processes or transportation steps
  if (procName == "UserStepMax" || procName == "Transportation") return 0;

  /// Basic electromagnetic (EM) interactions
  if (procName == "phot")
    return 1;  // Photoelectric effect: photon is absorbed, ejecting an electron.
  else if (procName == "compt")
    return 2;  // Compton scattering: photon scatters off an atomic electron.
  else if (procName == "conv")
    return 3;  // Pair production: photon converts into an electron-positron pair.
  else if (procName == "Rayl")
    return 4;  // Rayleigh scattering: elastic scattering of a photon by an atom.
  else if (procName == "eIoni")
    return 5;  // Electron ionization: electron knocks out another electron.
  else if (procName == "msc")
    return 6;  // Multiple Coulomb scattering: deflections due to repeated interactions.
  else if (procName == "annihil")
    return 7;  // e⁺e⁻ annihilation: positron annihilates with an electron, producing photons.
  else if (procName == "eBrem")
    return 8;  // Bremsstrahlung: radiation emitted due to electron deceleration near nuclei.

  /// Nuclear interactions and hadronic processes
  else if (procName == "RadioactiveDecay")
    return 9;  // Radioactive decay: spontaneous transformation of unstable nuclei.
  else if (procName == "hadElastic")
    return 10;  // Hadronic elastic scattering: e.g., elastic neutron interactions.
  else if (procName == "neutronInelastic")
    return 11;  // Inelastic neutron scattering: energy transfer with nuclear excitation.
  else if (procName == "nCapture")
    return 12;  // Neutron capture: neutron absorbed by nucleus, often followed by gamma emission.
  else if (procName == "protonInelastic")
    return 13;  // Inelastic proton-nucleus interaction.
  else if (procName == "alphaInelastic")
    return 14;  // Inelastic alpha-particle interactions.

  /// Ionization by heavy particles (protons, alpha particles, ions)
  else if (procName == "ionIoni")
    return 15;  // Ionization by heavy ions or charged particles.

  /// Production of secondary atomic electrons (shell effects)
  else if (procName == "Auger")
    return 16;  // Auger electron emission: non-radiative atomic de-excitation.
  else if (procName == "phot_fluo")
    return 17;  // X-ray fluorescence: radiative de-excitation following inner-shell ionization.
  else if (procName == "pixe")
    return 18;  // Proton Induced X-ray Emission (PIXE): X-rays generated by proton bombardment.

  /// Specialized processes for light particles (rare, but possible)
  else if (procName == "muIoni")
    return 19;  // Ionization caused by muons.
  else if (procName == "muBrems")
    return 20;  // Muon bremsstrahlung: radiation from muon deceleration.
  else if (procName == "muPairProd")
    return 21;  // Muon-induced pair production: e⁺e⁻ creation due to muon.

  /// Optional optical and electromagnetic effects
  else if (procName == "Cerenkov")
    return 22;  // Cerenkov radiation: light emission by charged particles exceeding the phase velocity in a medium.
  else if (procName == "Scintillation")
    return 23;  // Scintillation: light emission from material excited by ionizing radiation.
  else if (procName == "SynchrotronRadiation")
    return 24;  // Synchrotron radiation: emitted by relativistic charged particles in magnetic fields.

  /// Unknown or unexpected process
  return 99;
}

////////////////////////////////////////////////////////////////////////////////
///
/// Determines the origin of a secondary electron based on its creator process.
///
/// @param aStep – the current Geant4 simulation step, to readout a track – the G4Track object representing the electron.
/// @return int – an integer code representing the origin type of the secondary electron.

G4int VoxelHit::GetTrkIdElectronOriginTypeMappingList(G4Step* aStep) const { // TODO: !!! Why only for electron this was created? All particles and all interactions should be stored here
  if (aStep->GetTrack()->GetDynamicParticle()->GetDefinition() != G4Electron::Definition()) return -1;  // Not an electron

  auto creator = aStep->GetTrack()->GetCreatorProcess();
  if (!creator) return 0;  // Primary electron (no creator process)

  auto name = creator->GetProcessName();

  if (name == "phot") return 1;              // Photoelectric electron
  if (name == "compt") return 2;             // Compton-scattered electron
  if (name == "conv") return 3;              // Pair-production electron
  if (name == "Auger") return 4;             // Auger electron (non-radiative transition)
  if (name == "phot_fluo") return 5;         // Fluorescent electron (atomic de-excitation)
  if (name == "eIoni") return 6;             // Electron ionization electron
  if (name == "ionIoni") return 7;           // Ion-induced ionization electron
  if (name == "RadioactiveDecay") return 8;  // Beta electron from radioactive decay

  return 99;  // Other or unknown secondary process
}

////////////////////////////////////////////////////////////////////////////////
/// 
/// Fills the track information from a Geant4 step into the voxel hit.
void VoxelHit::FillTrack(G4Step* aStep) {
  if (m_store_tracks) {
    // TODO: !!! Store Parent ID or Primary ID -> for Purpose of a visualisation (OpenGL based for example)
    auto aTrack = aStep->GetTrack();
    auto trkID = aTrack->GetTrackID();
    auto ret = m_Voxel.m_trksId.insert(trkID);
    auto postStepPoint = aStep->GetPostStepPoint();
    auto preStepPoint = aStep->GetPreStepPoint();
    if (ret.second == true) {  // new element inserted
      m_Voxel.m_trksTypeId.emplace_back(GetTrackIdTypeMappingList(aStep));
      m_Voxel.m_trksProcessTypeId.emplace_back(GetTrkIdProcessTypeMappingList(aStep));
      m_Voxel.m_trksElectronOriginTypeId.emplace_back(GetTrkIdElectronOriginTypeMappingList(aStep));
      m_Voxel.m_trksE.emplace_back(aTrack->GetDynamicParticle()->GetKineticEnergy());
      // m_Voxel.m_trksPotentialE.emplace_back((aTrack->GetDynamicParticle()->GetTotalEnergy())-(aTrack->GetDynamicParticle()->GetKineticEnergy())) (>dla niefotonów)
      m_Voxel.m_trksTheta.emplace_back(aTrack->GetDynamicParticle()->GetMomentum().theta());
      m_Voxel.m_trksLength.emplace_back(aTrack->GetTrackLength());
      m_Voxel.m_trksPosition.emplace_back(preStepPoint->GetPosition());
    } else {
      // auto evtGlobalTime = aTrack->GetGlobalTime();
      auto edep = aStep->GetTotalEnergyDeposit();
    }
    // time: since the beginning of the event
    m_global_time = aTrack->GetGlobalTime();
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void VoxelHit::SetId(G4int xId, G4int yId, G4int zId) {
  if (m_Voxel.m_idx_x == -1 && m_Voxel.m_idx_y == -1 && m_Voxel.m_idx_z == -1) {
    m_Voxel.m_idx_x = xId;
    m_Voxel.m_idx_y = yId;
    m_Voxel.m_idx_z = zId;
  } else if (m_Voxel.m_idx_x != xId || m_Voxel.m_idx_y != yId || m_Voxel.m_idx_z != zId) {
    // TODO: handle this error!?
    G4cout << "[ERROR]::VoxelHit::SetChannelID:: unadjusted data" << G4endl;
    G4cout << "[ERROR]::VoxelHit::SetChannelID:: previous X " << m_Voxel.m_idx_x << " given " << xId << G4endl;
    G4cout << "[ERROR]::VoxelHit::SetChannelID:: previous Y " << m_Voxel.m_idx_y << " given " << yId << G4endl;
    G4cout << "[ERROR]::VoxelHit::SetChannelID:: previous Z " << m_Voxel.m_idx_z << " given " << zId << G4endl;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void VoxelHit::SetGlobalId(G4int xId, G4int yId, G4int zId) {
  if (m_Voxel.m_global_idx_x == -1 && m_Voxel.m_global_idx_y == -1 && m_Voxel.m_global_idx_z == -1) {
    m_Voxel.m_global_idx_x = xId;
    m_Voxel.m_global_idx_y = yId;
    m_Voxel.m_global_idx_z = zId;
  } else if (m_Voxel.m_global_idx_x != xId || m_Voxel.m_global_idx_y != yId || m_Voxel.m_global_idx_z != zId) {
    // TODO: handle this error!?
    G4cout << "[ERROR]::VoxelHit::SetGlobalChannelID:: unadjusted data" << G4endl;
    G4cout << "[ERROR]::VoxelHit::SetGlobalChannelID:: previous X " << m_Voxel.m_global_idx_x << " given " << xId << G4endl;
    G4cout << "[ERROR]::VoxelHit::SetGlobalChannelID:: previous Y " << m_Voxel.m_global_idx_y << " given " << yId << G4endl;
    G4cout << "[ERROR]::VoxelHit::SetGlobalChannelID:: previous Z " << m_Voxel.m_global_idx_z << " given " << zId << G4endl;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void VoxelHit::SetGravCentre(const G4ThreeVector& position) {
  // TODO: Replace running 1/2-average with true arithmetic mean.
  // Current implementation biases toward recent updates:
  // (((((p1 + p2)/2 + p3)/2 + p4)/2 + ... +pN)/2) != (p1 + p2 + ... + pN)/N
  // Suggest maintaining cumulative vector sum and count:
  //   m_sum += position; m_count++;
  //   m_GravitationalCentre = m_sum / m_count;

  if (m_Voxel.m_GravitationalCentre.x() == 0.)
    m_Voxel.m_GravitationalCentre.setX(position.x());
  else
    m_Voxel.m_GravitationalCentre.setX((position.x() + m_Voxel.m_GravitationalCentre.x()) / 2.); 

  if (m_Voxel.m_GravitationalCentre.y() == 0.)
    m_Voxel.m_GravitationalCentre.setY(position.y());
  else
    m_Voxel.m_GravitationalCentre.setY((position.y() + m_Voxel.m_GravitationalCentre.y()) / 2.);

  if (m_Voxel.m_GravitationalCentre.z() == 0.)
    m_Voxel.m_GravitationalCentre.setZ(position.z());
  else
    m_Voxel.m_GravitationalCentre.setZ((position.z() + m_Voxel.m_GravitationalCentre.z()) / 2.);
}

////////////////////////////////////////////////////////////////////////////////
///
G4int VoxelHit::GetID(G4int axisId) const {
  switch (axisId) {
    case 0:
      return m_Voxel.m_idx_x;
    case 1:
      return m_Voxel.m_idx_y;
    case 2:
      return m_Voxel.m_idx_z;
    default:
      G4cout << "[ERROR]::VoxelHit::GetID:: unset data" << G4endl;
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
///
G4int VoxelHit::GetGlobalID(G4int axisId) const {
  switch (axisId) {
    case 0:
      return m_Voxel.m_global_idx_x;
    case 1:
      return m_Voxel.m_global_idx_y;
    case 2:
      return m_Voxel.m_global_idx_z;
    default:
      G4cout << "[ERROR]::VoxelHit::GetID:: unset data" << G4endl;
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
///
void VoxelHit::Print() const { const_cast<VoxelHit*>(this)->Print(); }

////////////////////////////////////////////////////////////////////////////////
///
void VoxelHit::Print() {
  LOGSVC_INFO("Voxel ID ({},{},{})/({},{},{}) \n\tMass {}, Volume {}", m_Voxel.m_global_idx_x, m_Voxel.m_global_idx_y, m_Voxel.m_global_idx_z, m_Voxel.m_idx_x, m_Voxel.m_idx_y,
              m_Voxel.m_idx_z, m_Voxel.m_Mass, m_Voxel.m_Volume);
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<std::pair<G4int, G4double>> VoxelHit::GetTrkIdEnergyMappingList() const {
  std::vector<std::pair<G4int, G4double>> trkIdE;
  auto itE = m_Voxel.m_trksE.begin();
  for (auto itId = m_Voxel.m_trksId.begin(); itId != m_Voxel.m_trksId.end(); ++itId) trkIdE.emplace_back(*itId, *itE++);
  return trkIdE;
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<std::pair<G4int, G4double>> VoxelHit::GetTrkIdThetaMappingList() const {
  std::vector<std::pair<G4int, G4double>> trkIdTheta;
  auto itTheta = m_Voxel.m_trksTheta.begin();
  for (auto itId = m_Voxel.m_trksId.begin(); itId != m_Voxel.m_trksId.end(); ++itId) trkIdTheta.emplace_back(*itId, *itTheta++);
  return trkIdTheta;
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<std::pair<G4int, G4double>> VoxelHit::GetTrkIdLengthMappingList() const {
  std::vector<std::pair<G4int, G4double>> trkIdLength;
  auto itL = m_Voxel.m_trksLength.begin();
  for (auto itId = m_Voxel.m_trksId.begin(); itId != m_Voxel.m_trksId.end(); ++itId) trkIdLength.emplace_back(*itId, *itL++);
  return trkIdLength;
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<std::pair<G4int, G4double>> VoxelHit::GetTrkIdUserLengthMappingList() const {
  std::vector<std::pair<G4int, G4double>> trkIdLength;
  auto itL = m_Voxel.m_usrTrksLength.begin();
  for (auto itId = m_Voxel.m_usrTrksId.begin(); itId != m_Voxel.m_usrTrksId.end(); ++itId) trkIdLength.emplace_back(*itId, *itL++);
  return trkIdLength;
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<std::pair<G4int, G4ThreeVector>> VoxelHit::GetTrkIdPositionMappingList() const {
  std::vector<std::pair<G4int, G4ThreeVector>> trkPosition;
  auto itP = m_Voxel.m_trksPosition.begin();
  for (auto itId = m_Voxel.m_trksLength.begin(); itId != m_Voxel.m_trksLength.end(); ++itId) trkPosition.emplace_back(*itId, *itP++);
  return trkPosition;
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<std::pair<G4int, G4int>> VoxelHit::GetTrackIdTypeMappingList() const {
  std::vector<std::pair<G4int, G4int>> trkTypeId;
  auto itType = m_Voxel.m_trksTypeId.begin();
  for (auto itId = m_Voxel.m_trksId.begin(); itId != m_Voxel.m_trksId.end(); ++itId) trkTypeId.emplace_back(*itId, *itType++);
  return trkTypeId;
}

////////////////////////////////////////////////////////////////////////////////
/// Returns a list of pairs containing the track ID and the physical process type that occurred in that track.
///
std::vector<std::pair<G4int, G4int>> VoxelHit::GetTrkIdProcessTypeMappingList() const {
  // Vector to store the resulting pairs (Track ID, Process Type ID)
  std::vector<std::pair<G4int, G4int>> processTypeId;

  // Iterator for process types
  auto itProcType = m_Voxel.m_trksProcessTypeId.begin();
  for (auto itId = m_Voxel.m_trksId.begin(); itId != m_Voxel.m_trksId.end(); ++itId) {
    // Create a pair: (track ID, process type), e.g., (123, 2)
    processTypeId.emplace_back(*itId, *itProcType++);
  }

  return processTypeId;
}

////////////////////////////////////////////////////////////////////////////////
/// Returns a vector of pairs containing the track ID and the secondary electron origin classification.
std::vector<std::pair<G4int, G4int>> VoxelHit::GetTrkIdElectronOriginTypeMappingList() const {
  // Vector to store resulting pairs (Track ID, Electron Origin Type)
  std::vector<std::pair<G4int, G4int>> electronOriginTypeId;

  // Iterator for electron origin classification IDs
  auto itElectronType = m_Voxel.m_trksElectronOriginTypeId.begin();

  // Iterate simultaneously over track IDs and electron origin types
  for (auto itId = m_Voxel.m_trksId.begin(); itId != m_Voxel.m_trksId.end(); ++itId) {
    electronOriginTypeId.emplace_back(*itId, *itElectronType++);
  }

  return electronOriginTypeId;
}

////////////////////////////////////////////////////////////////////////////////
///
G4double VoxelHit::GetPrimaryTrkEnergy() const {
  for (auto& iIdE : GetTrkIdEnergyMappingList())
    if (iIdE.first == 1) return iIdE.second;
  return -1;  // No primary in this voxel hit
}

////////////////////////////////////////////////////////////////////////////////
///
G4double VoxelHit::GetMeanEnergyDeposit() const {
  if (m_Voxel.m_stepsEdep.size() > 0) {
    double sum = std::accumulate(m_Voxel.m_stepsEdep.begin(), m_Voxel.m_stepsEdep.end(), 0.0);
    return sum / m_Voxel.m_stepsEdep.size();
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
///
void VoxelHit::PrintEvtInfo() const {
  for (int i = 0; i < m_evtPrimariesIncidentE.size(); ++i) {
    G4cout << "[INFO]::VoxelHit::EvtInfo() incident primary(" << i << ") energy: " << m_evtPrimariesIncidentE.at(i) << " [keV]" << G4endl;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
std::size_t VoxelHit::GetGlobalHashedStrId() const { return svc::getHashedStrFromIndexes({m_Voxel.m_global_idx_x, m_Voxel.m_global_idx_y, m_Voxel.m_global_idx_z}); }

////////////////////////////////////////////////////////////////////////////////
///
std::size_t VoxelHit::GetHashedStrId() const {
  return svc::getHashedStrFromIndexes({m_Voxel.m_global_idx_x, m_Voxel.m_global_idx_y, m_Voxel.m_global_idx_z, m_Voxel.m_idx_x, m_Voxel.m_idx_y, m_Voxel.m_idx_z});
}

////////////////////////////////////////////////////////////////////////////////
///
bool VoxelHit::operator==(const VoxelHit& other) const {
  return m_Voxel.m_global_idx_x == other.m_Voxel.m_global_idx_x && m_Voxel.m_global_idx_y == other.m_Voxel.m_global_idx_y &&
         m_Voxel.m_global_idx_z == other.m_Voxel.m_global_idx_z && m_Voxel.m_idx_x == other.m_Voxel.m_idx_x && m_Voxel.m_idx_y == other.m_Voxel.m_idx_y &&
         m_Voxel.m_idx_z == other.m_Voxel.m_idx_z;
}

////////////////////////////////////////////////////////////////////////////////
///
bool VoxelHit::IsAligned(const VoxelHit& other, bool global_and_local) const {
  if (global_and_local) {
    return *this == other;
  }
  // only global
  return m_Voxel.m_global_idx_x == other.m_Voxel.m_global_idx_x && m_Voxel.m_global_idx_y == other.m_Voxel.m_global_idx_y && m_Voxel.m_global_idx_z == other.m_Voxel.m_global_idx_z;
}

////////////////////////////////////////////////////////////////////////////////
///
VoxelHit& VoxelHit::Cumulate(const VoxelHit& other, bool global_and_local_allignemnt_check) {
  // if(!global_and_local_allignemnt_check)
  //   Print(); LOGSVC_INFO("+"); other.Print();
  if (IsAligned(other, global_and_local_allignemnt_check)) {
    if (!global_and_local_allignemnt_check) {  // cell/voxel
      // G4cout << "[INFO]::VoxelHit::Cumulate m_Voxel.m_Dose pre summ "<<m_Voxel.m_Dose << G4endl;
      // G4cout << "[INFO]::VoxelHit::Cumulate other.GetVolume() "<<other.GetVolume() << G4endl;
      // G4cout << "[INFO]::VoxelHit::Cumulate GetVolume(); "<<GetVolume() << G4endl;

      m_Voxel.m_Dose += other.GetDose() * other.GetVolume() / GetVolume();

      // G4cout << "[INFO]::VoxelHit::Cumulate m_Voxel.m_Dose post summ "<<m_Voxel.m_Dose << G4endl;

    } else {
      return *this += other;
    }
  } else {
    LOGSVC_WARN("Trying to cumulate misaligned VoxelHits...");
    Print();
    LOGSVC_WARN("+");
    other.Print();
  }
  return *this;
}

////////////////////////////////////////////////////////////////////////////////
///
VoxelHit& VoxelHit::operator+=(const VoxelHit& other) {
  // TODO: m_Voxel.m_Dose+=other.GetDose()*other.GetVolume() / GetVolume(); Tu też?
  m_Voxel.m_Dose += other.GetDose();
  return *this;
}
