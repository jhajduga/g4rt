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
 * @brief Initialize a new voxel hit or update an existing one from a Geant4 step.
 *
 * If the voxel has not been initialized (primary ID < 0) this method:
 * - sets/updates the voxel gravitational center from the step pre-step position,
 * - computes voxel mass from material density and stored volume (if volume > 0),
 * - accumulates total and per-step energy deposition and updates dose (dose computed as Edep / mass in Grays),
 * - records track-related information via FillTrack(),
 * - records the incident (primary) particle type and kinetic energy.
 *
 * If the voxel is already initialized the method delegates to Update(aStep) to apply the step's changes.
 *
 * @param aStep Pointer to the Geant4 step providing position, material, energy deposit and track info.
 *
 * @note Dose is not updated when computed mass is zero; a warning is emitted instead.
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
 * @brief Update this voxel with information from a Geant4 step.
 *
 * Update the voxel's gravitational center using the step's pre-step position, accumulate
 * the step energy deposition into the voxel total, record per-step deposits (only if > 0),
 * and update the voxel dose when mass is set. Also records track-related data for the step.
 *
 * @param aStep The Geant4 step providing position, deposited energy, and track information.
 *
 * @note If the voxel mass is zero, the dose is not updated and a warning is emitted.
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
 * @brief Determine a discrete particle-type code for the track in the provided Geant4 step.
 *
 * Maps the track's particle definition to a small integer code useful for compact storage:
 *  - 1: gamma
 *  - 2: electron
 *  - 3: positron
 *  - 4: neutron
 *  - 5: proton
 *  - 6: alpha
 *  - -1: unknown or not one of the above
 *
 * @param aStep Geant4 step whose track's dynamic particle is queried.
 * @return G4int Integer code representing the particle type (see mapping above).
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
 * @brief Map a Geant4 step's post-step process to a discrete process-type code.
 *
 * Returns an integer code representing the dominant physical process that defined the given step,
 * based on the post-step process name. The codes are stable identifiers used for compact storage and
 * comparisons (not exhaustive of every Geant4 process).
 *
 * @param aStep Pointer to the Geant4 step whose post-step process will be classified.
 * @return G4int Process-type code:
 *   - -1: no process defined for the step
 *   -  0: UserStepMax or Transportation
 *   -  1: phot (photoelectric)
 *   -  2: compt (Compton)
 *   -  3: conv (pair production)
 *   -  4: Rayl (Rayleigh)
 *   -  5: eIoni (electron ionization)
 *   -  6: msc (multiple scattering)
 *   -  7: annihil (annihilation)
 *   -  8: eBrem (bremsstrahlung)
 *   -  9: RadioactiveDecay
 *   - 10: hadElastic
 *   - 11: neutronInelastic
 *   - 12: nCapture
 *   - 13: protonInelastic
 *   - 14: alphaInelastic
 *   - 15: ionIoni (ionization by heavy ions)
 *   - 16: Auger
 *   - 17: phot_fluo (fluorescence)
 *   - 18: pixe
 *   - 19: muIoni
 *   - 20: muBrems
 *   - 21: muPairProd
 *   - 22: Cerenkov
 *   - 23: Scintillation
 *   - 24: SynchrotronRadiation
 *   - 99: unknown / other process
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
 * @brief Determine the origin process code for an electron produced in the given Geant4 step.
 *
 * Examines the step's track and, if the particle is an electron, maps its creator process (or lack thereof)
 * to a small integer code that classifies how the electron was produced.
 *
 * @param aStep Pointer to the Geant4 step whose track/creator will be inspected.
 * @return G4int Integer code for the electron origin:
 *   - -1: Not an electron
 *   - 0: Primary electron (no creator process)
 *   - 1: Photoelectric effect
 *   - 2: Compton scattering
 *   - 3: Pair production (conversion)
 *   - 4: Auger electron
 *   - 5: Fluorescent (atomic de-excitation) electron
 *   - 6: Electron ionization
 *   - 7: Ion-induced ionization
 *   - 8: Electron from radioactive decay
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
 * @brief Record per-track information from the current Geant4 step into the voxel.
 *
 * When track storage is enabled (m_store_tracks == true) this routine inserts the
 * current track ID into the voxel's track set and, if the track is new to this voxel,
 * appends the track's type code, post-step process code, electron-origin code,
 * kinetic energy, momentum polar angle (theta), cumulative track length, and the
 * pre-step position into the corresponding per-track vectors. Regardless of whether
 * the track was newly inserted, the voxel's global event time (m_global_time) is
 * updated from the track.
 *
 * Side effects:
 * - Mutates m_Voxel.m_trksId and the parallel per-track vectors (m_trksTypeId,
 *   m_trksProcessTypeId, m_trksElectronOriginTypeId, m_trksE, m_trksTheta,
 *   m_trksLength, m_trksPosition) when a new track is encountered.
 * - Updates m_global_time from the track's global time.
 *
 * @param aStep Current Geant4 step providing track and step-point information.
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
 * @brief Update the voxel's gravitational center using a running half-average with the given position.
 *
 * Recomputes each coordinate of the stored gravitational center as the average of the current center and the provided position,
 * thereby biasing the center toward more recent updates (not a true arithmetic mean over all samples).
 *
 * @param position Position to incorporate into the gravitational center calculation.
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
 * @brief Retrieve the local voxel index for a given axis.
 *
 * @param axisId Axis identifier: 0 = x, 1 = y, 2 = z.
 * @return G4int Local voxel index for the specified axis; returns -1 if axisId is invalid.
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
 * @brief Retrieve the global voxel index for a given axis.
 *
 * @param axisId Axis identifier where 0 = x, 1 = y, 2 = z.
 * @return Global voxel index for the specified axis, or -1 if axisId is invalid.
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
 * @brief Print voxel hit information to the Geant4 output stream.
 *
 * Outputs the voxel's global and local indices, mass, and volume.
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
 * @brief Provide (track ID, kinetic energy) pairs for tracks stored in this voxel.
 *
 * The returned container preserves the internal storage order: each element is a pair
 * (trackID, kineticEnergy). If no tracks are recorded, the returned vector is empty.
 *
 * @return std::vector<std::pair<G4int, G4double>> Vector of (trackID, kineticEnergy) pairs in storage order.
 */
std::vector<std::pair<G4int, G4double>> VoxelHit::GetTrkIdEnergyMappingList() const {
  std::vector<std::pair<G4int, G4double>> trkIdE;
  auto itE = m_Voxel.m_trksE.begin();
  for (auto itId = m_Voxel.m_trksId.begin(); itId != m_Voxel.m_trksId.end(); ++itId) trkIdE.emplace_back(*itId, *itE++);
  return trkIdE;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Map stored track IDs to their corresponding momentum theta angles in stored order.
 *
 * The vector preserves the internal track order; each element is a pair {trackId, theta}.
 *
 * @return std::vector<std::pair<G4int, G4double>> Vector of (track ID, theta) pairs.
 */
std::vector<std::pair<G4int, G4double>> VoxelHit::GetTrkIdThetaMappingList() const {
  std::vector<std::pair<G4int, G4double>> trkIdTheta;
  auto itTheta = m_Voxel.m_trksTheta.begin();
  for (auto itId = m_Voxel.m_trksId.begin(); itId != m_Voxel.m_trksId.end(); ++itId) trkIdTheta.emplace_back(*itId, *itTheta++);
  return trkIdTheta;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Produce a list pairing stored track IDs with their corresponding recorded track lengths.
 *
 * @return std::vector<std::pair<G4int, G4double>> Vector of (track ID, track length) pairs; empty if no tracks are stored.
 */
std::vector<std::pair<G4int, G4double>> VoxelHit::GetTrkIdLengthMappingList() const {
  std::vector<std::pair<G4int, G4double>> trkIdLength;
  auto itL = m_Voxel.m_trksLength.begin();
  for (auto itId = m_Voxel.m_trksId.begin(); itId != m_Voxel.m_trksId.end(); ++itId) trkIdLength.emplace_back(*itId, *itL++);
  return trkIdLength;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Map stored user track IDs to their corresponding stored user-track lengths.
 *
 * Produces a vector of (userTrackId, userTrackLength) pairs where each pair contains
 * the i-th entry from the stored user track ID list and the i-th entry from the
 * stored user track length list.
 *
 * @return std::vector<std::pair<G4int, G4double>> Vector of (userTrackId, userTrackLength) pairs.
 *
 * @note If the ID and length lists differ in size, the result is truncated to the shorter list.
 */
std::vector<std::pair<G4int, G4double>> VoxelHit::GetTrkIdUserLengthMappingList() const {
  std::vector<std::pair<G4int, G4double>> trkIdLength;
  auto itL = m_Voxel.m_usrTrksLength.begin();
  for (auto itId = m_Voxel.m_usrTrksId.begin(); itId != m_Voxel.m_usrTrksId.end(); ++itId) trkIdLength.emplace_back(*itId, *itL++);
  return trkIdLength;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Map stored track lengths to their corresponding pre-step positions.
 *
 * Returns a list that pairs each stored track length with the pre-step position recorded
 * for that track, preserving the internal stored order.
 *
 * @return std::vector<std::pair<G4int, G4ThreeVector>> Vector of (track length, pre-step position) pairs in stored order.
 *
 * @note Track length is used as the key in the pair and may be non-unique across entries.
 */
std::vector<std::pair<G4int, G4ThreeVector>> VoxelHit::GetTrkIdPositionMappingList() const {
  std::vector<std::pair<G4int, G4ThreeVector>> trkPosition;
  auto itP = m_Voxel.m_trksPosition.begin();
  for (auto itId = m_Voxel.m_trksLength.begin(); itId != m_Voxel.m_trksLength.end(); ++itId) trkPosition.emplace_back(*itId, *itP++);
  return trkPosition;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Map stored track IDs to their particle-type codes in recorded order.
 *
 * The returned vector preserves the recording order by pairing each entry from the
 * track-ID container with the corresponding entry from the particle-type container.
 *
 * @return std::vector<std::pair<G4int, G4int>> Each pair is (track ID, particle type code) in recorded order.
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
 * @brief Build a mapping of tracked particle IDs to their post-step process type codes.
 *
 * The returned vector preserves the internal order of stored tracks: each element is
 * a pair where first is the track ID and second is the process-type code recorded for
 * that track. The method assumes the internal containers that hold track IDs and
 * process-type codes are parallel and aligned; if no tracks are stored, an empty
 * vector is returned.
 *
 * @return std::vector<std::pair<G4int,G4int>> List of (track ID, process type code) pairs.
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
 * @brief Get the kinetic energy of the primary track recorded in this voxel.
 *
 * @return The kinetic energy of the primary track (track ID 1), or -1 if no primary track is recorded in this voxel.
 */
G4double VoxelHit::GetPrimaryTrkEnergy() const {
  for (auto& iIdE : GetTrkIdEnergyMappingList())
    if (iIdE.first == 1) return iIdE.second;
  return -1;  // No primary in this voxel hit
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the mean energy deposited per recorded step in this voxel.
 *
 * Calculates the arithmetic mean of the values stored in m_Voxel.m_stepsEdep.
 *
 * @return The arithmetic mean of per-step energy deposits (simulation energy units), or -1 if no steps recorded.
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
 * @brief Print incident primary particle energies recorded for this event.
 *
 * Iterates the stored per-event primary incident energies and writes each value to G4cout.
 *
 * @note Energies are printed in kiloelectronvolts (keV) and output is sent to Geant4's G4cout stream.
 */
void VoxelHit::PrintEvtInfo() const {
  for (int i = 0; i < m_evtPrimariesIncidentE.size(); ++i) {
    G4cout << "[INFO]::VoxelHit::EvtInfo() incident primary(" << i << ") energy: " << m_evtPrimariesIncidentE.at(i) << " [keV]" << G4endl;
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Computes a hash derived from the voxel's global x, y, and z indices.
 *
 * @return std::size_t Hashed value representing the voxel's global position indices.
 */
std::size_t VoxelHit::GetGlobalHashedStrId() const { return svc::getHashedStrFromIndexes({m_Voxel.m_global_idx_x, m_Voxel.m_global_idx_y, m_Voxel.m_global_idx_z}); }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Compute a hash identifier from the voxel's global and local indices.
 *
 * The hash uniquely identifies the voxel by combining the global indices
 * (global x, y, z) and the local indices (local x, y, z).
 *
 * @return std::size_t Hash value derived from the voxel's global and local indices.
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
 * @brief Cumulate dose from another voxel hit when indices align.
 *
 * If the other VoxelHit is aligned with this one, its dose is added to this voxel's dose.
 * - When only global alignment is required (global_and_local_allignemnt_check == false),
 *   the other dose is scaled by (other.GetVolume() / GetVolume()) before adding.
 * - When both global and local alignment are required (global_and_local_allignemnt_check == true),
 *   doses are added directly via operator+=.
 *
 * No changes are made if the two hits are not aligned.
 *
 * @param other Source VoxelHit whose dose will be cumulated into this hit.
 * @param global_and_local_allignemnt_check If true, require both global and local indices to match;
 *                                           if false, require only global indices (apply volume weighting).
 * @return VoxelHit& Reference to this VoxelHit after cumulation.
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
 * @brief Additively accumulates dose from another VoxelHit.
 *
 * Increments this voxel's stored dose by the dose value from the provided
 * VoxelHit. This operation does not modify the other operand and does not
 * perform any alignment or volume-based scaling — it performs a direct dose
 * addition.
 *
 * @return Reference to this VoxelHit after accumulation.
 */
VoxelHit& VoxelHit::operator+=(const VoxelHit& other) {
  // TODO: m_Voxel.m_Dose+=other.GetDose()*other.GetVolume() / GetVolume(); Tu też?
  m_Voxel.m_Dose += other.GetDose();
  return *this;
}