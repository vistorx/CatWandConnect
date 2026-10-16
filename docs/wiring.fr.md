# Câbler l'électronique

Comment câbler les deux servos et la chaîne d'alimentation à la carte. Le firmware pilote les servos avec des impulsions PWM à 50 Hz ; la carte envoie seulement les signaux et n'alimente jamais les servos.

## Composants

| Qté | Pièce | Remarques |
|-----|-------|-----------|
| 1 | Carte de développement ESP32, ESP32-S3 ou ESP32-C3 | N'importe quelle carte avec USB, à partir d'environ 3 €. L'USB-C est pratique. |
| 2 | Servos MG996R (parfois vendus sous le nom SG996R) | Servos de taille standard à pignons métalliques. Version plus robuste : [DS3218](#servos-plus-robustes--ds3218). |
| 1 | Chargeur USB-C PD, 30 W minimum, 45 W conseillé | Doit proposer le 20 V (USB Power Delivery). Voir [Choisir le chargeur](#choisir-le-chargeur). |
| 1 | Carte de déclenchement USB-C PD (*PD trigger*), réglée sur 20 V | Demande 20 V au chargeur et les transmet. |
| 1 | Convertisseur abaisseur (*buck*), sortie 5 V, 5 A ou plus | Transforme le 20 V en 5 V pour les servos et la carte. Un 8 A donne de la marge. |
| 1 | Condensateur, 470–1000 µF, 10 V ou plus | Facultatif, contre les redémarrages et les à-coups. |
| 1 | Bornier, connecteurs type Wago ou plaque à trous | Les rails +5 V et GND. |
| | Fils, connecteurs Dupont | Assez épais pour le 5 V et la masse des servos. |

Une simple alimentation 5 V de 3 A ou plus marche aussi, à la place du chargeur, de la carte de déclenchement et du convertisseur : branche-la directement sur les rails +5 V et GND.

### Exemples de composants

Ce sont les composants du montage de référence. Les annonces changent avec le temps et n'importe quel composant équivalent convient. Les liens sont des exemples, ni sponsorisés ni affiliés.

| Pièce | Exemples d'annonces (AliExpress) | Ce qu'il faut chercher |
|-------|----------------------------------|------------------------|
| Servo MG996R (parfois vendu sous le nom SG996R) | [Exemple](https://fr.aliexpress.com/item/1005012099566939.html) | Taille standard, pignons métalliques, 180°. |
| Servo DS3218 (version plus robuste) | [Exemple](https://fr.aliexpress.com/item/1005010498819386.html) | La version **180°**, pignons métalliques, classe 20 kg·cm. Voir [Servos plus robustes](#servos-plus-robustes--ds3218). |
| Carte de déclenchement USB-C PD | [Exemple 1](https://www.aliexpress.com/item/1005010610660644.html), [exemple 2](https://www.aliexpress.com/item/1005005461879717.html) | Entrée USB-C, PD3.0/PD3.1 (QC3.0 en bonus), sortie au choix de 5 à 48 V, 5 A. |
| Convertisseur abaisseur | [8 A](https://www.aliexpress.com/item/1005009516551580.html), [5 A](https://www.aliexpress.com/item/1005009289437405.html) | Entrée jusqu'à 25 V, sortie au choix ou réglable (3,3/5/9/12 V), réglée sur 5 V. |

### Servos plus robustes : DS3218

Le DS3218 coûte plus cher que le MG996R, mais il est bien plus solide et a beaucoup plus de couple (classe 20 kg·cm, pignons métalliques). Il a la même taille de servo standard, il devrait donc tenir dans les mêmes supports (pas encore vérifié sur le Mk2).

- Prends la version **180°**. Le DS3218 existe aussi en 270°, ce qui fausserait les angles.
- Sa plage d'impulsions est d'environ 500–2500 µs : règle-la dans **Réglages → Matériel → Câblage**, puis relance la calibration. Voir [Plage d'impulsions des servos](#plage-dimpulsions-des-servos).
- Il tire plus de courant quand il force, jusqu'à environ 2,5–3 A chacun. Avec deux DS3218, prends le convertisseur 8 A et un chargeur de 45 W.

## Cartes compatibles

N'importe quelle carte de développement ESP32, ESP32-S3 ou ESP32-C3. Les cartes ESP8266 ne sont pas compatibles ([ADR 0008](adr/0008-no-esp8266-support.md), en anglais).

## Broches par défaut

| Puce | Servo horizontal | Servo vertical |
|------|------------------|----------------|
| ESP32 | GPIO 18 | GPIO 19 |
| ESP32-S3 | GPIO 4 | GPIO 5 |
| ESP32-C3 | GPIO 4 | GPIO 5 |

Sur les S3 et C3, les GPIO 18 et 19 servent à l'USB, d'où des broches par défaut différentes. Pour câbler autrement, choisis les broches dans **Réglages → Matériel → Câblage** ; le robot redémarre pour les appliquer. La liste ne propose que les broches sans danger sur ta puce.

## Alimentation

Le robot fonctionne avec un chargeur USB-C, à travers deux petites cartes :

1. **Chargeur USB-C** → **carte de déclenchement PD**, par un câble USB-C. La carte de déclenchement demande 20 V au chargeur.
2. **Carte de déclenchement PD** → **convertisseur abaisseur** : le 20 V entre dans le convertisseur.
3. **Convertisseur abaisseur** → **rails +5 V et GND** : sa sortie 5 V alimente les deux servos et la carte.

Règle les deux cartes **avant** de brancher les servos ou la carte :

1. Règle la carte de déclenchement sur **20 V** (avec son pont de soudure, son cavalier ou son bouton, comme l'explique son annonce), branche-la au chargeur et vérifie sa sortie au multimètre : environ 20 V.
2. Branche le convertisseur, règle sa sortie sur **5 V** et vérifie-la au multimètre : environ 5 V.
3. Seulement ensuite, branche la sortie du convertisseur sur les rails +5 V et GND.

**Ne choisis jamais une tension de déclenchement supérieure à l'entrée maximale du convertisseur.** Les convertisseurs donnés en exemple acceptent jusqu'à 25 V : 20 V est le bon choix, et 28 V ou plus les détruirait.

### Choisir le chargeur

Prends un bon chargeur USB-C PD qui propose le 20 V : **30 W minimum, 45 W c'est beaucoup mieux**. Deux MG996R peuvent tirer chacun plus de 1 A quand ils forcent, et les deux cartes perdent un peu de puissance en la convertissant. 30 W suffit ; 45 W laisse de la marge pour que le chargeur ne coupe pas quand les deux servos démarrent ensemble.

## Câblage

La sortie 5 V du convertisseur abaisseur alimente les deux servos et la carte.

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

![Câblage : un chargeur USB-C (30–45 W) alimente une carte de déclenchement PD réglée sur 20 V, qui alimente un convertisseur abaisseur réglé sur 5 V ; le convertisseur alimente un rail +5 V et un rail GND ; les fils rouge et marron des deux servos et les broches 5V et GND de la carte se branchent sur les rails ; le GPIO 18 va au fil orange du servo horizontal et le GPIO 19 à celui du servo vertical ; un condensateur est placé entre les rails, près des servos.](images/wiring.svg)

- La sortie 5 V du convertisseur abaisseur alimente tout ; la carte prend son alimentation sur sa broche **5V/VIN**.
- Ne branche l'USB de la carte sur un ordinateur que pour flasher ou lire les logs. La plupart des cartes ont une diode qui empêche le 5 V externe de remonter vers l'ordinateur, mais pas toutes : si tu n'es pas sûr de la tienne, débranche d'abord le chargeur USB-C.

## Règles

- **Masse commune.** Le GND de la carte doit être relié au − de la sortie du convertisseur, sinon les servos tremblent ou ignorent le signal.
- **N'alimente jamais les servos par la carte.** Ni par sa broche 3,3 V, ni par sa broche 5 V quand elle est alimentée par l'USB. Un MG996R peut tirer plus de 1 A quand il force.
- **Dimensionne l'alimentation.** Prévois 5 A ou plus en 5 V pour deux MG996R : un convertisseur de 5 A ou plus, un chargeur de 30 W ou plus (ou une simple alimentation 5 V de 3 A ou plus). Une alimentation trop faible fait redémarrer la carte quand les deux servos démarrent ensemble : on voit souvent le robot redémarrer quand la baguette bouge.
- **Fils d'alimentation épais.** Garde les fils 5 V et de masse vers les servos courts et assez épais.
- **Condensateur.** Si la carte redémarre ou si les mouvements saccadent, ajoute un condensateur (470–1000 µF, 10 V ou plus) entre les rails +5 V et GND, près des servos.

## Plage d'impulsions des servos

Les valeurs par défaut viennent de la preuve de concept : 400–2200 µs pour le servo horizontal et 600–2400 µs pour le vertical, pour 0° à 180°. Si tes servos ne vont pas au bout de leur course, ou grésillent en butée, ajuste la plage d'impulsions dans **Réglages → Matériel → Câblage**. La même section inverse un axe dont le sens est à l'envers. Les deux changements effacent la calibration : relance-la ensuite.

Ensuite : [Installer le logiciel](install.fr.md).
