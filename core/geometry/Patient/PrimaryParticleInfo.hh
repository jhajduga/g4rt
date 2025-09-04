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
    /**
 * @brief Constructs a PrimaryParticleInfo object with default-initialized values.
 */
    PrimaryParticleInfo() = default;

    /**
 * @brief Destroys the PrimaryParticleInfo object.
 *
 * Ensures proper cleanup of resources when a PrimaryParticleInfo instance is destroyed.
 */
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

    /**
 * @brief Returns the stored initial total energy of the primary particle.
 *
 * @return The initial total energy value.
 */
    G4double GetInitialTotalEnergy() const { return m_initial_total_energy; }
};

////////////////////////////////////////////////////////////////////////////////
///
extern G4ThreadLocal G4Allocator<PrimaryParticleInfo>* aPrimaryParticleInfoAllocator;

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Allocate memory for a single PrimaryParticleInfo using a thread-local allocator.
 *
 * Uses a thread-local G4Allocator<PrimaryParticleInfo> to obtain storage for one
 * PrimaryParticleInfo instance. The allocator is lazily created on first use
 * and MallocSingle() is used to perform the allocation.
 *
 * @param size Ignored; allocation is performed by the allocator for a single object.
 * @return void* Pointer to raw memory suitable for constructing a PrimaryParticleInfo.
 */
inline void* PrimaryParticleInfo::operator new(size_t){
  if(!aPrimaryParticleInfoAllocator)
    aPrimaryParticleInfoAllocator = new G4Allocator<PrimaryParticleInfo>;
  return (void*)aPrimaryParticleInfoAllocator->MallocSingle();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Deallocates a PrimaryParticleInfo allocated with the class allocator.
 *
 * Releases memory for the given PrimaryParticleInfo instance using the
 * thread-local G4Allocator<aPrimaryParticleInfoAllocator>. The pointer must
 * have been obtained from the matching PrimaryParticleInfo::operator new.
 *
 * @param aTrackInfo Pointer to the PrimaryParticleInfo to free.
 */
inline void PrimaryParticleInfo::operator delete(void *aTrackInfo){
  aPrimaryParticleInfoAllocator->FreeSingle((PrimaryParticleInfo*)aTrackInfo);
}
#endif // PRIMARY_PARTICLE_INFO_HH