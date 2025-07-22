# GTest

## Konfiguracja testów w  CMake
* **add_executable** - dla każdego pliku wykonywalnego z testami, który chcemy utworzyć
* **target_link_libraries** - (opcjonalnie) jeśli potrzebujemy linkować jakieś biblioteki do testów
   - (opcjonalnie) linkujemy biblioteke gtest_main, jeśli nie potrzebujemy pisać własnego main().
* **gtest_discover_tests** - dodaje testy zdefiniowane w pliku wykonywalny dla CTest. Lepszy od add_test, bo nie wymaga wykonywania cmake. Może mieć problemy przy cross-kompilacji, ale to nas nie dotyczy.
   - gtest_discover_tests(<nazwa_pliku_wykonywalnego>)
* **add_test()** - jak powyższe, ale wymaga wykonania cmake przy każdym dodaniu/zmienia testu.

## Dodawanie testów
* W pliku z testami dodajemy makro TEST(){} lub TEST_F(){}
   - TEST(<case_name>,<test_name>){<code>}
   - W przypadku gdy potrzebujemy wielu testów, które potrzebują tych samych danych, a ich inicjalizacja jest kosztowna, możemy zbudować klasę dziedziczącą po ::testing::Test:
     class case_name: public ::testing::Test {
         void SetUp() override {}
         void TearDown() override {}
     }
     I używać makra TEST_F(<case_name>,<test_name>){<code>}
* Właściwe testowanie danych wykonujemy przy pomocy makr:
   - **EXPECT_*** (np **EXPECT_EQ**,**EXPECT_GT**, itp) - testowanie danych
   - **ASSERT_*** (np. **ASSERT_EQ**, **ASSERT_NE**, itp). w przypadku niespełnienia warunków, przerywany jest sam test - stosować, gdy wykonanie pozostałych testów nie ma sensu.
* Gdy w testach musimy odwołać się do prywatnych pól klasy, możemy w kodzie testowanej klasy użyć makra: FRIEND_TEST(<case_name>,<test_name>); z pliku gtest/gtest_prod.h lub wprost użyć: friend class <case_name>_<test_name>_Test;

## Wykonywanie testu.
* Uruchamiamy ctest w katalogu z testami.
    - Przydatne opcje:
        + --test-dir dir - Gdy chcemy wykonać testy z innego katalogu niż jesteśmy.
        + -R pattern - wykonywanie tylko testów pasujących do wzorca
        + -L label - wykonywanie testów oznacznych znaczniekim "label"
        + -E pattern - wykluczenie testów pasujących do wzorca
        + -j jobs
        + --output-on-failure - pełny output dla nieudanych testów
        + --schedule-random - uruchamianie testów w przypadkowej kolejności, gdy może to mieć znaczenie
        + --repeat count - powtarzenie testów zadaną ilość razy
   
