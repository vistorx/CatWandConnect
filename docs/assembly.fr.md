# Guide d'assemblage : mécanisme Mk2

> **Bientôt.** Un guide pas à pas avec photos sera rédigé à partir du montage de référence. En attendant, le brouillon ci-dessous donne l'ordre, reconstitué à partir des pièces et de la liste des vis, et pas encore vérifié sur un vrai montage. Corrections et photos bienvenues.

Il te faut les pièces imprimées et mécaniques de [Matériel](hardware.fr.md), et le câblage de [Électronique](electronics.fr.md).

<details>
<summary><b>Brouillon de l'ordre d'assemblage</b></summary>

### Avant de commencer

- Imprime toutes les pièces de [`stls/mk2/`](../stls/mk2/).
- Nettoie le chemin de billes et le logement du roulement (fils, patte d'éléphant) pour que les billes roulent librement.
- Branche chaque servo sur une carte alimentée et flashée, et laisse le robot se ranger une fois : les deux servos sont alors à leur position de repos avant que tu montes le bras. Le bras reste ainsi centré dans sa course.

### Étapes

1. **Roulement.** Enfonce le roulement 608 dans son logement. Il doit affleurer et tourner sans frotter.
2. **Chemin de billes.** Dépose les billes en acier de 4 mm dans le chemin, en commençant par 16 et en en ajoutant jusqu'à ce que le plateau tourne en douceur, sans jeu. Il en faut en général 16 à 24.
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
