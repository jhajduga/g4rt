//
// Created by brachwal on 30.04.2020.
//

#include "VPatient.hh"
#include "VPatientSD.hh"
#include "WorldConstruction.hh"
#include "G4SDManager.hh"
#include "Services.hh"

////////////////////////////////////////////////////////////////////////////////
///
void VPatient::SetSensitiveDetector(const G4String& logicalVName, VPatientSD* sensitiveDetectorPtr){
  if(m_patientSD.Get()==0) // NOTE: this should be checked already from the caller!
    m_patientSD.Put(sensitiveDetectorPtr);
  INFO_GEO("Setting Sensitive Detector ({}) for {}",sensitiveDetectorPtr->GetName(),logicalVName);
  G4SDManager::GetSDMpointer()->AddNewDetector(m_patientSD.Get());
  Service<GeoSvc>()->World()->SetSensitiveDetector(logicalVName, m_patientSD.Get());
}

////////////////////////////////////////////////////////////////////////////////
///
G4double VPatient::GetVolume() const {
  if(m_volume<-1){
    G4String msg = "Volume of the "+GetName()+" no set or set with value < 0.";
    FATAL_GEO(msg.data());
    G4Exception("RunSvc", "DefineControlPoints", FatalErrorInArgument, msg);
  }
  return m_volume;
}