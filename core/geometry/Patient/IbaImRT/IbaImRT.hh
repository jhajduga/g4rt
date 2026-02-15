/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \author J.Hajduga (jhajduga@agh.edu.pl)
* \date 29.01.2025
*
*/

#ifndef D3DF_IbaImRT
#define D3DF_IbaImRT

#include "IPhysicalVolume.hh"
#include "G4ThreeVector.hh"

class IbaImRT: public IPhysicalVolume {
    private:
        ///
        IbaImRT();

        /**
 * @brief Defaulted private destructor.
 *
 * Defaulted and private to prevent external destruction of the singleton instance;
 * lifetime and cleanup of the single instance are managed by the class internals.
 */
        ~IbaImRT() = default;

        /**********************************************************************************
 * @brief Deleted copy constructor to prevent copying of the IbaImRT singleton.
 *********************************************************************************/
        IbaImRT(const IbaImRT &) = delete;
        /**
 * @brief Deleted copy assignment operator to prevent copying of the singleton instance.
 */
IbaImRT &operator=(const IbaImRT &) = delete;
        /**
 * @brief Move constructor is deleted to prevent moving of the singleton instance.
 */
IbaImRT(IbaImRT &&) = delete;
        /**
 * @brief Move assignment operator is deleted to enforce singleton behavior.
 *
 * Prevents moving assignment of the IbaImRT instance.
 */
IbaImRT &operator=(IbaImRT &&) = delete;

    public:
        /// 
        static IbaImRT *GetInstance();

        ///
        void Construct(G4VPhysicalVolume *parentPV) override;

        /**
 * @brief No-op override of IPhysicalVolume::Destroy.
 *
 * Intentionally does nothing — destruction is managed by the singleton lifecycle.
 */
        void Destroy() override {}

        /**
 * @brief Reports that the IbaImRT volume is up to date.
 *
 * @return `true` indicating the volume is up to date.
 */
        G4bool Update() override { return true; }

        /**
 * @brief Reset the IbaImRT volume to its default state.
 *
 * @note This implementation performs no operation; lifecycle and state are
 * managed externally by the singleton instance.
 */
        void Reset() override {}

        ///
        void WriteInfo() override {};

        /// Static member variable for translation to Iba Origin
        static G4ThreeVector IbaToLocalTranslation;

};


#endif //D3DF_IbaImRT