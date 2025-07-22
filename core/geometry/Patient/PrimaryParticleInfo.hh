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
 * @brief Returns the initial total energy of the primary particle.
 *
 * @return G4double The stored initial total energy value.
 */
    G4double GetInitialTotalEnergy() const { return m_initial_total_energy; }
};

////////////////////////////////////////////////////////////////////////////////
///
extern G4ThreadLocal G4Allocator<PrimaryParticleInfo>* aPrimaryParticleInfoAllocator;

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Allocates memory for a PrimaryParticleInfo object using a thread-local allocator.
 *
 * Ensures that memory for PrimaryParticleInfo instances is managed efficiently in a multi-threaded environment by utilizing a thread-local G4Allocator. The allocator is created on first use.
 *
 * @param size The size of the memory block to allocate (ignored).
 * @return Pointer to the allocated memory for a PrimaryParticleInfo object.
 */
inline void* PrimaryParticleInfo::operator new(size_t){
  if(!aPrimaryParticleInfoAllocator)
    aPrimaryParticleInfoAllocator = new G4Allocator<PrimaryParticleInfo>;
  return (void*)aPrimaryParticleInfoAllocator->MallocSingle();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Deallocates memory for a PrimaryParticleInfo object using the custom allocator.
 *
 * Frees the memory previously allocated for a PrimaryParticleInfo instance via the thread-local allocator.
 */
inline void PrimaryParticleInfo::operator delete(void *aTrackInfo){
  aPrimaryParticleInfoAllocator->FreeSingle((PrimaryParticleInfo*)aTrackInfo);
}
#endif // PRIMARY_PARTICLE_INFO_HH