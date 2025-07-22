//
// Created by brachwal on 29.02.2024.
//

#ifndef NTUPLE_RUN_ANALYSIS_HH
#define NTUPLE_RUN_ANALYSIS_HH

#include "globals.hh"

class G4Run;

class NTupleRunAnalysis {
    private:
        ///
        NTupleRunAnalysis() = default;

        ///
        ~NTupleRunAnalysis() = default;

        /// Delete the copy and move constructors
        NTupleRunAnalysis(const NTupleRunAnalysis &) = delete;
        NTupleRunAnalysis &operator=(const NTupleRunAnalysis &) = delete;
        NTupleRunAnalysis(NTupleRunAnalysis &&) = delete;
        NTupleRunAnalysis &operator=(NTupleRunAnalysis &&) = delete;

    public:
        ///
        static NTupleRunAnalysis* GetInstance();

        ///
        void WriteDoseToTFile(const G4Run* runPtr);
        void WriteFieldMaskToTFile(const G4Run* runPtr);
};

#endif //NTUPLE_RUN_ANALYSIS_HH