//
// Created by brachwal on 30.04.2020.
//

#include "VPatient.hh"
#include "VPatientSD.hh"
#include "WorldConstruction.hh"
#include "G4SDManager.hh"
#include "Services.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Assigns a sensitive detector to a logical volume.
 *
 * Associates the provided sensitive detector with the specified logical volume name, registers it with the Geant4 sensitive detector manager, and sets it in the world geometry.
 */
void VPatient::SetSensitiveDetector(const G4String& logicalVName, VPatientSD* sensitiveDetectorPtr){
  if(m_patientSD.Get()==0) // NOTE: this should be checked already from the caller!
    m_patientSD.Put(sensitiveDetectorPtr);
  INFO_GEO("Setting Sensitive Detector ({}) for {}",sensitiveDetectorPtr->GetName(),logicalVName);
  G4SDManager::GetSDMpointer()->AddNewDetector(m_patientSD.Get());
  Service<GeoSvc>()->World()->SetSensitiveDetector(logicalVName, m_patientSD.Get());
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the stored volume of the patient.
 *
 * If the volume is unset or invalid (less than -1), logs a fatal error and throws a Geant4 exception.
 * @return G4double The volume value.
 */
G4double VPatient::GetVolume() const {
  if(m_volume<-1){
    G4String msg = "Volume of the "+GetName()+" no set or set with value < 0.";
    FATAL_GEO(msg.data());
    G4Exception("RunSvc", "DefineControlPoints", FatalErrorInArgument, msg);
  }
  return m_volume;
}