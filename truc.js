let port;
let writer;

// Chaque bouton de couleur = un numéro de buzzer
const numeroBuzzer = {
    Red: 1,
    Blue: 2,
    Green: 3,
    Yellow: 4
};

// target html elements
const connectBtn = document.querySelector('#connectBtn');
const output = document.querySelector('#output');
const boutonsCouleur = document.querySelectorAll('.PlayButton');
const mixes = document.querySelectorAll('.mix');
const slider = document.querySelector('#Slider');


// ---------- Envoyer un message à l'Arduino Uno ----------
// Chaque message est une ligne de texte qui finit par "\n"
//   "B1"          -> joue le buzzer 1
//   "V50"         -> la vitesse vaut 50
//   "M2:10100110" -> le mix 2 (pour le buzzer 2), 8 cases
async function envoyer(message) {
    console.log('Envoi :', message);
    if (!writer) return; // pas encore connecté

    // and we send the message through serial ----v
    await writer.write(new TextEncoder().encode(message + '\n'));
}


// ---------- Connexion ----------
connectBtn.addEventListener('click', async function () {
    try {
        // Request the user to select the Arduino Uno serial/COM port
        port = await navigator.serial.requestPort();
        await port.open({ baudRate: 9600 }); // même vitesse que Serial.begin(9600)
        writer = port.writable.getWriter();

        // when connected, we can disable connect button
        connectBtn.disabled = true;
        output.innerHTML += `<li>Connected to port!</li>`;

        // Attention : le Uno redémarre quand on ouvre le port (~2 secondes).
        // On envoie donc la vitesse seulement quand il a dit "PRET" (voir plus bas).

        // Read incoming data from the Arduino Uno
        while (port.readable) {
            const textDecoder = new TextDecoderStream();
            const readableStreamClosed = port.readable.pipeTo(textDecoder.writable);
            const reader = textDecoder.readable.getReader();
            let texte = ''; // morceaux reçus en attente d'un "\n"

            try {
                while (true) {
                    const { value, done } = await reader.read();
                    if (done) break;

                    // Les données peuvent arriver en morceaux :
                    // on attend d'avoir une ligne complète avant de l'afficher
                    texte += value;
                    const lignes = texte.split('\n');
                    texte = lignes.pop(); // la dernière ligne n'est pas finie
                    lignes.forEach(ligne => {
                        ligne = ligne.trim();
                        output.innerHTML += `<li>new message: ${ligne}</li>`;

                        // Le Uno a fini de redémarrer : on lui envoie la vitesse du slider
                        if (ligne === 'PRET') {
                            envoyer('V' + slider.value);
                        }
                    });
                }

            // we keep this to handle the errors that might happen
            } catch (error) {
                console.error(error);
            } finally {
                reader.releaseLock();
            }
        }
    } catch (err) {
        console.error("Error opening serial port:", err);
    }
});


// ---------- Les 4 boutons de couleur ----------
boutonsCouleur.forEach(bouton => {
    bouton.addEventListener('click', () => {
        envoyer('B' + numeroBuzzer[bouton.id]); // ex : "Red" -> "B1"
    });
});


// ---------- Le slider (vitesse) ----------
slider.addEventListener('change', () => {
    envoyer('V' + slider.value); // ex : "V75"
});


// ---------- Les 4 mix ----------
// Fabrique le message d'un mix, ex : "M2:10100110"
function messageDuMix(mix) {
    const numero = mix.dataset.mix; // 1, 2, 3 ou 4 (vient de data-mix)
    const cases = mix.querySelectorAll('input[type="checkbox"]');

    // On transforme chaque case en 1 (cochée) ou 0 (vide)
    let motif = '';
    cases.forEach(c => {
        motif += c.checked ? '1' : '0';
    });

    return 'M' + numero + ':' + motif;
}

// Les 4 petits boutons : envoient leur mix, mais la musique ne démarre pas
// (elle démarre seulement avec le gros bouton)
mixes.forEach(mix => {
    const boutonValider = mix.querySelector('.valider');

    boutonValider.addEventListener('click', () => {
        envoyer(messageDuMix(mix));
    });
});


// ---------- Le gros bouton : valide tous les mix et lance la musique ----------
//   "P" -> lance la musique depuis le début (Play)
const boutonPrincipal = document.querySelector('#boutonPrincipal');

boutonPrincipal.addEventListener('click', async () => {
    // On envoie les 4 mix, puis on lance
    for (const mix of mixes) {
        await envoyer(messageDuMix(mix));
    }
    await envoyer('P');
});


// ---------- Le bouton stop : arrête et remet tout à zéro ----------
//   "R" -> arrête la musique, coupe les buzzers et efface les 4 mix (Reset)
const boutonStop = document.querySelector('#boutonStop');

boutonStop.addEventListener('click', async () => {
    await envoyer('R');
    await envoyer('S'); // arrête la musique

    // On décoche aussi toutes les cases sur la page
    document.querySelectorAll('.mix input[type="checkbox"]').forEach(c => {
        c.checked = false;
    });
});
