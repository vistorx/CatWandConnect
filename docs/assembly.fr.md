# Assembler le robot

Ce guide réunit les pièces imprimées, les servos et l'électronique en un robot qui fonctionne.

> [!IMPORTANT]
> **Installe d'abord le logiciel** ([étape 3](install.fr.md)). Le robot doit démarrer pour mettre les servos en position pendant le montage.

## Ce qu'il te faut

| Qté | Pièce | Remarques |
|-----|-------|-----------|
| 1 | Roulement de skateboard 608 | |
| 27–28 | Billes de 6 mm | Entre le plateau supérieur et le plateau rotor. Les billes d'airsoft font très bien l'affaire. |
| 19 | Vis M3 | Longueurs bientôt. 8 vis identiques pour les deux servos (4 chacun) ; 1 pour l'axe du servo horizontal (à travers le plateau rotor et le palonnier) ; 1 pour l'axe du servo vertical (à travers le bras et le palonnier) ; 4 pour le plateau supérieur ; 2 pour le support de roulement ; 3 pour la porte. |
| 2 | Palonniers de servo | Fournis avec les servos. |
| | Adhésif double face | Pour la carte de déclenchement USB-C. |
| | Graisse | Pour la gouttière des billes, par exemple de la graisse au lithium ou au PTFE. |
| 1 | Canne à pêche pour chat | N'importe quelle canne dont la tige rentre dans le bras. |
| | Les pièces imprimées, l'électronique câblée | De l'[étape 1](print.fr.md) et de l'[étape 2](wiring.fr.md), avec le firmware installé ([étape 3](install.fr.md)). |

Outils : un tournevis pour vis M3.

**[Liste complète des pièces →](bom.fr.md)**

## Étapes

<!-- video: assembling the robot -->
Photos et vidéo bientôt.

### 1. Retire les supports

Retire tous les supports d'impression.

<!-- photo: step 1 -->

### 2. Monte le servo horizontal

Insère le servo horizontal dans la pince et pose ses 4 vis M3.

<!-- photo: step 2 -->

### 3. Colle la carte de déclenchement

Colle la carte de déclenchement USB-C (la carte qui amène le 20 V) dans le plateau supérieur avec de l'adhésif double face.

<!-- photo: step 3 -->

### 4. Pose le plateau supérieur

Positionne le plateau supérieur en faisant passer les deux fils d'alimentation par le trou, dans la cavité technique, avec le convertisseur abaisseur dans le logement prévu pour lui. Pose les 4 vis M3 du plateau supérieur.

<!-- photo: step 4 -->

### 5. Branche l'électronique

Dans la cavité technique, pose les connecteurs à levier (Wago) et branche l'électronique : l'alimentation de la carte et le servo horizontal. Branche ensuite le chargeur USB-C : le robot démarre et le servo se met en position (neutre).

<!-- photo: step 5 -->

### 6. Mets un palonnier dans le plateau rotor

Insère un palonnier de servo dans le plateau rotor.

<!-- photo: step 6 -->

### 7. Graisse et pose les billes

Graisse la gouttière à billes du plateau supérieur et dépose-y les 27 à 28 billes.

<!-- photo: step 7 -->

### 8. Pose le plateau rotor

Emboîte le plateau rotor, avec son palonnier, sur le servo horizontal, et visse la vis M3 à travers le plateau rotor et le palonnier dans l'axe du servo.

<!-- photo: step 8 -->

### 9. Enfonce le roulement

Enfonce le roulement 608 dans le support de roulement.

<!-- photo: step 9 -->

### 10. Mets l'autre palonnier dans le bras

Insère l'autre palonnier de servo dans le bras.

<!-- photo: step 10 -->

### 11. Monte le servo vertical

Installe le servo vertical sur le plateau rotor et pose ses 4 vis M3.

<!-- photo: step 11 -->

### 12. Branche le servo vertical et pose le bras

Branche le servo vertical pour qu'il se mette en position. Conseillé : sur la page du robot, amène l'axe vertical à sa position la plus basse, puis emboîte le bras, avec son palonnier, sur le servo vertical et visse-le dans l'axe du servo avec une vis M3.

<!-- photo: step 12 -->

### 13. Pose le support de roulement

Place le roulement et son support sur le bras et pose les 2 vis M3 qui le tiennent.

<!-- photo: step 13 -->

### 14. Ferme la porte

Range l'électronique dans la cavité technique et ferme-la avec la porte et ses 3 vis M3.

<!-- photo: step 14 -->

Lance ensuite la [Calibration](install.fr.md#calibrer) depuis l'écran principal du robot, pour qu'il apprenne les limites de course de ton montage.

Ensuite, en option : [Configurer Home Assistant](home-assistant.fr.md). Ou joue, tout simplement !
