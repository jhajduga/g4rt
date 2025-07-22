//
// Created by brachwal on 14.05.2020.
//

#ifndef STEP_ANALYSIS_HH
#define STEP_ANALYSIS_HH

#include "globals.hh"
#include <vector>
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4Cache.hh"

class G4Event;
class G4Step;
class G4Track;
class G4Run;

class StepAnalysis {

  private:
  /**
 * @brief Default constructor for the StepAnalysis singleton.
 *
 * This constructor is private and used internally to enforce the singleton pattern.
 */
  StepAnalysis() = default;

  /**
 * @brief Destroys the StepAnalysis instance.
 *
 * As a singleton, destruction is managed automatically and should not be invoked directly.
 */
  ~StepAnalysis() = default;

  /**
 * @brief Deleted copy constructor to enforce singleton pattern.
 *
 * Prevents copying of StepAnalysis instances.
 */
  StepAnalysis(const StepAnalysis &) = delete;

  /**
 * @brief Deleted copy assignment operator to enforce singleton pattern.
 *
 * Prevents copying of StepAnalysis instances.
 */
StepAnalysis &operator=(const StepAnalysis &) = delete;

  /**
 * @brief Move constructor is deleted to enforce singleton pattern.
 *
 * Prevents moving of StepAnalysis instances to ensure only one instance exists.
 */
StepAnalysis(StepAnalysis &&) = delete;

  /**
 * @brief Move assignment operator is deleted to enforce singleton pattern.
 *
 * Prevents moving of StepAnalysis instances to ensure only one instance exists.
 */
StepAnalysis &operator=(StepAnalysis &&) = delete;

  ///
  G4Cache<G4int> m_hitsNtupleId;
  G4Cache<G4int> m_trkNtupleId;


  public:
  ///
  static StepAnalysis* GetInstance();

  ///
  void FillEvent(G4double evtEnergyDeposit);

  ///
  void FillHit(G4Step* aStep);

  ///
  void FillTrack(G4Track* aTrack);

  /// Shared between threads!
  G4VectorCache<G4double> m_hitsX;
  G4VectorCache<G4double> m_hitsY;
  G4VectorCache<G4double> m_hitsZ;
  G4VectorCache<G4double> m_hitsEDeposit;
  G4VectorCache<G4double> m_hitsProcessDefId; // -> ProcessDefinedStep

  G4VectorCache<G4double> m_trkE;
  G4VectorCache<G4double> m_trkX;
  G4VectorCache<G4double> m_trkY;
  G4VectorCache<G4double> m_trkZ;
  G4VectorCache<G4double> m_trkTheta;
  G4VectorCache<G4int> m_trkId;
  G4VectorCache<G4int> m_trkTypeId;
  G4VectorCache<G4int> m_trkCreatorProcessId;
  G4Cache<G4int> m_nHits;

  ///
  void BeginOfRun(const G4Run* runPtr, G4bool isMaster);

  ///
  void EndOfEventAction(const G4Event *evt);

  ///
  void ClearEventData();

  ///
  void FillStep(G4Step* aStep);

};
#endif //STEP_ANALYSIS_HH
