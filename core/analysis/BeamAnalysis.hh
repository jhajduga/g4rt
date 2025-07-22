//
// Created by brachwal on 19.05.2020.
//

#ifndef BEAM_ANALYSIS_HH
#define BEAM_ANALYSIS_HH

#include "globals.hh"
#include <vector>
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4Cache.hh"

class G4Event;
class G4Run;
class G4Step;

class BeamAnalysis {

  private:
  ///
  BeamAnalysis();

  ///
  ~BeamAnalysis() = default;

  /// Delete the copy and move constructors
  BeamAnalysis(const BeamAnalysis &) = delete;

  BeamAnalysis &operator=(const BeamAnalysis &) = delete;

  BeamAnalysis(BeamAnalysis &&) = delete;

  BeamAnalysis &operator=(BeamAnalysis &&) = delete;

  ///
  G4Cache<G4int> m_ntupleId;

  ///
  G4double m_usrPlaneZP1;
  G4double m_usrPlaneZP2;
  G4double m_usrPlaneZP3;
  G4VectorCache<G4int> m_monitoringPlaneId;

  ///
  G4VectorCache<G4double> m_particleE;
  G4VectorCache<G4double> m_particleEx;
  G4VectorCache<G4double> m_particleEy;
  G4VectorCache<G4double> m_particleX;
  G4VectorCache<G4double> m_particleY;
  G4VectorCache<G4double> m_particleZ;
  G4VectorCache<G4double> m_primaryP;
  G4VectorCache<G4double> m_primaryPx;
  G4VectorCache<G4double> m_primaryPy;
  G4VectorCache<G4double> m_primaryPz;
  G4VectorCache<G4double> m_primaryTheta;
  G4VectorCache<G4int> m_particleId;
  G4VectorCache<G4int> m_trkId;
  G4Cache<G4int> m_particleN;

  ///
  std::map<G4int, G4int> m_particleCodesMapping;

  public:
  ///
  static BeamAnalysis* GetInstance();

  ///
  void BeginOfRun(const G4Run* runPtr, G4bool isMaster);

  ///
  void EndOfEventAction(const G4Event *evt);

  ///
  void FillParticlesNTuple();

  ///
  void FillParticles(G4Step *step = nullptr, G4int scoringPlaneId=0);

  ///
  void ClearParticlesEventData();

};
#endif //BEAM_ANALYSIS_HH
