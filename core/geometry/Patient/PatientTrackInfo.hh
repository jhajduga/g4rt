//
// Created by brachwal on 27.05.21.
//

#ifndef TRACK_INFO_HH
#define TRACK_INFO_HH

#include "globals.hh"
#include "G4Allocator.hh"
#include "G4VUserTrackInformation.hh"

class G4Step;

class PatientTrackInfo : public G4VUserTrackInformation {

  private:
    // trackingStatus = 0 : track which has not reached to patient volume
    //                = 1 : track which has reached to patient volume
    G4int                 m_trackingStatus      = 0;

    // track length within the patient volume
    G4double              m_trackLength         = 0.;

  public:
    /**
 * @brief Constructs a PatientTrackInfo object with default values.
 *
 * Initializes tracking status and track length for a new particle track within the patient volume.
 */
    PatientTrackInfo() = default;

    /**
 * @brief Destroys the PatientTrackInfo object.
 *
 * Ensures proper cleanup of resources when a PatientTrackInfo instance is destroyed.
 */
    virtual ~PatientTrackInfo() = default;
    
    ///
    inline void *operator new(size_t);

    ///
    inline void operator delete(void *aTrackInfo);

    ///
    PatientTrackInfo& operator =(const PatientTrackInfo& right);
    
    ///
    virtual void Print() const;

    ///
    void FillInfo(G4Step* aStep);

    /**
 * @brief Returns the track length within the patient volume.
 *
 * @return G4double The length of the track inside the patient volume.
 */
    G4double GetTrackLength() const { return m_trackLength; }
};

////////////////////////////////////////////////////////////////////////////////
///
extern G4ThreadLocal G4Allocator<PatientTrackInfo>* aPatientTrackInfoAllocator;

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Allocates memory for a PatientTrackInfo object using a thread-local allocator.
 *
 * Ensures the thread-local G4Allocator is initialized before allocating memory for a single PatientTrackInfo instance.
 * @return Pointer to the allocated memory for a PatientTrackInfo object.
 */
inline void* PatientTrackInfo::operator new(size_t){
  if(!aPatientTrackInfoAllocator)
    aPatientTrackInfoAllocator = new G4Allocator<PatientTrackInfo>;
  return (void*)aPatientTrackInfoAllocator->MallocSingle();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Deallocates memory for a PatientTrackInfo object using the thread-local allocator.
 *
 * Frees the memory previously allocated for a PatientTrackInfo instance via the custom allocator.
 */
inline void PatientTrackInfo::operator delete(void *aTrackInfo){
  aPatientTrackInfoAllocator->FreeSingle((PatientTrackInfo*)aTrackInfo);
}


#endif // TRACK_INFO_HH