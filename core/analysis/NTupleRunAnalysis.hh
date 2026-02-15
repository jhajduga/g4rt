//
// Created by brachwal on 29.02.2024.
//

#ifndef NTUPLE_RUN_ANALYSIS_HH
#define NTUPLE_RUN_ANALYSIS_HH

#include "globals.hh"

class G4Run;

class NTupleRunAnalysis {
    private:
        /**
 * @brief Default constructor for NTupleRunAnalysis.
 *
 * Constructor is private to enforce the singleton pattern and prevent direct instantiation.
 */
        NTupleRunAnalysis() = default;

        /**
 * @brief Destroys the NTupleRunAnalysis singleton instance.
 *
 * This destructor is private to enforce the singleton pattern and prevent external destruction.
 */
        ~NTupleRunAnalysis() = default;

        /**
 * @brief Deleted copy constructor to prevent copying of the singleton instance.
 */
        NTupleRunAnalysis(const NTupleRunAnalysis &) = delete;
        /**
 * @brief Deleted copy assignment operator to enforce singleton pattern.
 *
 * Prevents copying of NTupleRunAnalysis instances.
 */
NTupleRunAnalysis &operator=(const NTupleRunAnalysis &) = delete;
        /**
 * @brief Deleted move constructor to prevent moving of NTupleRunAnalysis instances.
 */
NTupleRunAnalysis(NTupleRunAnalysis &&) = delete;
        /**
 * @brief Deleted move-assignment operator.
 *
 * Deleted to enforce singleton semantics: prevents move-assignment of
 * NTupleRunAnalysis instances so ownership cannot be transferred and
 * multiple instances cannot be created via move operations.
 */
NTupleRunAnalysis &operator=(NTupleRunAnalysis &&) = delete;

    public:
        ///
        static NTupleRunAnalysis* GetInstance();

        ///
        void WriteDoseToTFile(const G4Run* runPtr);
        void WriteFieldMaskToTFile(const G4Run* runPtr);
};

#endif //NTUPLE_RUN_ANALYSIS_HH