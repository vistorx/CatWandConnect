# Assembler le mécanisme

> **Bientôt.** Un guide pas à pas avec photos sera rédigé à partir du montage de référence. En attendant, le brouillon ci-dessous donne l'ordre, reconstitué à partir des pièces et de la liste des vis, et pas encore vérifié sur un vrai montage. Corrections et photos bienvenues.

> **Mk3 :** ce brouillon a été écrit pour le Mk2 et n'est pas encore vérifié sur le Mk3. Le Mk3 est autonome : la carte et les cartes d'alimentation se placent dans la cavité technique de la pince, que la porte imprimée referme, et le connecteur USB-C est le seul qui reste dehors.

Il te faut les [pièces imprimées](print.fr.md) et les pièces ci-dessous. Les servos se câblent dans [Câbler l'électronique](wiring.fr.md).

## Pièces à acheter

| Qté | Pièce | Remarques |
|-----|-------|-----------|
| 1 | Roulement de skateboard 608 | |
| 27–28 | Billes de 6 mm | Entre le plateau supérieur et le plateau rotor. Les billes d'airsoft de 6 mm font très bien l'affaire. Le Mk2 utilise à la place 16 à 24 billes en acier de 4 mm. |
| 4 | Vis M3×8 à tête bombée | Fixation du servo horizontal |
| 2 | Vis M3×25 à tête bombée | Plateau inférieur de l'axe horizontal sur le chemin de billes |
| 1 | Vis M3×8 à tête fraisée | Plateau de l'arbre du servo horizontal |
| 1 | Vis M3×20 à tête fraisée | Arbre du servo vertical |
| 4 | Vis M3×8 à tête bombée | Fixation du servo vertical |
| 3 | Vis M3×30 à tête fraisée | Roulement sur l'arbre vertical |
| 1 | Une canne à pêche pour chat | N'importe quelle canne dont la tige rentre dans le bras |


<details>
<summary><b>Brouillon de l'ordre d'assemblage</b></summary>

### Avant de commencer

- [Imprime](print.fr.md) toutes les pièces de [`stls/mk3/`](../stls/mk3/) (ou de [`stls/mk2/`](../stls/mk2/) pour le mécanisme précédent).
- Nettoie le chemin de billes et le logement du roulement (fils, patte d'éléphant) pour que les billes roulent librement.
- Branche chaque servo sur une carte alimentée et flashée, et laisse le robot se ranger une fois : les deux servos sont alors à leur position de repos avant que tu montes le bras. Le bras reste ainsi centré dans sa course.

### Étapes

1. **Roulement.** Enfonce le roulement 608 dans son logement. Il doit affleurer et tourner sans frotter.
2. **Chemin de billes.** Dépose les billes de 6 mm dans le chemin entre le plateau supérieur et le plateau rotor : 27 à 28, jusqu'à ce que le plateau tourne en douceur, sans jeu. Sur le Mk2 : 16 à 24 billes en acier de 4 mm.
3. **Servo horizontal.** Fixe-le avec quatre vis M3×8 à tête bombée, puis fixe le plateau de l'arbre du servo horizontal avec la vis M3×8 à tête fraisée.
4. **Axe horizontal.** Fixe le plateau inférieur de l'axe horizontal sur le chemin de billes avec les deux vis M3×25 à tête bombée.
5. **Servo vertical.** Fixe-le avec quatre vis M3×8 à tête bombée, et fixe son arbre avec la vis M3×20 à tête fraisée.
6. **Roulement sur l'arbre vertical.** Fixe-le avec les trois vis M3×30 à tête fraisée.
7. **Bras et baguette.** Monte le bras et attache la baguette.
8. **Pince.** Fixe le robot en place avec la pince et sa vis.

### Vérifications

- Hors tension, les deux axes tournent à la main sans point dur, et le roulement ne frotte nulle part.
- Aucune vis n'écrase une pièce imprimée. Serre jusqu'au contact, pas plus.
- Fais passer les câbles des servos de façon qu'ils ne puissent pas s'accrocher quand les axes bougent.
- Mets sous tension et reste à côté : la baguette doit rejoindre doucement sa position de repos. Essaie ensuite le joystick, doucement. Voir la [checklist de recette](acceptance-checklist.fr.md).

</details>

Ensuite : [Câbler l'électronique](wiring.fr.md).
