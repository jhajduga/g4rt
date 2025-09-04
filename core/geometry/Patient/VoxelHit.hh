//
// Created by brachwal on 07.05.20.
//

#ifndef VOXELHIT_HH
#define VOXELHIT_HH

#include <G4RunManager.hh>
#include <memory>
#include <set>

#include "G4Allocator.hh"
#include "G4Step.hh"
#include "G4THitsCollection.hh"
#include "G4ThreeVector.hh"
#include "G4VHit.hh"
#include "tls.hh"

class VoxelHit final : public G4VHit {
 private:
  ///
  class Voxel {
   public:
    /// Voxel can be numerated internally and externally, thus:
    /// Local indexes of the voxel
    G4int m_idx_x = -1;
    G4int m_idx_y = -1;
    G4int m_idx_z = -1;

    /// Global indexes of the voxel
    G4int m_global_idx_x = -1;
    G4int m_global_idx_y = -1;
    G4int m_global_idx_z = -1;

    /// Energy deposit
    G4double m_Edep{0.};
    G4double m_Dose{0.};

    /// Energy of the track / particle entering the voxel paired with its ID
    /// It's assumed to keep single trk of given ID (first occurrence)
    /// Note: Primary tracks have a track ID = 1, secondary tracks have an ID > 1
    std::set<G4int> m_trksId;
    std::set<G4Track*> m_trksPtr;
    std::vector<G4int> m_trksTypeId;         // 1:G4Gamma, 2:G4Electron, 3:G4Positron, 4:G4Neutron, 5:G4Proton
    std::vector<G4int> m_trksProcessTypeId;  // typr of physical process
    std::vector<G4int> m_trksElectronOriginTypeId;
    std::vector<G4double> m_trksE;
    std::vector<G4double> m_trksTheta;
    std::vector<G4double> m_trksLength;
    std::vector<G4ThreeVector> m_trksPosition;
    /// remember history for all steps
    std::vector<G4double> m_stepsEdep;

    /// User custom track's length measure (e.g. start length accumulation within specific volume)
    std::set<G4int> m_usrTrksId;
    std::vector<G4double> m_usrTrksLength;

    ///
    G4double m_incidentE = 0.;
    G4int m_primaryID = -1;  // ID of the primary particle.

    ///
    G4double m_Mass = 0.;

    ///
    G4double m_Volume = 0.;

    ///
    G4ThreeVector m_Centre{0., 0., 0.};

    ///
    G4ThreeVector m_GlobalCentre{0., 0., 0.};

    ///
    G4ThreeVector m_GravitationalCentre{0., 0., 0.};
  };

  std::vector<G4double> m_evtPrimariesIncidentE;
  G4int m_evtPrimariesIncidentMultiplicity = 0;

  Voxel m_Voxel;

  ///
  G4String m_label;

  ///
  G4double m_global_time = 0;

  ///
  G4double m_field_scaling_factor{1.};

  G4double m_angle_scaling_factor{1.};

  ///
  void FillTrack(G4Step* aStep);

  ///
  void FillEvtInfo();

  bool m_store_tracks = false;
  ///
  VoxelHit& operator+=(const VoxelHit& other);

 public:
  /**
 * @brief Constructs a VoxelHit object with default-initialized members.
 */
  VoxelHit() = default;

  /**
 * @brief Destroys the VoxelHit object and releases associated resources.
 */
  ~VoxelHit() = default;

  /**
 * @brief Enables or disables storage of track information for the voxel hit.
 *
 * @param flag If true, track information will be stored; if false, it will not.
 */
void SetStoreTracks(bool flag) { m_store_tracks = flag; }
  ///
  inline void* operator new(size_t);

  ///
  inline void operator delete(void*);

  ///
  bool operator==(const VoxelHit& other) const;
  bool IsAligned(const VoxelHit& other, bool global_and_local = true) const;

  ///
  VoxelHit& Cumulate(const VoxelHit& other, bool global_and_local_allignemnt_check = true);

  ///
  void Draw() override {};

  ///
  void Print() override;  // shit happens
  void Print() const;

  ///
  void SetId(G4int xId, G4int yId, G4int zId);

  ///
  void SetGlobalId(G4int xId, G4int yId, G4int zId);

  /**
 * @brief Set the voxel's local center position.
 *
 * Sets the voxel's center in the voxel's local coordinate system.
 *
 * @param position Local center coordinates (G4ThreeVector).
 */
  void SetCentre(const G4ThreeVector& position) { m_Voxel.m_Centre = position; }

  /**
 * @brief Set the voxel's centre in the global coordinate system.
 *
 * @param position Global coordinates of the voxel centre.
 */
  void SetGlobalCentre(const G4ThreeVector& position) { m_Voxel.m_GlobalCentre = position; }

  ///
  void SetGravCentre(const G4ThreeVector& position);

  /**
 * @brief Sets the label for this voxel hit.
 *
 * Assigns a descriptive string label to the voxel hit, which can be used for identification or categorization.
 */
  void SetLabel(const G4String& label) { m_label = label; }

  /**
 * @brief Returns the label associated with this voxel hit.
 *
 * @return The label string.
 */
  G4String GetLabel() const { return m_label; }

  /**
 * @brief Store the voxel's geometric volume.
 *
 * Sets the internal volume value for this voxel; the stored value is returned by GetVolume().
 *
 * @param volume Volume to assign to the voxel.
 */
  void SetVolume(G4double volume) { m_Voxel.m_Volume = volume; }

  /**
 * @brief Returns the volume of the voxel.
 *
 * @return The voxel's volume.
 */
  G4double GetVolume() const { return m_Voxel.m_Volume; }

  /**
 * @brief Set the voxel mass.
 *
 * Assigns the given mass value to this voxel's stored mass.
 *
 * @param mass Mass value expressed in the code's unit system (e.g. Geant4 units).
 */
  void SetMass(G4double mass) { m_Voxel.m_Mass = mass; }

  /**
 * @brief Get the voxel's gravitational centre in global coordinates.
 *
 * @return G4ThreeVector Global position of the voxel's gravitational centre.
 */
  G4ThreeVector GetGravCentre() const { return m_Voxel.m_GravitationalCentre; }

  /**
 * @brief Get the voxel's local geometric center.
 *
 * Returns the voxel center expressed in the voxel's local coordinate frame.
 *
 * @return G4ThreeVector The local centre position (returned by value).
 */
  G4ThreeVector GetCentre() const { return m_Voxel.m_Centre; }

  // G4ThreeVector GetCentreInGlobal() const { return m_Voxel.m_Centre + m_Voxel.m_GlobalCentre; } // if d3d detector will be broken cause of fix then we will use this...

  /**
 * @brief Get the voxel center in global coordinates.
 *
 * Returns the voxel's center position expressed in the global coordinate system.
 *
 * @return G4ThreeVector The global centre position.
 */
  G4ThreeVector GetGlobalCentre() const { return m_Voxel.m_GlobalCentre; }

  ///
  G4int GetID(G4int axisId) const;

  ///
  G4int GetGlobalID(G4int axisId) const;

  /**
 * @brief Returns the absorbed dose for the voxel.
 *
 * @return The dose value stored in the voxel, typically in units of energy per mass.
 */
  G4double GetDose() const { return m_Voxel.m_Dose; }
  /**
   * @brief Set the absorbed dose for this voxel and return the updated value.
   *
   * Sets the voxel's absorbed dose to the provided value and returns the stored dose.
   *
   * @param val Dose value to assign.
   * @return G4double The updated absorbed dose stored in the voxel.
   */
  G4double SetDose(G4double val) {
    m_Voxel.m_Dose = val;
    return m_Voxel.m_Dose;
  }

  /**
 * @brief Returns the total energy deposited in the voxel.
 *
 * @return The energy deposited in the voxel.
 */
  inline G4double GetEnergyDeposit() const { return m_Voxel.m_Edep; }

  ///
  G4double GetMeanEnergyDeposit() const;

  /**
   * @brief Sets the primary particle ID and its incident energy for the voxel.
   *
   * @param fId The primary particle's ID.
   * @param fEn The incident energy of the primary particle.
   */
  void SetPrimary(G4int fId, G4double fEn) {
    m_Voxel.m_primaryID = fId;
    m_Voxel.m_incidentE = fEn;
  }

  /**
 * @brief Get the incident energy of the primary particle recorded for this voxel.
 *
 * Returns the primary particle's incident energy value stored in the voxel
 * (member m_Voxel.m_incidentE). This reflects the incident energy associated
 * with the primary that contributed to this voxel hit.
 *
 * @return G4double Incident energy recorded for the primary (as stored in the voxel).
 */
  G4double GetPrimaryEnergy() const { return m_Voxel.m_incidentE; }

  /**
 * @brief Retrieve the primary particle ID recorded for this voxel hit.
 *
 * @return G4int Primary particle ID associated with the hit.
 */
  G4int GetPrimaryId() const { return m_Voxel.m_primaryID; }

  ///
  std::vector<std::pair<G4int, G4double>> GetTrkIdEnergyMappingList() const;
  std::vector<std::pair<G4int, G4double>> GetTrkIdThetaMappingList() const;
  std::vector<std::pair<G4int, G4double>> GetTrkIdLengthMappingList() const;
  std::vector<std::pair<G4int, G4double>> GetTrkIdUserLengthMappingList() const;
  std::vector<std::pair<G4int, G4ThreeVector>> GetTrkIdPositionMappingList() const;
  std::vector<std::pair<G4int, G4int>> GetTrackIdTypeMappingList() const;
  std::vector<std::pair<G4int, G4int>> GetTrkIdProcessTypeMappingList() const;
  std::vector<std::pair<G4int, G4int>> GetTrkIdElectronOriginTypeMappingList() const;

  ///
  G4double GetPrimaryTrkEnergy() const;

  /**
 * @brief Returns the number of unique track types recorded in the voxel.
 *
 * @return G4int Number of unique track IDs stored for this voxel.
 */
  inline G4int GetNTrkType() const { return m_Voxel.m_trksId.size(); }

  ///
  void Fill(G4Step* aStep);

  ///
  void Update(G4Step* aStep);

  ///
  G4int GetTrackIdType(G4Step* aStep) const;

  ///
  G4int GetTrkIdProcessType(G4Step* aStep) const;

  ///
  G4int GetTrkIdElectronOriginType(G4Step* aStep) const;

  /**
 * @brief Get the global time recorded for this voxel hit.
 *
 * @return G4double The stored global time value for the hit.
 */
  G4double GetGlobalTime() const { return m_global_time; }

  ///
  template <typename T>
  void FillTrackUserInfo(G4Step* aStep);

  ///
  template <typename T>
  void FillPrimaryParticleUserInfo();

  ///
  void PrintEvtInfo() const;

  /**
 * @brief Get the incident energies of primaries for the current event.
 *
 * Returns a read-only reference to a vector of incident energies (G4double) for
 * primary particles recorded for the current event. The vector is populated by
 * FillPrimaryParticleUserInfo<T>() on first access for the event and may be
 * empty if no primaries were recorded.
 *
 * @return const std::vector<G4double>& Incident energies of event primaries.
 */
  const std::vector<G4double>& GetEvtPrimariesEnergy() const { return m_evtPrimariesIncidentE; }

  /**
 * @brief Returns the number of primary particles in the current event.
 * @return G4int The number of primary particles for the current event.
 */
  G4int GetEvtNPrimaries() const { return m_evtPrimariesIncidentMultiplicity; }

  /**
   * @brief Increments the event primary particle multiplicity by a specified value.
   *
   * @param n Number to add to the current primary particle multiplicity.
   * @return The updated total number of primary particles for the event.
   */
  G4int AddNEvtPrimary(G4int n) {
    m_evtPrimariesIncidentMultiplicity += n;
    return m_evtPrimariesIncidentMultiplicity;
  }

  /**
 * @brief Set the field scaling factor for this voxel hit.
 *
 * This factor multiplies field-dependent quantities associated with the voxel
 * (default is 1.0). It is used by calculations that apply a per-voxel field
 * scaling; use GetFieldScalingFactor() to retrieve the current value.
 *
 * @param sf Scaling factor to apply (unitless).
 */
  void SetFieldScalingFactor(double sf) { m_field_scaling_factor = sf; }

  /**
 * @brief Returns the field scaling factor applied to the voxel hit.
 *
 * @return The current field scaling factor.
 */
  G4double GetFieldScalingFactor() const { return m_field_scaling_factor; }

  /**
 * @brief Set the multiplicative scaling factor applied to angle-dependent quantities for this voxel hit.
 *
 * This factor is used to scale any angle-related calculations or weights associated with the voxel.
 * Default value is 1.0; provide a positive factor to increase or decrease angular contributions.
 *
 * @param sf Multiplicative angle-scaling factor (typically > 0).
 */
  void SetAngleScalingFactor(double sf) { m_angle_scaling_factor = sf; }

  /**
 * @brief Returns the angle scaling factor applied to the voxel hit.
 *
 * @return The current angle scaling factor.
 */
  G4double GetAngleScalingFactor() const { return m_angle_scaling_factor; }

  ///
  std::size_t GetGlobalHashedStrId() const;
  std::size_t GetHashedStrId() const;
};

////////////////////////////////////////////////////////////////////////////////
///
using VoxelHitsCollection = G4THitsCollection<VoxelHit>;
extern G4ThreadLocal G4Allocator<VoxelHit>* VoxelHitAllocator;

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Allocate storage for a single VoxelHit using the thread-local allocator.
 *
 * Ensures the thread-local VoxelHitAllocator is initialized (lazily) and returns
 * a pointer to memory for one VoxelHit obtained via G4Allocator::MallocSingle().
 *
 * @return void* Pointer to uninitialized memory suitable for placement of a VoxelHit.
 */
inline void* VoxelHit::operator new(size_t) {
  if (!VoxelHitAllocator) VoxelHitAllocator = new G4Allocator<VoxelHit>;
  return (void*)VoxelHitAllocator->MallocSingle();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Deallocates a VoxelHit object using the thread-local allocator.
 *
 * Releases memory for a VoxelHit instance by returning it to the thread-local VoxelHitAllocator.
 */
inline void VoxelHit::operator delete(void* aHit) { VoxelHitAllocator->FreeSingle((VoxelHit*)aHit); }

////////////////////////////////////////////////////////////////////////////////
///
template <typename T>
/**
 * @brief Record a user-provided track length for the current step's track if it is not already stored for this voxel.
 *
 * When track storage is enabled (m_store_tracks == true) this function checks the stepping track's ID and,
 * if that ID has not been seen before for this voxel, attempts to read its user information as type `T`.
 * If the cast succeeds, the track length returned by `T::GetTrackLength()` is appended to the voxel's
 * per-user-track length list (m_Voxel.m_usrTrksLength) and the track ID is recorded so it won't be stored again.
 *
 * @tparam T Type of the track user-information object. Must provide a callable `GetTrackLength()` that returns
 *           the track length (numeric type compatible with storage).
 * @param aStep The Geant4 step containing the track whose user information will be queried.
 */
void VoxelHit::FillTrackUserInfo(G4Step* aStep) {
  if (m_store_tracks) {
    auto aTrack = aStep->GetTrack();
    auto trkID = aTrack->GetTrackID();
    auto ret = m_Voxel.m_usrTrksId.insert(trkID);
    if (ret.second == true) {  // new element inserted
      auto trackInfo = dynamic_cast<T*>(aTrack->GetUserInformation());
      if (trackInfo) {
        auto trkLength = trackInfo->GetTrackLength();
        m_Voxel.m_usrTrksLength.emplace_back(trkLength);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
///
template <typename T>
/**
 * @brief Populate the vector of incident total energies for primaries in the current event.
 *
 * Reads each primary particle in the current G4 event once per event and, if the primary's
 * kinetic energy is positive and it carries user information of type `T`, appends the value
 * returned by `T::GetInitialTotalEnergy()` to the internal `m_evtPrimariesIncidentE` vector.
 * After the first successful population this function becomes a no-op for the remainder of the event.
 *
 * This also updates `m_evtPrimariesIncidentMultiplicity` to the number of collected primaries.
 *
 * @tparam T Type of the user-information object attached to primaries; must implement
 *           `double GetInitialTotalEnergy() const`.
 */
void VoxelHit::FillPrimaryParticleUserInfo() {
  if (m_evtPrimariesIncidentE.empty()) {  // it should be done only once!
    auto evt = G4RunManager::GetRunManager()->GetCurrentEvent();
    auto nvrtx = evt->GetNumberOfPrimaryVertex();
    for (int i = 0; i < nvrtx; ++i) {
      auto vrtx = evt->GetPrimaryVertex(i);
      auto particle = vrtx->GetPrimary(0);
      if (particle->GetKineticEnergy() > 0) {
        auto particleInfo = particle->GetUserInformation();
        if (particleInfo) {
          m_evtPrimariesIncidentE.emplace_back(dynamic_cast<T*>(particleInfo)->GetInitialTotalEnergy());
        }
      }
    }
    m_evtPrimariesIncidentMultiplicity = m_evtPrimariesIncidentE.size();
  }
}

#endif  // VOXELHIT_HH
