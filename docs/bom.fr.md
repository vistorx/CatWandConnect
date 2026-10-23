# Liste complète des pièces

Tout ce qu'il te faut pour fabriquer un CatWandConnect, au même endroit. Chaque guide ne liste que ce dont son étape a besoin.

## Pièces imprimées

Les sept pièces du mécanisme Mk3, imprimées à l'[étape 1](print.fr.md). Les fichiers sont dans [`stls/mk3/`](../stls/mk3/), et [`catwandconnect-mk3.3mf`](../stls/mk3/catwandconnect-mk3.3mf) contient le plateau complet.

| Qté | Pièce | Fichier |
|-----|-------|---------|
| 1 | Bras | [`catwandconnect-mk3-arm.stl`](../stls/mk3/catwandconnect-mk3-arm.stl) |
| 1 | Support de roulement | [`catwandconnect-mk3-bearing_holder.stl`](../stls/mk3/catwandconnect-mk3-bearing_holder.stl) |
| 1 | Pince | [`catwandconnect-mk3-clamp.stl`](../stls/mk3/catwandconnect-mk3-clamp.stl) |
| 1 | Porte | [`catwandconnect-mk3-door.stl`](../stls/mk3/catwandconnect-mk3-door.stl) |
| 1 | Plateau rotor | [`catwandconnect-mk3-rotor_plate.stl`](../stls/mk3/catwandconnect-mk3-rotor_plate.stl) |
| 1 | Vis | [`catwandconnect-mk3-screw.stl`](../stls/mk3/catwandconnect-mk3-screw.stl) |
| 1 | Plateau supérieur | [`catwandconnect-mk3-top_plate.stl`](../stls/mk3/catwandconnect-mk3-top_plate.stl) |

## Électronique

Câblée à l'[étape 2](wiring.fr.md).

| Qté | Pièce | Remarques |
|-----|-------|-----------|
| 1 | Carte de développement ESP32, ESP32-S3 ou ESP32-C3 | N'importe quelle carte avec USB, à partir d'environ 3 €. Pas d'ESP8266 ([ADR 0008](adr/0008-no-esp8266-support.md), en anglais). |
| 2 | Servos MG996R (parfois vendus sous le nom SG996R) | Servos de taille standard à pignons métalliques, 180°. Chacun est livré avec ses palonniers. |
| 1 | Chargeur USB-C PD, 30 W minimum, 45 W conseillé | Doit proposer le 20 V (USB Power Delivery). |
| 1 | Carte de déclenchement USB-C PD, réglée sur 20 V | Demande 20 V au chargeur. |
| 1 | Convertisseur abaisseur, sortie 5 V, 5 A ou plus | Un 8 A donne de la marge. |
| 1 | Condensateur, 470–1000 µF, 10 V ou plus | Facultatif, contre les redémarrages et les à-coups. |
| 2 | Connecteurs à levier (type Wago 221), 2 entrées | Les rails +5 V et GND. |
| 2 | Rallonges de servo, environ 30 cm | Coupées en deux pour les servos et la carte. |
| 40 cm | Fil d'environ 0,5 mm² (AWG 20) et connecteurs Dupont femelles | 2 × 5 cm et 2 × 15 cm de fil, voir le [guide de câblage](wiring.fr.md#monter-pas-à-pas). |

Des exemples d'annonces pour les servos, la carte de déclenchement et le convertisseur sont dans les [Exemples de composants](wiring.fr.md#exemples-de-composants) du guide de câblage. Pour [installer le logiciel](install.fr.md) (étape 3), il ne faut rien de plus qu'un câble USB de données et un ordinateur avec Chrome ou Edge.

## Quincaillerie d'assemblage

Utilisée à l'[étape 4](assembly.fr.md).

| Qté | Pièce | Remarques |
|-----|-------|-----------|
| 1 | Roulement de skateboard 608 | |
| 27–28 | Billes de 6 mm | Entre le plateau supérieur et le plateau rotor. Les billes d'airsoft font très bien l'affaire. |
| 11 | Vis M3×8 tête bombée | 8 pour les deux servos (4 chacun), souvent fournies avec eux ; 3 pour la porte. |
| 5 | Vis M3×20 tête fraisée | 4 pour le plateau supérieur ; 1 pour l'axe du servo vertical. |
| 1 | Vis M3×14 tête fraisée | Pour l'axe du servo horizontal. |
| 2 | Vis M3×30 tête fraisée | Pour le support de roulement. |
| 2 | Palonniers de servo | Fournis avec les servos. |
| | Adhésif double face | Tient la carte de déclenchement USB-C dans le plateau supérieur. |
| | Graisse | Pour la gouttière des billes, par exemple de la graisse au lithium ou au PTFE. |
| 1 | Canne à pêche pour chat | N'importe quelle canne dont la tige rentre dans le bras. |

## Outils

- Une imprimante 3D, ou un service d'impression ou un fablab ([étape 1](print.fr.md)).
- Un fer à souder et un multimètre ([étape 2](wiring.fr.md)).
- Un ordinateur avec Chrome ou Edge, et un câble USB de données ([étape 3](install.fr.md)).
- Un tournevis pour vis M3 ([étape 4](assembly.fr.md)).

## Améliorations facultatives

| Pièce | Pourquoi |
|-------|----------|
| 2 servos DS3218, version 180° | Bien plus solides et avec beaucoup plus de couple que le MG996R, même taille standard. Prends avec eux le convertisseur 8 A et un chargeur de 45 W. Voir [Servos plus robustes : DS3218](wiring.fr.md#servos-plus-robustes--ds3218). |
