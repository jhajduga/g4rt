/*
 * Copyright (C) 2009 International Atomic Energy Agency
 * -----------------------------------------------------------------------------
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *-----------------------------------------------------------------------------
 *
 *   AUTHORS:
 *
 *   Miguel Antonio Cortes Giraldo
 *   e-mail: miancortes -at- us.es
 *   University of Seville, Spain
 *   Dep. Fisica Atomica, Molecular y Nuclear
 *   Ap. 1065, E-41080 Seville, SPAIN
 *   Phone: +34-954550928; Fax: +34-954554445
 *
 *   Jose Manuel Quesada Molina, PhD
 *   e-mail: quesada -at- us.es
 *   University of Seville, Spain
 *   Dep. Fisica Atomica, Molecular y Nuclear
 *   Ap. 1065, E-41080 Seville, SPAIN
 *   Phone: +34-954559508; Fax: +34-954554445
 *
 *   Roberto Capote Noy, PhD
 *   e-mail: R.CapoteNoy -at- iaea.org (rcapotenoy -at- yahoo.com)
 *   International Atomic Energy Agency
 *   Nuclear Data Section, P.O.Box 100
 *   Wagramerstrasse 5, Vienna A-1400, AUSTRIA
 *   Phone: +431-260021713; Fax: +431-26007
 *
 **********************************************************************************
 * For documentation
 * see http://www-nds.iaea.org/phsp
 *
 * - 07/12/2009: public version 1.0
 *
 **********************************************************************************
 * NOTE by B.Rachwal & J.Hajduga:
 * Currenttly this reader is refactored to being run in single thread, or within the
 * lock meachanism, e.g. 
 * G4Mutex PrimGenMutex = G4MUTEX_INITIALIZER;
 * G4AutoLock lock(&PrimGenMutex);
 * ...
 * 
 **********************************************************************************/
#ifndef G4IAEAphspReader_h
#define G4IAEAphspReader_h 1

#include "G4VPrimaryGenerator.hh"

#include "iaea_phsp.h"

#include <vector>

#include "G4ThreeVector.hh"
#include "globals.hh"

class G4Event;
class G4PrimaryVertex;

class G4IAEAphspReader : public G4VPrimaryGenerator {
  // ========== Constructors and destructors ==========

  public:
  // 'filename' must include the path if needed, but NOT the extension
  G4IAEAphspReader(char *filename, G4int sourceId);

  G4IAEAphspReader(G4String filename, G4int sourceId=0);

  private:
  G4IAEAphspReader() {
    G4Exception("G4IAEAphspReader::G4IAEAphspReader()", "IAEAreader001", FatalErrorInArgument,
                "Cannot use G4IAEAphspReader void constructor");
  }

  public:
  ~G4IAEAphspReader();

  // ========== Class Methods ==========

  public:
  void GeneratePrimaryVertex(G4Event *evt) override;  // Mandatory
  std::vector<G4PrimaryVertex*> ReadThisEventPrimaryVertexVector(G4int evtId);

  private:
  void InitializeSource(char *filename);

  void ReadAndStoreFirstParticle();

  void PrepareThisEvent();

  void ReadThisEvent();

  std::vector<G4PrimaryVertex*> FillThisEventPrimaryVertexVector(G4int evtId); 

  void PerformRotations(G4ThreeVector &mom);

  void PerformGlobalRotations(G4ThreeVector &mom);

  void PerformHeadRotations(G4ThreeVector &mom);

  void RestartSourceFile();

  // ========== Set/Get inline methods ==========

  public:
  // inline void SetTotalParallelRuns(G4int nParallelRuns) { theTotalParallelRuns = nParallelRuns; }

  // inline void SetParallelRun(G4int parallelRun) {
  //   if (parallelRun > theTotalParallelRuns && parallelRun < 1)
  //     G4Exception("G4IAEAReader::SetParallelRun()", "IAEAreader002", FatalErrorInArgument,
  //                 "Error in G4IAEAphspReader::SetParallelRun()");
  //   theParallelRun = parallelRun;
  // }

  inline void SetTimesRecycled(G4int ntimes) { theTimesRecycled = ntimes; }

  inline void SetGlobalPhspTranslation(const G4ThreeVector &pos) { theGlobalPhspTranslation = pos; }

  inline void SetRotationOrder(G4int ord) { theRotationOrder = ord; }

  inline void SetRotationX(G4double alpha) { theAlpha = alpha; }

  inline void SetRotationY(G4double beta) { theBeta = beta; }

  inline void SetRotationZ(G4double gamma) { theGamma = gamma; }

  inline void SetIsocenterPosition(const G4ThreeVector &pos) { theIsocenterPosition = pos; }

  void SetCollimatorRotationAxis(const G4ThreeVector &axis);

  void SetGantryRotationAxis(const G4ThreeVector &axis);

  inline void SetCollimatorAngle(G4double ang) { theCollimatorAngle = ang; }

  inline void SetGantryAngle(G4double ang) { theGantryAngle = ang; }

  inline G4String GetFileName() const { return theFileName; }

  inline G4int GetSourceReadId() const { return theSourceReadId; }

  inline G4long GetOriginalHistories() const { return theOriginalHistories; }

  inline G4long GetUsedOriginalParticles() const { return theUsedOriginalParticles; }

  // inline G4long GetTotalParticles() const { return theTotalParticles; }

  inline G4int GetNumberOfExtraFloats() const { return theNumberOfExtraFloats; }

  inline G4int GetNumberOfExtraInts() const { return theNumberOfExtraInts; }

  inline const std::vector<G4int>& GetExtraFloatsTypes() const { return theExtraFloatsTypes; }

  inline const std::vector<G4int>& GetExtraIntsTypes() const { return theExtraIntsTypes; }

  G4long GetTotalParticlesOfType(G4String type) const;

  G4double GetConstantVariable(const G4int index) const;

  inline const std::vector<G4int>& GetParticleTypeVector() const { return theParticleTypeVector; }

  inline const std::vector<G4double>& GetEnergyVector() const { return theEnergyVector; }

  inline const std::vector<G4ThreeVector>& GetPositionVector() const { return thePositionVector; }

  inline const std::vector<G4ThreeVector>& GetMomentumVector() const { return theMomentumVector; }

  inline const std::vector<G4double>& GetWeightVector() const { return theWeightVector; }

  inline const std::vector<std::vector<G4double> >& GetExtraFloatsVector() const { return theExtraFloatsVector; }

  inline const std::vector<std::vector<G4long> >& GetExtraIntsVector() const { return theExtraIntsVector; }

  // inline G4int GetTotalParallelRuns() const { return theTotalParallelRuns; }

  // inline G4int GetParallelRun() const { return theParallelRun; } 

  inline G4int GetTimesRecycled() const { return theTimesRecycled; }

  inline G4ThreeVector GetGlobalPhspTranslation() const { return theGlobalPhspTranslation; }

  inline G4int GetRotationOrder() const { return theRotationOrder; }

  inline G4double GetRotationX() const { return theAlpha; }

  inline G4double GetRotationY() const { return theBeta; }

  inline G4double GetRotationZ() const { return theGamma; }

  inline G4ThreeVector GetIsocenterPosition() const { return theIsocenterPosition; }

  inline G4double GetCollimatorAngle() const { return theCollimatorAngle; }

  inline G4double GetGantryAngle() const { return theGantryAngle; }

  inline G4ThreeVector GetCollimatorRotationAxis() const { return theCollimatorRotAxis; }

  inline G4ThreeVector GetGantryRotationAxis() const { return theGantryRotAxis; }

  // ========== Data members ==========

  private:
  // ----------------------
  // FILE GLOBAL PROPERTIES
  // ----------------------

  G4String theFileName;
  // Must include the path, but NOT the IAEA extension

  // TODO: create setter. For multiple file & pararel readout?  
  static const G4int theSourceReadId = 0; 
  // The Id the file source has for the IAEA routines

  static const G4int theAccessRead = 1;
  // A value needed to open the file in the IAEA codes

  G4long theOriginalHistories = -1;
  // Number of original histories which generated the phase space file

  G4long theTotalParticles = -1;
  // Number of particles stored in the phase space file

  G4int theNumberOfExtraFloats, theNumberOfExtraInts;
  // Number of extra variables stored for each particle

  std::vector<G4int> theExtraFloatsTypes;// = new std::vector<G4int>;
  std::vector<G4int> theExtraIntsTypes;// = new std::vector<G4int>;
  // Identification to classify the different extra variables

  // ---------------------
  // PARTICLE PROPERTIES
  // ---------------------

  std::vector<G4int> theParticleTypeVector;
  std::vector<G4double> theEnergyVector;// =  new std::vector<G4double>;
  std::vector<G4ThreeVector> thePositionVector;// = new std::vector<G4ThreeVector>;
  std::vector<G4ThreeVector> theMomentumVector;// = new std::vector<G4ThreeVector>;
  std::vector<G4double> theWeightVector;// = new std::vector<G4double>;
  std::vector<std::vector<G4double> > theExtraFloatsVector;// = new std::vector<std::vector<G4double> >;
  std::vector<std::vector<G4long> > theExtraIntsVector;// = new std::vector<std::vector<G4long> >;

  // -------------------
  // COUNTERS AND FLAGS
  // -------------------

  // G4int theTotalParallelRuns;
  // // For parallel runs, number of fragments in which the PSF is divided into

  // G4int theParallelRun;
  // // Sets the fragment of PSF where the particles must be read from

  G4int theTimesRecycled = 0;
  // Set the number of times that each particle is recycled (not repeated)


  G4long theUsedOriginalParticles = 0;
  // Variable that stores the number of particles readed so far

  G4long theCurrentParticle = 0;
  // Number to store the current particle position in PSF

  G4bool theEndOfFile = false;
  // Flag active when the file has reached the end

  G4bool theLastGenerated = true; // MAC to make the file 'restart' in the first event.
  // Flag active only when the last particle has been simulated

  // ------------------------
  // SPATIAL TRANSFORMATIONS
  // ------------------------

  G4ThreeVector theGlobalPhspTranslation;
  // Global translation performed to particles

  G4int theRotationOrder =  123;
  // Variable to decide first, second and third rotations
  // For example, 132 means rotations using X, Z and Y global axis

  G4double theAlpha = 0;
  G4double theBeta = 0;
  G4double theGamma = 0;
  // Angles of rotations around global axis

  G4ThreeVector theIsocenterPosition;
  // Position of the isocenter if needed

  G4double theCollimatorAngle = 0;
  G4double theGantryAngle = 0;
  G4ThreeVector theCollimatorRotAxis = G4ThreeVector(0.0, 0.0, 1.0);  // Z Axis
  G4ThreeVector theGantryRotAxis = G4ThreeVector(0.0, 1.0, 0.0);      // Y Axis
  // Angles and axis of isocentric rotations in the machine
  // The collimator ALWAYS rotates first.
};

#endif
