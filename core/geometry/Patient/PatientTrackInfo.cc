#include "PatientTrackInfo.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include "G4Step.hh"

////////////////////////////////////////////////////////////////////////////////
///
G4ThreadLocal G4Allocator<PatientTrackInfo>* aPatientTrackInfoAllocator = nullptr;

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Print the patient's tracking information.
 *
 * Outputs human-readable tracking details for this PatientTrackInfo instance.
 * Currently implemented as a no-op; present for API completeness and may be
 * overridden or extended to emit tracking diagnostics in the future.
 */
void PatientTrackInfo::Print() const { }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Update this object's tracking state and accumulate step length.
 *
 * If tracking is already active (m_trackingStatus == 1), adds the step length
 * from the provided G4Step to m_trackLength. If tracking is not active, this
 * call activates tracking by setting m_trackingStatus to 1 and does not add
 * length for the current step.
 *
 * @param aStep Pointer to the current Geant4 step from which the step length is taken.
 */
void PatientTrackInfo::FillInfo(G4Step* aStep){
    if(m_trackingStatus==1){
        m_trackLength += aStep->GetStepLength();
    }
    else {
        m_trackingStatus = 1;
    }
}



