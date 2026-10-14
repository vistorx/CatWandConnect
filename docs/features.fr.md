# Fonctionnalités

Tout ce que fait le firmware CatWandConnect, regroupé par usage. Tout est dans la V1, sauf mention contraire.

## Jouer

- **Joystick sur le téléphone.** Ouvre `http://catwandconnect.local/` dans n'importe quel navigateur sur ton Wi-Fi et fais glisser le joystick : la baguette suit en temps réel. Un petit mouvement la déplace lentement ; un mouvement complet la déplace à pleine vitesse. Relâche et elle s'arrête. Fonctionne avec le téléphone en portrait ou en paysage.
- **Curseur de vitesse.** Règle la vivacité de la baguette, de la vitesse minimale à la vitesse maximale de chaque axe, directement sur l'écran principal. Tous les téléphones ouverts restent synchronisés.
- **Position en direct.** L'écran principal montre où se trouve chaque axe.
- **Plusieurs téléphones à la fois.** Tout le monde sur ton Wi-Fi peut ouvrir l'interface pour regarder ou jouer.

## Sécurité

- **Limites de course.** Chaque axe a un minimum et un maximum que le robot ne dépasse jamais, quelle que soit l'origine de la commande (téléphone, API ou Home Assistant). La baguette freine en douceur avant de les atteindre.
- **Sûr tant qu'il n'est pas calibré.** À la sortie de la boîte, les mouvements restent dans une plage prudente de 60° à 120° sur chaque axe.
- **Ranger.** Un grand bouton, toujours visible, ramène la baguette à sa position de repos et arrête tout le reste. Rien ne peut l'interrompre. Au repos, les servos sont relâchés : ils ne grésillent pas et ne chauffent pas.
- **Règle de l'homme mort.** Si ton téléphone se déconnecte ou n'envoie plus rien pendant une demi-seconde, la baguette s'arrête.
- **Mouvements doux.** Vitesse minimale, vitesse maximale et accélération par axe : les départs et les arrêts ne secouent jamais le mécanisme et le réglage le plus lent ne saccade pas. Les réglages enregistrés par la 0.1.0 sont conservés à la mise à jour, avec une vitesse minimale égale au dixième de la maximale.
- **Réglage des vitesses.** Réglages → Mouvement → Régler les vitesses : le robot balaie un axe d'une limite à l'autre pendant que tu touches Plus vite ou Plus lent, puis tu gardes la vitesse qui te semble juste comme maximum de cet axe (juste avant qu'il tremble) ou comme minimum (juste au-dessus de là où il saccade). Disponible une fois le robot calibré ; Ranger, le joystick ou Annuler gardent la valeur précédente.
- **Démarrage en douceur.** Le robot se souvient d'où la baguette s'est arrêtée en dernier, repart de là pour que les servos ne sautent pas, puis rejoint lentement sa position de repos.
- **Code PIN des réglages, facultatif.** Les invités peuvent jouer et ranger, mais modifier les limites, la calibration, le câblage, le nom, le Wi-Fi, Home Assistant ou le firmware demande le code PIN. Après plusieurs codes faux, il se bloque pour une durée croissante.

## Installation

- **Flasher web.** Installe le firmware en un clic depuis Chrome ou Edge sur un ordinateur, sans outil à installer. Le bon fichier pour ta puce est choisi automatiquement. Choisis le canal Stable ou Bêta. Réinstaller sur un robot garde son Wi-Fi et ses réglages, sauf si tu coches « Erase device ». Il est sur <https://vistorx.github.io/CatWandConnect/flasher/>, à un clic du [site du projet](https://vistorx.github.io/CatWandConnect/fr/), qui présente le README et cette documentation, avec une recherche.
- **Le Wi-Fi de deux façons.** Saisis ton Wi-Fi dans le flasher web juste après l'installation, ou connecte-toi au hotspot `CatWandConnect-XXXX` du robot : sa page de configuration s'ouvre toute seule et liste les réseaux proches, du plus fort au plus faible. Ou touche « Continuer sans Wi-Fi » et utilise le robot depuis son hotspot ; connecte-le plus tard depuis Réglages → Réseau → Wi-Fi.
- **Wi-Fi qui se rétablit tout seul.** Si ta box disparaît, le hotspot revient ; le robot continue d'essayer ton réseau et quitte le hotspot dès qu'il revient.
- **Assistant de calibration.** Un guide pas à pas pour régler les limites de course et la position de repos de ton montage : chaque étape amène la baguette à la valeur actuelle et indique à quelle position du joystick elle correspond (minimum = tout à gauche ou tout en bas, maximum = tout à droite ou tout en haut), et tu l'ajustes jusqu'à la bonne extrémité ; si l'axe tourne dans le mauvais sens, coche « Inverser cet axe » sur place (la case montre le câblage actuel), sans redémarrer ; choisis la position de repos (l'assistant rappelle ce que tu as déjà validé et la plage autorisée), puis regarde un lent balayage de test avant d'enregistrer. Annule à tout moment pour garder les valeurs précédentes ; quand une calibration est déjà enregistrée, l'assistant propose « Recommencer la calibration », et « Annuler la nouvelle calibration » te ramène à l'écran principal.
- **Ton câblage.** Choisis quel GPIO pilote chaque servo (seules les broches sans danger sur ta puce sont proposées), inverse un axe et, sous Course du servo (avancé), règle la plage d'impulsions de chaque servo : jusqu'où il tourne, avec un retour aux valeurs par défaut.

## Au quotidien

- **Réglages en onglets.** Mouvement (limites, calibration, vitesse), Matériel (câblage), Réseau (Wi-Fi, Home Assistant) et Robot (nom, langue, code PIN, mise à jour du firmware, Infos). Un lien peut ouvrir directement un onglet, par exemple `settings.html#network`.
- **Donne un nom à ton robot.** « CatWandConnect salon » devient `http://catwandconnect-salon.local/` et nomme aussi son hotspot et son appareil Home Assistant.
- **Anglais et français.** L'interface suit la langue de ton téléphone, ou celle que tu choisis dans les Réglages.
- **Icône sur l'écran d'accueil.** Ajoute l'interface à l'écran d'accueil de ton téléphone ; elle s'ouvre en plein écran, comme une appli.
- **Page Infos.** Version du firmware, puce, signal Wi-Fi, adresse IP, durée de fonctionnement, température et mémoire, avec un bouton Copier pour les rapports de bug.
- **Rapport de diagnostic.** Sur la page Infos, un rapport prêt à joindre aux rapports de bug : firmware, puce, un identifiant d'appareil aléatoire, la raison du dernier redémarrage, la mémoire, l'état du Wi-Fi et de Home Assistant, les réglages, le câblage et le log récent du robot. Le nom de ton Wi-Fi, tes adresses et les détails du broker sont masqués. Copie-le ou télécharge-le en fichier texte ; une version complète, pour ton usage personnel uniquement, les garde.
- **Page Aide.** Depuis Réglages → Robot ou la page Infos : pose une question sur Discord, ou signale un problème. Tu le décris, tu vérifies le texte exact qui sera public, et le robot ouvre un rapport de bug GitHub prérempli avec ta description et son rapport de diagnostic anonymisé (les lignes de log les plus récentes qui tiennent), puis télécharge le rapport complet à joindre. Sans compte GitHub, **Envoyer au support** le publie pour toi via le relais de support (contact facultatif ; la page affiche le lien de l'issue créée) ; si le service est injoignable, l'issue préremplie reste disponible. Coche **Suivre sur Discord** pour avoir aussi un post `#support` lié à l'issue, où tu es prévenu quand le correctif sort.
- **Signaler plus tard depuis le hotspot.** Sur le hotspot de configuration, le téléphone n'a pas internet : la page de configuration propose de télécharger le rapport de diagnostic et indique où demander de l'aide, et l'Aide peut enregistrer ta description sur le robot. Une fois le robot sur ton Wi-Fi, l'écran principal propose de l'envoyer ou de la supprimer. Elle n'est jamais envoyée toute seule.

## Aide

- **Où trouver de l'aide.** Un [serveur Discord](https://discord.gg/Rbqge3v9dk) pour les questions, l'aide au montage et le partage de vos robots, en plusieurs langues et sans compte GitHub ; des formulaires d'issue GitHub pour les bugs et les problèmes d'installation ; un signalement privé pour les problèmes de sécurité. Voir [Obtenir de l'aide](../SUPPORT.fr.md).

## Mises à jour

- **Mises à jour du firmware depuis le navigateur.** Envoie le nouveau fichier dans les Réglages. Les réglages et la calibration sont conservés, et si l'envoi échoue ou est interrompu, le robot garde son firmware actuel.
- **Versions publiées.** Chaque version est publiée avec des fichiers prêts à l'emploi pour chaque puce compatible.
- **Canaux Stable et Bêta.** Les versions stables sont pour tout le monde. Les versions bêta reçoivent les nouveautés en premier pour être testées sur de vrais robots ; le flasher web te laisse choisir l'une ou l'autre.

## Intégrations

- **Home Assistant (facultatif).** Saisis ton broker MQTT dans les Réglages et le robot apparaît tout seul dans Home Assistant, avec un bouton Ranger, la position et la vitesse maximale de chaque axe, et son activité. Les commandes suivent les mêmes règles de sécurité que le téléphone. Voir [Home Assistant](home-assistant.fr.md).
- **API ouverte.** Tout ce que fait l'interface passe par une API HTTP, WebSocket et MQTT documentée, pour tes propres scripts et intégrations. Voir [API du robot](api.fr.md).

## Matériel

- **Cartes :** ESP32, ESP32-S3 et ESP32-C3. Les cartes ESP8266 ne sont pas compatibles ([ADR 0008](adr/0008-no-esp8266-support.md), en anglais).
- **Mécanisme :** le Mk2, avec deux servos MG996R.

## Prévu

- **Sessions de jeu automatiques** (V1.1) : le robot joue tout seul, dans ses limites de course.
- **Une intégration Home Assistant dédiée**, construite sur la même API ([ADR 0005](adr/0005-home-assistant-via-mqtt-first.md), en anglais).
- **Pilotage en Bluetooth.**
