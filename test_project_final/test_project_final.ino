// Carte : Arduino Uno
// Vitesse série : 9600 (doit être la même que baudRate dans truc.js)
//
// Messages reçus de la page web (une ligne = un message) :
//   "B1"          -> joue le buzzer 1 (bouton Red)
//   "V50"         -> la vitesse des mix vaut 50 (0 = lent, 100 = rapide)
//   "M2:10100110" -> le mix 2 (pour le buzzer 2) : 8 cases, 1 = bip, 0 = silence
//   "P"           -> lance la musique (gros bouton)
//   "S"           -> arrête la musique
//   "R"           -> arrête tout, coupe les buzzers et efface les 4 mix (bouton stop)

// ---------- Les 4 buzzers ----------
// Buzzer 1 (Red)    -> broche 2
// Buzzer 2 (Blue)   -> broche 4
// Buzzer 3 (Green)  -> broche 7
// Buzzer 4 (Yellow) -> broche 8
const int NB_BUZZERS = 4;
const int BROCHES[NB_BUZZERS] = {2, 4, 7, 8};

// Chaque buzzer a sa propre note (en Hz) : do, mi, sol, do aigu
const unsigned int NOTES[NB_BUZZERS] = {262, 330, 392, 523};

const unsigned long DUREE_BOUTON = 300; // durée d'un bip quand on clique un bouton (ms)

// ---------- État de chaque buzzer ----------
// On fait vibrer les buzzers "à la main" (HIGH / LOW très vite), comme avant,
// mais sans delay() : comme ça les 4 buzzers peuvent sonner en même temps.
// (la fonction tone() de l'Uno ne sait faire sonner qu'un seul buzzer à la fois)
unsigned long demiPeriode[NB_BUZZERS];   // temps entre deux changements HIGH/LOW (µs)
unsigned long dernierChangement[NB_BUZZERS];
bool brocheHaute[NB_BUZZERS];
bool sonne[NB_BUZZERS];
unsigned long finDuSon[NB_BUZZERS];      // moment où le bip s'arrête (ms)

// ---------- Les mix ----------
const int NB_CASES = 8;
bool mix[NB_BUZZERS][NB_CASES];          // mix[buzzer][case] = true si la case est cochée
int vitesse = 50;
bool musiqueEnCours = false;             // devient vrai avec "P" (gros bouton)
int caseActuelle = 0;
unsigned long dernierPas = 0;

// ---------- Lecture série ----------
char ligne[20];
int longueur = 0;


void setup() {
  for (int i = 0; i < NB_BUZZERS; i++) {
    pinMode(BROCHES[i], OUTPUT);
    digitalWrite(BROCHES[i], LOW);
    demiPeriode[i] = 1000000UL / NOTES[i] / 2;
    sonne[i] = false;
  }

  Serial.begin(9600);
  // Le Uno redémarre quand la page ouvre le port série.
  // On prévient la page qu'on est prêt à recevoir des messages.
  Serial.println("PRET");
}


void loop() {
  lireSerie();
  avancerMix();
  faireVibrerBuzzers();
}


// Démarre un bip sur un buzzer (numéro 0 à 3) pendant "duree" millisecondes
void jouer(int buzzer, unsigned long duree) {
  sonne[buzzer] = true;
  finDuSon[buzzer] = millis() + duree;
}


// Fait vibrer les buzzers qui doivent sonner, et coupe ceux qui ont fini
void faireVibrerBuzzers() {
  for (int i = 0; i < NB_BUZZERS; i++) {
    if (!sonne[i]) continue;

    if ((long)(millis() - finDuSon[i]) >= 0) {
      // Le bip est fini
      sonne[i] = false;
      brocheHaute[i] = false;
      digitalWrite(BROCHES[i], LOW);
    } else if (micros() - dernierChangement[i] >= demiPeriode[i]) {
      // C'est le moment de passer de HIGH à LOW (ou l'inverse)
      dernierChangement[i] = micros();
      brocheHaute[i] = !brocheHaute[i];
      digitalWrite(BROCHES[i], brocheHaute[i] ? HIGH : LOW);
    }
  }
}


// Durée d'une case du mix selon le slider : 0 -> 600 ms (lent), 100 -> 100 ms (rapide)
unsigned long dureeCase() {
  return map(vitesse, 0, 100, 600, 100);
}


// Joue les mix en boucle : à chaque case, les buzzers cochés font un bip
void avancerMix() {
  // La musique ne joue que si on a appuyé sur le gros bouton
  if (!musiqueEnCours) return;

  // Est-ce qu'au moins une case est cochée dans un des mix ?
  bool auMoinsUneCase = false;
  for (int b = 0; b < NB_BUZZERS; b++) {
    for (int c = 0; c < NB_CASES; c++) {
      if (mix[b][c]) auMoinsUneCase = true;
    }
  }
  if (!auMoinsUneCase) {
    caseActuelle = 0;
    return;
  }

  if (millis() - dernierPas < dureeCase()) return; // pas encore le moment
  dernierPas = millis();

  for (int b = 0; b < NB_BUZZERS; b++) {
    if (mix[b][caseActuelle]) {
      jouer(b, dureeCase() * 2 / 3); // un petit silence entre deux bips
    }
  }

  caseActuelle = (caseActuelle + 1) % NB_CASES;
}


// Lit les caractères qui arrivent, et traite le message quand la ligne est complète
// (sans attendre : les buzzers continuent de sonner pendant la lecture)
void lireSerie() {
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (longueur > 0) {
        ligne[longueur] = '\0';
        traiterMessage();
        longueur = 0;
      }
    } else if (longueur < (int)sizeof(ligne) - 1) {
      ligne[longueur] = c;
      longueur++;
    }
  }
}


void traiterMessage() {
  // "B1" à "B4" : jouer un buzzer
  if (ligne[0] == 'B') {
    int numero = atoi(ligne + 1);
    if (numero >= 1 && numero <= NB_BUZZERS) {
      jouer(numero - 1, DUREE_BOUTON);
      Serial.print("Buzzer ");
      Serial.println(numero);
    }
  }

  // "V50" : nouvelle vitesse
  else if (ligne[0] == 'V') {
    vitesse = constrain(atoi(ligne + 1), 0, 100);
    Serial.print("Vitesse ");
    Serial.println(vitesse);
  }

  // "M2:10100110" : nouveau mix pour le buzzer 2
  else if (ligne[0] == 'M' && ligne[2] == ':') {
    int numero = ligne[1] - '0';
    if (numero >= 1 && numero <= NB_BUZZERS) {
      for (int c = 0; c < NB_CASES; c++) {
        mix[numero - 1][c] = (ligne[3 + c] == '1');
      }
      Serial.print("Mix ");
      Serial.print(numero);
      Serial.print(" : ");
      Serial.println(ligne + 3);
    }
  }

  // "P" : lancer la musique depuis la première case
  else if (ligne[0] == 'P') {
    musiqueEnCours = true;
    caseActuelle = 0;
    dernierPas = millis() - dureeCase(); // la première case joue tout de suite
    Serial.println("Musique lancee");
  }

  // "S" : arrêter la musique
  else if (ligne[0] == 'S') {
    musiqueEnCours = false;
    caseActuelle = 0;
    Serial.println("Musique arretee");
  }

  // "R" : tout arrêter et remettre à zéro
  else if (ligne[0] == 'R') {
    musiqueEnCours = false;
    caseActuelle = 0;
    for (int b = 0; b < NB_BUZZERS; b++) {
      // on coupe le buzzer tout de suite, même s'il était en train de biper
      sonne[b] = false;
      brocheHaute[b] = false;
      digitalWrite(BROCHES[b], LOW);
      // on efface son mix
      for (int c = 0; c < NB_CASES; c++) {
        mix[b][c] = false;
      }
    }
    Serial.println("Stop : musique arretee et mix effaces");
  }

  else {
    Serial.print("Message inconnu : ");
    Serial.println(ligne);
  }
}
