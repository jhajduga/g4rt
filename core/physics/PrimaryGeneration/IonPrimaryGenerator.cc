#include "IonPrimaryGenerator.hh"
#include "Services.hh"
#include "PrimariesAnalysis.hh"
#include "G4IonTable.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4GeneralParticleSourceData.hh"

////////////////////////////////////////////////////////////////////////////////
///
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
///
void IonPrimaryGenerator::GeneratePrimaryVertex(G4Event *evt) {
  G4GeneralParticleSource::GeneratePrimaryVertex(evt);
}