# MyShell: monolitna ukazna lupina za Linux sisteme
Projekt je bil izdelan v okviru predmeta Operacijski sistemi. Sam sem na koncu dodal še nekaj dodatnih funkcionalnosti 
(označene z *).
## Navodila za zagon programa
- obvezno v terminalu operacijskega sistema Linux:
  ```bash
  gcc mysh.c
  ./a.out
## Funkcionalnosti lupine
### Struktura ukaza
- Najprej navedemo ime ukaza skupaj z njegovimi argumenti, na koncu ukaza imamo možnost preusmeritve vhoda (znak <), preusmeritve izhoda (znak >)
  in/ali izvajanje v ozadju (znak &). 
  ```bash
  imeUkaza arg1 arg2 arg3 ... <imeVhodneDatoteke >imeIzhodneDatoteke &
  ```
### Izvajanje ukazov
- Lupina podpira tako izvajanje **vgrajenih** ukazov (našteti spodaj) kot tudi poljubnih **zunanjih** ukazov (programov).
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
### Cevovod
- ```pipes "stopnja 1" "stopnja 2" "stopnja 3" ...```
  - Prisotni morata biti vsaj dve stopnji.
  - Primer cevovoda v lupini mysh:
    ```bash
    pipes "cat /etc/passwd" "cut -d: -f7" "sort" "uniq -c"
    ```
    kar je enakovredno kot naslednji cevovod v bash:
    ```bash
    cat /etc/passwd | cut -d: -f7 | sort | uniq -c
    ```
  - Znotraj stopenj se preusmerjanje in izvajanje v ozadju ne izvaja.
  - Možna pa je seveda preusmeritev na nivoju celotnega cevovoda in tudi njegovo izvajanje v ozadju.
### Spremenljivke *
- ```set ime vrednost```
  - Nastavi spremenljivko "ime" na podano vrednost.
  - Če spremenljivka ne obstaja, jo ustvari in ji dodeli podano vrenost.
- ```get ime```
  - Izpiše vrednost spremenljivke "ime".
  - Če spremenljivka ne obstaja, vrne izhodni status 1.
- Primeri uporabe:
  ```bash
  set a 1
  get a
  set str "Hello World"
  get str
  ```
### Združevanje ukazov *
- V isti vrstici lahko zapišemo tudi več ukazov.
- Ukaze med seboj ločimo s podpičjem (;).
- Ukazi se izvedejo zaporedno (eden za drugim).
- Primeri uporabe:
  ```bash
  echo prvi ; echo drugi ; echo tretji
  echo start ; sleep 2 ; echo stop
  ```
### Podlupina z zamenjavo izpisa *
- Ukaz se izvede znotraj podlupine, nato se njegov izpis uporabi v glavnem ukazu.
- Ukaz, ki se bo izvajal v podlupini označimo takole: ```$(ukaz)```
- Primeri uporabe:
  ```bash
  wc $(ls)
  set a $(calc $(get a) + 1)     #isto kot a++
  ```
### Vgrajeni primerjalni ukazi in logični operatorji *
- ```lt a b```
  - Izpiše "true", če velja ```a < b```.
  - Sicer izpiše "false".
- ```le a b```
  - Izpiše "true", če velja ```a <= b```.
  - Sicer izpiše "false".
- ```eq a b```
  - Izpiše "true", če velja ```a = b```.
  - Sicer izpiše "false".
  - Deluje tudi za primerjavo enakosti nizov.
- Primeri uporabe:
```bash
lt 1 1       #izpiše false
le 1 1       #izpiše true
eq 1 1       #izpiše true
eq niz niz   #izpiše true
```
- ```and arg1 arg2 ...```
  - Izpiše "true", če imajo vsi argumenti vrednost "true".
  - Sicer izpiše "false".
- ```or arg1 arg2 ...```
  - Izpiše "false", če imajo vsi argumenti vrednost "false".
  - Sicer izpiše "true".
- ```not arg1 ```
  - Izpiše "true", če ima argument vrednost "false".
  - Sicer izpiše "false".
- Primeri uporabe:
```bash
and true true           #izpiše true
or false false true     #izpiše true
not true                #izpiše false
not $(eq 1 1)           #izpiše false
```
### if stavek in while zanka *
- ```if pogoj then "ukaz" [else "ukaz"]```
  - else del je opcijski.
  - Primeri uporabe:
  ```bash
  if true then "echo true"                        #izpiše true
  if $(lt 2 1) then "echo 2<1" else "echo 2>=1"   #izpiše 2>=1
  ```
- ```while "pogoj" do "ukaz"```
  - Pogoj mora biti obvezno v dvojnih narekovajih (zato, ker se mora preverjati (izvajanje podlupin) pri vsakem obhodu zanke).
  - Primer neskončne zanke:
  ```bash
  while true do "echo loop"
  ```
  - Še en primer zanke:
  ```bash
  set i 0      #nastavi i na 0
  while "$(lt $(get i) 5)" do "set i $(calc $(get i) + 1)"    #povečuje i dokler je manjši od 5
  echo $(get i)    #izpiše i (izpiše 5)
  ```
  - Zanka, ki izpiše števila med 1 in 100:
  ```bash
  set i 1 ; set n 100
  while "$(le $(get i) $(get n))" do "echo $(get i) ; set i $(calc $(get i) + 1)"
  #nato z if stavkom preverimo ali je i enak 101:
  if $(eq $(get i) 101) then "echo true" else "echo false"    #izpiše true
  ```













