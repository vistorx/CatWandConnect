# Installer le logiciel

## Installer le firmware

Aucun outil à installer : le firmware va sur la carte directement depuis ton navigateur. Tu peux l'installer sur une carte nue avant de [construire le robot](assembly.fr.md).

<p align="center">
  <a href="https://vistorx.github.io/CatWandConnect/flasher/"><img src="../media/buttons/install.svg" alt="Installer depuis le navigateur" width="340"></a>
</p>

1. **Ouvre** le [flasher web](https://vistorx.github.io/CatWandConnect/flasher/) dans **Chrome ou Edge** sur un ordinateur.
2. **Branche** la carte ESP32 en USB, avec un câble qui transporte les données.
3. **Installe :** appuie sur **Connecter et installer** et choisis le port de la carte. Le bon firmware pour ta puce est choisi pour toi.
4. **Connecte :** saisis ton Wi-Fi quand on te le demande, puis choisis **Visit device**.

> [!NOTE]
> Le flasher web installe par défaut la version **Stable**. Le canal **Bêta** reçoit les nouveautés en premier et peut encore contenir des bugs. Voir les [canaux de publication](releasing.md#channels) (en anglais).

## Mettre à jour, compiler depuis les sources

- **Mettre à jour un robot déjà installé :** télécharge le fichier `-ota.bin` de ta puce dans la [dernière version publiée](https://github.com/vistorx/CatWandConnect/releases), puis envoie-le depuis **Réglages → Robot → Mise à jour du firmware** du robot. Les réglages et la calibration sont conservés.
- **Compiler depuis les sources** avec [PlatformIO](https://platformio.org/) ; [Bien démarrer](getting-started.fr.md) présente les outils :

  ```sh
  cd firmware
  pio run -e esp32 -t upload   # ou esp32s3 / esp32c3
  ```

- **La carte n'apparaît pas ?** Essaie un autre câble, puis installe le pilote de sa puce USB : voir [Installer avec le flasher web](troubleshooting.fr.md#installer-avec-le-flasher-web) pour les liens de téléchargement.

## Premier démarrage

1. Si tu as saisi ton Wi-Fi dans le flasher web, passe à l'étape 3. Sinon, alimente le robot : sans Wi-Fi configuré, il ouvre un hotspot **`CatWandConnect-XXXX`**.
2. Connecte-toi à ce hotspot depuis ton téléphone. La page de configuration s'ouvre toute seule ; sinon, va sur `http://192.168.4.1/`. Choisis ton Wi-Fi et saisis son mot de passe, ou touche **Continuer sans Wi-Fi** pour utiliser le robot depuis le hotspot pour l'instant.
3. Reviens sur ton Wi-Fi habituel et ouvre **`http://catwandconnect.local/`** (ou l'adresse IP affichée sur la page de configuration).
4. **Calibre.** L'écran principal t'y invite. L'assistant te fait amener chaque axe aux extrémités de sa course sans danger et choisir la position de repos, puis il parcourt lentement les nouvelles limites pour que tu vérifies que rien ne touche. Une fois les limites enregistrées, il balaie chaque axe pendant que tu règles sa vitesse minimale et maximale, puis lui fait faire de courts mouvements pendant que tu règles son accélération (Passer garde une valeur telle quelle). D'ici là, les mouvements restent dans une plage prudente de 60° à 120° sur chaque axe.
5. **Joue.** Fais glisser le joystick pour bouger la baguette et relâche pour l'arrêter. Appuie sur **Ranger** pour la ranger.
6. **Personnalise-le.** Donne un nom au robot dans les Réglages ; son adresse suit, par exemple `http://catwandconnect-salon.local/`. Utilise « Ajouter à l'écran d'accueil » de ton navigateur pour avoir une icône comme une appli. Réglages → Réseau affiche l'adresse IP du robot, et **Changer de réseau Wi-Fi** le fait passer sur un autre réseau plus tard (nouvelle box, déménagement).

Un problème ? Voir le [dépannage](troubleshooting.fr.md), puis [Obtenir de l'aide](../SUPPORT.fr.md).

Ensuite, en option : [Configurer Home Assistant](home-assistant.fr.md).
