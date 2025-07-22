//
// Created by brachwal on 27.06.22.
//

#ifndef PRIMARY_PARTICLE_INFO_HH
#define PRIMARY_PARTICLE_INFO_HH

#include "globals.hh"
#include "G4Allocator.hh"
#include "G4VUserPrimaryParticleInformation.hh"

class G4PrimaryParticle ;

class PrimaryParticleInfo : public G4VUserPrimaryParticleInformation {

  private:
    //
    G4double m_initial_total_energy = 0.;

  public:
    ///
    PrimaryParticleInfo() = default;

    ///
    virtual ~PrimaryParticleInfo() = default;
    
    ///
    inline void *operator new(size_t);

    ///
    inline void operator delete(void *aTrackInfo);

    ///
    PrimaryParticleInfo& operator =(const PrimaryParticleInfo& right);
    
    ///
    virtual void Print() const;

    ///
    void FillInfo(G4PrimaryParticle* pparticle);

    ///
    G4double GetInitialTotalEnergy() const { return m_initial_total_energy; }
};

////////////////////////////////////////////////////////////////////////////////
///
extern G4ThreadLocal G4Allocator<PrimaryParticleInfo>* aPrimaryParticleInfoAllocator;

////////////////////////////////////////////////////////////////////////////////
///
inline void* PrimaryParticleInfo::operator new(size_t){
  if(!aPrimaryParticleInfoAllocator)
    aPrimaryParticleInfoAllocator = new G4Allocator<PrimaryParticleInfo>;
  return (void*)aPrimaryParticleInfoAllocator->MallocSingle();
}

////////////////////////////////////////////////////////////////////////////////
///
inline void PrimaryParticleInfo::operator delete(void *aTrackInfo){
  aPrimaryParticleInfoAllocator->FreeSingle((PrimaryParticleInfo*)aTrackInfo);
}
#endif // PRIMARY_PARTICLE_INFO_HH