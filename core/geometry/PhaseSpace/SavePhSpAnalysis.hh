//
// Created by brachwal on 06.05.2020.
//

#ifndef SAVEPHSPANALYSIS_HH
#define SAVEPHSPANALYSIS_HH

#include "globals.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"

class G4Event;
class G4Run;
class G4Step;

class SavePhSpAnalysis {

  private:
  ///
  SavePhSpAnalysis() = default;

  ///
  ~SavePhSpAnalysis() = default;

  /// Delete the copy and move constructors
  SavePhSpAnalysis(const SavePhSpAnalysis &) = delete;

  SavePhSpAnalysis &operator=(const SavePhSpAnalysis &) = delete;

  SavePhSpAnalysis(SavePhSpAnalysis &&) = delete;

  SavePhSpAnalysis &operator=(SavePhSpAnalysis &&) = delete;

  ///
  G4int m_ntupleId = 0;

  public:
  ///
  static SavePhSpAnalysis* GetInstance();

  ///
  void FillPhSp(G4Step* step);

  ///
  void BeginOfRun(const G4Run* runPtr, G4bool isMaster);

};
#endif //SAVEPHSPANALYSIS_HH
