//
// Created by brachwal on 14.05.2020.
//

#ifndef PRIMARIES_ANALYSIS_HH
#define PRIMARIES_ANALYSIS_HH

#include "globals.hh"
#include <vector>
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4Cache.hh"

class G4Event;
class G4Run;
class G4PrimaryVertex;

class PrimariesAnalysis {

  private:
  ///
  PrimariesAnalysis() = default;

  ///
  ~PrimariesAnalysis() = default;

  /// Delete the copy and move constructors
  PrimariesAnalysis(const PrimariesAnalysis &) = delete;

  PrimariesAnalysis &operator=(const PrimariesAnalysis &) = delete;

  PrimariesAnalysis(PrimariesAnalysis &&) = delete;

  PrimariesAnalysis &operator=(PrimariesAnalysis &&) = delete;

  ///
  G4Cache<G4int> m_ntupleId;
  
  ///
  G4VectorCache<G4double> m_gammaE;
  G4Cache<G4int> m_gammaN;
  G4VectorCache<G4double> m_electronE;
  G4Cache<G4int> m_electronN;
  G4VectorCache<G4double> m_positronE;
  G4Cache<G4int> m_positronN;
  G4VectorCache<G4double> m_neutronE;
  G4Cache<G4int> m_neutronN;
  G4VectorCache<G4double> m_protonE;
  G4Cache<G4int> m_protonN;

  ///
  G4VectorCache<G4double> m_primaryE;
  G4VectorCache<G4double> m_primaryEx;
  G4VectorCache<G4double> m_primaryEy;
  G4VectorCache<G4double> m_primaryP;
  G4VectorCache<G4double> m_primaryPx;
  G4VectorCache<G4double> m_primaryPy;
  G4VectorCache<G4double> m_primaryPz;
  G4VectorCache<G4double> m_primaryTheta;
  G4VectorCache<G4double> m_primaryX;
  G4VectorCache<G4double> m_primaryY;
  G4VectorCache<G4double> m_primaryZ;
  G4VectorCache<G4double> m_primaryVertexId;
  G4VectorCache<G4double> m_primaryWeight;
  G4Cache<G4int> m_primaryN; // event multiplicity

  public:
  ///
  static PrimariesAnalysis* GetInstance();

  ///
  void BeginOfRun(const G4Run* runPtr, G4bool isMaster);

  ///
  void EndOfEventAction(const G4Event *evt);

  ///
  void FillPrimariesNTuple();

  ///
  void FillPrimaries(const G4Event *evt);

  ///
  void ClearPrimariesEventData();

};
#endif //PRIMARIES_ANALYSIS_HH
