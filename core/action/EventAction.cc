#include "EventAction.hh"

#include "G4Event.hh"
#include "G4EventManager.hh"
#include "RunAnalysis.hh"
#include "BeamAnalysis.hh"
#include "PrimariesAnalysis.hh"
#include "StepAnalysis.hh"
#include "NTupleEventAnalisys.hh"
#include "Services.hh"
#include "G4SDManager.hh"
#include "G4UImanager.hh"

/////////////////////////////////////////////////////////////////////////////
/**
     * @brief Construct an EventAction and initialize the progress print frequency.
     *
     * Initializes the member `printProgress` by reading the "PrintProgressFrequency" key
     * from the "RunSvc" section of the ConfigSvc.
     */
EventAction::EventAction()
    : printProgress(Service<ConfigSvc>()->GetValue<double>("RunSvc", "PrintProgressFrequency")) {}

/////////////////////////////////////////////////////////////////////////////
/**
 * @brief Record the run's total number of events at the start of an event.
 *
 * Queries the RunSvc for the current control point and stores its total event
 * count into EventAction::totalNoOfEvents. The provided event pointer is not
 * used.
 *
 * @param evt Pointer to the current G4Event (unused).
 */
void EventAction::BeginOfEventAction(const G4Event *) {
  totalNoOfEvents = Service<RunSvc>()->CurrentControlPoint()->GetNEvts();
}

/////////////////////////////////////////////////////////////////////////////
/// default methods to get date and time in C++ are accurate up to seconds
// to be more precise we do a bit of hacking and include also microseconds
// following https://stackoverflow.com/questions/24686846/get-current-time-in-milliseconds-or-hhmmssmmm-format/35157784
/**
 * @brief Return the current local date and time with millisecond precision.
 *
 * Returns a timestamp formatted as "YYYY-MM-DD HH:MM:SS.mmm" (milliseconds = `mmm`), using the local timezone.
 *
 * @return std::string Timestamp string with millisecond precision.
 */
std::string time_in_HH_MM_SS_MMM() {
  using namespace std::chrono;

  // get current time
  auto now = system_clock::now();

  // get number of milliseconds for the current second
  // (remainder after division into seconds)
  auto current_ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

  // convert to std::time_t in order to convert to std::tm (broken time)
  auto timer = system_clock::to_time_t(now);

  // convert to broken time
  std::tm bt = *std::localtime(&timer);

  std::ostringstream oss;

  oss << std::put_time(&bt, "%F %T");  // YY-MM-DD HH:MM:SS
  oss << '.' << std::setfill('0') << std::setw(3) << current_ms.count();

  return oss.str();
}

/////////////////////////////////////////////////////////////////////////////
/// Print progress information according to progress frequency defined by user
/**
 * @brief Perform end-of-event processing: progress reporting and analysis dispatch.
 *
 * Prints a timestamped progress line (millisecond precision) when the current event index hits the configured reporting cadence or when the run finishes. After reporting, conditionally calls EndOfEventAction(evt) on enabled analysis modules according to RunSvc configuration; NTuple analysis is invoked only if any TTree is defined.
 *
 * @param evt Pointer to the current Geant4 event.
 */
void EventAction::EndOfEventAction(const G4Event *evt) {
  auto eventID = evt->GetEventID();
  if ((eventID % std::lround(printProgress * totalNoOfEvents) == 0)) {
    std::ostringstream oss;
    oss << "---> " << 100.0 * printProgress * eventID / std::lround(printProgress * totalNoOfEvents) << " %";
    oss << " ( Event: " << eventID << " / " << totalNoOfEvents << " )  ";
    oss << time_in_HH_MM_SS_MMM();
    oss << G4endl;
    G4cout << oss.str() << std::flush;
  } else if (eventID == totalNoOfEvents - 1) {
    std::ostringstream oss;
    oss << "---> 100% ( Event: " << eventID << " / " << totalNoOfEvents << " )  ";
    oss << time_in_HH_MM_SS_MMM();
    oss << G4endl;
    G4cout << oss.str() << std::flush;
  }
  auto configSvc = Service<ConfigSvc>();

  if (configSvc->GetValue<bool>("RunSvc", "BeamAnalysis"))
    BeamAnalysis::GetInstance()->EndOfEventAction(evt);

  if (configSvc->GetValue<bool>("RunSvc", "PrimariesAnalysis"))
    PrimariesAnalysis::GetInstance()->EndOfEventAction(evt);

  if (configSvc->GetValue<bool>("RunSvc", "StepAnalysis"))
    StepAnalysis::GetInstance()->EndOfEventAction(evt);
  
  if (configSvc->GetValue<bool>("RunSvc", "NTupleAnalysis") && NTupleEventAnalisys::IsAnyTTreeDefined() ) //  
    NTupleEventAnalisys::GetInstance()->EndOfEventAction(evt);

  if (configSvc->GetValue<bool>("RunSvc", "RunAnalysis"))
    RunAnalysis::GetInstance()->EndOfEventAction(evt);
}