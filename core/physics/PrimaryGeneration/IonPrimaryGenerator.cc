#include "IonPrimaryGenerator.hh"
#include "Services.hh"
#include "PrimariesAnalysis.hh"
#include "G4IonTable.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4GeneralParticleSourceData.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Adds a new ion particle source with specified properties to the simulation.
 *
 * Retrieves the ion definition using atomic number, mass number, and excitation energy. If the ion is not defined, a fatal exception is thrown. The function creates a new particle source with the given strength, sets its particle definition and charge, and returns the index of the newly added source.
 *
 * @param m_Z Atomic number (Z) of the ion.
 * @param m_A Mass number (A) of the ion.
 * @param m_Q Charge state of the ion (in units of elementary charge).
 * @param m_E Excitation energy of the ion (in MeV).
 * @param m_Strength Relative strength or intensity of the source.
 * @return G4int Index of the newly added particle source.
 */
G4int IonPrimaryGenerator::AddSource(G4int m_Z, G4int m_A, G4int m_Q, G4double m_E, G4double m_Strength) {
  auto ion = G4IonTable::GetIonTable()->GetIon(m_Z, m_A, m_E);
  if(!ion){
    G4ExceptionDescription msg;
    msg << "Ion with Z=" << m_Z << " A=" << m_A << " is not defined";
    G4Exception("IonPrimaryGenerator", "IonPrimaryGenerator", FatalErrorInArgument, msg);
  }
  G4GeneralParticleSource::AddaSource(m_Strength); // keeps the lock / unlock mechanism

  // Note: G4GeneralParticleSource::GPSData is private, however the it's a signleton so...
  auto gpsData = G4GeneralParticleSourceData::Instance(); // this keeps collection of G4SingleParticleSource objects..
   // gpsData->SetCurrentSourceIntensity(m_Strength) // it's already done with the AddaSource() argument
  auto sourceIdx = gpsData->GetCurrentSourceIdx();
  auto source = gpsData->GetCurrentSource();
  source->SetParticleDefinition(ion);
  source->SetParticleCharge(m_Q*eplus);
  // TODO: finalise soruce specification
  // https://gitlab.physik.uni-kiel.de/geant4/geant4/-/blob/master/source/event/src/G4GeneralParticleSourceMessenger.cc
  // source->SetPosDisType(newValues);
  // source->SetPosDisShape(newValues);
  // source->SetCentreCoords(centreCmd1->GetNew3VectorValue(newValues));

  /// TODO:: configure:
  //G4SingleParticleSource::G4SPSPosDistribution
  //G4SingleParticleSource::G4SPSAngDistribution
  //G4SingleParticleSource::G4SPSEneDistribution
  //G4SingleParticleSource::G4SPSRandomGenerator

  return sourceIdx;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Generates the primary vertex for the given event.
 *
 * Delegates primary vertex generation to the base general particle source for the specified Geant4 event.
 *
 * @param evt Pointer to the Geant4 event to which the primary vertex will be added.
 */
void IonPrimaryGenerator::GeneratePrimaryVertex(G4Event *evt) {
  G4GeneralParticleSource::GeneratePrimaryVertex(evt);
}