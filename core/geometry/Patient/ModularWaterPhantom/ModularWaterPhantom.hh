#ifndef D3DF_MODULARWATERPHANTOM
#define D3DF_MODULARWATERPHANTOM

#include "IPhysicalVolume.hh"
#include "G4ThreeVector.hh"

class ModularWaterPhantom: public IPhysicalVolume {
    private:
        ///
        ModularWaterPhantom();

        /**
 * @brief Destroys the ModularWaterPhantom instance.
 *
 * Default destructor with no custom cleanup.
 */
        ~ModularWaterPhantom() = default;

        /**
 * @brief Deleted copy constructor to enforce singleton pattern.
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
 * @brief Sets the rotation matrix for the modular water phantom.
 *
 * Updates the internal rotation matrix pointer used to define the phantom's orientation.
 */
        void SetRotation(G4RotationMatrix* rotation) { m_rotation = rotation; }

        /**
 * @brief Returns the current rotation matrix associated with the modular water phantom.
 *
 * @return Pointer to the internal G4RotationMatrix, or nullptr if not set.
 */
        G4RotationMatrix* GetRotation() const { return m_rotation; }

        ///
        G4RotationMatrix* m_rotation;

};


#endif //D3DF_MODULARWATERPHANTOM