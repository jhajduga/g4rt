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
 * @brief Attach a sensitive detector to a named logical volume.
 *
 * Stores the provided VPatientSD as this patient's sensitive detector (if one is not already set),
 * registers it with the Geant4 sensitive detector manager, and assigns it to the world geometry
 * for the given logical volume name.
 *
 * The call is idempotent with respect to the stored detector: if a detector has already been
 * stored, the existing one is reused and the provided pointer will not replace it.
 *
 * @param logicalVName Name of the target logical volume to receive the sensitive detector.
 * @param sensitiveDetectorPtr Pointer to the sensitive detector to attach (used only if no detector
 *                             is currently stored).
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
 * @brief Returns the stored patient volume.
 *
 * Returns the stored volume value. If the internal volume is less than -1 (invalid/unset),
 * logs a fatal geometry error and throws a G4Exception with severity FatalErrorInArgument.
 *
 * @return G4double The stored patient volume.
 *
 * @throws G4Exception Thrown with severity FatalErrorInArgument when the stored volume is invalid (< -1).
 */
G4double VPatient::GetVolume() const {
  if(m_volume<-1){
    G4String msg = "Volume of the "+GetName()+" no set or set with value < 0.";
    FATAL_GEO(msg.data());
    G4Exception("RunSvc", "DefineControlPoints", FatalErrorInArgument, msg);
  }
  return m_volume;
}