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
///
EventAction::EventAction()
    : printProgress(Service<ConfigSvc>()->GetValue<double>("RunSvc", "PrintProgressFrequency")) {}

/////////////////////////////////////////////////////////////////////////////
///
void EventAction::BeginOfEventAction(const G4Event *) {
  totalNoOfEvents = Service<RunSvc>()->CurrentControlPoint()->GetNEvts();
}

/////////////////////////////////////////////////////////////////////////////
/// default methods to get date and time in C++ are accurate up to seconds
// to be more precise we do a bit of hacking and include also microseconds
// following https://stackoverflow.com/questions/24686846/get-current-time-in-milliseconds-or-hhmmssmmm-format/35157784
/// \return string with current date and time including microseconds
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
/// \param evt
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
