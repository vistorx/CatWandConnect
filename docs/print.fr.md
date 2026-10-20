# Imprimer les pièces

Ce guide te permet d'obtenir les sept pièces imprimées du mécanisme Mk3, prêtes à assembler.

## Ce que tu vas fabriquer

Le **Mk3** est autonome : la carte et les cartes d'alimentation tiennent dans le robot, dans la cavité technique de la pince fermée par une porte imprimée, et un seul connecteur USB-C l'alimente. Tous les fichiers sont dans [`stls/mk3/`](../stls/mk3/), et [`catwandconnect-mk3.3mf`](../stls/mk3/catwandconnect-mk3.3mf) contient le plateau complet, déjà orienté.

| Pièce | Fichier |
|-------|---------|
| Bras | [`catwandconnect-mk3-arm.stl`](../stls/mk3/catwandconnect-mk3-arm.stl) |
| Support de roulement | [`catwandconnect-mk3-bearing_holder.stl`](../stls/mk3/catwandconnect-mk3-bearing_holder.stl) |
| Pince | [`catwandconnect-mk3-clamp.stl`](../stls/mk3/catwandconnect-mk3-clamp.stl) |
| Porte | [`catwandconnect-mk3-door.stl`](../stls/mk3/catwandconnect-mk3-door.stl) |
| Plateau rotor | [`catwandconnect-mk3-rotor_plate.stl`](../stls/mk3/catwandconnect-mk3-rotor_plate.stl) |
| Vis | [`catwandconnect-mk3-screw.stl`](../stls/mk3/catwandconnect-mk3-screw.stl) |
| Plateau supérieur | [`catwandconnect-mk3-top_plate.stl`](../stls/mk3/catwandconnect-mk3-top_plate.stl) |

![Les sept pièces du Mk3 tranchées sur un plateau dans PrusaSlicer, dans leur orientation d'impression, avec les supports de la pince en vert.](../media/screenshots/prusa-slicer-plate.jpg)

Tu veux modifier une pièce ? Les sources CAO, exportées d'Onshape en STEP, sont dans [`cads/mk3/`](../cads/mk3/) : un fichier par pièce, et [`catwandconnect-mk3.step`](../cads/mk3/catwandconnect-mk3.step) pour le mécanisme complet. N'importe quel logiciel de CAO (FreeCAD, Fusion, Onshape…) les ouvre.

## Ce qu'il te faut

| Quoi | Remarques |
|------|-----------|
| Une imprimante 3D | Un plateau de 300 × 300 mm prend toutes les pièces d'un coup ; une imprimante plus petite y arrive en plusieurs plateaux. Pas d'imprimante ? N'importe quel service d'impression ou fablab peut les imprimer. |
| Du filament | PETG ou ABS (ou équivalent) conseillé ; le PLA peut faire l'affaire. |
| Un trancheur | PrusaSlicer ou un autre. |

**[Liste complète des pièces →](bom.fr.md)**

## Étapes

<!-- video: printing and cleaning the parts -->
Photos et vidéo bientôt.

### 1. Choisis la matière

Le PETG ou l'ABS (ou un équivalent) est conseillé : plus résistant quand le chat attaque le bras. Le PLA peut aussi faire l'affaire.

<!-- photo: step 1 -->

### 2. Règle le trancheur

- **Remplissage :** gyroïde, 25 %.
- **Parois :** 3.
- **Couches du dessus et du dessous :** 6 chacune.
- **Hauteur de couche :** 0,2 mm, avec une buse de 0,4 mm.

<!-- photo: step 2 -->

### 3. Répartis les pièces sur plusieurs plateaux

Imprime les pièces en plusieurs plateaux, même si ton imprimante peut toutes les prendre d'un coup (300 × 300 mm) : une impression ratée ne coûte alors qu'un plateau.

<!-- photo: step 3 -->

### 4. Oriente les pièces

Imprime le plateau supérieur et le plateau rotor avec leur gouttière à billes tournée vers le plateau de l'imprimante. Oriente les autres pièces comme dans le [`.3mf`](../stls/mk3/catwandconnect-mk3.3mf).

<!-- photo: step 4 -->

### 5. Des supports pour la pince seulement

Aucune pièce n'a besoin de supports, sauf la pince ; la porte s'imprime sans. Sur la pince, mets des supports sous ses surfaces horizontales uniquement : dans PrusaSlicer, active les supports avec un **seuil de surplomb de 5°** pour qu'ils ne remplissent pas ses pentes ni ses trous.

<!-- photo: step 5 -->

### 6. Nettoie les pièces

Retire les supports de la pince, et nettoie les fils et la patte d'éléphant dans la gouttière à billes pour que les billes roulent librement. Si une pièce ne rentre pas, ponce-la plutôt que de forcer.

<!-- photo: step 6 -->

Des réglages qui marchent bien sur ton imprimante ? Partage-les sur [Discord](https://discord.gg/Rbqge3v9dk). Une pièce imprimée casse ? Réimprime-la : c'est tout l'intérêt d'un design ouvert.

Ensuite : [Câbler l'électronique](wiring.fr.md).
