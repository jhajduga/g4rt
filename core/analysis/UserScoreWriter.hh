/**
*
* \author L.Grzanka
* \date 14.11.2017
*
*/

#ifndef UserScoreWriter_h
#define UserScoreWriter_h 1

#include "G4VScoreWriter.hh"
#include "globals.hh"

///\class UserScoreWriter
///\note Based on https://github.com/Geant4/geant4/tree/geant4-10.4-release/examples/extended/runAndEvent/RE03
class UserScoreWriter : public G4VScoreWriter {
  public:
  UserScoreWriter();

  virtual ~UserScoreWriter();

  public:
  // store a quantity into a file
  void DumpQuantityToFile(const G4String &psName, const G4String &fileName, const G4String &option);
};

#endif
