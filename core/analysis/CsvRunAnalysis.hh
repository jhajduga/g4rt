//
// Created by brachwal on 13.02.2024.
//

#ifndef CSV_RUN_ANALYSIS_HH
#define CSV_RUN_ANALYSIS_HH

#include "globals.hh"
#include "LogSvc.hh"

class G4Run;

class CsvRunAnalysis {
    private:
        /**
 * @brief Private default constructor.
 *
 * Enforces the singleton pattern by preventing direct instantiation outside the class.
 */
        CsvRunAnalysis() = default;

        /**
 * @brief Private destructor for CsvRunAnalysis.
 *
 * Defaulted destructor kept private to enforce controlled singleton lifetime;
 * destruction is managed by the class internals and performs no special cleanup.
 */
        ~CsvRunAnalysis() = default;

        /**
 * @brief Deleted copy constructor to prevent copying of the singleton instance.
 */
        CsvRunAnalysis(const CsvRunAnalysis &) = delete;
        /**
 * @brief Deleted copy assignment operator — copying is disallowed to enforce singleton semantics.
 */
CsvRunAnalysis &operator=(const CsvRunAnalysis &) = delete;
        /**
 * @brief Move constructor is deleted to prevent moving of the singleton instance.
 */
CsvRunAnalysis(CsvRunAnalysis &&) = delete;
        /**
 * @brief Deleted move assignment operator to prevent moving instances of CsvRunAnalysis.
 */
CsvRunAnalysis &operator=(CsvRunAnalysis &&) = delete;

    public:
        ///
        static CsvRunAnalysis* GetInstance();

        ///
        void WriteDoseToCsv(const G4Run* runPtr);
        void WriteFieldMaskToCsv(const G4Run* runPtr);
};

#endif //CSV_RUN_ANALYSIS_HH