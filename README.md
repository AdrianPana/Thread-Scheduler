*Adrian Pana 323CC*
# Tema 2 SO - Planificator de thread-uri

## Rubrici
1. [TCB](README.md#thread-control-block-tcb)
2. [Scheduler](README.md#scheduler)
3. [I/O](README.md#io)
4. [Detalii de implementare](README.md#detalii-de-implementare-sau-ce-hackereala-am-facut-eu-sa-fie-programul-boier1-winksunglasses)
5. [Dificultati intalnite](README.md#dificultati-intalnite)
6. [Mentiuni](README.md#mentiuni)

## Thread Control Block (TCB)
- Customizat pentru nevoile temei
- Contine date pentru prioritate si status, pentru functia care trebuie
rulata si un semafor pentru blocare
- Memorate intr-un array dinamic pentru a tine evidenta inclusiv a celor
in state-ul de TERMINATED/BLOCKED
- Thread-urile READY sunt memorate si intr-o coada de prioritati implementata
cu liste

## Scheduler
- Initializat in [`so_init()`](so_scheduler.c#L97-L134), se aloca memorie, inclusiv pentru array si coada
- In [`so_end()`](so_scheduler.c#L234-L265) asteapta terminarea tuturor thread-urilor inainte de eliberarea
memoriei
- [`check_scheduler()`](so_scheduler.c#L24-L59) verifica planificatorul si thread-ul curent. Acesta din urma
poate fi preemptat sau lasat sa ruleze in continuare (si eventual i se reseteaza
timpul pe procesor)
- Pentru preemptare, [`schedule_next_thread()`](so_scheduler.c#L9-L22) scoate un thread nou din coada si
il deblocheaza. Thread-ul anterior va fi blocat

## I/O
- [`so_wait()`](so_scheduler.c#L184-L199) actualizeaza thread-ul ca asteptand pentru **io** si il preempteaza
- [`so_fork()`](so_scheduler.c#L201-L220) trece prin array-ul de thread-uri si le deblocheaza pe cele care
asteapta pentru **io**, apoi se verifica daca trebuie preemptat

## Detalii de implementare (sau ce hackereala am facut eu sa fie programul boier[^1] :wink::sunglasses:)
- Thread-urile nu au nevoie sa aiba cuante de timp proprii, a fost destul sa
lucrez cu o cuanta de timp pe procesor, care corespundea thread-ului care
rula pe el in acel moment
- Dupa orice operatie facuta de un thread, se consuma timp pe procesor si se
verifica daca acesta trebuie preemptat. `so_exec()` face doar asta, asa ca
se poate implementa aceasta functionalitate in toate functiile cu operatii
prin apelarea `so_exec()` la final

## Dificultati intalnite
- Am uitat sa dau `pthread_join()` in [`so_end()`](so_scheduler.c#L234-L265) si
am petrecut jumatate de zi incercand sa-mi dau seama de ce da timeout la toate
testele de la exec incolo. 
- Cam chill testele, mi-am dat seama ca nu implementasem bine coada doar cand
am vazut ca merge tot pana in ultimele 3-4 teste (~~Glumesc, foarte bune testele, lasati asa~~)
- `pthread_create()` in care apelezi handler-ul cu mai mult de un argument a
necesitat research suplimentar
- Creierul, pentru ca ti-e cel mai mare inamic. Dar aia e, stiti cum se zice: 
- > Altii au murit si n-au patit nimic :skull:

## Mentiuni
- Misto tema, dar am imbatranit in ultimele doua saptamani vreo doi ani
- M-a ajutat sa inteleg cum functioneaza un scheduler si cum sa lucrez cu threadurile:
- Apasa :point_down:
- <a href="https://www.youtube.com/watch?v=dQw4w9WgXcQ"><img src="https://i.kym-cdn.com/photos/images/original/002/139/875/272.png"></img></a>

[^1]: Atunci cand iti merge si tie programul ala obosit si te prefaci ca ai facut mari eficientizari
