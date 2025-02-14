/**
*
* \author B. Rachwal (brachwal [at] agh.edu.pl)
* \date 10.04.2020
*
*/

#ifndef Dose3D_PATIENTSD_HH
#define Dose3D_PATIENTSD_HH

#include "G4ThreeVector.hh"
#include "VoxelHit.hh"
#include "G4VSensitiveDetector.hh"
#include "globals.hh"
#include "G4Box.hh"
#include "Logable.hh"

class G4HCofThisEvent;
class G4Step;
class G4Box;

class VPatientSD : public G4VSensitiveDetector, public Logable {
    public:
      class ScoringVolume: public Logable {
        private:
          ///
          G4bool IsInsideFarmer30013(const G4ThreeVector& position) const;

          ///

        public:
          ScoringVolume():Logable("GeoAndScoring"){}
          /// 
          G4int m_nVoxelsX = 0;
          G4int m_nVoxelsY = 0;
          G4int m_nVoxelsZ = 0;

          ///
          G4String m_run_collection;

          ///
          std::unique_ptr<G4Box> m_envelopeBoxPtr = nullptr;

          ///
          G4double m_rangeMinX=0, m_rangeMaxX=0;
          G4double m_rangeMinY=0, m_rangeMaxY=0;
          G4double m_rangeMinZ=0, m_rangeMaxZ=0;

          /// Linearized channel (nVoxels) position (defined for X * Y * Z)
          std::vector<G4ThreeVector> m_channelCentrePosition;


          /// The HC ID that is namaged by SDManager. Here it's placed as helper duplicate
          G4int id = -1; // TODO: Do I really need this here?

          /// This is raw ptr since it's being managed by G4HCofThisEvent
          VoxelHitsCollection*  m_voxelHCollectionPtr = nullptr;

          ///
          G4double GetSizeX() const { return 2.*m_envelopeBoxPtr->GetXHalfLength(); }
          G4double GetSizeY() const { return 2.*m_envelopeBoxPtr->GetYHalfLength(); }
          G4double GetSizeZ() const { return 2.*m_envelopeBoxPtr->GetZHalfLength(); }

          ///
          G4double GetVolume() const { return GetSizeX()*GetSizeY()*GetSizeZ(); }
          // G4double GetVoxelVolume() const { return (GetSizeX()/m_nVoxelsX)*(GetSizeY()/m_nVoxelsY)*(GetSizeZ()/m_nVoxelsZ); }
          G4double GetVoxelVolume() const;
          G4ThreeVector GetVoxelCentre(int idX, int idY, int idZ) const;
          G4ThreeVector GetVoxelCentre(int linearizedId) const; 

          /// Linearized channel (nVoxels) hit collection index (defined for X * Y * Z)
          std::vector<G4int> m_channelHCollectionIndex;

          ///
          G4String m_shape = "Box"; // or Farmer30013

          /// axisId = 0,1,2 -> X,Y,Z
          G4int GetVoxelID(G4int axisId, const G4ThreeVector& hitPosition)  const;

          ///
          G4int GetLinearizedVoxelID(const G4ThreeVector& hitPosition) const;
          
          ///
          G4int LinearizeIndex(int idX, int idY, int idZ) const;

          ///
          G4bool IsInside(const G4ThreeVector& position) const;
          G4bool IsOnBorder(const G4ThreeVector& position) const;

          bool operator == (const ScoringVolume& other);

          ///
          bool IsVoxelised() const;
      };

  private:
      ///
      void InitializeChannelsID();
      
      ///
      bool IsHitsCollectionExist(const G4String& hitsCollName) const;

      /// The actual volumes for scoring within given SD in which HC's are defined
      /// - as many as number of HitsCollections added by the user
      std::vector<std::pair<G4String,std::unique_ptr<ScoringVolume>>> m_scoring_volumes;
      
      ///
      void SetScoringVolume(const G4String& hitsCollName, const G4Box& envelopBox, const G4ThreeVector& translation);

      ///
      void SetScoringVolume(G4int scoringSdIdx, const G4Box& envelopBox, const G4ThreeVector& translation);

      ///
      void SetScoringShape(const G4String& hitsCollName, const G4String& shapeName);


      ///
      void AddHitsCollection(const G4String&runCollName, const G4String& hitsCollName);
      
      ///
      void AcknowledgeHitsCollection(const G4String&runCollName,const std::pair<G4String,std::unique_ptr<ScoringVolume>>& scoring_volume);

      /// 
          
  protected:
      /// Centre of SensitiveDetector
      /// NOTE: In global frame!
      G4ThreeVector m_sd_centre;

      ///
      G4int m_id_x = -1;
      G4int m_id_y = -1;
      G4int m_id_z = -1;

      ///
      bool m_tracks_analysis = false;

  public:

    ///
    VPatientSD(const G4String& sdName);

    ///
    VPatientSD(const G4String& sdName, const G4ThreeVector& centre);

    ///
    ~VPatientSD() = default;

    ///
    G4ThreeVector GetSDCentre() const;

    ///
    void Initialize(G4HCofThisEvent*) override;

    ///
    void EndOfEvent(G4HCofThisEvent* HCE) override;

    ///
    void AddScoringVolume(const G4String& runCollName, const G4String& hitsCollName, const G4Box& scoringBox, int scoringNX, int scoringNY, int scoringNZ, const G4ThreeVector& translation=G4ThreeVector());

    ///
    G4int GetScoringVolumeIdx(const G4String& hitsCollName) const;

    /// Return the ID of the HC that is managed by SDManager
    G4int GetScoringHcId(const G4String& hitsCollName) const;

    /// Return the ID of the HC that is managed by SDManager
    G4int GetScoringHcId(const G4int scoringSdIdx) const;

    ///
    G4String GetScoringHcName(G4int scoringSdIdx) const;

    ///
    void SetScoringParameterization(const G4String& hitsCollName, int nX, int nY, int nZ);

    ///
    void SetTracksAnalysis(bool flag) { m_tracks_analysis = flag; }

    ///
    ScoringVolume* GetRunCollectionReferenceScoringVolume(const G4String& runCollName, bool voxelisation_check = false) const;

    protected:

    ///
    // G4double GetSizeX(G4int scoringSdIdx) const {return m_scoring_volumes.at(scoringSdIdx).second->GetSizeX();}
    // G4double GetSizeY(G4int scoringSdIdx) const {return m_scoring_volumes.at(scoringSdIdx).second->GetSizeY();}
    // G4double GetSizeZ(G4int scoringSdIdx) const {return m_scoring_volumes.at(scoringSdIdx).second->GetSizeZ();}

    ///
    ScoringVolume* GetScoringVolumePtr(G4int scoringSdIdx) const;
    ScoringVolume* GetScoringVolumePtr(G4int scoringSdIdx);
    ScoringVolume* GetScoringVolumePtr(const G4String& hitsCollName);
    ///
    std::vector<G4String> GetScoringVolumeNames() const;

    ///
    void ProcessHitsCollection(const G4String& hitsCollectionName, G4Step* aStep);

};

#endif //Dose3D_PATIENTSD_HH
