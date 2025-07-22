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
    ///
    PatientTrackInfo() = default;

    ///
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

    ///
    G4double GetTrackLength() const { return m_trackLength; }
};

////////////////////////////////////////////////////////////////////////////////
///
extern G4ThreadLocal G4Allocator<PatientTrackInfo>* aPatientTrackInfoAllocator;

////////////////////////////////////////////////////////////////////////////////
///
inline void* PatientTrackInfo::operator new(size_t){
  if(!aPatientTrackInfoAllocator)
    aPatientTrackInfoAllocator = new G4Allocator<PatientTrackInfo>;
  return (void*)aPatientTrackInfoAllocator->MallocSingle();
}

////////////////////////////////////////////////////////////////////////////////
///
inline void PatientTrackInfo::operator delete(void *aTrackInfo){
  aPatientTrackInfoAllocator->FreeSingle((PatientTrackInfo*)aTrackInfo);
}


#endif // TRACK_INFO_HH