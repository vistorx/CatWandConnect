# Câbler l'électronique

Comment câbler les deux servos et l'alimentation à la carte. Le firmware pilote les servos avec des impulsions PWM à 50 Hz ; la carte envoie seulement les signaux et n'alimente jamais les servos.

## Composants

| Qté | Pièce | Remarques |
|-----|-------|-----------|
| 1 | Carte de développement ESP32, ESP32-S3 ou ESP32-C3 | N'importe quelle carte avec USB, à partir d'environ 3 €. L'USB-C est pratique. |
| 2 | Servos MG996R | Servos de taille standard à pignons métalliques. |
| 1 | Alimentation 5 V, 3 A ou plus | Alimente les servos et la carte. |
| 1 | Condensateur, 470–1000 µF, 10 V ou plus | Facultatif, contre les redémarrages et les à-coups. |
| 1 | Bornier, connecteurs type Wago ou plaque à trous | Les rails +5 V et GND. |
| | Fils, connecteurs Dupont | Assez épais pour le 5 V et la masse des servos. |

## Cartes compatibles

N'importe quelle carte de développement ESP32, ESP32-S3 ou ESP32-C3. Les cartes ESP8266 ne sont pas compatibles ([ADR 0008](adr/0008-no-esp8266-support.md), en anglais).

## Broches par défaut

| Puce | Servo horizontal | Servo vertical |
|------|------------------|----------------|
| ESP32 | GPIO 18 | GPIO 19 |
| ESP32-S3 | GPIO 4 | GPIO 5 |
| ESP32-C3 | GPIO 4 | GPIO 5 |

Sur les S3 et C3, les GPIO 18 et 19 servent à l'USB, d'où des broches par défaut différentes. Pour câbler autrement, choisis les broches dans **Réglages → Matériel → Câblage** ; le robot redémarre pour les appliquer. La liste ne propose que les broches sans danger sur ta puce.

## Câblage

Le robot fonctionne avec une seule alimentation 5 V qui alimente les deux servos et la carte.

Chaque MG996R a trois fils : **orange** (signal), **rouge** (+5 V) et **marron** (masse).

| Fil | À brancher sur |
|-----|----------------|
| Rouge des servos | +5 V |
| Marron des servos | GND |
| Orange du servo horizontal | GPIO 18 (S3/C3 : GPIO 4) |
| Orange du servo vertical | GPIO 19 (S3/C3 : GPIO 5) |
| Broche 5V / VIN de la carte | +5 V |
| Broche GND de la carte | GND |

Utilise un petit bornier, un connecteur type Wago ou une plaque à trous comme « rails » +5 V et GND sur lesquels tout se branche.

![Câblage : une alimentation 5 V alimente un rail +5 V et un rail GND ; les fils rouge et marron des deux servos et les broches 5V et GND de la carte se branchent sur les rails ; le GPIO 18 va au fil orange du servo horizontal et le GPIO 19 à celui du servo vertical ; un condensateur est placé entre les rails, près des servos.](images/wiring.svg)

- Une alimentation 5 V de **3 A ou plus** alimente tout ; la carte prend son alimentation sur sa broche **5V/VIN**.
- Ne branche l'USB de la carte sur un ordinateur que pour flasher ou lire les logs. La plupart des cartes ont une diode qui empêche le 5 V externe de remonter vers l'ordinateur, mais pas toutes : si tu n'es pas sûr de la tienne, débranche d'abord l'alimentation 5 V.

## Règles

- **Masse commune.** Le GND de la carte doit être relié au − de l'alimentation, sinon les servos tremblent ou ignorent le signal.
- **N'alimente jamais les servos par la carte.** Ni par sa broche 3,3 V, ni par sa broche 5 V quand elle est alimentée par l'USB. Un MG996R peut tirer plus de 1 A quand il force.
- **Dimensionne l'alimentation.** Prévois 3 A ou plus pour deux MG996R. Une alimentation trop faible fait redémarrer la carte quand les deux servos démarrent ensemble : on voit souvent le robot redémarrer quand la baguette bouge.
- **Fils d'alimentation épais.** Garde les fils 5 V et de masse vers les servos courts et assez épais.
- **Condensateur.** Si la carte redémarre ou si les mouvements saccadent, ajoute un condensateur (470–1000 µF, 10 V ou plus) sur l'alimentation des servos, près des servos.

## Plage d'impulsions des servos

Les valeurs par défaut viennent de la preuve de concept : 400–2200 µs pour le servo horizontal et 600–2400 µs pour le vertical, pour 0° à 180°. Si tes servos ne vont pas au bout de leur course, ou grésillent en butée, ajuste la plage d'impulsions dans **Réglages → Matériel → Câblage**. La même section inverse un axe dont le sens est à l'envers. Les deux changements effacent la calibration : relance-la ensuite.

Ensuite : [Installer le logiciel](install.fr.md).
