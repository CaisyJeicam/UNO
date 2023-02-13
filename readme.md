# Serwer TCP C++ 
## gra UNO

Projekt jest implementacją serwera gry w UNO, wraz z konsolową aplikacją klienta
napisaną w języku C++.

---

Stronę serwera można podzielić na dwie części:
- część reprezentującą mechaniki i logikę gry,
- część odpowiedzialną za komunikację między serwerem i klientami,

Serwer dla każdego klienta tworzy wątek sprawdzający czy otrzymał 
wiadomość od serwera. Serwer wykrywa odłączenia się i błędy transmisji.

Zaimplementowane są wszytkie karty dostępne w grze.
