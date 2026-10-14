# Configurer Home Assistant

CatWandConnect fonctionne sans Home Assistant. Si tu l'utilises, le robot peut y apparaître comme un appareil avec un interrupteur de session de jeu et sa durée, un bouton Ranger, son activité, et la position et la vitesse maximale de chaque axe, prêt pour tes tableaux de bord et tes automatisations.

Il y a deux façons de le connecter. Choisis-en une, configure-la, puis copie le [tableau de bord tout prêt](#tableaux-de-bord-tout-prêts) qui va avec.

## Choisir ta connexion

| | Intégration native | Connexion MQTT |
|---|---|---|
| **Comment ça marche** | Home Assistant trouve le robot sur ton réseau et lui parle directement. | Le robot et Home Assistant s'échangent des messages par un *broker* MQTT qui tourne à côté de Home Assistant. |
| **Il te faut** | [HACS](https://hacs.xyz/), Home Assistant 2026.10 ou plus récent, le firmware 0.2.0-beta.2 ou plus récent sur le robot. | Un broker MQTT (l'application Mosquitto broker). Marche avec tout Home Assistant récent et toutes les versions du firmware. |
| **Installation** | 5 minutes environ : installer depuis HACS, cliquer sur **Ajouter** (et saisir le code PIN des réglages s'il y en a un). | Une dizaine de minutes : installer le broker, créer un utilisateur, ajouter l'intégration MQTT, saisir le broker dans le robot. |
| **Noms des entités** | Dans la langue de Home Assistant (français ou anglais). | En anglais. |
| **Idéal pour** | La plupart des propriétaires, surtout si tu n'utilises pas MQTT. | Ceux qui utilisent déjà MQTT (Zigbee2MQTT, Tasmota…), ou qui ne veulent pas de HACS. |

Les deux donnent les mêmes entités, suivent les mêmes règles de sécurité et restent sur ton réseau local, sans cloud. La connexion MQTT ne va pas disparaître : si elle marche déjà chez toi, garde-la.

> [!WARNING]
> N'utilise qu'une seule connexion par robot. Avec les deux, Home Assistant affiche le robot deux fois, comme deux appareils, et le second jeu d'entités reçoit des identifiants qui finissent par `_2` (`button.catwandconnect_stow_2`), que les tableaux de bord tout prêts et les automatisations ci-dessous n'utilisent pas. Pour changer, retire d'abord l'ancienne : pour la connexion MQTT, décoche **Activé** dans Réglages → Réseau → Home Assistant sur le robot, puis supprime l'appareil dans Home Assistant ; pour l'intégration native, supprime son entrée dans **Paramètres → Appareils et services → CatWandConnect**.

## Intégration native

L'intégration native s'installe avec [HACS](https://hacs.xyz/), la boutique communautaire de Home Assistant, depuis son propre dépôt, [`vistorx/ha-catwandconnect`](https://github.com/vistorx/ha-catwandconnect). Elle ne demande ni broker ni utilisateur en plus.

### Avant de commencer

- **Home Assistant 2026.10 ou plus récent** : **Paramètres → À propos** affiche la version.
- **HACS installé** : si **HACS** n'est pas dans la barre latérale de Home Assistant, suis d'abord le [guide d'installation de HACS](https://hacs.xyz/docs/use/) (en anglais).
- Le robot **installé et sur ton Wi-Fi** ([Installer le logiciel](install.fr.md)), sur le même réseau que Home Assistant, avec le **firmware 0.2.0-beta.2 ou plus récent** : sa page Infos (Réglages → Robot → Infos) affiche la version. Pour le mettre à jour, voir [Mettre à jour](install.fr.md#mettre-à-jour-compiler-depuis-les-sources).
- Un compte **administrateur** dans Home Assistant.

### 1. Installer l'intégration depuis HACS

1. Ouvre **HACS** dans la barre latérale.
2. Clique sur les trois points en haut à droite → **Dépôts personnalisés**.
3. Remplis :
   - **Dépôt** : `https://github.com/vistorx/ha-catwandconnect` ;
   - **Type** : **Intégration**.
4. Clique sur **Ajouter**, puis ferme la fenêtre.
5. Cherche **CatWandConnect** dans HACS, ouvre-le et clique sur **Télécharger**, puis encore sur **Télécharger** pour confirmer la version.
6. Redémarre Home Assistant : **Paramètres → Système**, l'icône d'alimentation en haut à droite → **Redémarrer Home Assistant**. (HACS affiche aussi une réparation *Redémarrage requis* dans **Paramètres**.)

HACS te propose tout seul les mises à jour de l'intégration quand une nouvelle version sort.

### 2. Ajouter le robot

1. Va dans **Paramètres → Appareils et services**.
2. En général, le robot attend sous *Découverts*, sous son nom : clique sur **Ajouter**, puis **Valider**.
3. S'il n'y est pas : **Ajouter une intégration** en bas à droite, cherche **CatWandConnect**, et saisis l'adresse du robot : son nom en `.local` (`catwandconnect.local`, ou le nom que tu lui as donné) ou son adresse IP, par exemple `192.168.1.20`.
4. **Code PIN des réglages** : si tu en as défini un dans les Réglages du robot, Home Assistant le demande une fois. Sans code PIN, il n'y a rien à saisir. Un code incorrect est signalé et tu peux réessayer ; après trop d'essais, attends le temps affiché.
5. Choisis une pièce si tu veux, puis **Terminer**.

Si le firmware du robot est trop ancien, l'ajout s'arrête avec *Le firmware de ce robot est trop ancien pour l'intégration* : [mets le robot à jour](install.fr.md#mettre-à-jour-compiler-depuis-les-sources) en 0.2.0-beta.2 ou plus récent, puis ajoute-le à nouveau.

Le robot n'est ajouté qu'une fois, même s'il est découvert de nouveau ; quand ta box lui donne une nouvelle adresse IP, l'intégration le suit.

### 3. Vérifier ses entités

Va dans **Paramètres → Appareils et services → CatWandConnect**, puis sur l'appareil du robot. Sa page liste ces entités, nommées dans la langue de Home Assistant :

| Entité | Type | Rôle |
|--------|------|------|
| Ranger | Bouton | Range la baguette et relâche les servos. Passe avant toute autre commande. |
| Position horizontale, Position verticale | Nombre (degrés) | Indique où se trouve chaque axe. Changer la valeur déplace l'axe à cet endroit, à la vitesse du pilotage manuel, dans les limites de course. |
| Vitesse maximale horizontale, Vitesse maximale verticale | Nombre (10–360 °/s) | Le même réglage que dans les Réglages du robot. Une vitesse maximale inférieure ou égale à la vitesse minimale ramène la minimale au dixième de la nouvelle maximale. |
| Activité | Capteur | `idle`, `manual`, `stowing`, `calibrating`, `tuning_speed` ou `playing` (affichés *Au repos*, *Pilotage manuel*… à l'écran). |
| Session de jeu | Interrupteur | Allumé, il lance une session de jeu pour la durée de session ; éteint, il l'arrête et range la baguette. Il est allumé tant qu'une session tourne, qu'elle ait été lancée depuis Home Assistant ou depuis l'interface du robot, et s'éteint tout seul à la fin de la session. Refusé, avec un message qui dit pourquoi, tant que le robot n'est pas calibré, et pendant un rangement, une calibration ou un réglage de vitesse. |
| Durée de session | Nombre (1–60 min) | La durée de la prochaine session de jeu, la même valeur que sur l'écran principal du robot. |

Les entités se mettent à jour en moins d'une seconde quand quelqu'un utilise le joystick ou l'écran du robot. Quand le robot est éteint ou perd le Wi-Fi, elles apparaissent comme indisponibles, et reviennent toutes seules à son retour. La page de l'appareil montre aussi le modèle, la puce et la version du firmware du robot, et un lien **Visiter** vers son interface.

**Quand tu changes le code PIN du robot**, Home Assistant te demande le nouveau : le robot apparaît en haut de **Paramètres → Appareils et services** avec **Reconfigurer**. Clique dessus et saisis le nouveau code PIN.

Les commandes venant de Home Assistant suivent les mêmes règles de sécurité que le téléphone : limites de course, limites de vitesse et priorités. Rien depuis Home Assistant ne peut pousser la baguette au-delà de ses limites, et le rangement ne peut pas être interrompu.

## Connexion MQTT

Le robot parle à Home Assistant par **MQTT**, un petit service de messages qui tourne à côté de Home Assistant (le *broker*). Le robot y envoie son état et y reçoit ses commandes ; Home Assistant lit et écrit les mêmes messages. Compte une dizaine de minutes :

1. [Vérifier les prérequis](#1-vérifier-les-prérequis)
2. [Installer le broker MQTT](#2-installer-le-broker-mqtt)
3. [Créer un utilisateur pour le robot](#3-créer-un-utilisateur-pour-le-robot)
4. [Ajouter l'intégration MQTT](#4-ajouter-lintégration-mqtt)
5. [Connecter le robot](#5-connecter-le-robot)
6. [Le retrouver dans Home Assistant](#6-le-retrouver-dans-home-assistant)

Tu utilises déjà MQTT (Zigbee2MQTT, Tasmota…) ? Ton broker et ton intégration sont prêts : crée un utilisateur (étape 3) et passe à l'[étape 5](#5-connecter-le-robot).

### 1. Vérifier les prérequis

- **Home Assistant OS** (un Home Assistant Green ou Yellow, un Raspberry Pi, une machine virtuelle). Pour le vérifier : **Paramètres → À propos**, ligne *Type d'installation*. Sur **Home Assistant Container** ou **Core**, il n'y a pas de menu *Applications* : installe toi-même un broker Mosquitto (par exemple son image Docker) et passe à l'[étape 4](#4-ajouter-lintégration-mqtt).
- Un compte **administrateur** dans Home Assistant.
- Le robot **installé et sur ton Wi-Fi** ([Installer le logiciel](install.fr.md)), sur le même réseau que Home Assistant.
- L'**adresse IP** de Home Assistant : **Paramètres → Système → Réseau**, sous *Adaptateur réseau*, par exemple `192.168.1.124`. Note-la pour l'étape 5. Donne-lui une adresse fixe dans ta box (une *réservation DHCP*) pour qu'elle ne change jamais.

### 2. Installer le broker MQTT

1. Dans Home Assistant, va dans **Paramètres → Applications → Installer une application** (appelées *Modules complémentaires* avant Home Assistant 2026.2).
2. Cherche **Mosquitto broker** et ouvre-le.
3. Clique sur **Installer**, patiente, puis active **Démarrer au démarrage** et clique sur **Démarrer**.
4. Ouvre l'onglet **Journal** : au bout de quelques secondes, il doit se terminer sans erreur.

Laisse son onglet *Configuration* tel quel : le broker accepte les utilisateurs de Home Assistant, c'est ce qu'utilise l'étape 3.

### 3. Créer un utilisateur pour le robot

Le broker refuse les connexions anonymes : le robot a besoin de son propre identifiant.

1. Active le **Mode avancé** : clique sur ton nom en bas à gauche, puis active *Mode avancé* (sinon l'onglet *Utilisateurs* est caché).
2. Va dans **Paramètres → Personnes → Utilisateurs** et clique sur **Ajouter un utilisateur**.
3. Remplis :
   - **Nom d'affichage** : `CatWandConnect` ;
   - **Nom d'utilisateur** : `catwandconnect` (pas `homeassistant` ni `addons` : le broker les réserve) ;
   - **Mot de passe** : un nouveau mot de passe, utilisé seulement par le robot. Note-le pour l'étape 5.
4. Laisse **Administrateur** désactivé, puis clique sur **Créer**.

Cet utilisateur ne se connecte jamais à l'interface de Home Assistant ; il permet seulement au robot de parler au broker.

### 4. Ajouter l'intégration MQTT

C'est ce qui relie Home Assistant lui-même au broker.

1. Va dans **Paramètres → Appareils et services**.
2. En général, **MQTT** attend sous *Découverts* : clique sur **Ajouter**, puis **Valider**.
3. S'il n'y est pas : **Ajouter une intégration** en bas à droite, cherche **MQTT**, choisis **MQTT**, et confirme le broker Mosquitto.
4. **MQTT** apparaît maintenant dans la liste des intégrations, sans erreur.

Laisse *Activer la découverte* coché et le préfixe de découverte à `homeassistant` (les valeurs par défaut) : c'est là que le robot s'annonce.

### 5. Connecter le robot

1. Ouvre l'interface du robot (`http://catwandconnect.local/`, ou le nom que tu lui as donné), puis **Réglages → Réseau → Home Assistant**.
2. Coche **Activé**, et remplis :
   - **Broker** : l'adresse IP de Home Assistant notée à l'étape 1, par exemple `192.168.1.124`. `homeassistant.local` marche aussi sur la plupart des réseaux, mais l'IP est plus fiable ;
   - **Port** : `1883` ;
   - **Utilisateur** et **Mot de passe** : l'utilisateur de l'étape 3.
3. Clique sur **Enregistrer**. Si un code PIN des réglages est défini, le robot le demande.
4. En quelques secondes, **État** indique **Connecté**. S'il affiche une erreur, voir [Dépannage de la connexion MQTT](#dépannage-de-la-connexion-mqtt).

Le robot se reconnecte tout seul après une coupure de courant ou un redémarrage de Home Assistant.

### 6. Le retrouver dans Home Assistant

Va dans **Paramètres → Appareils et services → MQTT**, puis **Appareils**. Le robot apparaît sous son nom, avec les mêmes entités qu'avec l'intégration native ([listées plus haut](#3-vérifier-ses-entités)), mais nommées en anglais : Stow, Horizontal position, Vertical position, Horizontal maximum speed, Vertical maximum speed, Activity, Play session et Session duration. Les différences :

- Horizontal position et Vertical position vont de 0 à 180° ; le robot garde l'axe dans ses limites de course.
- Une session de jeu refusée par le robot (pas calibré, occupé) remet l'interrupteur sur éteint sans message ; le log du robot dit pourquoi.
- Session duration est mémorisée dès que le robot est au repos.

Quand le robot est éteint ou perd le Wi-Fi, ses entités apparaissent comme indisponibles.

## Tableaux de bord tout prêts

Un panneau de contrôle pour le robot, fait uniquement de cartes intégrées (rien à installer depuis HACS) : l'interrupteur de session de jeu et sa durée, Ranger, l'activité, la position et la vitesse maximale de chaque axe. Il y en a un pour chaque connexion ; ils se ressemblent.

1. Va dans **Paramètres → Tableaux de bord → Ajouter un tableau de bord → Nouveau tableau de bord à partir de zéro**, nomme-le, par exemple « Canne à chat », et ouvre-le.
2. Clique sur le crayon (**Modifier le tableau de bord**), puis sur les trois points → **Éditeur de configuration brute**.
3. Remplace tout par le YAML de ta connexion ci-dessous et **Enregistre**.

Sur un téléphone, les sections s'empilent : la session de jeu d'abord, puis les positions et les vitesses. Pour ajouter les cartes à un tableau de bord existant, copie seulement les trois sections `- type: grid` dans une de ses vues en sections.

**Robot renommé ?** Les identifiants ci-dessous sont ceux d'un robot qui a le nom par défaut. Les identifiants des entités suivent le nom du robot : pour un robot nommé « CatWandConnect salon », remplace chaque `catwandconnect_` par `catwandconnect_salon_` (rechercher et remplacer dans l'éditeur). Les identifiants exacts sont sur la page de l'appareil, dans **Paramètres → Appareils et services**, puis **CatWandConnect** ou **MQTT**.

### Tableau de bord pour l'intégration native

<!-- ha-dashboard-native -->
```yaml
title: Canne à chat
views:
  - title: Canne à chat
    path: catwandconnect
    type: sections
    max_columns: 3
    sections:
      - type: grid
        cards:
          - type: heading
            heading: Session de jeu
            icon: mdi:cat
            badges:
              - type: entity
                entity: sensor.catwandconnect_activity
                show_state: true
          - type: tile
            entity: switch.catwandconnect_play_session
            name: Session de jeu
            icon: mdi:play-circle-outline
            features:
              - type: toggle
            features_position: inline
          - type: tile
            entity: number.catwandconnect_session_duration
            name: Durée de session
            features:
              - type: numeric-input
                style: buttons
            features_position: inline
          - type: tile
            entity: button.catwandconnect_stow
            name: Ranger
            color: red
            tap_action:
              action: perform-action
              perform_action: button.press
              target:
                entity_id: button.catwandconnect_stow
      - type: grid
        cards:
          - type: heading
            heading: Position
            icon: mdi:axis-arrow
          - type: tile
            entity: number.catwandconnect_horizontal_position
            name: Horizontal
            features:
              - type: numeric-input
                style: slider
          - type: tile
            entity: number.catwandconnect_vertical_position
            name: Vertical
            features:
              - type: numeric-input
                style: slider
      - type: grid
        cards:
          - type: heading
            heading: Vitesse maximale
            icon: mdi:speedometer
          - type: tile
            entity: number.catwandconnect_horizontal_maximum_speed
            name: Horizontal
            features:
              - type: numeric-input
                style: buttons
            features_position: inline
          - type: tile
            entity: number.catwandconnect_vertical_maximum_speed
            name: Vertical
            features:
              - type: numeric-input
                style: buttons
            features_position: inline
```

Les curseurs de position vont d'une limite de course à l'autre.

### Tableau de bord pour la connexion MQTT

<!-- ha-dashboard -->
```yaml
title: Canne à chat
views:
  - title: Canne à chat
    path: catwandconnect
    type: sections
    max_columns: 3
    sections:
      - type: grid
        cards:
          - type: heading
            heading: Session de jeu
            icon: mdi:cat
            badges:
              - type: entity
                entity: sensor.catwandconnect_activity
                show_state: true
          - type: tile
            entity: switch.catwandconnect_play_session
            name: Session de jeu
            icon: mdi:play-circle-outline
            features:
              - type: toggle
            features_position: inline
          - type: tile
            entity: number.catwandconnect_session_duration
            name: Durée de session
            features:
              - type: numeric-input
                style: buttons
            features_position: inline
          - type: tile
            entity: button.catwandconnect_stow
            name: Ranger
            color: red
            tap_action:
              action: perform-action
              perform_action: button.press
              target:
                entity_id: button.catwandconnect_stow
      - type: grid
        cards:
          - type: heading
            heading: Position
            icon: mdi:axis-arrow
          - type: tile
            entity: number.catwandconnect_horizontal_position
            name: Horizontal
            features:
              - type: numeric-input
                style: slider
          - type: tile
            entity: number.catwandconnect_vertical_position
            name: Vertical
            features:
              - type: numeric-input
                style: slider
      - type: grid
        cards:
          - type: heading
            heading: Vitesse maximale
            icon: mdi:speedometer
          - type: tile
            entity: number.catwandconnect_horizontal_maximum_speed
            name: Horizontal
            features:
              - type: numeric-input
                style: buttons
            features_position: inline
          - type: tile
            entity: number.catwandconnect_vertical_maximum_speed
            name: Vertical
            features:
              - type: numeric-input
                style: buttons
            features_position: inline
```

Les curseurs de position vont de 0 à 180° ; le robot s'arrête à ses limites de course.

## Automatisations

Dans **Paramètres → Automatisations et scènes → Créer une automatisation → Créer une nouvelle automatisation**, trois points → **Modifier en YAML**, colle l'un de ces exemples, puis **Enregistrer**. Ils marchent avec les deux connexions.

Ranger la baguette tous les soirs à 22 h :

```yaml
alias: Ranger la canne à chat le soir
triggers:
  - trigger: time
    at: "22:00:00"
actions:
  - action: button.press
    target:
      entity_id: button.catwandconnect_stow
```

Lancer une session de jeu de 15 minutes quand tu pars au travail en semaine :

```yaml
alias: Jouer avec le chat après notre départ
triggers:
  - trigger: time
    at: "08:30:00"
conditions:
  - condition: time
    weekday: [mon, tue, wed, thu, fri]
actions:
  - action: number.set_value
    target:
      entity_id: number.catwandconnect_session_duration
    data:
      value: 15
  - action: switch.turn_on
    target:
      entity_id: switch.catwandconnect_play_session
```

Les identifiants des entités suivent le nom du robot : `button.catwandconnect_stow` par défaut, `button.catwandconnect_salon_stow` pour un robot nommé « CatWandConnect salon ». Ils restent en anglais, et sont les mêmes avec les deux connexions ; des identifiants qui finissent par `_2` veulent dire que le robot est connecté deux fois (voir [Choisir ta connexion](#choisir-ta-connexion)). Les automatisations sur l'activité utilisent ses états `idle`, `manual`, `stowing`, `calibrating`, `tuning_speed` et `playing`, quelle que soit la langue dans laquelle Home Assistant les affiche.

## Dépannage

### Dépannage de l'intégration native

| Problème | À vérifier |
|----------|------------|
| Le robot n'est pas sous *Découverts* | Ajoute-le à la main : **Ajouter une intégration → CatWandConnect**, avec son adresse IP (sur sa page Infos) ou son nom en `.local`. La découverte passe par le mDNS, qui ne traverse pas les réseaux : si Home Assistant et le robot sont sur des VLAN ou des sous-réseaux différents (un réseau IoT, un Wi-Fi invité), ajoute-le par son IP et donne au robot une adresse fixe dans ta box (une *réservation DHCP*), ou active un relais mDNS dans ton routeur. Sur Home Assistant Container, la découverte demande le réseau en mode hôte. |
| *CatWandConnect* n'est pas dans **Ajouter une intégration** | Home Assistant n'a pas été redémarré après le téléchargement ([étape 1](#1-installer-lintégration-depuis-hacs)), ou HACS n'a rien téléchargé : ouvre HACS et vérifie que l'intégration apparaît comme téléchargée. Recharge ensuite la page du navigateur. |
| *Le firmware de ce robot est trop ancien pour l'intégration* | Le robot a un firmware plus ancien que 0.2.0-beta.2. [Mets-le à jour](install.fr.md#mettre-à-jour-compiler-depuis-les-sources), puis ajoute-le à nouveau. |
| Impossible de joindre le robot | Le robot est allumé, sur le même réseau que Home Assistant, et son interface s'ouvre dans un navigateur à l'adresse que tu as saisie. |
| Home Assistant redemande le code PIN | Le code PIN des réglages du robot a changé. Clique sur **Reconfigurer** et saisis le code PIN actuel. |
| Entités indisponibles | Le robot est éteint ou hors Wi-Fi. Elles reviennent toutes seules dans les 30 secondes environ qui suivent sa reconnexion. Si elles restent indisponibles, vérifie que l'interface du robot s'ouvre ; si son IP a changé et que la découverte ne le voit pas (autre VLAN), réserve son adresse dans ta box. |
| La session de jeu ne démarre pas | Home Assistant dit pourquoi : le robot n'est pas encore calibré, ou il est en train de se ranger, de se calibrer ou de régler une vitesse. Calibre-le depuis son écran principal. |

Pour un rapport de bug, ajoute les diagnostics de l'intégration : sur la page de l'appareil du robot, les trois points → **Télécharger les diagnostics**. Le nom de ton Wi-Fi, tes adresses et les détails du broker y sont masqués, comme dans le rapport de diagnostic.

### Dépannage de la connexion MQTT

| État dans les Réglages du robot | À vérifier |
|---------------------------------|------------|
| Broker injoignable | L'adresse du broker et le port `1883`, et que l'application Mosquitto broker est démarrée ([étape 2](#2-installer-le-broker-mqtt)). Mets l'IP de Home Assistant au lieu de `homeassistant.local`. |
| Le broker a refusé l'utilisateur ou le mot de passe | L'utilisateur et le mot de passe de l'[étape 3](#3-créer-un-utilisateur-pour-le-robot). Retape le mot de passe dans le robot (il n'affiche jamais celui enregistré). Le nom d'utilisateur ne peut pas être `homeassistant` ni `addons`. |
| Connecté, mais aucun appareil dans Home Assistant | L'intégration MQTT est installée ([étape 4](#4-ajouter-lintégration-mqtt)), avec la découverte activée et le préfixe `homeassistant`. |
| Entités indisponibles | Le robot est éteint ou hors Wi-Fi. Elles reviennent dans la minute qui suit sa reconnexion. |
| L'interrupteur de session de jeu revient sur éteint | Le robot n'est pas encore calibré, ou il est en train de se ranger, de se calibrer ou de régler une vitesse. Calibre-le depuis son écran principal. |

Les topics MQTT du robot sont documentés dans l'[API du robot](api.fr.md#home-assistant-mqtt). La page de l'appareil dans Home Assistant a un lien **Visiter** vers l'interface du robot (la découverte MQTT n'autorise qu'un lien par appareil, il ne peut donc pas pointer directement vers la page Aide).

### Toujours bloqué ?

Voir [Obtenir de l'aide](../SUPPORT.fr.md), ou ouvre **Aide** dans Réglages → Robot. Les bugs de l'intégration native se signalent aussi dans les [issues de CatWandConnect](https://github.com/vistorx/CatWandConnect/issues/new/choose) : choisis *Home Assistant integration (HACS)* comme domaine.
