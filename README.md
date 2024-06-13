# MyShell: monolitna ukazna lupina za Linux sisteme
Projekt je bil izdelan v okviru predmeta Operacijski sistemi. Sam sem na koncu dodal še nekaj dodatnih funkcionalnosti.
## Navodila za zagon programa
- obvezno v terminalu operacijskega sistema Linux:
  ```bash
  gcc mysh.c
  ./a.out
## Funkcionalnosti lupine
### Struktura ukaza
Najprej navedemo ime ukaza skupaj z njegovimi argumenti, na koncu ukaza imamo možnost preusmeritve vhoda (znak <), preusmeritve izhoda (znak >)
in/ali izvajanje v ozadju (znak &). 
```bash
imeUkaza arg1 arg2 arg3 ... <imeVhodneDatoteke >imeIzhodneDatoteke &
```
### Izvajanje ukazov
Lupina podpira tako izvajanje **vgrajenih** ukazov (našteti spodaj) kot tudi poljubnih **zunanjih** ukazov (programov).
### Osnovni vgrajeni ukazi
- ```debug level ```
  - Opcijski številčni argument level podaja nivo razhroščevanja (debug level).
  - Če argument ni podan, se izpiše trenutni nivo razhroščevanja.
  - Če pa je podan, se nastavi nivo razhroščevanja na podani level. Če uporabnik poda število v napačni obliki, potem se privzame 0.
- ```prompt poziv```
  - S tem ukazom izpišemo ali nastavimo izpis pozivnika (prompt).
  - Če argument ni podan, potem se izpiše trenutni poziv.
  - Če je podan, pa se nastavi nov pozivnik. Podano ime lahko vsebuje do 8 znakov, v primeru daljšega imena, se vrne izhodni status 1.
- ```status```
  - Izpiše se izhodni status zadnjega izvedenega ukaza.
  - Ta ukaz izjemoma status pusti nespremenjen.
- ```exit status```
  - Izvajanje lupine se zaključi s podanim izhodnim statusom.
  - Če argument ni podane, se lupina konča s statusom zadnjega izvedenega ukaza.
- ```help```
  - Izpiše spisek podprtih ukazov.
- ```print args...```
  - Izpiše podane argumente na standardni izhod (brez končnega skoka v novo vrstico).
- ```echo args...```
  - Kot ukaz print, le da izpiše še skok v novo vrstico.
- ```len args...```
  - Izpiše skupno dolžino vseh argumentov (kot nizi)
- ```sum args...```
  - Sešteje svoje argumente (cela števila) in izpiše vsoto.
- ```calc arg1 op arg2```
  - Nad argumentoma arg1 in arg2 izvede operacijo op in izpiše rezultat.
  - Argumenta sta števili.
  - Operacija je lahko +, -, *, /, %.
### Vgrajeni ukazi za delo z imeniki in datotekami
- ```basename arg```
  - Izpiše osnovno ime podane poti arg.
- ```dirname arg```
  - Izpiše imenik podane poti arg.
- ```dirch imenik```
  - Zamenjava trenutnega delovnega imenika (working directory).
  - Če imenika ne podamo, naj skoči na korenski imenik.
- ```dirwd mode```
  - Izpis trenutnega delovnega imenika.
  - Če je mode enako full se izpiše celotna pot.
  - Če je mode enako base, se izpiše le osnova imena (basename).
  - Če argumenta ne podamo, se privzame base.
- ```dirmk imenik```
  - Ustvarjanje podanega imenika.
- ```dirrm imenik```
  - Brisanje podanega imenika.
- ```dirls imenik```
  - Preprost izpis vsebine imenika.
  - Izpišejo se le imena datotek, ločena z dvema presledkoma.
  - Če imena imenika ne podamo, se privzame trenutni delovni imenik.
- ```rename izvor ponor```
  - Preimenovanje datoteke izvor v ponor.
- ```unlink ime```
  - Odstrani datoteko s podanim imenom.
  - Natančneje: odstrani le imeniški vnos s podanim imenom.
  - Gre za sistemski klic unlink().
- ```remove ime```
  - Odstranjevanje datoteke ali imenika s podanim imenom.
  - Gre za sistemski klic remove().
- ```linkhard cilj ime```
  - Ustvari trdo povezavo s podanim imenom na cilj.
- ```linksoft cilj ime```
  - Ustvari simbolično povezavo s podanim imenom na cilj.
- ```linkread ime```
  - Izpiše cilj podane simbolične povezave.
- ```linklist ime```
  - V trenutnem delovnem imeniku poišče vse trde povezave na datoteko s podanim imenom.
  - Povezave se izpišejo ločene z dvema presledkoma.
- ```cpcat izvor ponor```
  - Znana ukaza cp in cat združena v enega.
### Vgrajeni ukazi za delo s procesi
- ```pid```
  - Izpis PID procesa lupine.
- ```ppid```
  - Izpis PID starševskega procesa lupine.
- ```uid```
  - Izpis UID uporabnika, ki je lastnik procesa lupine
- ```euid```
  - Izpis UID uporabnika, ki je aktualni lastnik procesa lupine
- ```gid```
  - Izpis GID skupine, kateri pripada procesa lupine
- ```egid```
  - Izpis GID skupine, kateri aktualno pripada procesa lupine
- ```sysinfo```
  - Izpiše osnovne informacije v sistemu.
  - sistemski klic uname().
  - Izpiše polja: sysname, nodename, release, version, machine.
- ```proc pot```
  - Nastavitev poti do procfs datotečnega sistema.
  - Brez argumenta se izpiše trenutna nastavitev. Privzeta nastavitev je /proc.
  - Če pa podamo argument, se nastavi nova pot do imenika, ki vsebuje procfs.
  - Pred nastavitvijo poti ta ukaz tudi preveri ali podana pot dejansko obstaja.
  - Če ne obstaja, se pot ne nastavi in vrne izhodni status 1.
- ```pids```
  - Izpiše PIDe trenutnih procesov, ki jih pridobi iz procfs.
- ```pinfo```
  - Izpiše informacije o trenutnih procesih (PID, PPID, STANJE, IME), ki jih pridobi iz datoteke stat v procfs.
- ```waitone pid```
  - Počaka na otroka s podanim pidom.
  - Če pida ne podamo, počaka na enega poljubnega otroka.
  - Če otrok ne obstaja, vrne izhodni status 0, sicer pa izhodni status končanega otroka.
- ```waitall```
  - Počaka na vse otroke.





























