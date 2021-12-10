#include <LiquidCrystal.h>  // Ladataan tarvittu kirjasto


LiquidCrystal lcd = LiquidCrystal(2,3,4,5,6,7);   // Luodaan näytölle objekti.
// Tästä alkaa settings menuun sisältyvät tiedot
#define button_T    A0
#define button_A    A1
#define button_Y    A2
#define button_E    A3

// buttonit T,A,Y,E tulevat sanoista:
// T = takaisin
// A = alas
// Y = ylös
// E = enter
// tässä on myös määritelty mitä arduinon pinnejä napit käyttävät

#define DEFAULT_DELAY 300

//Seuraavaksi alustetaan lista vastauksista TODO: funktio tekemään muutoksia serialin kautta.
char *actors[] = {"Vastaukseni on ei", "Ala luota siihen", "Lähteeni sanoo ei", "Epäilen", "Lopputulema ei ole hyvä", "Parempi etten kerro nyt", "En voi ennustaa nyt", "Kysy uudestaan myöhemmin", "Epäselvä kysymys, kysy uudelleen", "Keskity ja kysy uudelleen", "Lopputulema on hyvä", "Todennäköisesti", "Merkit viittaavat että kyllä", "Miten näen asian, kyllä", "Kyllä", "Voit luottaa siihen",
 "Epäilemättä", "Kyllä, ehdottomasti", "Se on juurikin näin", "Se on varma"};
 
// nämä myös sisältyvät settingsMenu() toimintaan
char nappiPaino = '0';

byte menuTaso = 0;     //  Taso 1 näyttää kotinäytön
                        // Taso 2 näyttää päävalikon
                        // Taso 3 näyttää niin sanotun "submenun"
byte menu = 1;
byte sub = 1;

unsigned long hNumero_1 = 0;
unsigned long hNumero_2 = 0;
unsigned long hNumero_3 = 0;

bool LED_STATE = false; // bool-pitää sisällään joko true or false tietoa

bool currState_T = HIGH;
bool currState_A = HIGH;
bool currState_Y = HIGH;
bool currState_E = HIGH;
          
bool prevState_T = HIGH; 
bool prevState_A = HIGH; 
bool prevState_Y = HIGH; 
bool prevState_E = HIGH; 
// seuraavat muuttujat ovat unsigned longeja koska aika jota mitataan
// millisekuntteina tuottaa nopeasti suuremman luvun mitä voidaan tallentaa int muuttujaan
unsigned long prevTime_T = 0;
unsigned long prevTime_A = 0;
unsigned long prevTime_Y = 0;
unsigned long prevTime_E = 0;

unsigned long waitTime_T = 50;
unsigned long waitTime_A = 50;
unsigned long waitTime_Y = 50;
unsigned long waitTime_E = 50;

//Määritellään ledien animaatiossa käytettävät pinnit ja setupissa outputeiksi
const int latchPin = 10;
const int clockPin = 11;
const int dataPin = 12;

//Animaatiossa käytettävä taulukko
int datArray[2];  //Todnäk ei tarvitse olla globaali, mutta jostain syystä ei toiminut testatessa kuin globaalina

bool ravistusLippu = TRUE;      //Lippu ravistusfunktion toiminnalle
int sensorValue = 0;            //Muuttuja johon luku kiihtyvyysanturilta
unsigned long aika = millis();  //Muuttuja jossa aika millisekunteina
int ravistusLaskuri = 0;        //Laskurit ravistuksentunnistusta varten
int nollausLaskuri = 0;
const int analogInPin = A4;     //Kiihtyvyysanturin sarjakytkennän sisääntulo
int animaatioLippu = 0;         //lcd-animaation lippu, mahollistaa animoinnin esim loopin sykleissä

void setup() {
  lcd.begin(16, 2); // määritetään näytön koko
  Serial.begin(9600);   //TODO: onko tälle tarvetta enään
  randomSeed(600); //TODO: tähän analogi pinni jos vapaana
  pinMode(button_T, INPUT_PULLUP); //määritellään pinnien tehtävät pinMode(), kuuluvat settingsMenu()
  pinMode(button_A, INPUT_PULLUP); //määritellään pinnien tehtävät pinMode(), kuuluvat settingsMenu()
  pinMode(button_Y, INPUT_PULLUP); //määritellään pinnien tehtävät pinMode(), kuuluvat settingsMenu()
  pinMode(button_E, INPUT_PULLUP); //määritellään pinnien tehtävät pinMode(), kuuluvat settingsMenu()
  pinMode(A4,INPUT);
  pinMode(latchPin, OUTPUT); //Ledien  animaatioon käytettävät pinnit outputeiksi
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  //Testiajot funktiolle
  melodia(10, 1); //pyritään tässä kutsumaan default
  lediAnimaatio();
  //TODO: näytölle testifunk
}

int randomNumero() {   //funktio rng:lle
  long actor1;
  actor1 = random(sizeof(actors)/sizeof(char*)); //numero on välillä 0-[listan pituus]
  return actor1;
}

void tulostusFunk(int a = 0) {     //funktio tulostukselle
    lcd.print(actors[a]); // printataan ensimmäiselle riville
    delay(2000);  //aika kauanko esitetään vastausta näytöllä
    lcd.clear(); //näytön tyhjennys
}

void ravistus(){

  unsigned long kulunutAika;      //Lasketaan kahden peräkkäisen mittaustuloksen kulmakerroin.
  kulunutAika = millis() - aika;  
  aika = millis();
  
  int sensorPrevious = sensorValue;
  sensorValue = analogRead(analogInPin);
  
  int kulmakerroin = (sensorValue - sensorPrevious) / (int)kulunutAika;

  if (kulmakerroin != 0) {        //Nostetaan laskuria kun saatava kulmakerroin ei 0
    ravistusLaskuri++;
    nollausLaskuri=0;
  }
  if(kulmakerroin == 0){          //Nostetaan nollauslaskuria kun kulma 0
    nollausLaskuri++;
  }

  if(nollausLaskuri>=6){          //Tätä vertailua muuttamalla herkkyyden muutos
    ravistusLippu = TRUE;         
    ravistusLaskuri=0;
    nollausLaskuri=0;
  }
  
  if (ravistusLaskuri >= 11) {    //Tietyn määrän jälkeen led päälle ja if 0 looppiin
    ravistusLippu = FALSE;        //vertailtavaa lukua muuttamalla herkkyyden säätö
    ravistusLaskuri = 0;          //Tähän tietenkin sisälle koodi joka signaloi
  }
 
  //HUOM! Funktiossa delay koska funktion toiminta prosessorin kellotaajuudella aivan liian herkkä. Ehkä tarvetta keksiä jokin muu ratkaisu?
  delay(50);
  
}

void moottoriBrrr() { //TODO: funktio jolla voi päristää moottoria tietyissä tilanteissa
  //Käytetään vaikka arduinon porttia 15
}

void melodia(int a = 0, int b = 1) { //Tässä a on melodia mitä halutaan soittaa ja b on toistojen määrä
  switch(a){ // 0 caseksi lyhyt piippaus, pitemmät melodiat valintojen taakse
    case 0: //2400 = tahti
      for(int n = 0; n < b; n++) {    
      tone(9, 293, 200); //d
      delay(400);
      tone(9, 440, 200); //a
      delay(400);
      tone(9, 329, 200); //e
      delay(400);
      tone(9, 440, 200); //a
      delay(400);
      //Tämä kommentti on tahtiviiva
      tone(9, 349, 600); //f
      tone(9, 392, 200); //g
      delay(100);
      tone(9, 440, 200); //a
      delay(100);
      tone(9, 349, 200); //f
      delay(400);
      tone(9, 493, 200); //b
      delay(400);
      //Tämä kommentti on tahtiviiva
      tone(9, 293, 200); //d
      delay(200);
      tone(9, 440, 200); //a
      delay(200);
      tone(9, 329, 200); //e
      delay(200);
      tone(9, 440, 200); //f
      delay(200);
      tone(9, 293, 200); //e
      delay(200);
      tone(9, 440, 200); //f
      tone(9, 329, 200); //e
      tone(9, 440, 200); //d
      delay(200);
      tone(9, 440, 200); //c
      delay(200);
      //Tämä kommentti on tahtiviiva
      tone(9, 440, 200); //a
      delay(200);
      tone(9, 261, 200); //c
      delay(200);
      tone(9, 392, 200); //g
      delay(200);
      tone(9, 440, 200); //a
      delay(200);
      tone(9, 349, 600); //f
      }
    break;

    case 1: 
    //melodia
    break;

    case 2:
    //melodia
    break; 

    default:  //virheen kuuloinen piippaus esim terssi alas g-d#
    tone(9, 196, 100); //g
    tone(9, 155, 200); //d#
  }
}

void buttonCheck() { // navigointia varten tehty funktio

  // Tässä niin sanotusti "debouncataan" painonapit, jotta varmistutaan että
  // että napin painallukset antavat luotettavia tuloksia
  bool currRead_T = digitalRead(button_T);
  bool currRead_A = digitalRead(button_A);
  bool currRead_Y = digitalRead(button_Y);
  bool currRead_E = digitalRead(button_E);

  if (currRead_T != prevState_T) {
    prevTime_T = millis();
  }
  if (currRead_A != prevState_A) {
    prevTime_A = millis();
  }
  if (currRead_Y != prevState_Y) {
    prevTime_Y = millis();
  }
  if (currRead_E != prevState_E) {
    prevTime_E = millis();
  }

  if ((millis() - prevTime_T) > waitTime_T) {
    if (currRead_T != currState_T) {
      currState_T = currRead_T;
      if (currState_T == LOW) {
        nappiPaino = 'T';  // määrittää napin painamisen toiminnoksi 'takaisin'
      } 
    }
  } else nappiPaino = '0';
  if ((millis() - prevTime_A) > waitTime_A) {
    if (currRead_A != currState_A) {
      currState_A = currRead_A;
      if (currState_A == LOW) {
        nappiPaino = 'A'; // määrittää napin painamisen toiminnoksi 'alas'
      } 
    }
  } else nappiPaino = '0';
  if ((millis() - prevTime_Y) > waitTime_Y) {
    if (currRead_Y != currState_Y) {
      currState_Y = currRead_Y;
      if (currState_Y == LOW) {
        nappiPaino = 'Y'; // määrittää napin painamisen toiminnoksi 'ylös'
      } else {
        //nappiPaino = '0';
      }
    }
  } else nappiPaino = '0';
  if ((millis() - prevTime_E) > waitTime_E) {
    if (currRead_E != currState_E) {
      currState_E = currRead_E;
      if (currState_E == LOW) {
        nappiPaino = 'E'; // määrittää napin painamisen toiminnoksi 'entteri'
      } 
    }
  } else nappiPaino = '0';

  prevState_T = currRead_T;
  prevState_A = currRead_A;
  prevState_Y = currRead_Y;
  prevState_E = currRead_E;

  settinsgMenu(nappiPaino);

}

void settingsMenu(char nappiPaino) { // asetusvalikon ohjelma

  switch(menuTaso) {    //käytetään switchcasea navigoinnissa
    case 0: // 
      switch ( nappiPaino ) {
        case 'E': // Entteri
          menuTaso = 1;
          menu = 1;
          paivitaMenu();     // Siirtää näkymän päävalikkoon
          delay(DEFAULT_DELAY);
          break;
        case 'Y': // ylös
          break;
        case 'A': // alas
          break;
        case 'T': // takaisin
          break;
        default:
          break;
      }
      break;
    case 1: // Taso 1, päävalikko
      switch ( nappiPaino ) {
        case 'E': // Enter
          paivitaSub();   // Sub = submenu
          menuTaso = 2;  // menee submenuun
          paivitaSub();
          delay(DEFAULT_DELAY);
          break;
        case 'Y': // ylös
          menu++;
          paivitaMenu();
          delay(DEFAULT_DELAY);
          break;
        case 'A': // alas
          menu--;
          paivitaMenu();
          delay(DEFAULT_DELAY);
          break;
        case 'T': // takaisin
          menuTaso = 0;  // piilottaa menun ja siirtyy takaisin kotinaytolle
          naytaKotinaytto();
          delay(DEFAULT_DELAY);
          break;
        default:
          break;
      } 
      break;
    case 2: // taso 2, submenu
      switch ( nappiPaino ) {
        case 'E': 
          menuTaso = 1;
          paivitaMenu();
          delay(DEFAULT_DELAY);
          break;
        case 'Y': // U
          if (menu == 1) {
            if (hNumero_1 < 3600000) {  // 1 tunti
              hNumero_1 = hNumero_1 + 1;
            } else {
              hNumero_1 = 3600000;
            }
          } else if (menu == 2) {       
            if (hNumero_2 < 3600000) {  // 1 tunti
              hNumero_2 = hNumero_2 + 1;
            } else {
              hNumero_2 = 3600000;
            }
          } else if (menu == 3) {
            if (hNumero_3 < 3600000) {  // 1 tunti  
              hNumero_3 = hNumero_3 + 1;
            } else {
              hNumero_3 = 3600000;
            }
          }
          paivitaSub();
          delay(DEFAULT_DELAY);
          break;
        case 'A': // A
          if (menu == 1) {
            if (hNumero_1 == 0) {
              hNumero_1 = 0;
            } else {
              hNumero_1 = hNumero_1 - 1;
            }
          } else if (menu == 2) {
            if (hNumero_2 == 0) {
              hNumero_2 = 0;
            } else {
              hNumero_2 = hNumero_2 - 1;
            }
          } else if (menu == 3) {
            if (hNumero_3 == 0) {
              hNumero_3 = 0;
            } else {
              hNumero_3 = hNumero_3 - 1;
            }
          }
          paivitaSub();
          delay(DEFAULT_DELAY);
          break;
        case 'T': // L
          menuTaso = 1;  // takaisin päävalikkoon
          paivitaMenu();
          delay(DEFAULT_DELAY);
          break;
        default:  
          break;
      } 
      break;
    case 3: // Taso 3, jos lisättäisiin sub menulle vielä toinen sub menu
    
      break;
    default:
      break;
  }
  
}

void paivitaMenu() {   // Avataan päävalikko josta voidaan valita numero jota halutaan muokata
  
  switch (menu) {
    case 0:
      menu = 1;
      break;
    case 1:
      lcd.clear();
      lcd.print(">Hnumero 1 "); // ">" käytetään kursorina ohjaamaan haluttua submenua
      lcd.print(hNumero_1);
      lcd.setCursor(0, 1);
      lcd.print(" Hnumero 2 ");
      break;
    case 2:
      lcd.clear();
      lcd.print(" Hnumero 1 ");
      lcd.setCursor(0, 1);
      lcd.print(">Hnumero 2 "); // ">" käytetään kursorina ohjaamaan haluttua submenua
      lcd.print(hNumero_2);
      break;
    case 3:
      lcd.clear();
      lcd.print(">Hnumero 3 "); // ">" käytetään kursorina ohjaamaan haluttua submenua
      lcd.print(hNumero_3);
      lcd.setCursor(0, 1);
      lcd.print("             ");
      break;
    case 4:
      menu = 3;
      break;
  }
  
}

void paivitaSub() {
  switch (menu) {
    case 0:
    // kun submenu on valittu avautuu kyseinen näkymä riipputen siitä mikä valittu
      break;
    case 1:
      lcd.clear();
      lcd.print(" Vastaus 1:");
      lcd.setCursor(0, 1);
      lcd.print("  Numero 1 = ");
      lcd.print(hNumero_1);
      break;
    case 2:
      lcd.clear();
      lcd.print(" Vastaus 2:");
      lcd.setCursor(0, 1);
      lcd.print(" Numero 2 = ");
      lcd.print(hNumero_2);
      break;
    case 3:
      lcd.clear();
      lcd.print(" Vastasu 3:");
      lcd.setCursor(0, 1);
      lcd.print(" Numero 3 = ");
      lcd.print(hNumero_3);
      break;
    case 4:
      menu = 3;
      break;
  }
}

void naytaKotinaytto() { // Kotinäyttö näkymä
  lcd.clear();
  lcd.println(" Taika 8-pallo  ");
  lcd.setCursor(0,1);
  lcd.println(" - Paina enter  ");
}

void lediAnimaatio() {
 //perustuu https://dronebotworkshop.com/shift-registers/ löytyvään koodiin
 //kohdasta "74HC595 and 74HC165 Sketch 2 – Exciting!"

 datArray[0] = B10101010;
 datArray[1] = B01010101;
        
 //Ledien väläyttely
 for (int i=0; i < 2; i++){
   //latch "kiinni"
   digitalWrite(latchPin, LOW);
   
   //Bitit sisään, datapin kirjoittaa isoimman bitin ekana joka luetaan clockpinin sykkeellä
   //Tässä tapauksessa animaatiot tulevat x arvosta riippuen, i pyörityksellä käydään läpi
   shiftOut(dataPin,clockPin,MSBFIRST,datArray[i]);
   
   //latch "auki"
   digitalWrite(latchPin, HIGH);
 }
}

void lcdAnimaatio(){ //Vaatii hieromista riippuen arkkitehtuurista
                     //atm tulostaa lipun arvosta riippuen jomman kumman konfiguraation ja vaihtaa lipun tilaa
                     //Seuraavalla kutsukerralla tulostaa vastakkaisen konfiguraation koska lippu vaihtui ja muuttaa lippua taas jne jne
  if (animaatioLippu == 0){
    lcd.setCursor(0, 0);
    lcd.print("<><><><><><><><>");
    lcd.setCursor(0,1);
    lcd.print("><><><><><><><><");
  
    animaatioLippu = 1;
  }
  if (animaatioLippu != 0){
    lcd.setCursor(0, 0);
    lcd.print("><><><><><><><><");
    lcd.setCursor(0,1);
    lcd.print("<><><><><><><><>");
   
    animaatioLippu = 0;
  }
 
}


void loop() {   //perustoiminto loop
  //tästä otettu pois asetusmenun käyttö vielä toistaiseksi
  tulostusFunk(randomNumero()); //kutsutaan satunnaisella numerolla tulostus funktio
  melodia(0, 2); //Testimielessä rallattelut
  delay(1000); //Tämä delay on turha kunhan saadaan sensoridatan kuuntelulle funktio
}
