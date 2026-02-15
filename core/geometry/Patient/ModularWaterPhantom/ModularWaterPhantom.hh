#ifndef D3DF_MODULARWATERPHANTOM
#define D3DF_MODULARWATERPHANTOM

#include "IPhysicalVolume.hh"
#include "G4ThreeVector.hh"

class ModularWaterPhantom: public IPhysicalVolume {
    private:
        ///
        ModularWaterPhantom();

        /**
 * @brief Private defaulted destructor.
 *
 * Uses the compiler-generated destructor for normal cleanup and prevents external deletion of the singleton instance.
 */
        ~ModularWaterPhantom() = default;

        /**
 * @brief Deleted copy constructor — copying is disallowed.
 *
 * Prevents creating a copy of the singleton instance to enforce single-instance semantics.
 */
        ModularWaterPhantom(const ModularWaterPhantom &) = delete;
        /**
 * @brief Deleted copy assignment operator to enforce singleton pattern.
 *
 * Prevents copying of ModularWaterPhantom instances.
 */
ModularWaterPhantom &operator=(const ModularWaterPhantom &) = delete;
        /**
 * @brief Move constructor is deleted to enforce the singleton pattern.
 */
ModularWaterPhantom(ModularWaterPhantom &&) = delete;
        /**
 * @brief Deleted move assignment operator to prevent moving of ModularWaterPhantom instances.
 */
ModularWaterPhantom &operator=(ModularWaterPhantom &&) = delete;

    public:
        /// 
        static ModularWaterPhantom *GetInstance();

        ///
        void Construct(G4VPhysicalVolume *parentPV) override;

        /**
 * @brief Destroys the modular water phantom.
 *
 * This method is required by the interface but has no effect for this class.
 */
        void Destroy() override {}

        /**
 * @brief Indicates that the modular water phantom is always up to date.
 *
 * @return G4bool Always returns true.
 */
        G4bool Update() override { return true; }

        /**
 * @brief Resets the modular water phantom state.
 *
 * This implementation performs no action.
 */
        void Reset() override {}

        ///
        void WriteInfo() override {};

        /**
 * @brief Set the rotation matrix used to orient the phantom.
 *
 * Stores the provided pointer without copying and without taking ownership;
 * the caller remains responsible for the pointer's lifetime. Passing nullptr clears the rotation.
 *
 * @param rotation Pointer to a G4RotationMatrix to use as the phantom's rotation, or nullptr to unset.
 */
        void SetRotation(G4RotationMatrix* rotation) { m_rotation = rotation; }

        /**
 * @brief Get the current rotation matrix associated with the modular water phantom.
 *
 * @return Pointer to the internal G4RotationMatrix, or `nullptr` if no rotation is set.
 *         Ownership is not transferred; the caller must not delete the returned pointer.
 */
        G4RotationMatrix* GetRotation() const { return m_rotation; }

        ///
        G4RotationMatrix* m_rotation;

};


#endif //D3DF_MODULARWATERPHANTOM