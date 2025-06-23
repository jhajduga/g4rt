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

        ///
        ~IbaImRT() = default;

        /// Delete the copy and move constructors
        IbaImRT(const IbaImRT &) = delete;
        IbaImRT &operator=(const IbaImRT &) = delete;
        IbaImRT(IbaImRT &&) = delete;
        IbaImRT &operator=(IbaImRT &&) = delete;

    public:
        /// 
        static IbaImRT *GetInstance();

        ///
        void Construct(G4VPhysicalVolume *parentPV) override;

        ///
        void Destroy() override {}

        ///
        G4bool Update() override { return true; }

        ///
        void Reset() override {}

        ///
        void WriteInfo() override {};

        /// Static member variable for translation to Iba Origin
        static G4ThreeVector IbaToLocalTranslation;

};


#endif //D3DF_IbaImRT