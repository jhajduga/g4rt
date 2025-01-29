# Part 1:
## 🔥 **NanoLog – dostosowanie do bardziej wymagających projektów (Geant4, HPC, itp.)**
NanoLog zapisuje logi **w formacie binarnym**, co sprawia, że jest superszybki, ale trzeba pamiętać, żeby na końcu je "wypakować" do czytelnej postaci. **W Geant4 czy innych symulacjach** może to mieć sens, bo nie spowalnia głównej pętli obliczeniowej.

### 📝 **Podstawowa konfiguracja NanoLog (zapis do pliku)**
```cpp
#include "NanoLog.hpp"
#include <iostream>

int main() {
    // Ustawienie pliku logów
    NanoLog::setLogFile("logs.txt");

    nanoLog(NanoLog::LogLevels::Info, "Starting simulation...");
    nanoLog(NanoLog::LogLevels::Warn, "Something might go wrong: {}", 42);
    nanoLog(NanoLog::LogLevels::Error, "Critical error at step: {}", 123);

    return 0;
}
```
**Co tu się dzieje?**
- **NanoLog::setLogFile("logs.txt")** → zapisuje logi do pliku
- **nanoLog(LogLevels::Info, ...)** → standardowe poziomy logowania

---

## 🎯 **1. Obsługa własnych typów (np. klasy z Geant4)**
Może właśnie tu spdlog sprawiał problemy? NanoLog domyślnie nie umie wypisać obiektów użytkownika, ale można to obejść!

🔧 **Przykład dla własnej klasy**:
```cpp
#include "NanoLog.hpp"
#include <string>

// Przykładowa klasa Particle (np. obiekt z Geant4)
class Particle {
public:
    std::string name;
    double energy;

    Particle(std::string n, double e) : name(n), energy(e) {}

    // Funkcja do stringowania obiektu
    std::string toString() const {
        return name + " (Energy: " + std::to_string(energy) + " MeV)";
    }
};

// Własny operator wypisywania do NanoLog
inline std::ostream& operator<<(std::ostream& os, const Particle& p) {
    return os << p.toString();
}

int main() {
    NanoLog::setLogFile("simulation.log");

    Particle p("Proton", 150.0);
    nanoLog(NanoLog::LogLevels::Info, "Simulating particle: {}", p);

    return 0;
}
```
💡 **Jeśli obiekt nie obsługuje std::ostream, NanoLog się zbuntuje!**  
Dlatego potrzebujemy **operator<<** albo `toString()`.

---

## 🛠 **2. Logowanie numerów linii, plików, funkcji**
Warto wiedzieć, **gdzie** wystąpił problem – można dodać te informacje automatycznie!

📌 **Użyj makra `__FILE__`, `__LINE__`, `__FUNCTION__`**
```cpp
#define LOG_INFO(msg, ...) nanoLog(NanoLog::LogLevels::Info, "{}:{} {}() -> " msg, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

int main() {
    NanoLog::setLogFile("detailed.log");

    LOG_INFO("Starting the physics simulation");
    LOG_INFO("Processing event {}", 42);
}
```
📌 **Przykładowy log w pliku:**
```
simulation.cpp:10 main() -> Starting the physics simulation
simulation.cpp:11 main() -> Processing event 42
```
✔ **Zaleta?** W razie awarii wiesz dokładnie, gdzie jej szukać.

---

## 📊 **3. Asynchroniczne logowanie**
Jeśli masz **dużo logów w dużym projekcie**, warto je zapisywać asynchronicznie, żeby **nie blokować głównej pętli symulacji**.

Domyślnie NanoLog **i tak działa asynchronicznie**, ale jeśli chcesz to jeszcze bardziej dopracować:
```cpp
#include "NanoLog.hpp"
#include <thread>

void background_logging() {
    for (int i = 0; i < 1000; ++i) {
        nanoLog(NanoLog::LogLevels::Info, "Background log {}", i);
    }
}

int main() {
    NanoLog::setLogFile("async.log");

    std::thread t(background_logging);
    t.join();

    return 0;
}
```
✔ **Zaleta?** Nie czekasz na zapis – główny kod symulacji działa bez opóźnień.

---

## 🔥 **Podsumowanie – co NanoLog potrafi w bardziej wymagających projektach?**
| 🔧 Funkcja | 📌 Jak to zrobić? |
|-----------|----------------|
| ✅ Logowanie do pliku | `NanoLog::setLogFile("file.log");` |
| ✅ Obsługa własnych klas | Definiujesz `operator<<` lub `toString()` |
| ✅ Automatyczne info o pliku/linie | `__FILE__, __LINE__, __FUNCTION__` |
| ✅ Asynchroniczne logowanie | Domyślnie działa w tle |
| ✅ Ultraszybkość | Zapis binarny, praktycznie zerowy narzut |

**Czy NanoLog to dobry wybór do Geant4?**  
Tak – bo Geant4 często **ma dużą ilość obliczeń i eventów**, więc:
- **Szybkość NanoLoga** jest plusem (nie blokuje symulacji).
- **Binarny zapis logów** może pomóc w analizie wyników.
- **Makra do linii i plików** ułatwią debugowanie.

🔹 **Możesz też kombinować ze spdlogiem (zwłaszcza jeśli potrzebujesz formatowania i kolorów), ale jeśli chodzi o minimalizację narzutu CPU, NanoLog jest lepszym wyborem.**

# Part 2:

### 🎯 **Cel:**  
Chcemy, żeby każdy moduł aplikacji (np. "Simulation", "Physics", "UI") miał **swój własny plik logów**, zamiast wrzucać wszystko do jednego.  

---

### 🔥 **Jak to zrobić w NanoLog?**  

NanoLog domyślnie zapisuje logi do jednego pliku, ale możemy obejść to, tworząc **różne instancje loggera** dla różnych modułów.

📌 **Podstawowa koncepcja:**
1. Każdy moduł ma swój osobny plik logów.
2. Definiujemy wrapper wokół `nanoLog()`, żeby dynamicznie zmieniać plik.
3. Używamy `thread_local`, żeby wątki miały własne logi (jeśli potrzebne).

---

### 📝 **Implementacja – Dynamiczne Logowanie do Osobnych Plików**
```cpp
#include "NanoLog.hpp"
#include <unordered_map>
#include <mutex>

// Globalny rejestr plików logów
std::unordered_map<std::string, std::string> log_files;
std::mutex log_mutex;

// Funkcja do ustawiania pliku dla modułu
void setLogFileForModule(const std::string& module, const std::string& filename) {
    std::lock_guard<std::mutex> lock(log_mutex);
    log_files[module] = filename;
}

// Wrapper do logowania do odpowiedniego pliku
#define MODULE_LOG(module, level, msg, ...) { \
    std::lock_guard<std::mutex> lock(log_mutex); \
    auto it = log_files.find(module); \
    if (it != log_files.end()) { \
        NanoLog::setLogFile(it->second); \
    } \
    nanoLog(level, "[" #module "] " msg, ##__VA_ARGS__); \
}

int main() {
    // Ustawiamy różne pliki dla różnych modułów
    setLogFileForModule("Physics", "physics.log");
    setLogFileForModule("Simulation", "simulation.log");
    setLogFileForModule("UI", "ui.log");

    // Logowanie do odpowiednich plików
    MODULE_LOG("Physics", NanoLog::LogLevels::Info, "Starting physics calculations...");
    MODULE_LOG("Simulation", NanoLog::LogLevels::Warn, "Simulation might be unstable at step {}", 42);
    MODULE_LOG("UI", NanoLog::LogLevels::Error, "UI encountered a fatal error!");

    return 0;
}
```

---

### 📌 **Co tu się dzieje?**
- **`setLogFileForModule()`** – Rejestruje, do jakiego pliku ma logować dany moduł.
- **Makro `MODULE_LOG()`**:
  - Pobiera odpowiedni plik logów dla danego modułu.
  - Ustawia go przed logowaniem.
  - Loguje komunikat z nazwą modułu w `[module]`.
- **`std::mutex`** – Zapewnia bezpieczeństwo wielowątkowe (jeśli logujemy z wielu wątków).

---

### 📊 **Przykładowe pliki logów po uruchomieniu:**

#### **physics.log**
```
[Physics] Starting physics calculations...
```
#### **simulation.log**
```
[Simulation] Simulation might be unstable at step 42
```
#### **ui.log**
```
[UI] UI encountered a fatal error!
```

---

### 🚀 **Zalety tej metody**
✔ **Każdy moduł ma osobny plik logów** – łatwiej analizować błędy.  
✔ **Bezpieczeństwo wielowątkowe** – działa w symulacjach (np. Geant4).  
✔ **Elastyczność** – Można dodać dowolną liczbę modułów i plików logów.  

# Part 3:

### 📌 **Co to jest obracanie logów (log rotation)?**  

Obracanie logów (ang. **log rotation**) to technika **zarządzania plikami logów**, żeby nie rosły bez końca i nie zajmowały całego dysku.  
Działa na kilka sposobów:

1. **Obrót po rozmiarze** – np. jak plik osiągnie **10 MB**, zmieniamy jego nazwę na `log_1.txt` i tworzymy nowy `log.txt`.
2. **Obrót po czasie** – np. co **24 godziny** zaczynamy nowy plik (`log_2025-01-30.txt`).
3. **Kombinacja** – np. co 10 MB lub co 24 godziny.
4. **Limit liczby plików** – np. trzymamy **tylko 5 ostatnich logów**, a starsze usuwamy.

---

### 🔥 **Jak to działa w praktyce?**

#### 📂 **Przykład rotacji logów po rozmiarze**
Załóżmy, że ustawiamy limit **10 MB na plik**:

```
logs/
│── log_1.txt  (10MB, stary log)
│── log_2.txt  (10MB, starszy log)
│── log_3.txt  (10MB, jeszcze starszy)
└── log.txt    (bieżący log)
```

Gdy **log.txt osiągnie 10 MB**, system:
1. Zmienia `log.txt` na `log_1.txt`.
2. Jeśli `log_1.txt` już istnieje, to `log_1.txt` → `log_2.txt`, `log_2.txt` → `log_3.txt`, itd.
3. Tworzy nowy pusty `log.txt`.

Takie rozwiązanie **chroni przed zalewaniem dysku**, bo mamy zawsze określoną ilość logów.

---

### 🛠 **Jak dodać rotację logów do NanoLog?**
NanoLog nie ma wbudowanej rotacji logów, ale możemy to zrobić sami.

📌 **Rozwiązanie:** co jakiś czas sprawdzamy wielkość pliku i zmieniamy go na nowy.

```cpp
#include "NanoLog.hpp"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>

#define LOG_FILE "simulation.log"
#define MAX_LOG_SIZE 10 * 1024 * 1024 // 10 MB

void rotateLogs() {
    namespace fs = std::filesystem;

    if (fs::exists(LOG_FILE) && fs::file_size(LOG_FILE) >= MAX_LOG_SIZE) {
        std::string backupName = "simulation_" + std::to_string(std::time(nullptr)) + ".log";
        fs::rename(LOG_FILE, backupName);
        NanoLog::setLogFile(LOG_FILE); // Ustawienie nowego pliku logów
    }
}

int main() {
    NanoLog::setLogFile(LOG_FILE);

    while (true) {
        nanoLog(NanoLog::LogLevels::Info, "Running simulation step...");
        std::this_thread::sleep_for(std::chrono::seconds(1));

        rotateLogs(); // Sprawdzamy, czy trzeba obrócić logi
    }

    return 0;
}
```

---

### 🕒 **Rotacja co określony czas (np. co dzień)**
Jeśli chcesz **nowy plik logów codziennie**, można zmieniać plik o północy:

```cpp
#include <ctime>
#include <iomanip>
#include <sstream>

std::string getLogFileName() {
    std::time_t now = std::time(nullptr);
    std::tm* timeStruct = std::localtime(&now);

    std::ostringstream oss;
    oss << "logs/log_" << std::put_time(timeStruct, "%Y-%m-%d") << ".log";
    return oss.str();
}

int main() {
    std::string logFile = getLogFileName();
    NanoLog::setLogFile(logFile);

    while (true) {
        nanoLog(NanoLog::LogLevels::Info, "Simulation is running...");
        std::this_thread::sleep_for(std::chrono::seconds(10));

        std::string newFile = getLogFileName();
        if (newFile != logFile) {
            logFile = newFile;
            NanoLog::setLogFile(logFile); // Nowy log każdego dnia
        }
    }

    return 0;
}
```

---

### 📊 **Podsumowanie – czy Ci się przyda?**
| 🔧 Metoda | 📌 Jak działa? | 🏆 Zalety | ❌ Wady |
|-----------|--------------|----------|---------|
| **Obrót po rozmiarze** | Nowy plik po osiągnięciu X MB | Nie zapełni Ci dysku | Można zgubić stare logi |
| **Obrót po czasie** | Nowy plik np. co 24h | Łatwe do analizy według dat | Może nie działać dobrze przy restartach |
| **Limit liczby plików** | Zawsze trzymamy np. 5 logów | Nie ma ryzyka przepełnienia | Może nadpisać istotne logi |

🔹 **Kiedy to się przyda?**
- Jeśli aplikacja działa długo i generuje **dużo logów**.
- Jeśli musisz **analizować błędy sprzed kilku dni** (rotacja dzienna).
- Jeśli Geant4 generuje ogromne dane i nie chcesz **gigantycznych plików**.

Jeśli tego nie potrzebujesz teraz, ale kiedyś w Geant4 zobaczysz, że **log.txt ma 5 GB**, to wtedy warto to dodać. 😎  

# Part 4:

Dobre spostrzeżenie! **NanoLog ma dwie wersje** – klasyczną (`NanoLog.hpp`) i wersję **C++17** (`NanoLogCpp17.h`).  
Skoro już używasz **C++17**, warto się zastanowić, czy korzystać z tej drugiej opcji.  

---

### 📌 **Co daje C++17 NanoLog w porównaniu do zwykłej wersji?**
| **Funkcja** | **Standardowa NanoLog** | **C++17 NanoLog** |
|------------|----------------------|-------------------|
| 📄 **Plik nagłówkowy** | `#include "NanoLog.hpp"` | `#include "NanoLogCpp17.h"` |
| 🚀 **Wydajność** | Superszybka | **Jeszcze szybsza** dzięki optymalizacji kompilatora |
| 💾 **Format zapisu** | Standardowy log binarny | **Mocno skompresowany** log binarny |
| 🔄 **Decompressor** | Niepotrzebny (log czytelny od razu) | **Potrzebny** do odzyskania pełnych logów |
| 🛠 **Linkowanie** | Wystarczy dodać NanoLog | **Trzeba linkować `-lrt -pthread`** |
| 🔍 **Ochrona przed błędami** | Brak dodatkowych zabezpieczeń | **`-Werror=format` wykrywa błędy formatowania** |

---

### 🔥 **Główna różnica – sposób zapisu logów**
- **Standardowa wersja** → logi zapisane **normalnie** (łatwo je odczytać od razu).
- **C++17 NanoLog** → logi zapisane **w super skompresowanym formacie**, trzeba je później zdekompresować za pomocą `./decompressor`.

💡 **Przykład użycia C++17 NanoLog:**
```cpp
#include "NanoLogCpp17.h"

int main() {
    NanoLog::setLogFile("compressed.log");
    
    nanoLog(NanoLog::LogLevels::Info, "This is an optimized log.");
    nanoLog(NanoLog::LogLevels::Error, "Error at step: {}", 42);

    return 0;
}
```
Potem, żeby odczytać logi:
```sh
./decompressor compressed.log > readable.log
cat readable.log
```
✔ **Efekt?** Plik `compressed.log` jest **dużo mniejszy**, ale wymaga `decompressor` do odczytu.

---

### 🏆 **Czy warto używać C++17 NanoLog?**
✔ **TAK, jeśli:**
- Logi **są ogromne** i musisz oszczędzać miejsce na dysku.
- Potrzebujesz **ekstremalnej wydajności** (np. w Geant4 symulacje lecą na maxa).
- **Nie musisz od razu czytać logów** – możesz je zdekompresować później.

❌ **NIE, jeśli:**
- **Chcesz widzieć logi od razu w czytelnej formie**.
- **Nie chcesz dodatkowej zależności (`./decompressor`)**.
- Twój projekt **nie generuje ekstremalnej ilości logów**.

---

### 🔧 **Jak to uwzględnić w CMake?**
Jeśli zdecydujesz się na **C++17 NanoLog**, trzeba dodać do `CMakeLists.txt`:
```cmake
target_link_libraries(my_project PRIVATE NanoLog rt pthread)
target_compile_options(my_project PRIVATE -Werror=format)
```
✔ `-Werror=format` → wykrywa błędy w formatowaniu logów na etapie kompilacji (ważne w tej wersji!).  
✔ `-lrt -pthread` → wymagane dla `libNanoLog.a`.

---

### 🚀 **Podsumowanie**
- Jeśli **masz dużo logów i zależy Ci na wydajności** → **C++17 NanoLog**.
- Jeśli **chcesz prostotę i czytelność od razu** → **klasyczna wersja NanoLog**.

📌 **Moja rada:**  
Najpierw użyj **zwykłej wersji** NanoLoga. Jeśli **logi zaczną zajmować dużo miejsca**, wtedy przejdź na **C++17 NanoLog**.  
Bo jak logi nie są problemem, to po co dokładać sobie `decompressor` i `-Werror=format`? 😎

