# Installer le logiciel

Ce guide installe le firmware sur la carte depuis ton navigateur, connecte le robot à ton Wi-Fi et vérifie que les servos bougent. Environ 15 minutes, aucun outil à installer.

<!-- github-only -->
[![CatWandConnect tutorial - Install firmware and first tests](https://i.ytimg.com/vi/rjmGGUYjKDc/hqdefault.jpg)](https://youtu.be/rjmGGUYjKDc)
<!-- /github-only -->
<iframe style="width:100%;aspect-ratio:16/9;border:0" src="https://www.youtube-nocookie.com/embed/rjmGGUYjKDc" title="CatWandConnect tutorial - Install firmware and first tests" allow="accelerometer; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>

La vidéo montre toutes les étapes ci-dessous ([voir sur YouTube](https://youtu.be/rjmGGUYjKDc)).

## Ce qu'il te faut

| Quoi | Remarques |
|------|-----------|
| L'électronique câblée et les deux servos | De l'[étape 2](wiring.fr.md). |
| Le chargeur USB-C | Celui du guide de câblage. |
| Un câble USB qui transporte les données | Certains câbles ne font que charger : si la carte n'apparaît pas, essaies-en un autre. |
| Un ordinateur avec Chrome ou Edge | Le flasher web a besoin de Web Serial, absent des autres navigateurs. |

<p align="center">
  <a href="https://vistorx.github.io/CatWandConnect/flasher/"><img src="../media/buttons/install.svg" alt="Installer depuis le navigateur" width="260"></a>
</p>

## Étapes

### 1. Branche les servos

Branche chaque servo sur la prise d'un câble de servo du guide de câblage : le fil marron du servo du côté du − (noir) du câble. Laisse le chargeur débranché.

<!-- photo: step 1 -->

### 2. Branche la carte

Branche la carte sur l'ordinateur en USB. Sa LED s'allume : la carte est alimentée.

La carte n'apparaît pas ensuite, ou sa LED reste éteinte ? Essaie un autre câble, puis vois [Installer avec le flasher web](troubleshooting.fr.md#installer-avec-le-flasher-web) pour le pilote de sa puce USB.

<!-- photo: step 2 -->

### 3. Ouvre le flasher web

Ouvre le [flasher web](https://vistorx.github.io/CatWandConnect/flasher/) dans **Chrome ou Edge** sur l'ordinateur.

> [!NOTE]
> Le flasher web installe par défaut la version **Stable**. Le canal **Bêta** reçoit les nouveautés en premier et peut encore contenir des bugs. Voir les [canaux de publication](releasing.md#channels) (en anglais).

<!-- photo: step 3 -->

### 4. Installe le firmware

Appuie sur **Connecter et installer**, choisis le port de la carte, puis suis les instructions. Le bon firmware pour ta puce est choisi pour toi. Ça prend environ une minute.

<!-- photo: step 4 -->

### 5. Connecte-le à ton Wi-Fi

Quand le flasher web te le demande, choisis ton Wi-Fi et saisis son mot de passe.

<details markdown="1">
<summary>Tu as passé cette étape, ou ça n'a pas marché ?</summary>

Alimente le robot : sans Wi-Fi configuré, il ouvre un hotspot **`CatWandConnect-XXXX`**. Connecte-toi à ce hotspot depuis ton téléphone. La page de configuration s'ouvre toute seule ; sinon, va sur `http://192.168.4.1/`. Choisis ton Wi-Fi et saisis son mot de passe, ou touche **Continuer sans Wi-Fi** pour utiliser le robot depuis le hotspot pour l'instant.

</details>

<!-- photo: step 5 -->

### 6. Ouvre la page du robot

Choisis **Visit device** dans le flasher web, ou ouvre depuis ton téléphone l'adresse qu'il affiche, par exemple **`http://catwandconnect-3f9a.local/`**.

<details markdown="1">
<summary>À propos de l'adresse, du nom et de l'écran d'accueil</summary>

- Avec le nom par défaut, l'adresse `.local` du robot se termine par 4 caractères qui lui sont propres, pour que deux robots ne se gênent jamais ; les robots installés avant la version 0.2.1 gardent `catwandconnect.local`.
- Ouverte par adresse IP, la page propose d'abord **Utilise l'adresse permanente du robot** : touche **Y aller** pour passer à l'adresse `.local`, qui reste valable quand l'IP change. Elle ne le propose que si ton téléphone ou ton ordinateur arrive à l'ouvrir ; sinon, vois [l'adresse `.local` ne s'ouvre pas](troubleshooting.fr.md#wi-fi).
- Donne un nom au robot dans les Réglages ; son adresse suit, par exemple `http://catwandconnect-salon.local/`.
- Sur un téléphone, la première visite montre comment ajouter le robot à ton écran d'accueil pour avoir une icône comme une appli (**Compris** masque l'astuce ; Réglages → Robot ou Aide → **Ajouter à ton écran d'accueil** la réaffiche).
- Réglages → Réseau affiche l'adresse IP du robot, et **Changer de réseau Wi-Fi** le fait passer sur un autre réseau plus tard (nouvelle box, déménagement).

</details>

<!-- photo: step 6 -->

### 7. Mets les servos en position neutre

Débranche le câble USB, puis branche le chargeur. Le robot démarre et les deux servos tournent au milieu de leur course : leur position neutre. Laisse-les ainsi et ne tourne pas leurs axes à la main : l'[assemblage](assembly.fr.md) monte les pièces dans cette position.

La page du robot se reconnecte toute seule au bout de quelques secondes.

<!-- photo: step 7 -->

### 8. Vérifie que les servos bougent

Sur la page du robot, fais glisser doucement le joystick : un servo tourne à gauche et à droite, l'autre en haut et en bas. Relâche pour arrêter. Jusqu'à la calibration, chaque axe reste dans une plage prudente autour du milieu (60° à 120°).

Appuie ensuite sur **Ranger** : les deux servos reviennent au milieu. Le robot est prêt à être assemblé.

<!-- photo: step 8 -->

## Calibrer

Fais-le une fois le robot [assemblé](assembly.fr.md). L'écran principal t'y invite. L'assistant te fait amener chaque axe aux extrémités de sa course sans danger et choisir la position de repos, puis il parcourt lentement les nouvelles limites pour que tu vérifies que rien ne touche. Une fois les limites enregistrées, il balaie chaque axe pendant que tu règles sa vitesse minimale et maximale, puis lui fait faire de courts mouvements pendant que tu règles son accélération (Passer garde une valeur telle quelle). D'ici là, les mouvements restent dans une plage prudente de 60° à 120° sur chaque axe.

Ensuite, **joue** : fais glisser le joystick pour bouger la baguette et relâche pour l'arrêter. Appuie sur **Ranger** pour la ranger.

## Mettre à jour, compiler depuis les sources

- **Mettre à jour un robot déjà installé :** quand une nouvelle version sort, l'écran principal du robot affiche *Version X disponible* avec **Nouveautés** et **Mettre à jour**. Touche **Mettre à jour**, puis **Installer maintenant** : ton téléphone télécharge la version pour la puce du robot et l'envoie au robot, qui redémarre dessus environ une minute plus tard. Garde la page ouverte jusqu'à ce qu'elle annonce le retour du robot. Les réglages et la calibration sont conservés. On retrouve tout ça dans **Réglages → Robot → Mise à jour du firmware**, où *Impossible de vérifier les mises à jour* veut dire que ton téléphone n'a pas accès à Internet (touche **Réessayer**). Termine ou arrête d'abord une Session de jeu ou une Calibration.
- **Stable ou Bêta :** **Réglages → Robot → Mise à jour du firmware → Canal de mise à jour** choisit les versions proposées au robot ([canaux de publication](releasing.md#channels)). Le robot garde ce choix après un redémarrage ou une mise à jour. Tant que tu n'as pas choisi, il suit la version installée : une bêta propose des bêtas.
- **Pas d'Internet sur ton téléphone** (par exemple sur le hotspot du robot) : télécharge à l'avance le fichier `-ota.bin` de ta puce dans les [versions publiées](https://github.com/vistorx/CatWandConnect/releases), puis envoie-le depuis **Réglages → Robot → Mise à jour du firmware → Installer depuis un fichier**.
- **Compiler depuis les sources** avec [PlatformIO](https://platformio.org/) ; [Bien démarrer](getting-started.fr.md) présente les outils :

  ```sh
  cd firmware
  pio run -e esp32 -t upload   # ou esp32s3 / esp32c3
  ```

Un problème ? Voir le [dépannage](troubleshooting.fr.md), puis [Obtenir de l'aide](../SUPPORT.fr.md).

Ensuite : [Assembler le robot](assembly.fr.md).
