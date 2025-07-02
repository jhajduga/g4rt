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
  ///
  VoxelHit() = default;

  ///
  ~VoxelHit() = default;

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

  ///
  void SetCentre(const G4ThreeVector& position) { m_Voxel.m_Centre = position; }

  ///
  void SetGlobalCentre(const G4ThreeVector& position) { m_Voxel.m_GlobalCentre = position; }

  ///
  void SetGravCentre(const G4ThreeVector& position);

  ///
  void SetLabel(const G4String& label) { m_label = label; }

  ///
  G4String GetLabel() const { return m_label; }

  ///
  void SetVolume(G4double volume) { m_Voxel.m_Volume = volume; }

  ///
  G4double GetVolume() const { return m_Voxel.m_Volume; }

  ///
  void SetMass(G4double mass) { m_Voxel.m_Mass = mass; }

  ///
  G4ThreeVector GetGravCentre() const { return m_Voxel.m_GravitationalCentre; }

  ///
  G4ThreeVector GetCentre() const { return m_Voxel.m_Centre; }

  // G4ThreeVector GetCentreInGlobal() const { return m_Voxel.m_Centre + m_Voxel.m_GlobalCentre; } // if d3d detector will be broken cause of fix then we will use this...

  ///
  G4ThreeVector GetGlobalCentre() const { return m_Voxel.m_GlobalCentre; }

  ///
  G4int GetID(G4int axisId) const;

  ///
  G4int GetGlobalID(G4int axisId) const;

  ///
  G4double GetDose() const { return m_Voxel.m_Dose; }
  G4double SetDose(G4double val) {
    m_Voxel.m_Dose = val;
    return m_Voxel.m_Dose;
  }

  ///
  inline G4double GetEnergyDeposit() const { return m_Voxel.m_Edep; }

  ///
  G4double GetMeanEnergyDeposit() const;

  ///
  void SetPrimary(G4int fId, G4double fEn) {
    m_Voxel.m_primaryID = fId;
    m_Voxel.m_incidentE = fEn;
  }

  ///
  G4double GetPrimaryEnergy() const { return m_Voxel.m_incidentE; }

  ///
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

  ///
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

  ///
  G4double GetGlobalTime() const { return m_global_time; }

  ///
  template <typename T>
  void FillTrackUserInfo(G4Step* aStep);

  ///
  template <typename T>
  void FillPrimaryParticleUserInfo();

  ///
  void PrintEvtInfo() const;

  ///
  const std::vector<G4double>& GetEvtPrimariesEnergy() const { return m_evtPrimariesIncidentE; }

  ///
  G4int GetEvtNPrimaries() const { return m_evtPrimariesIncidentMultiplicity; }

  //
  G4int AddNEvtPrimary(G4int n) {
    m_evtPrimariesIncidentMultiplicity += n;
    return m_evtPrimariesIncidentMultiplicity;
  }

  ///
  void SetFieldScalingFactor(double sf) { m_field_scaling_factor = sf; }

  ///
  G4double GetFieldScalingFactor() const { return m_field_scaling_factor; }

  ///
  void SetAngleScalingFactor(double sf) { m_angle_scaling_factor = sf; }

  ///
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
///
inline void* VoxelHit::operator new(size_t) {
  if (!VoxelHitAllocator) VoxelHitAllocator = new G4Allocator<VoxelHit>;
  return (void*)VoxelHitAllocator->MallocSingle();
}

////////////////////////////////////////////////////////////////////////////////
///
inline void VoxelHit::operator delete(void* aHit) { VoxelHitAllocator->FreeSingle((VoxelHit*)aHit); }

////////////////////////////////////////////////////////////////////////////////
///
template <typename T>
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
