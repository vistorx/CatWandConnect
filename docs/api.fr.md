# API du robot

L'API HTTP et WebSocket du robot, servie sur le port 80 à `http://<device-name>.local/`. L'interface du robot n'utilise que cette API, et elle est ouverte aux tiers : scripts, tableaux de bord et l'intégration native pour Home Assistant.

Chaque commande passe par le Robot core : les limites de course, les limites de vitesse et les priorités s'appliquent quelle que soit la source.

Les angles sont en degrés, tels qu'envoyés aux servos (0–180).

## Version de l'API

La version actuelle de l'API est **1**. `GET /api/info` la renvoie dans `api`, et le robot l'annonce sur le réseau (voir [Découverte](#découverte-mdns)), pour qu'une intégration puisse la vérifier avant d'utiliser le robot.

La version de l'API n'augmente que quand un changement casserait une intégration existante : une route ou un champ supprimé ou renommé, un sens ou une unité qui change. Les ajouts (une nouvelle route, un nouveau champ, une nouvelle valeur) la gardent ; les clients ignorent les champs qu'ils ne connaissent pas.

| Version de l'API | Premier firmware | Ce qu'elle contient |
|------------------|------------------|---------------------|
| 1 | 0.2.0-beta.2 | Tout ce qui est sur cette page, dont `api` et `id` dans `GET /api/info`, `POST /api/position` et le service `_catwandconnect._tcp`. Un robot dont `/api/info` n'a pas de champ `api` est plus ancien. Ajoutés depuis, sans nouvelle version de l'API : `channel` et `chipFamily` dans `/api/info`, `channel` dans les réglages et `POST /api/update/available` (0.2.0-beta.4). |

## Découverte (mDNS)

Une fois sur le Wi-Fi, le robot répond à `<hostname>.local` et annonce deux services sur le port 80 :

- `_http._tcp`, pour les navigateurs et les outils réseau ;
- `_catwandconnect._tcp`, que cherche l'intégration native (le zeroconf de Home Assistant), avec ces enregistrements TXT :

| Clé | Valeur | Exemple |
|-----|--------|---------|
| `id` | Identifiant d'appareil : 8 chiffres hexadécimaux, tirés une fois et gardés en flash. Il ne change ni au renommage, ni avec l'adresse IP, ni avec les mises à jour ; sers-t'en pour distinguer les robots. | `3f9a12c4` |
| `api` | Version de l'API. | `1` |
| `version` | Version du firmware. | `0.2.0` |
| `name` | Le nom du robot, tel que le propriétaire l'a saisi (UTF-8). | `CatWandConnect` |

Les enregistrements sont fixés au démarrage ; renommer le robot le redémarre, donc `name` suit.

## Code PIN des réglages

Le propriétaire peut définir un code PIN facultatif (désactivé par défaut). Tant qu'il est actif, les requêtes marquées 🔒 ci-dessous répondent `401 {"error":"pin_required"}` sauf si elles portent un jeton de session dans un en-tête `Authorization: Bearer <token>`. Le pilotage manuel (y compris `/api/position`), le rangement, le curseur de vitesse (`playSpeed`), la session de jeu (`/api/play` et `sessionMinutes`) et tous les `GET` n'en ont jamais besoin.

| Requête | Réponse |
|---------|---------|
| `GET /api/auth` | `{"enabled":true,"authorized":false,"lockedFor":0}`. `authorized` indique si le jeton envoyé, s'il y en a un, est valide. |
| `POST /api/auth/unlock` avec `{"pin":"2468"}` | `200 {"token":"…"}`. Un code faux renvoie `401 {"error":"wrong_pin"}`. Après 5 codes faux, le déverrouillage est refusé pendant 30 s, durée doublée à chaque nouvel échec jusqu'à 15 min : `429 {"error":"locked_out","retryAfter":30}`. |
| 🔒 `PUT /api/auth/pin` avec `{"pin":"2468"}` | Définit ou change le code (4 à 8 chiffres) et renvoie `{"enabled":true,"token":"…"}`, un nouveau jeton pour l'appelant. `{"pin":""}` le supprime. Un code invalide renvoie `400 invalid_pin`. |

Les jetons vivent dans la mémoire du robot. Ils expirent après 12 h sans utilisation, quand le code change, ou quand le robot redémarre. Un code oublié ne peut être effacé qu'en effaçant et reflashant le robot. En HTTP simple, le code et les jetons ne sont pas chiffrés : le code empêche les invités de modifier la configuration, mais ne protège pas contre quelqu'un qui espionne ton Wi-Fi.

## WebSocket : `/ws`

Messages texte JSON, un par trame, chacun avec un type dans `t`.

### Client → robot

| Message | Effet |
|---------|-------|
| `{"t":"move","h":0.5,"v":-1}` | Pilotage manuel. `h` et `v` sont les déplacements du joystick dans [-1, 1] pour l'axe horizontal et l'axe vertical, en fraction de la vitesse maximale. Les valeurs hors plage sont plafonnées. |
| `{"t":"stop"}` | Termine le pilotage manuel. La baguette ralentit et s'arrête. |
| `{"t":"stow"}` | Ranger : arrête tout le reste, amène la baguette à la position de repos et y relâche les servos. `move` est ignoré jusqu'à la fin du rangement. |

**Règle de l'homme mort :** pendant un mouvement, renvoie `move` au moins toutes les 500 ms ; l'interface l'envoie toutes les 100 ms. Si aucun `move` n'arrive pendant 500 ms, ou si un client se déconnecte, le robot s'arrête.

### Robot → client

`state` est envoyé à la connexion d'un client, à chaque changement (au plus toutes les 100 ms), et toutes les 2 s sinon :

```json
{"t":"state","h":92.5,"v":88.0,"activity":"playing","held":true,"playSpeed":0.6,"sessionMinutes":10,"play":{"remaining":587}}
```

| Champ | Signification |
|-------|---------------|
| `h`, `v` | Position actuelle de chaque axe. |
| `activity` | `idle`, `manual`, `stowing`, `calibrating`, `tuning_speed` ou `playing`. |
| `held` | Si les servos sont pilotés. Ils ne sont relâchés qu'une fois la baguette au repos à la position de repos. |
| `playSpeed` | La valeur du curseur de vitesse, pour que toutes les interfaces ouvertes restent synchronisées. |
| `calibrated` | Si la calibration a été faite. |
| `calibration` | L'étape de calibration en cours, `none` hors calibration (voir plus bas). |
| `tuning` | Seulement quand `activity` vaut `tuning_speed` : l'objet du réglage des vitesses (voir plus bas), réglage de l'accélération compris. |
| `sessionMinutes` | La durée de session, en minutes, pour que toutes les interfaces ouvertes restent synchronisées. |
| `play` | Seulement quand `activity` vaut `playing` : `{"remaining": <secondes>}`, le temps restant de la session de jeu, arrondi au-dessus. |
| `name` | Le nom du robot. |

## HTTP

### Ranger

`POST /api/stow` (sans corps) fait la même chose que le message WebSocket `stow` et renvoie `202`.

### Position

`POST /api/position` avec `{"axis":"horizontal","degrees":90}` amène un axe à une position et renvoie `202` : c'est le pilotage manuel par position, comme régler une position depuis Home Assistant en MQTT. L'axe y va à la vitesse du pilotage manuel puis s'arrête ; la cible est ramenée dans les limites de course (avant la calibration, dans la plage par défaut 60–120°). Comme tout pilotage manuel, elle termine une session de jeu sans ranger, et elle est ignorée pendant le rangement. `axis` vaut `horizontal` ou `vertical` ; tout autre axe, ou un `degrees` qui n'est pas un nombre, renvoie `400 {"error":"invalid_request"}`. Pas de code PIN.

### Priorités

Au démarrage, le robot commande la position où il s'est arrêté en dernier (ramenée dans les limites de course), puis se range à la vitesse la plus lente, 10 °/s ; `activity` vaut `stowing` pendant ce temps.

Le rangement passe avant toute autre commande et ne peut pas être interrompu. Le pilotage manuel, quelle que soit sa source, annule toute autre activité, y compris une session de jeu. Rien ne reprend tout seul ensuite.

### Session de jeu

La baguette joue toute seule pendant la durée de session, puis se range. Chaque axe tourne en boucle de son côté, jamais au même rythme que l'autre : il va à une position au hasard n'importe où dans ses limites de course, à une vitesse au hasard entre sa vitesse minimale et sa vitesse maximale (`minSpeed`–`maxSpeed`), puis se repose un temps au hasard dans sa plage de pause (`pauseMin`–`pauseMax`), et recommence.

`GET /api/play` renvoie :

```json
{"active":true,"remaining":587,"minutes":10}
```

`active` indique si une session est en cours, `remaining` est le temps restant en secondes (0 sans session) et `minutes` la durée de session.

`POST /api/play` (sans code PIN) avec `{"action": …}` la pilote et renvoie le même objet :

| Action | Effet |
|--------|-------|
| `{"action":"start"}` | Lance une session de la durée de session, depuis zéro même si une session est en cours. |
| `{"action":"start","sessionMinutes":20}` | Pareil avec cette durée (1–60), qui devient la durée de session. |
| `{"action":"stop"}` | Termine la session par un rangement, sans finir le mouvement en cours. Ne fait rien sans session en cours. |

Erreurs : `409 {"error":"not_calibrated"}` avant la calibration, `409 {"error":"busy"}` pendant un rangement, une calibration ou un réglage de vitesse, `400 session_duration_out_of_range`, `400 invalid_request` pour une action inconnue.

Une session se termine par un rangement quand son temps est écoulé, sur `stop` et sur un rangement venu de n'importe quelle source. Le pilotage manuel (le joystick, ou une position envoyée par Home Assistant) y met fin aussitôt, sans rangement, et laisse la baguette à la personne. Fermer la page ou perdre la connexion ne l'arrête pas. Elle ne reprend jamais et ne survit pas à un redémarrage.

Pour changer la durée de session sans lancer de session, envoie `PUT /api/settings` avec seulement `{"sessionMinutes": 15}` (sans code PIN, comme `playSpeed`). Elle est écrite en flash une fois le robot au repos, et seulement si elle a changé.

### Calibration

`GET /api/calibration` renvoie l'étape en cours et les valeurs enregistrées jusque-là :

```json
{"step":"horizontal_max","draft":{"horizontal":{"min":32,"max":120,"stow":90},"vertical":{"min":60,"max":120,"stow":90}}}
```

🔒 `POST /api/calibration` avec `{"action": …}` la pilote et renvoie le même objet :

| Action | Effet |
|--------|-------|
| `{"action":"start"}` | Lance la calibration à partir des limites de course actuelles. |
| `{"action":"nudge","axis":"horizontal","degrees":-5}` | Déplace l'axe de l'étape (les deux axes à l'étape `stow`) de ce nombre de degrés, à 20 °/s, n'importe où entre 0 et 180°. |
| `{"action":"mark"}` | Enregistre la position actuelle pour l'étape et passe à la suivante. Renvoie `400` avec `travel_order` ou `stow_out_of_travel` (et `axis`) si la position n'est pas acceptable. |
| `{"action":"confirm"}` | À l'étape `confirm`, enregistre les nouvelles limites et la position de repos, marque le robot comme calibré et le range. |
| `{"action":"cancel"}` | Abandonne la calibration en gardant les réglages précédents, et range le robot. |
| `{"action":"reverse","axis":"horizontal"}` | Seulement à l'étape du minimum ou du maximum de cet axe. Inverse le sens de l'axe sans bouger la baguette ni redémarrer : chaque angle de l'axe (brouillon, limites enregistrées et position de repos) est inversé (`a` → `180 − a`), le câblage est enregistré avec le nouveau sens, et l'axe reprend à son étape de minimum. `400 invalid_request` à toute autre étape. |

Les étapes sont, dans l'ordre : `horizontal_min`, `horizontal_max`, `vertical_min`, `vertical_max`, `stow`, puis `sweep` (le robot parcourt les quatre coins des nouvelles limites et revient seul à la position de repos), puis `confirm`. Le rangement et le pilotage manuel abandonnent aussi la calibration.

### Réglage des vitesses

`GET /api/speed-tuning` renvoie `{"active":false}`, ou pendant le réglage :

```json
{"active":true,"target":"speed","axis":"horizontal","bound":"max","speed":110}
```

Pendant le réglage de l'accélération, `target` vaut `accel`, `speed` est la vitesse maximale de l'axe (celle des mouvements) et `accel` l'accélération essayée, en °/s² :

```json
{"active":true,"target":"accel","axis":"horizontal","bound":"max","speed":90,"accel":410}
```

🔒 `POST /api/speed-tuning` avec `{"action": …}` le pilote et renvoie le même objet :

| Action | Effet |
|--------|-------|
| `{"action":"start","axis":"horizontal","bound":"max"}` | Commence à balayer l'axe entre ses limites de course à sa vitesse maximale (`max`) ou minimale (`min`) actuelle ; l'autre axe reste immobile. `400 invalid_request` avant la calibration, pendant un rangement ou une calibration. |
| `{"action":"start","axis":"horizontal","target":"accel"}` | Réglage de l'accélération : lance de courts allers-retours sur la moitié centrale des limites de course de l'axe, chacun depuis l'arrêt avec une pause de 0,6 s à chaque bout, à la vitesse maximale de l'axe et à son accélération actuelle ; l'autre axe reste immobile. Refusé dans les mêmes cas qu'une vitesse. `target` vaut `speed` par défaut. |
| `{"action":"faster"}` / `{"action":"slower"}` | Un cran : 2 °/s en dessous de 30 °/s, 5 °/s jusqu'à 90 °/s, 10 °/s au-delà. Une vitesse maximale reste entre 10 et 360 °/s, une minimale entre 1 et 360 °/s. Pendant le réglage de l'accélération, plus nerveux / plus doux : 20 °/s² en dessous de 200 °/s², 50 °/s² jusqu'à 500 °/s², 100 °/s² au-delà, entre 20 et 2000 °/s². |
| `{"action":"keep"}` | Enregistre la vitesse (ou l'accélération, comme `maxAccel`) essayée et range le robot. Garder une vitesse maximale inférieure ou égale à la minimale ramène la minimale au dixième de la nouvelle maximale. Une vitesse minimale qui n'est pas inférieure à la maximale renvoie `400 min_speed_out_of_range` et le réglage continue. |
| `{"action":"cancel"}` | Garde la valeur précédente et range le robot. |

Le rangement et le pilotage manuel terminent aussi le réglage des vitesses et celui de l'accélération, sans enregistrer.

### Identité et infos

`GET /api/info` renvoie ce qu'affiche la page Infos :

```json
{"name":"CatWandConnect","hostname":"catwandconnect.local","version":"0.2.0","api":1,"id":"3f9a12c4","channel":"stable",
 "chipFamily":"ESP32","chip":"ESP32-D0WD-V3",
 "chipRevision":3,"cores":2,"cpuMhz":240,"flashMb":4,"freeHeap":182000,"mac":"24:6F:28:00:00:01",
 "ssid":"Home","ip":"192.168.1.20","rssi":-61,"uptime":3600,"temperature":47.5}
```

`api` est la [version de l'API](#version-de-lapi) et `id` l'identifiant d'appareil (le même que l'enregistrement TXT `id`). `channel` est le Canal de mise à jour du robot (`stable` ou `beta`, voir [Réglages](#réglages)) et `chipFamily` sa famille de puce telle que la nomme le [flux de mises à jour](#flux-de-mises-à-jour) : `ESP32`, `ESP32-S3` ou `ESP32-C3` ; `chip` est le modèle exact. `uptime` est en secondes, `rssi` en dBm et `temperature` en °C. Le capteur interne est approximatif : regarde-le comme une tendance.

`GET /api/diagnostics` renvoie le rapport de diagnostic en texte brut (`text/plain`) : version du firmware, puce, un identifiant d'appareil aléatoire (tiré une fois, gardé en flash, non personnel, pour que le support puisse regrouper les rapports d'un même robot sans son adresse MAC), raison du dernier redémarrage, durée de fonctionnement, mémoire, état du code PIN, état du Wi-Fi et de Home Assistant (MQTT), réglages, câblage et les 200 dernières lignes de log (12 Ko au plus). Il est anonymisé par défaut : le nom du robot et son nom d'hôte (sauf s'ils sont ceux par défaut), le réseau Wi-Fi, les adresses IP et MAC, l'adresse du broker et l'utilisateur MQTT sont remplacés par des marqueurs comme `<wifi-network>`, y compris dans les lignes de log. `GET /api/diagnostics?full=1` les garde, pour un usage local uniquement. Le mot de passe MQTT n'est jamais inclus.

`/api/diagnostics/pending` contient le rapport en attente, un problème décrit sur le hotspot et envoyé plus tard :

| Méthode | Corps | Réponse |
|---|---|---|
| `GET` | | `{"pending": true, "description": "...", "report": "..."}` ou `{"pending": false}` |
| `PUT` | `{"description": "..."}` | Enregistre la description (600 caractères au plus) avec le rapport anonymisé que le robot construit lui-même, en remplaçant le précédent. Le rapport garde son résumé et ses lignes de log les plus récentes dans la limite de 3 Ko. `400` sans description. |
| `DELETE` | | Le supprime. |

Le robot ne l'envoie jamais : l'interface du robot le propose sur l'écran principal une fois le robot sur le réseau de son propriétaire.

```text
CatWandConnect diagnostic report
Redacted: yes

Firmware: 0.1.0-beta.2
Last reset: power-on
Wi-Fi network: <wifi-network>
...
Recent log:
[    1.204] [wifi] connecting to "<wifi-network>"
```

🔒 `PUT /api/device` avec `{"name":"CatWandConnect salon"}` renomme le robot et renvoie `{"name":…,"hostname":"catwandconnect-salon.local","restart":true}`. Le robot redémarre environ une seconde plus tard.

- **Noms valides :** 1 à 32 octets, sans caractère de contrôle, avec au moins une lettre ou un chiffre. Sinon la requête renvoie `400` avec `invalid_name`.
- **Noms dérivés :** le nom d'hôte est le nom en ASCII minuscule, sans accents, les autres caractères étant remplacés par des tirets. Le hotspot s'appelle `<name>-XXXX`.

### Mise à jour du firmware

🔒 `POST /api/update` avec un corps `multipart/form-data` contenant l'image de l'application (`firmware.bin`, le fichier `-ota.bin` d'une version publiée).

- **Avant d'écrire**, le robot vérifie que le fichier est une image ESP compilée pour sa puce. Il refuse `not_firmware` et `wrong_chip` avec `400`.
- **Pendant l'écriture**, il arrête le pilotage manuel et écrit l'image dans l'emplacement OTA inactif. Seule une image complète et vérifiée est marquée comme démarrable.
- **Succès :** `200 {"ok":true,"restart":true}`, et le robot redémarre sur le nouveau firmware environ une seconde plus tard.
- **Échec :** `400 {"error":"update_failed","detail":"…"}`. Si l'envoi est interrompu, le robot continue avec son firmware actuel.

Les réglages, la calibration, le Wi-Fi, le nom, le code PIN et le Canal de mise à jour sont en NVS et sont conservés. Une session de jeu en cours se termine au début de la mise à jour. Exemple : `curl -F firmware=@catwandconnect-esp32-ota.bin http://catwandconnect.local/api/update`.

#### Flux de mises à jour

Le robot ne se connecte jamais à Internet ([ADR 0012](adr/0012-the-browser-fetches-updates-for-the-robot.md)). L'interface du robot et l'intégration native lisent le **flux de mises à jour**, `https://vistorx.github.io/CatWandConnect/flasher/channels.json` (servi avec `Access-Control-Allow-Origin: *`), prennent l'entrée du `channel` du robot, et la proposent quand sa `version` est strictement plus récente que celle du robot (SemVer avec pré-versions, un `v` initial ignoré : `0.2.0-beta.3` < `0.2.0-beta.10` < `0.2.0`). Elles téléchargent `ota[chipFamily]`, relatif à l'URL du flux, et l'envoient à `POST /api/update` :

```json
{
  "stable": {"version": "0.2.0", "tag": "v0.2.0",
             "notes": "https://github.com/vistorx/CatWandConnect/releases/tag/v0.2.0",
             "summary": "Premier paragraphe du changelog de la version, texte brut, peut être vide",
             "ota": {"ESP32": "stable/catwandconnect-esp32-ota.bin",
                     "ESP32-S3": "stable/catwandconnect-esp32s3-ota.bin",
                     "ESP32-C3": "stable/catwandconnect-esp32c3-ota.bin"}},
  "beta": {"version": "0.3.0-beta.1", "tag": "v0.3.0-beta.1", "notes": "…", "summary": "…", "ota": {"…": "…"}}
}
```

Un canal sans version publiée est absent. Les images sont les fichiers `-ota.bin` des versions publiées, copiés à côté du flux parce qu'un navigateur ne peut pas télécharger les fichiers des versions GitHub (pas de CORS).

`POST /api/update/available` avec `{"version":"0.2.0-beta.4","notes":"https://…"}` indique au robot la version la plus récente que le flux propose pour son canal, pour l'entité Firmware de MQTT ; `{"version":""}` l'efface. Sans code PIN : cela ne change que ce qu'affiche Home Assistant. Le robot la garde en mémoire jusqu'à son redémarrage et renvoie `200` avec ce qu'il a enregistré ; `400 invalid_request` sans chaîne `version` (32 caractères au plus, `notes` 200). L'interface du robot l'envoie après chaque vérification.

### Réglages Home Assistant (MQTT)

`GET /api/mqtt` renvoie `{"enabled":false,"host":"","port":1883,"user":"","passwordSet":false,"connected":false,"error":"","baseTopic":"catwandconnect/catwandconnect"}`. Le mot de passe n'est jamais renvoyé.

🔒 `PUT /api/mqtt` avec n'importe lesquels de `enabled`, `host`, `port`, `user` et `password` les enregistre et se reconnecte ; sans `password`, le mot de passe actuel est gardé. Renvoie le même objet que `GET`.

- **Invalide :** `400` avec `mqtt_host_required` (activation sans broker) ou `mqtt_port_invalid`.
- **`error`** indique le dernier échec de connexion : `unreachable`, `bad_credentials` ou `refused`.

### Matériel

`GET /api/hardware` renvoie le câblage et les broches autorisées sur cette puce :

```json
{
  "chip": "ESP32",
  "allowedPins": [4, 13, 14, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33],
  "axes": {
    "horizontal": {"pin": 18, "inverted": false, "minPulse": 400, "maxPulse": 2200},
    "vertical":   {"pin": 19, "inverted": false, "minPulse": 600, "maxPulse": 2400}
  }
}
```

🔒 `PUT /api/hardware` avec n'importe quel sous-ensemble de `axes` le modifie.

- **Les broches autorisées** sont celles que la puce peut utiliser sans danger. Les broches de la flash, de la PSRAM, de l'USB, de la console série, de configuration au démarrage (strapping) et celles en entrée seule sont exclues, et les deux servos ne peuvent pas partager une broche.
- **Les impulsions** (`minPulse` à 0°, `maxPulse` à 180°) doivent être entre 300 et 2700 µs, avec au moins 500 µs d'écart.
- **Valide :** `200` avec le nouveau câblage plus `"restart": true`, et le robot redémarre environ une seconde plus tard pour l'appliquer. `"calibrationReset": true` signifie que le changement de sens ou de plage d'impulsions a rendu les anciennes limites de course sans objet : la calibration est oubliée et les mouvements reviennent à 60°–120°.
- **Invalide :** `400` avec `pin_not_allowed`, `pin_duplicate` ou `pulse_out_of_range`, plus `axis`.

### Réglages

`GET /api/settings` renvoie les réglages du Robot core :

```json
{
  "playSpeed": 0.6,
  "calibrated": false,
  "axes": {
    "horizontal": {"min": 60, "max": 120, "stow": 90, "maxSpeed": 90, "minSpeed": 9, "maxAccel": 360, "pauseMin": 0, "pauseMax": 2},
    "vertical":   {"min": 60, "max": 120, "stow": 90, "maxSpeed": 90, "minSpeed": 9, "maxAccel": 360, "pauseMin": 0, "pauseMax": 2}
  },
  "sessionMinutes": 10,
  "channel": "stable"
}
```

| Champ | Signification | Plage valide |
|-------|---------------|--------------|
| `playSpeed` | Le curseur de vitesse de l'écran principal : le pilotage manuel va à `minSpeed` à 0,1 et à `maxSpeed` à 1, linéairement entre les deux. | 0.1–1 |
| `calibrated` | Si la calibration a été faite. D'ici là, les mouvements restent dans 60°–120° quoi que disent `min` et `max`. Lecture seule : seule la calibration le définit. | |
| `min`, `max` | Limites de course, en degrés. | 0–180, `min` < `max` |
| `stow` | Position de repos, en degrés. | entre `min` et `max` |
| `maxSpeed` | Vitesse maximale, en °/s. | 10–360 |
| `minSpeed` | Vitesse minimale, en °/s : la vitesse du pilotage manuel en bas du curseur de vitesse. Par défaut, le dixième de `maxSpeed`. | de 1 à moins que `maxSpeed` |
| `maxAccel` | Accélération, en °/s². | 20–2000 |
| `pauseMin`, `pauseMax` | Plage de pause, en secondes : combien de temps l'axe se repose entre deux mouvements d'une session de jeu. `0` signifie sans arrêt. | 0–60, `pauseMin` ≤ `pauseMax` |
| `sessionMinutes` | Durée de session : combien de temps dure une session de jeu, en minutes. Nombres entiers. | 1–60 |
| `channel` | Canal de mise à jour : les versions proposées au robot. Gardé en NVS, à part des réglages du cœur du robot, après un redémarrage ou une mise à jour. Tant qu'il n'est pas choisi, il suit le firmware en cours : `beta` pour une version `-beta`, `stable` sinon. | `stable`, `beta` |

🔒 `PUT /api/settings` (sauf un corps ne contenant que `playSpeed` et/ou `sessionMinutes`) avec un corps JSON contenant n'importe quel sous-ensemble de ces champs les modifie ; les champs absents gardent leur valeur. Les réglages sont validés comme un tout :

- **Valide :** `200` avec l'ensemble des nouveaux réglages. Ils sont enregistrés et survivent aux redémarrages et aux mises à jour du firmware.
- **Invalide :** `400` avec `{"error":"<code>","axis":"horizontal"|"vertical"}`, et rien ne change. Les codes sont `travel_order`, `travel_out_of_range`, `stow_out_of_travel`, `speed_out_of_range`, `min_speed_out_of_range`, `accel_out_of_range`, `pause_out_of_range`, `pause_order`, `play_speed_out_of_range` et `session_duration_out_of_range` (ces deux-là sans `axis`), `invalid_channel` (seul, sans `axis`) et `invalid_json`.

Chaque `PUT` réussi écrit en flash : envoie-le quand une valeur est fixée, pas à chaque cran du curseur.

### Wi-Fi

| Requête | Réponse |
|---------|---------|
| `GET /api/wifi/status` | `{"mode":"hotspot"\|"connecting"\|"connected","failed":bool,"ssid":"…","ip":"…","host":"catwandconnect.local","hotspot":bool,"failedSsid":"…"}`. `failed` vaut true quand la dernière tentative a échoué. `hotspot` vaut true tant que le hotspot `CatWandConnect-XXXX` est actif. `failedSsid` donne le réseau qu'un changement depuis le réseau habituel n'a pas pu rejoindre, pendant que le robot revient (puis est revenu) sur le réseau précédent ; vide sinon. |
| `GET /api/wifi/networks` | `{"scanning":true}` pendant une recherche (interroge à nouveau), puis `{"scanning":false,"networks":[{"ssid":"…","rssi":-60,"secure":true}]}`. |
| 🔒 `POST /api/wifi` | Champs de formulaire `ssid` et `password`. Renvoie 202 et se connecte en arrière-plan (environ 1 s plus tard quand la demande vient du réseau habituel, pour que la réponse parte) ; suis la progression avec `/api/wifi/status`. Accepté depuis le hotspot et depuis le réseau habituel. Le nouveau réseau n'est enregistré qu'une fois rejoint ; si un changement demandé depuis le réseau habituel échoue, le robot revient sur le réseau précédent, puis ouvre son hotspot si ça échoue aussi ([ADR 0011](adr/0011-wifi-change-falls-back-to-the-previous-network.md), en anglais). |

### Fichiers de l'interface

`/` sert la page de configuration du Wi-Fi tant que le robot n'est pas sur un réseau Wi-Fi, et l'écran principal ensuite. Les fichiers de l'interface (`/app.js`, `/lang/en.json`, `/manifest.webmanifest`, …) sont servis compressés en gzip.

## Home Assistant (MQTT)

Quand c'est activé dans les Réglages, le robot se connecte à un broker MQTT et s'annonce avec la découverte MQTT de Home Assistant. La configuration est décrite dans [Home Assistant](home-assistant.fr.md). Les topics utilisent le nom d'hôte du robot, par exemple `catwandconnect/catwandconnect-salon`, noté `<base>` ci-dessous.

| Topic | Sens | Contenu |
|-------|------|---------|
| `<base>/status` | robot → | `online` / `offline` (retenu ; `offline` est le dernier message, « last will ») |
| `<base>/state` | robot → | `{"h":92,"v":88,"hs":90,"vs":90,"activity":"idle","play":"OFF","sm":10}` (retenu) : positions en degrés, vitesses maximales en °/s ; `activity` comme dans `/ws` (`playing` pendant une session de jeu) ; `play` vaut `ON` tant qu'une session de jeu tourne ; `sm` est la durée de session en minutes |
| `<base>/stow/set` | → robot | `PRESS` |
| `<base>/horizontal/position/set`, `<base>/vertical/position/set` | → robot | Un nombre de degrés. L'axe s'y déplace à la vitesse du pilotage manuel, ramené dans les limites de course. |
| `<base>/horizontal/max_speed/set`, `<base>/vertical/max_speed/set` | → robot | Un nombre de °/s. Validé comme la valeur des Réglages. |
| `<base>/play_session/set` | → robot | `ON` lance une session de jeu pour la durée de session, `OFF` l'arrête (rangement). Un lancement refusé (pas calibré, occupé) est noté dans le log et l'état est republié avec `play` toujours à `OFF`. |
| `<base>/session_duration/set` | → robot | Des minutes entières, de 1 à 60. Enregistrée dès que le robot est au repos ; les autres valeurs sont ignorées. |
| `<base>/update` | robot → | `{"installed_version":"0.2.0-beta.3","latest_version":"0.2.0-beta.4","release_url":"https://…","title":"CatWandConnect firmware"}` (retenu), pour l'entité de mise à jour Firmware. `latest_version` est celle indiquée par `POST /api/update/available` si elle est plus récente que la version installée, sinon la version installée (à jour, sans `release_url`). Il n'y a pas de commande d'installation : le robot ne peut pas télécharger. |
| `homeassistant/<component>/catwandconnect_<mac>/<object>/config` | robot → | Messages de découverte (retenus) pour le bouton Ranger, les cinq nombres, le capteur d'activité, l'interrupteur de session de jeu et l'entité de mise à jour Firmware. |

Les commandes passent par le Robot core comme le pilotage manuel : les limites de course, les limites de vitesse et les priorités s'appliquent, et le rangement passe avant tout.
