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
/**
 * @brief Initializes or updates voxel hit data from a Geant4 step.
 *
 * If the voxel is new, sets its position, calculates mass, accumulates energy deposition, updates dose, records track information, and stores primary particle type and energy. If the voxel has already been initialized, updates its data with the new step.
 */
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
    auto trkType = GetTrackIdType(aStep);
    auto trkEnergy = aStep->GetPreStepPoint()->GetKineticEnergy();
    SetPrimary(trkType, trkEnergy);

  } else {  // this voxel hit has already been set, it should be updated..
    Update(aStep);
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Updates the voxel hit with new energy deposition and track information from a Geant4 step.
 *
 * Adds the energy deposited in the current step to the voxel, updates the dose if mass is set, recalculates the gravitational center, and records track information.
 */
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
/**
 * @brief Returns an integer code representing the particle type of the track in the given Geant4 step.
 *
 * The returned code is:
 * - 1 for gamma
 * - 2 for electron
 * - 3 for positron
 * - 4 for neutron
 * - 5 for proton
 * - 6 for alpha
 * - -1 if the particle type is unknown or not one of the above.
 *
 * @return G4int Integer code for the particle type.
 */
G4int VoxelHit::GetTrackIdType(G4Step* aStep) const {
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
/**
 * @brief Returns an integer code representing the physical process type that occurred at the given simulation step.
 *
 * The returned code identifies the main interaction or process (e.g., electromagnetic, nuclear, optical) that defined the step, based on the process name in Geant4. Returns -1 if no process is defined, or 99 if the process is unknown.
 *
 * @return G4int Integer code corresponding to the physical process type at this step.
 */

G4int VoxelHit::GetTrkIdProcessType(G4Step* aStep) const {
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
/**
 * @brief Returns an integer code indicating the origin process of a secondary electron in a Geant4 step.
 *
 * For electron tracks, identifies the physical process that created the electron, such as photoelectric effect, Compton scattering, pair production, Auger effect, fluorescence, ionization, or radioactive decay.
 *
 * @return G4int Code representing the electron's origin process:
 *   - -1: Not an electron
 *   - 0: Primary electron (no creator process)
 *   - 1: Photoelectric effect
 *   - 2: Compton scattering
 *   - 3: Pair production
 *   - 4: Auger electron
 *   - 5: Fluorescent electron
 *   - 6: Electron ionization
 *   - 7: Ion-induced ionization
 *   - 8: Radioactive decay
 *   - 99: Other or unknown process
 */

G4int VoxelHit::GetTrkIdElectronOriginType(G4Step* aStep) const { // TODO: !!! Why only for electron this was created? All particles and all interactions should be stored here
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
/**
 * @brief Records track information from a Geant4 step into the voxel hit.
 *
 * If track storage is enabled, inserts the track ID from the current step. For new tracks, stores particle type, process type, electron origin type, kinetic energy, momentum angle, track length, and pre-step position. Updates the global event time from the track.
 */
void VoxelHit::FillTrack(G4Step* aStep) {
  if (m_store_tracks) {
    // TODO: !!! Store Parent ID or Primary ID -> for Purpose of a visualisation (OpenGL based for example)
    auto aTrack = aStep->GetTrack();
    auto trkID = aTrack->GetTrackID();
    auto ret = m_Voxel.m_trksId.insert(trkID);
    auto postStepPoint = aStep->GetPostStepPoint();
    auto preStepPoint = aStep->GetPreStepPoint();
    if (ret.second == true) {  // new element inserted
      m_Voxel.m_trksTypeId.emplace_back(GetTrackIdType(aStep));
      m_Voxel.m_trksProcessTypeId.emplace_back(GetTrkIdProcessType(aStep));
      m_Voxel.m_trksElectronOriginTypeId.emplace_back(GetTrkIdElectronOriginType(aStep));
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
/**
 * @brief Sets the local voxel indices if they have not been assigned.
 *
 * If the local voxel indices are already set and differ from the provided values, an error message is printed.
 *
 * @param xId Local voxel index along the x-axis.
 * @param yId Local voxel index along the y-axis.
 * @param zId Local voxel index along the z-axis.
 */
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
/**
 * @brief Sets the global voxel indices if unset; reports an error if attempting to change existing indices.
 *
 * Assigns the global voxel indices (x, y, z) if they have not been previously set. If the indices are already set and differ from the provided values, an error message is printed.
 */
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
/**
 * @brief Updates the voxel's gravitational center using a running half-average with the given position.
 *
 * The gravitational center is recalculated by averaging each coordinate with the new position, biasing toward recent updates. This is not a true arithmetic mean.
 *
 * @param position The position vector to incorporate into the gravitational center calculation.
 */
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
/**
 * @brief Returns the local voxel index for the specified axis.
 *
 * @param axisId Axis identifier: 0 for x, 1 for y, 2 for z.
 * @return Local voxel index along the specified axis, or -1 if the axisId is invalid.
 */
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
/**
 * @brief Returns the global voxel index for the specified axis.
 *
 * @param axisId Axis identifier: 0 for x, 1 for y, 2 for z.
 * @return Global voxel index along the specified axis, or -1 if the axisId is invalid.
 */
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
/**
 * @brief Prints voxel hit information, including IDs, mass, and volume.
 *
 * Calls the non-const Print() method to output details about the voxel hit.
 */
void VoxelHit::Print() const { const_cast<VoxelHit*>(this)->Print(); }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Prints the voxel's global and local indices, mass, and volume.
 *
 * Outputs voxel identification and physical properties for debugging or logging purposes.
 */
void VoxelHit::Print() {INFO_GEO("Voxel ID ({},{},{})/({},{},{}) \n\tMass {}, Volume {}", 
                        m_Voxel.m_global_idx_x, m_Voxel.m_global_idx_y, m_Voxel.m_global_idx_z, m_Voxel.m_idx_x, 
                        m_Voxel.m_idx_y, m_Voxel.m_idx_z, m_Voxel.m_Mass, m_Voxel.m_Volume);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns a list mapping track IDs to their kinetic energies for all tracks recorded in the voxel.
 *
 * @return Vector of pairs, each containing a track ID and its corresponding kinetic energy.
 */
std::vector<std::pair<G4int, G4double>> VoxelHit::GetTrkIdEnergyMappingList() const {
  std::vector<std::pair<G4int, G4double>> trkIdE;
  auto itE = m_Voxel.m_trksE.begin();
  for (auto itId = m_Voxel.m_trksId.begin(); itId != m_Voxel.m_trksId.end(); ++itId) trkIdE.emplace_back(*itId, *itE++);
  return trkIdE;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns a list mapping track IDs to their momentum theta angles.
 *
 * @return Vector of pairs, each containing a track ID and its corresponding momentum theta angle.
 */
std::vector<std::pair<G4int, G4double>> VoxelHit::GetTrkIdThetaMappingList() const {
  std::vector<std::pair<G4int, G4double>> trkIdTheta;
  auto itTheta = m_Voxel.m_trksTheta.begin();
  for (auto itId = m_Voxel.m_trksId.begin(); itId != m_Voxel.m_trksId.end(); ++itId) trkIdTheta.emplace_back(*itId, *itTheta++);
  return trkIdTheta;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns a list mapping track IDs to their corresponding track lengths within the voxel.
 *
 * @return Vector of pairs, each containing a track ID and its associated track length.
 */
std::vector<std::pair<G4int, G4double>> VoxelHit::GetTrkIdLengthMappingList() const {
  std::vector<std::pair<G4int, G4double>> trkIdLength;
  auto itL = m_Voxel.m_trksLength.begin();
  for (auto itId = m_Voxel.m_trksId.begin(); itId != m_Voxel.m_trksId.end(); ++itId) trkIdLength.emplace_back(*itId, *itL++);
  return trkIdLength;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns a list mapping user track IDs to their corresponding user track lengths.
 *
 * @return Vector of pairs, each containing a user track ID and its associated user track length.
 */
std::vector<std::pair<G4int, G4double>> VoxelHit::GetTrkIdUserLengthMappingList() const {
  std::vector<std::pair<G4int, G4double>> trkIdLength;
  auto itL = m_Voxel.m_usrTrksLength.begin();
  for (auto itId = m_Voxel.m_usrTrksId.begin(); itId != m_Voxel.m_usrTrksId.end(); ++itId) trkIdLength.emplace_back(*itId, *itL++);
  return trkIdLength;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns a list mapping track lengths to their corresponding pre-step positions.
 *
 * @return Vector of pairs, each containing a track length and its associated pre-step position.
 *
 * @note The mapping uses track lengths as keys, which may be unintended; typically, track IDs are used for such mappings.
 */
std::vector<std::pair<G4int, G4ThreeVector>> VoxelHit::GetTrkIdPositionMappingList() const {
  std::vector<std::pair<G4int, G4ThreeVector>> trkPosition;
  auto itP = m_Voxel.m_trksPosition.begin();
  for (auto itId = m_Voxel.m_trksLength.begin(); itId != m_Voxel.m_trksLength.end(); ++itId) trkPosition.emplace_back(*itId, *itP++);
  return trkPosition;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns a list mapping track IDs to their particle type codes for this voxel.
 *
 * @return Vector of pairs, each containing a track ID and its corresponding particle type code.
 */
std::vector<std::pair<G4int, G4int>> VoxelHit::GetTrackIdTypeMappingList() const {
  std::vector<std::pair<G4int, G4int>> trkTypeId;
  auto itType = m_Voxel.m_trksTypeId.begin();
  for (auto itId = m_Voxel.m_trksId.begin(); itId != m_Voxel.m_trksId.end(); ++itId) trkTypeId.emplace_back(*itId, *itType++);
  return trkTypeId;
}

////////////////////////////////////////////////////////////////////////////////
/// Returns a list of pairs containing the track ID and the physical process type that occurred in that track.
/**
 * @brief Returns a list mapping track IDs to their corresponding process type codes for this voxel.
 *
 * @return Vector of pairs, each containing a track ID and its associated process type code.
 */
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
/**
 * @brief Retrieves a list mapping track IDs to their secondary electron origin type codes.
 *
 * @return Vector of pairs, each containing a track ID and its corresponding electron origin classification code.
 */
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
/**
 * @brief Returns the kinetic energy of the primary track in the voxel.
 *
 * @return The kinetic energy of the primary track (track ID 1), or -1 if no primary track is present in this voxel hit.
 */
G4double VoxelHit::GetPrimaryTrkEnergy() const {
  for (auto& iIdE : GetTrkIdEnergyMappingList())
    if (iIdE.first == 1) return iIdE.second;
  return -1;  // No primary in this voxel hit
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the mean energy deposited per step in the voxel.
 *
 * @return The average energy deposited per step, or -1 if no steps have been recorded.
 */
G4double VoxelHit::GetMeanEnergyDeposit() const {
  if (m_Voxel.m_stepsEdep.size() > 0) {
    double sum = std::accumulate(m_Voxel.m_stepsEdep.begin(), m_Voxel.m_stepsEdep.end(), 0.0);
    return sum / m_Voxel.m_stepsEdep.size();
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Prints the energies of incident primary particles for the current event.
 *
 * Outputs the energy of each primary particle stored in the event to the console.
 */
void VoxelHit::PrintEvtInfo() const {
  for (int i = 0; i < m_evtPrimariesIncidentE.size(); ++i) {
    G4cout << "[INFO]::VoxelHit::EvtInfo() incident primary(" << i << ") energy: " << m_evtPrimariesIncidentE.at(i) << " [keV]" << G4endl;
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns a hashed identifier based on the global voxel indices.
 *
 * The hash is computed from the voxel's global x, y, and z indices to provide a unique identifier for the voxel's global position.
 *
 * @return std::size_t Hashed value representing the global voxel indices.
 */
std::size_t VoxelHit::GetGlobalHashedStrId() const { return svc::getHashedStrFromIndexes({m_Voxel.m_global_idx_x, m_Voxel.m_global_idx_y, m_Voxel.m_global_idx_z}); }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns a hashed identifier string based on both global and local voxel indices.
 *
 * The hash uniquely represents the voxel using its global and local indices.
 *
 * @return std::size_t Hashed identifier for the voxel.
 */
std::size_t VoxelHit::GetHashedStrId() const {
  return svc::getHashedStrFromIndexes({m_Voxel.m_global_idx_x, m_Voxel.m_global_idx_y, m_Voxel.m_global_idx_z, m_Voxel.m_idx_x, m_Voxel.m_idx_y, m_Voxel.m_idx_z});
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Checks if two voxel hits have identical global and local voxel indices.
 *
 * @param other The VoxelHit to compare with.
 * @return true if both global and local indices match; false otherwise.
 */
bool VoxelHit::operator==(const VoxelHit& other) const {
  return m_Voxel.m_global_idx_x == other.m_Voxel.m_global_idx_x && m_Voxel.m_global_idx_y == other.m_Voxel.m_global_idx_y &&
         m_Voxel.m_global_idx_z == other.m_Voxel.m_global_idx_z && m_Voxel.m_idx_x == other.m_Voxel.m_idx_x && m_Voxel.m_idx_y == other.m_Voxel.m_idx_y &&
         m_Voxel.m_idx_z == other.m_Voxel.m_idx_z;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Checks if this voxel hit is aligned with another voxel hit.
 *
 * @param other The voxel hit to compare with.
 * @param global_and_local If true, checks both global and local voxel indices for alignment; if false, checks only global indices.
 * @return true if the voxel hits are aligned according to the specified criteria; false otherwise.
 */
bool VoxelHit::IsAligned(const VoxelHit& other, bool global_and_local) const {
  if (global_and_local) {
    return *this == other;
  }
  // only global
  return m_Voxel.m_global_idx_x == other.m_Voxel.m_global_idx_x && m_Voxel.m_global_idx_y == other.m_Voxel.m_global_idx_y && m_Voxel.m_global_idx_z == other.m_Voxel.m_global_idx_z;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Cumulates dose and related data from another VoxelHit if aligned.
 *
 * If the provided VoxelHit is aligned (based on the specified alignment check), adds its dose to this voxel hit. For global-only alignment, the dose is weighted by the ratio of the other voxel's volume to this voxel's volume. For both global and local alignment, doses are added directly. If the voxel hits are not aligned, a warning is printed and no cumulation occurs.
 *
 * @param other The VoxelHit to cumulate data from.
 * @param global_and_local_allignemnt_check If true, requires both global and local indices to match for cumulation; if false, only global indices are checked.
 * @return Reference to this VoxelHit after cumulation.
 */
VoxelHit& VoxelHit::Cumulate(const VoxelHit& other, bool global_and_local_allignemnt_check) {
  // if(!global_and_local_allignemnt_check)
  // Print(); INFO_GEO("+"); other.Print();
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
      WARN_GEO("Trying to cumulate misaligned VoxelHits...");
    Print();
      WARN_GEO("+");
    other.Print();
  }
  return *this;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Adds the dose from another voxel hit to this voxel hit.
 *
 * Increases this voxel's dose by the dose value from the other voxel hit.
 *
 * @return Reference to this voxel hit after accumulation.
 */
VoxelHit& VoxelHit::operator+=(const VoxelHit& other) {
  // TODO: m_Voxel.m_Dose+=other.GetDose()*other.GetVolume() / GetVolume(); Tu też?
  m_Voxel.m_Dose += other.GetDose();
  return *this;
}
