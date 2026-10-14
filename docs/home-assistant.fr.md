# Home Assistant

CatWandConnect fonctionne sans Home Assistant. Si tu l'utilises, le robot peut y apparaître tout seul grâce à la **découverte MQTT** : tu obtiens un appareil avec un bouton Ranger, la position et la vitesse maximale de chaque axe, son activité, et un interrupteur de session de jeu avec sa durée de session, prêt pour tes tableaux de bord et tes automatisations.

## Ce qu'il te faut

- Home Assistant avec un **broker MQTT**. Le plus simple est le module complémentaire officiel **Mosquitto broker**.
- L'**intégration MQTT** de Home Assistant, connectée à ce broker.
- Le robot sur le même réseau.

## 1. Installer le broker

Si tu utilises déjà MQTT, passe à l'étape 2.

1. Dans Home Assistant, va dans **Paramètres → Modules complémentaires → Boutique des modules complémentaires**, installe **Mosquitto broker** et démarre-le.
2. Crée un utilisateur pour le robot : **Paramètres → Personnes → Utilisateurs → Ajouter un utilisateur**, par exemple `catwandconnect`, avec un mot de passe. Active le mode avancé dans ton profil si l'onglet Utilisateurs n'apparaît pas. Mosquitto accepte les utilisateurs de Home Assistant.
3. Home Assistant découvre généralement le broker : accepte l'intégration **MQTT** qu'il propose dans **Paramètres → Appareils et services**.

## 2. Connecter le robot

1. Ouvre l'interface du robot, puis **Réglages → Réseau → Home Assistant**.
2. Coche **Activé**, et saisis :
   - **Broker** : l'adresse de ton Home Assistant, par exemple `homeassistant.local` ou son IP ;
   - **Port** : `1883` ;
   - **Utilisateur** et **Mot de passe** : l'utilisateur de l'étape 1.
3. Enregistre. En quelques secondes, l'état doit indiquer **Connecté**.

Si un code PIN des réglages est défini, l'enregistrement le demande.

## 3. Le retrouver dans Home Assistant

Va dans **Paramètres → Appareils et services → MQTT → Appareils**. Le robot apparaît sous son nom, avec :

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

## Exemple d'automatisation

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
| Broker injoignable | L'adresse et le port du broker, et que le module Mosquitto tourne. Essaie l'IP au lieu de `homeassistant.local`. |
| Le broker a refusé l'utilisateur ou le mot de passe | L'utilisateur et le mot de passe ; l'utilisateur doit exister dans Home Assistant (ou dans les identifiants propres à Mosquitto). |
| Connecté, mais aucun appareil dans Home Assistant | L'intégration MQTT est installée et utilise le préfixe de découverte par défaut, `homeassistant`. |

Les topics MQTT du robot sont documentés dans l'[API du robot](api.fr.md#home-assistant-mqtt). Toujours bloqué ? Voir [Obtenir de l'aide](../SUPPORT.fr.md), ou ouvre **Aide** dans Réglages → Robot : la page de l'appareil dans Home Assistant a un lien **Visiter** vers l'interface du robot (la découverte MQTT n'autorise qu'un lien par appareil, il ne peut donc pas pointer directement vers la page Aide).

Une intégration Home Assistant dédiée pourra venir plus tard ([ADR 0005](adr/0005-home-assistant-via-mqtt-first.md), en anglais).
