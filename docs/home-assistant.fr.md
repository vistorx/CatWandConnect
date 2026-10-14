# Configurer Home Assistant

CatWandConnect fonctionne sans Home Assistant. Si tu l'utilises, le robot peut y apparaître tout seul : un appareil avec un interrupteur de session de jeu et sa durée, un bouton Ranger, son activité, et la position et la vitesse maximale de chaque axe, prêt pour tes tableaux de bord et tes automatisations.

Le robot parle à Home Assistant par **MQTT**, un petit service de messages qui tourne à côté de Home Assistant (le *broker*). Le robot y envoie son état et y reçoit ses commandes ; Home Assistant lit et écrit les mêmes messages. Compte une dizaine de minutes :

1. [Vérifier les prérequis](#1-vérifier-les-prérequis)
2. [Installer le broker MQTT](#2-installer-le-broker-mqtt)
3. [Créer un utilisateur pour le robot](#3-créer-un-utilisateur-pour-le-robot)
4. [Ajouter l'intégration MQTT](#4-ajouter-lintégration-mqtt)
5. [Connecter le robot](#5-connecter-le-robot)
6. [Le retrouver dans Home Assistant](#6-le-retrouver-dans-home-assistant)

Tu utilises déjà MQTT (Zigbee2MQTT, Tasmota…) ? Ton broker et ton intégration sont prêts : crée un utilisateur (étape 3) et passe à l'[étape 5](#5-connecter-le-robot).

## 1. Vérifier les prérequis

- **Home Assistant OS** (un Home Assistant Green ou Yellow, un Raspberry Pi, une machine virtuelle). Pour le vérifier : **Paramètres → À propos**, ligne *Type d'installation*. Sur **Home Assistant Container** ou **Core**, il n'y a pas de menu *Applications* : installe toi-même un broker Mosquitto (par exemple son image Docker) et passe à l'[étape 4](#4-ajouter-lintégration-mqtt).
- Un compte **administrateur** dans Home Assistant.
- Le robot **installé et sur ton Wi-Fi** ([Installer le logiciel](install.fr.md)), sur le même réseau que Home Assistant.
- L'**adresse IP** de Home Assistant : **Paramètres → Système → Réseau**, sous *Adaptateur réseau*, par exemple `192.168.1.124`. Note-la pour l'étape 5. Donne-lui une adresse fixe dans ta box (une *réservation DHCP*) pour qu'elle ne change jamais.

## 2. Installer le broker MQTT

1. Dans Home Assistant, va dans **Paramètres → Applications → Installer une application** (appelées *Modules complémentaires* avant Home Assistant 2026.2).
2. Cherche **Mosquitto broker** et ouvre-le.
3. Clique sur **Installer**, patiente, puis active **Démarrer au démarrage** et clique sur **Démarrer**.
4. Ouvre l'onglet **Journal** : au bout de quelques secondes, il doit se terminer sans erreur.

Laisse son onglet *Configuration* tel quel : le broker accepte les utilisateurs de Home Assistant, c'est ce qu'utilise l'étape 3.

## 3. Créer un utilisateur pour le robot

Le broker refuse les connexions anonymes : le robot a besoin de son propre identifiant.

1. Active le **Mode avancé** : clique sur ton nom en bas à gauche, puis active *Mode avancé* (sinon l'onglet *Utilisateurs* est caché).
2. Va dans **Paramètres → Personnes → Utilisateurs** et clique sur **Ajouter un utilisateur**.
3. Remplis :
   - **Nom d'affichage** : `CatWandConnect` ;
   - **Nom d'utilisateur** : `catwandconnect` (pas `homeassistant` ni `addons` : le broker les réserve) ;
   - **Mot de passe** : un nouveau mot de passe, utilisé seulement par le robot. Note-le pour l'étape 5.
4. Laisse **Administrateur** désactivé, puis clique sur **Créer**.

Cet utilisateur ne se connecte jamais à l'interface de Home Assistant ; il permet seulement au robot de parler au broker.

## 4. Ajouter l'intégration MQTT

C'est ce qui relie Home Assistant lui-même au broker.

1. Va dans **Paramètres → Appareils et services**.
2. En général, **MQTT** attend sous *Découverts* : clique sur **Ajouter**, puis **Valider**.
3. S'il n'y est pas : **Ajouter une intégration** en bas à droite, cherche **MQTT**, choisis **MQTT**, et confirme le broker Mosquitto.
4. **MQTT** apparaît maintenant dans la liste des intégrations, sans erreur.

Laisse *Activer la découverte* coché et le préfixe de découverte à `homeassistant` (les valeurs par défaut) : c'est là que le robot s'annonce.

## 5. Connecter le robot

1. Ouvre l'interface du robot (`http://catwandconnect.local/`, ou le nom que tu lui as donné), puis **Réglages → Réseau → Home Assistant**.
2. Coche **Activé**, et remplis :
   - **Broker** : l'adresse IP de Home Assistant notée à l'étape 1, par exemple `192.168.1.124`. `homeassistant.local` marche aussi sur la plupart des réseaux, mais l'IP est plus fiable ;
   - **Port** : `1883` ;
   - **Utilisateur** et **Mot de passe** : l'utilisateur de l'étape 3.
3. Clique sur **Enregistrer**. Si un code PIN des réglages est défini, le robot le demande.
4. En quelques secondes, **État** indique **Connecté**. S'il affiche une erreur, voir [Dépannage](#dépannage).

Le robot se reconnecte tout seul après une coupure de courant ou un redémarrage de Home Assistant.

## 6. Le retrouver dans Home Assistant

Va dans **Paramètres → Appareils et services → MQTT**, puis **Appareils**. Le robot apparaît sous son nom. Sa page liste ces entités :

| Entité | Type | Rôle |
|--------|------|------|
| Stow | Bouton | Range la baguette et relâche les servos. Passe avant toute autre commande. |
| Horizontal position, Vertical position | Nombre (0–180°) | Indique où se trouve chaque axe. Changer la valeur déplace l'axe à cet endroit, à la vitesse du pilotage manuel, dans les limites de course. |
| Horizontal maximum speed, Vertical maximum speed | Nombre (10–360 °/s) | Le même réglage que dans les Réglages du robot. Une vitesse maximale inférieure ou égale à la vitesse minimale ramène la minimale au dixième de la nouvelle maximale. |
| Activity | Capteur | `idle`, `manual`, `stowing`, `calibrating`, `tuning_speed` ou `playing`. |
| Play session | Interrupteur | Allumé, il lance une session de jeu pour la durée de session ; éteint, il l'arrête et range la baguette. Il est allumé tant qu'une session tourne, qu'elle ait été lancée depuis Home Assistant ou depuis l'interface du robot, et s'éteint tout seul à la fin de la session. Refusé (l'interrupteur revient sur éteint) tant que le robot n'est pas calibré, et pendant un rangement, une calibration ou un réglage de vitesse. |
| Session duration | Nombre (1–60 min) | La durée de la prochaine session de jeu, la même valeur que sur l'écran principal du robot. Mémorisée dès que le robot est au repos. |

Les noms des entités restent en anglais. Quand le robot est éteint ou perd le Wi-Fi, ses entités apparaissent comme indisponibles.

Les commandes venant de Home Assistant suivent les mêmes règles de sécurité que le téléphone : limites de course, limites de vitesse et priorités. Rien depuis Home Assistant ne peut pousser la baguette au-delà de ses limites, et le rangement ne peut pas être interrompu.

## Tableau de bord tout prêt

Un panneau de contrôle pour le robot, fait uniquement de cartes intégrées (rien à installer depuis HACS) : l'interrupteur de session de jeu et sa durée, Ranger, l'activité, la position et la vitesse maximale de chaque axe.

1. Va dans **Paramètres → Tableaux de bord → Ajouter un tableau de bord → Nouveau tableau de bord à partir de zéro**, nomme-le, par exemple « Canne à chat », et ouvre-le.
2. Clique sur le crayon (**Modifier le tableau de bord**), puis sur les trois points → **Éditeur de configuration brute**.
3. Remplace tout par le YAML ci-dessous et **Enregistre**.

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

Sur un téléphone, les sections s'empilent : la session de jeu d'abord, puis les positions et les vitesses. Pour ajouter les cartes à un tableau de bord existant, copie seulement les trois sections `- type: grid` dans une de ses vues en sections.

**Robot renommé ?** Les identifiants ci-dessus sont ceux d'un robot qui a le nom par défaut. Les identifiants des entités suivent le nom du robot : pour un robot nommé « CatWandConnect salon », remplace chaque `catwandconnect_` par `catwandconnect_salon_` (rechercher et remplacer dans l'éditeur). Les identifiants exacts sont sur la page de l'appareil, dans **Paramètres → Appareils et services → MQTT**.

## Automatisations

Dans **Paramètres → Automatisations et scènes → Créer une automatisation → Créer une nouvelle automatisation**, trois points → **Modifier en YAML**, colle l'un de ces exemples, puis **Enregistrer**.


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

Les identifiants des entités suivent le nom du robot : `button.catwandconnect_stow` par défaut, `button.catwandconnect_salon_stow` pour un robot nommé « CatWandConnect salon ».

## Dépannage

| État dans les Réglages du robot | À vérifier |
|---------------------------------|------------|
| Broker injoignable | L'adresse du broker et le port `1883`, et que l'application Mosquitto broker est démarrée (étape 2). Mets l'IP de Home Assistant au lieu de `homeassistant.local`. |
| Le broker a refusé l'utilisateur ou le mot de passe | L'utilisateur et le mot de passe de l'étape 3. Retape le mot de passe dans le robot (il n'affiche jamais celui enregistré). Le nom d'utilisateur ne peut pas être `homeassistant` ni `addons`. |
| Connecté, mais aucun appareil dans Home Assistant | L'intégration MQTT est installée (étape 4), avec la découverte activée et le préfixe `homeassistant`. |
| Entités indisponibles | Le robot est éteint ou hors Wi-Fi. Elles reviennent dans la minute qui suit sa reconnexion. |
| L'interrupteur de session de jeu revient sur éteint | Le robot n'est pas encore calibré, ou il est en train de se ranger, de se calibrer ou de régler une vitesse. Calibre-le depuis son écran principal. |

Les topics MQTT du robot sont documentés dans l'[API du robot](api.fr.md#home-assistant-mqtt). Toujours bloqué ? Voir [Obtenir de l'aide](../SUPPORT.fr.md), ou ouvre **Aide** dans Réglages → Robot : la page de l'appareil dans Home Assistant a un lien **Visiter** vers l'interface du robot (la découverte MQTT n'autorise qu'un lien par appareil, il ne peut donc pas pointer directement vers la page Aide).

Une intégration Home Assistant dédiée pourra venir plus tard ([ADR 0005](adr/0005-home-assistant-via-mqtt-first.md), en anglais).
