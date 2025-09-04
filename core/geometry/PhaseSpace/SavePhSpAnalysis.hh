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
  /**
 * @brief Default constructor for the SavePhSpAnalysis singleton.
 *
 * This constructor is private to enforce the singleton pattern and prevent direct instantiation.
 */
  SavePhSpAnalysis() = default;

  /**
 * @brief Destroys the SavePhSpAnalysis singleton instance.
 */
  ~SavePhSpAnalysis() = default;

  /**
 * @brief Deleted copy constructor to prevent copying of the singleton instance.
 */
  SavePhSpAnalysis(const SavePhSpAnalysis &) = delete;

  /**
 * @brief Deleted copy-assignment operator to enforce singleton semantics.
 *
 * Prevents assigning one SavePhSpAnalysis instance to another.
 */
SavePhSpAnalysis &operator=(const SavePhSpAnalysis &) = delete;

  /**
 * @brief Move constructor is deleted to enforce the singleton pattern.
 */
SavePhSpAnalysis(SavePhSpAnalysis &&) = delete;

  /**
 * @brief Deleted move assignment operator to enforce singleton behavior.
 *
 * Prevents moving assignment of SavePhSpAnalysis instances.
 */
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
