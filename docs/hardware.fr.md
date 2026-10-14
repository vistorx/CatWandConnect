# Matériel : mécanisme Mk2

https://github.com/user-attachments/assets/914092e7-1f09-4832-85f9-01eeb9a11499

<p align="center"><em>La présentation du Mk2 (50 s, animation 3D).</em></p>

Ce qu'il faut imprimer et acheter pour le Mk2, le mécanisme actuel. Les composants électroniques (carte, servos, alimentation) sont dans [Électronique](electronics.fr.md) ; le montage est dans le [guide d'assemblage](assembly.fr.md).

## Pièces imprimées

Le **Mk2** est le mécanisme actuel. Il est renforcé par un roulement de skateboard 608 et un chemin de billes en acier de 4 mm. Tous les fichiers sont dans [`stls/mk2/`](../stls/mk2/), et [`catwandconnect-mk2.3mf`](../stls/mk2/catwandconnect-mk2.3mf) contient le plateau complet.

| Pièce | Fichier |
|-------|---------|
| Bras | [`catwandconnect-mk2-arm.stl`](../stls/mk2/catwandconnect-mk2-arm.stl) |
| Roulement | [`catwandconnect-mk2-bearing.stl`](../stls/mk2/catwandconnect-mk2-bearing.stl) |
| Pince | [`catwandconnect-mk2-clamp.stl`](../stls/mk2/catwandconnect-mk2-clamp.stl) |
| Vis | [`catwandconnect-mk2-screw.stl`](../stls/mk2/catwandconnect-mk2-screw.stl) |
| Plateau rotor | [`catwandconnect-mk2-rotor_plate.stl`](../stls/mk2/catwandconnect-mk2-rotor_plate.stl) |
| Plateau supérieur | [`catwandconnect-mk2-top_plate.stl`](../stls/mk2/catwandconnect-mk2-top_plate.stl) |

## Conseils d'impression

> Des points de départ, pas encore vérifiés sur le montage de référence. Partage les réglages qui marchent pour toi sur [Discord](https://discord.gg/Rbqge3v9dk) et ils seront ajoutés ici.

- **Matière :** le PLA convient pour un robot qui reste à l'intérieur ; le PETG est plus résistant si le chat attaque le bras.
- **Solidité :** 3 parois ou plus et 20 à 30 % de remplissage. Le bras, la pince et les pièces qui tiennent les servos encaissent les efforts.
- **Chemin de billes :** imprime-le à plat avec une première couche fine, et retire les fils et la patte d'éléphant pour que les billes roulent librement.
- **Ajustement :** si le roulement ou un servo ne rentre pas, ponce la pièce plutôt que de forcer.
- Le [`.3mf`](../stls/mk2/catwandconnect-mk2.3mf) contient le plateau complet en un seul fichier.

## Pièces mécaniques

| Qté | Pièce | Remarques |
|-----|-------|-----------|
| 1 | Roulement de skateboard 608 | |
| 16–24 | Billes en acier de 4 mm | Adapte le nombre à la finition de ton impression. |
| 4 | Vis M3×8 à tête bombée | Fixation du servo horizontal |
| 2 | Vis M3×25 à tête bombée | Plateau inférieur de l'axe horizontal sur le chemin de billes |
| 1 | Vis M3×8 à tête fraisée | Plateau de l'arbre du servo horizontal |
| 1 | Vis M3×20 à tête fraisée | Arbre du servo vertical |
| 4 | Vis M3×8 à tête bombée | Fixation du servo vertical |
| 3 | Vis M3×30 à tête fraisée | Roulement sur l'arbre vertical |
| 1 | Une canne à pêche pour chat | N'importe quelle canne dont la tige rentre dans le bras |

Une pièce imprimée casse ? Réimprime-la : c'est tout l'intérêt d'un design ouvert.
